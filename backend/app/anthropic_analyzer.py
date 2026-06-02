import base64
import json
import os
import re
from pathlib import Path

from anthropic import Anthropic, APIError


class AnthropicAnalyzerError(Exception):
    def __init__(self, message: str, status_code: int = 502):
        self.message = message
        self.status_code = status_code
        super().__init__(message)


class AnthropicAnalyzer:
    """Encapsulates the Anthropic call and response parsing."""

    PROMPT_SISTEMA = """Sos un tutor visual parlante para un centro educativo infantil.
Mira la imagen (dibujo o foto de un niño) y genera contenido en español rioplatense, simple y cálido.

Responde ÚNICAMENTE con un objeto JSON válido (sin markdown, sin texto extra) con estas claves:
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

    def __init__(self):
        self._api_key = self._read_api_key()
        self._model = os.getenv("ANTHROPIC_MODEL", "claude-sonnet-4-6").strip()
        self._max_tokens = int(os.getenv("ANTHROPIC_MAX_TOKENS", "1024"))

    @staticmethod
    def _read_api_key() -> str:
        raw = os.getenv("ANTHROPIC_API_KEY", "")
        return raw.strip().strip('"').strip("'")

    def analyze(self, imagen_base64: str, nombre_archivo: str | None) -> dict:
        if not self._api_key:
            raise AnthropicAnalyzerError(
                "ANTHROPIC_API_KEY no configurada en el servidor",
                status_code=503,
            )

        imagen_data = self._normalize_base64(imagen_base64)
        media_type = self._media_type(nombre_archivo)
        client = Anthropic(api_key=self._api_key)

        try:
            message = client.messages.create(
                model=self._model,
                max_tokens=self._max_tokens,
                system=self.PROMPT_SISTEMA,
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
            detalle = str(exc)
            if "401" in detalle or "authentication" in detalle.lower():
                raise AnthropicAnalyzerError(
                    "Clave de Anthropic invalida o no configurada en el VPS. "
                    "Revisa ANTHROPIC_API_KEY en el archivo .env del servidor y reinicia el backend.",
                    status_code=503,
                ) from exc
            if "404" in detalle or "not_found" in detalle.lower():
                raise AnthropicAnalyzerError(
                    f"Modelo Anthropic no encontrado: {self._model}. "
                    "Usa por ejemplo claude-sonnet-4-6 en ANTHROPIC_MODEL del .env.",
                    status_code=503,
                ) from exc
            raise AnthropicAnalyzerError(f"Error de Anthropic: {exc}") from exc
        except Exception as exc:
            raise AnthropicAnalyzerError(f"Error al llamar a Anthropic: {exc}") from exc

        texto = self._extract_text_from_response(message)
        return self._parse_analysis_json(texto)

    def _normalize_base64(self, imagen_base64: str) -> str:
        data = imagen_base64.strip()
        if data.startswith("data:") and "," in data:
            data = data.split(",", 1)[1]
        data = re.sub(r"\s+", "", data)
        try:
            base64.b64decode(data, validate=True)
        except Exception as exc:
            raise AnthropicAnalyzerError("imagen_base64 invalida", status_code=400) from exc
        return data

    @classmethod
    def _media_type(cls, nombre_archivo: str | None) -> str:
        if not nombre_archivo:
            return "image/jpeg"
        ext = Path(nombre_archivo).suffix.lower()
        return cls.MEDIA_TYPES.get(ext, "image/jpeg")

    @staticmethod
    def _extract_text_from_response(message) -> str:
        partes: list[str] = []
        for block in message.content:
            if block.type == "text":
                partes.append(block.text)
        texto = "\n".join(partes).strip()
        if not texto:
            raise AnthropicAnalyzerError("Anthropic no devolvio texto en la respuesta")
        return texto

    @staticmethod
    def _parse_analysis_json(texto: str) -> dict:
        limpio = texto.strip()
        if limpio.startswith("`"):
            limpio = re.sub(r"^`(?:json)?\s*", "", limpio, flags=re.IGNORECASE)
            limpio = re.sub(r"\s*`$", "", limpio)

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
