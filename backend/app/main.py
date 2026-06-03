"""
main.py — VPS-POO API (todo en un solo archivo)
================================================
Principios POO aplicados:
  - Abstracción:    UsuarioRepositorio, AnalisisRepositorio (ABCs / interfaces)
  - Herencia:       MySQLUsuarioRepositorio, MySQLAnalisisRepositorio heredan de las ABCs
  - Polimorfismo:   los servicios aceptan cualquier implementación de los repos
  - Encapsulamiento: atributos privados (_repo, _secret, etc.) y métodos internos (_hashear, etc.)
"""

# ─────────────────────────── imports ────────────────────────────────────
from abc import ABC, abstractmethod
from datetime import datetime, timedelta, timezone
from typing import Optional
import os

import bcrypt
import mysql.connector
import psutil
from fastapi import FastAPI, HTTPException, Request, status
from fastapi.middleware.cors import CORSMiddleware
from jose import JWTError, jwt
from pydantic import BaseModel

from anthropic_analyzer import AnthropicAnalyzerError, analizar_imagen_con_anthropic

# ══════════════════════════════════════════════════════════════════════════
#  CAPA 1 — ABSTRACCIÓN (interfaces / contratos)
# ══════════════════════════════════════════════════════════════════════════

class UsuarioRepositorio(ABC):
    """Contrato abstracto para acceso a datos de usuarios."""

    @abstractmethod
    def buscar_por_username(self, username: str) -> Optional[dict]: ...

    @abstractmethod
    def crear(self, username: str, hashed_password: str, email: str) -> None: ...


class AnalisisRepositorio(ABC):
    """Contrato abstracto para acceso a datos de análisis de imágenes."""

    @abstractmethod
    def guardar(
        self,
        usuario_id: int,
        nombre_archivo: Optional[str],
        descripcion: str,
        pregunta: str,
        historia: str,
    ) -> None: ...

    @abstractmethod
    def listar_por_usuario(self, usuario_id: int) -> list[dict]: ...


# ══════════════════════════════════════════════════════════════════════════
#  CAPA 2 — HERENCIA (implementaciones concretas MySQL)
# ══════════════════════════════════════════════════════════════════════════

class MySQLUsuarioRepositorio(UsuarioRepositorio):
    """Implementación MySQL — hereda de UsuarioRepositorio."""

    def __init__(self, conn_factory):
        self._conn_factory = conn_factory       # encapsulado

    def buscar_por_username(self, username: str) -> Optional[dict]:
        conn = self._conn_factory()
        cursor = conn.cursor(dictionary=True)
        cursor.execute("SELECT * FROM usuarios WHERE username = %s LIMIT 1", (username,))
        user = cursor.fetchone()
        conn.close()
        return user

    def crear(self, username: str, hashed_password: str, email: str) -> None:
        conn = self._conn_factory()
        cursor = conn.cursor()
        cursor.execute(
            "INSERT INTO usuarios (username, password, nombre, apellido, email) VALUES (%s,%s,%s,%s,%s)",
            (username, hashed_password, username, username, email),
        )
        conn.commit()
        conn.close()


class MySQLAnalisisRepositorio(AnalisisRepositorio):
    """Implementación MySQL — hereda de AnalisisRepositorio."""

    def __init__(self, conn_factory):
        self._conn_factory = conn_factory       # encapsulado

    def guardar(self, usuario_id, nombre_archivo, descripcion, pregunta, historia):
        conn = self._conn_factory()
        cursor = conn.cursor()
        cursor.execute(
            "INSERT INTO analisis_imagenes (usuario_id, nombre_archivo, descripcion, pregunta, historia) "
            "VALUES (%s,%s,%s,%s,%s)",
            (usuario_id, nombre_archivo, descripcion, pregunta, historia),
        )
        conn.commit()
        conn.close()

    def listar_por_usuario(self, usuario_id: int) -> list[dict]:
        conn = self._conn_factory()
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            "SELECT id, nombre_archivo, descripcion, pregunta, historia, created_at "
            "FROM analisis_imagenes WHERE usuario_id = %s ORDER BY created_at DESC",
            (usuario_id,),
        )
        data = cursor.fetchall()
        conn.close()
        return data


# ══════════════════════════════════════════════════════════════════════════
#  CAPA 3 — SERVICIOS (lógica de negocio encapsulada)
# ══════════════════════════════════════════════════════════════════════════

class ServicioAutenticacion:
    """
    Encapsula autenticación, hashing y tokens JWT.
    Polimorfismo: acepta cualquier UsuarioRepositorio (MySQL, memoria, etc.)
    """

    def __init__(self, usuario_repo: UsuarioRepositorio, jwt_secret: str, jwt_expire_minutes: int):
        self._repo            = usuario_repo    # privado
        self._secret          = jwt_secret      # privado
        self._expire_minutes  = jwt_expire_minutes

    # — API pública —

    def registrar(self, username: str, password: str) -> None:
        if self._repo.buscar_por_username(username):
            raise HTTPException(status_code=400, detail="El usuario ya existe")
        self._repo.crear(username, self._hashear_password(password), f"{username}@sin-email.local")

    def login(self, username: str, password: str) -> dict:
        user = self._repo.buscar_por_username(username)
        if not user or not self._verificar_password(password, user["password"]) or not user["activo"]:
            raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Credenciales invalidas")
        token = self._crear_token({
            "sub": user["username"], "user_id": user["id"],
            "nombre": user["nombre"], "apellido": user["apellido"], "email": user["email"],
        })
        return {"access_token": token, "token_type": "bearer", "user": self._serializar_usuario(user)}

    def decodificar_token(self, token: str) -> dict:
        try:
            return jwt.decode(token, self._secret, algorithms=["HS256"])
        except JWTError as exc:
            raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Token invalido") from exc

    # — Métodos privados (encapsulamiento) —

    def _hashear_password(self, plain: str) -> str:
        return bcrypt.hashpw(plain.encode(), bcrypt.gensalt()).decode()

    def _verificar_password(self, plain: str, hashed: str) -> bool:
        try:
            return bcrypt.checkpw(plain.encode(), hashed.encode())
        except Exception:
            return False

    def _crear_token(self, payload: dict) -> str:
        payload.update({"exp": datetime.now(timezone.utc) + timedelta(minutes=self._expire_minutes)})
        return jwt.encode(payload, self._secret, algorithm="HS256")

    def _serializar_usuario(self, user: dict) -> dict:
        return {k: user[k] for k in ("id", "username", "nombre", "apellido", "email")}


class ServicioAnalisisImagenes:
    """
    Encapsula el análisis de imágenes con IA.
    Polimorfismo: acepta cualquier AnalisisRepositorio.
    """

    def __init__(self, analisis_repo: AnalisisRepositorio):
        self._repo = analisis_repo              # privado

    def analizar_y_guardar(self, usuario_id: int, imagen_base64: str, nombre_archivo: Optional[str]) -> dict:
        try:
            resultado = analizar_imagen_con_anthropic(imagen_base64, nombre_archivo)
        except AnthropicAnalyzerError as exc:
            raise HTTPException(status_code=exc.status_code, detail=exc.message) from exc

        self._repo.guardar(usuario_id, nombre_archivo,
                           resultado["descripcion"], resultado["pregunta"], resultado["historia"])
        return resultado

    def obtener_analisis_de_usuario(self, usuario_id: int) -> list[dict]:
        return self._repo.listar_por_usuario(usuario_id)


class ExtractorToken:
    """
    Encapsula la lectura del JWT desde los headers HTTP.
    Polimorfismo: si cambiás a API-Key o OAuth, solo cambiás esta clase.
    """

    def __init__(self, auth_service: ServicioAutenticacion):
        self._auth = auth_service               # privado

    def obtener_payload(self, request: Request) -> dict:
        return self._auth.decodificar_token(self._extraer_token(request))

    def _extraer_token(self, request: Request) -> str:
        auth   = request.headers.get("authorization", "")
        xtoken = request.headers.get("x-access-token", "")
        if auth.lower().startswith("bearer "):
            return auth.split(" ", 1)[1].strip()
        if xtoken.lower().startswith("bearer "):
            return xtoken.split(" ", 1)[1].strip()
        if xtoken:
            return xtoken.strip()
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Token faltante")


# ══════════════════════════════════════════════════════════════════════════
#  CAPA 4 — CONFIGURACIÓN E INYECCIÓN DE DEPENDENCIAS
# ══════════════════════════════════════════════════════════════════════════

JWT_SECRET         = os.getenv("JWT_SECRET", "dev_secret_change_me")
JWT_EXPIRE_MINUTES = int(os.getenv("JWT_EXPIRE_MINUTES", "60"))

DB_CONFIG = {
    "host":     os.getenv("DB_HOST", "db"),
    "port":     int(os.getenv("DB_PORT", "3306")),
    "user":     os.getenv("DB_USER", "poo_user"),
    "password": os.getenv("DB_PASSWORD", "poo_pass"),
    "database": os.getenv("DB_NAME", "vps-poo"),
}

def _conn_factory():
    return mysql.connector.connect(**DB_CONFIG)

# Instancias concretas — se pueden reemplazar sin tocar el resto del código
_usuario_repo    = MySQLUsuarioRepositorio(_conn_factory)
_analisis_repo   = MySQLAnalisisRepositorio(_conn_factory)
_auth_service    = ServicioAutenticacion(_usuario_repo, JWT_SECRET, JWT_EXPIRE_MINUTES)
_analisis_service = ServicioAnalisisImagenes(_analisis_repo)
_extractor       = ExtractorToken(_auth_service)

# ══════════════════════════════════════════════════════════════════════════
#  CAPA 5 — PRESENTACIÓN (rutas FastAPI — sin lógica de negocio)
# ══════════════════════════════════════════════════════════════════════════

app = FastAPI(title="VPS-POO API", root_path="/api")

cors_origins = [o.strip() for o in os.getenv("CORS_ORIGINS", "").split(",") if o.strip()] or ["*"]
app.add_middleware(
    CORSMiddleware,
    allow_origins=cors_origins,
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)

# — Modelos Pydantic —

class LoginRequest(BaseModel):
    username: str
    password: str

class LoginResponse(BaseModel):
    access_token: str
    token_type: str
    user: dict

class RegistroRequest(BaseModel):
    username: str
    password: str

class ImagenRequest(BaseModel):
    imagen_base64: str
    nombre_archivo: str | None = None

# — Endpoints —

@app.get("/health")
def health():
    return {
        "status": "ok",
        "cpu":  psutil.cpu_percent(),
        "ram":  psutil.virtual_memory().percent,
        "disk": psutil.disk_usage("/").percent,
    }

@app.post("/auth/registro")
def registro(payload: RegistroRequest):
    _auth_service.registrar(payload.username, payload.password)
    return {"ok": True, "mensaje": "Usuario registrado correctamente"}

@app.post("/auth/login", response_model=LoginResponse)
def login(payload: LoginRequest):
    return _auth_service.login(payload.username, payload.password)

@app.get("/auth/me")
def me(request: Request):
    data = _extractor.obtener_payload(request)
    return {"username": data.get("sub"), "nombre": data.get("nombre"),
            "apellido": data.get("apellido"), "email": data.get("email")}

@app.post("/analizar-imagen")
def analizar_imagen(payload: ImagenRequest, request: Request):
    usuario = _extractor.obtener_payload(request)
    return _analisis_service.analizar_y_guardar(
        usuario.get("user_id"), payload.imagen_base64, payload.nombre_archivo
    )

@app.get("/mis-analisis")
def mis_analisis(request: Request):
    usuario = _extractor.obtener_payload(request)
    return _analisis_service.obtener_analisis_de_usuario(usuario.get("user_id"))