import base64
import json
import os
import re
from pathlib import Path

from anthropic import Anthropic, APIError

ANTHROPIC_API_KEY = os.getenv("ANTHROPIC_API_KEY", "")
ANTHROPIC_MODEL = os.getenv("ANTHROPIC_MODEL", "claude-sonnet-4-20250514")
ANTHROPIC_MAX_TOKENS = int(os.getenv("ANTHROPIC_MAX_TOKENS", "1024"))

PROMPT_SISTEMA = """Sos un tutor visual parlante para un centro educativo infantil.
Mirá la imagen (dibujo o foto de un niño) y generá contenido en español rioplatense, simple y cálido.

Respondé ÚNICAMENTE con un objeto JSON válido (sin markdown, sin texto extra) con estas claves:
- "descripcion": qué ves en la imagen, 2 o 3 oraciones cortas.
- "pregunta": una pregunta abierta y amable para que el niño hable de su dibujo.
- "historia": un cuento corto de 4 a 6 oraciones inspirado en la imagen, apto para niños pequeños."""

MEDIA_TYPES = {
    ".jpg": "image/jpeg",
    ".jpeg": "image/jpeg",
    ".png": "image/png",
    ".gif": "image/gif",
    ".webp": "image/webp",
}


class AnthropicAnalyzerError(Exception):
    def __init__(self, message: str, status_code: int = 502):
        self.message = message
        self.status_code = status_code
        super().__init__(message)


def _normalizar_base64(imagen_base64: str) -> str:
    data = imagen_base64.strip()
    if data.startswith("data:") and "," in data:
        data = data.split(",", 1)[1]
    data = re.sub(r"\s+", "", data)
    try:
        base64.b64decode(data, validate=True)
    except Exception as exc:
        raise AnthropicAnalyzerError("imagen_base64 invalida", status_code=400) from exc
    return data


def _media_type(nombre_archivo: str | None) -> str:
    if not nombre_archivo:
        return "image/jpeg"
    ext = Path(nombre_archivo).suffix.lower()
    return MEDIA_TYPES.get(ext, "image/jpeg")


def _extraer_texto_respuesta(message) -> str:
    partes: list[str] = []
    for block in message.content:
        if block.type == "text":
            partes.append(block.text)
    texto = "\n".join(partes).strip()
    if not texto:
        raise AnthropicAnalyzerError("Anthropic no devolvio texto en la respuesta")
    return texto


def _parsear_json_analisis(texto: str) -> dict:
    limpio = texto.strip()
    if limpio.startswith("```"):
        limpio = re.sub(r"^```(?:json)?\s*", "", limpio, flags=re.IGNORECASE)
        limpio = re.sub(r"\s*```$", "", limpio)

    try:
        data = json.loads(limpio)
    except json.JSONDecodeError as exc:
        raise AnthropicAnalyzerError(
            "No se pudo interpretar la respuesta de Anthropic como JSON"
        ) from exc

    for clave in ("descripcion", "pregunta", "historia"):
        valor = data.get(clave)
        if not isinstance(valor, str) or not valor.strip():
            raise AnthropicAnalyzerError(f"Falta o es invalida la clave '{clave}' en la respuesta")

    return {
        "descripcion": data["descripcion"].strip(),
        "pregunta": data["pregunta"].strip(),
        "historia": data["historia"].strip(),
    }


def analizar_imagen_con_anthropic(imagen_base64: str, nombre_archivo: str | None) -> dict:
    if not ANTHROPIC_API_KEY:
        raise AnthropicAnalyzerError(
            "ANTHROPIC_API_KEY no configurada en el servidor",
            status_code=503,
        )

    imagen_data = _normalizar_base64(imagen_base64)
    media_type = _media_type(nombre_archivo)
    client = Anthropic(api_key=ANTHROPIC_API_KEY)

    try:
        message = client.messages.create(
            model=ANTHROPIC_MODEL,
            max_tokens=ANTHROPIC_MAX_TOKENS,
            system=PROMPT_SISTEMA,
            messages=[
                {
                    "role": "user",
                    "content": [
                        {
                            "type": "image",
                            "source": {
                                "type": "base64",
                                "media_type": media_type,
                                "data": imagen_data,
                            },
                        },
                        {
                            "type": "text",
                            "text": "Analizá esta imagen y devolvé el JSON pedido.",
                        },
                    ],
                }
            ],
        )
    except APIError as exc:
        raise AnthropicAnalyzerError(f"Error de Anthropic: {exc}") from exc
    except Exception as exc:
        raise AnthropicAnalyzerError(f"Error al llamar a Anthropic: {exc}") from exc

    texto = _extraer_texto_respuesta(message)
    return _parsear_json_analisis(texto)
