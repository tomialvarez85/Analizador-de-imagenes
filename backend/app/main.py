from abc import ABC, abstractmethod
from datetime import datetime, timedelta, timezone
import os
import psutil
from pydantic import BaseModel
from fastapi import FastAPI, HTTPException, Request, status
from fastapi.middleware.cors import CORSMiddleware
from jose import JWTError, jwt
import bcrypt
import mysql.connector

from app.anthropic_analyzer import AnthropicAnalyzer, AnthropicAnalyzerError


# --------- CONFIGURACION ---------

class AppConfig:
    """Encapsulates application configuration."""

    @staticmethod
    def jwt_secret() -> str:
        return os.getenv("JWT_SECRET", "dev_secret_change_me")

    @staticmethod
    def jwt_expire_minutes() -> int:
        return int(os.getenv("JWT_EXPIRE_MINUTES", "60"))

    @staticmethod
    def db_config() -> dict:
        return {
            "host": os.getenv("DB_HOST", "db"),
            "port": int(os.getenv("DB_PORT", "3306")),
            "user": os.getenv("DB_USER", "poo_user"),
            "password": os.getenv("DB_PASSWORD", "poo_pass"),
            "database": os.getenv("DB_NAME", "vps-poo"),
        }

    @staticmethod
    def cors_origins() -> list[str]:
        origins = [o.strip() for o in os.getenv("CORS_ORIGINS", "").split(",") if o.strip()]
        return origins or ["*"]


class Database:
    """Responsible for MySQL database connections."""

    def __init__(self, config: dict):
        self._config = config

    def connect(self):
        return mysql.connector.connect(**self._config)


class BaseRepository(ABC):
    """Base repository providing database access."""

    def __init__(self, database: Database):
        self._database = database

    def _connect(self):
        return self._database.connect()


class UserRepository(BaseRepository):
    """Encapsulates user queries."""

    def find_by_username(self, username: str) -> dict | None:
        conn = self._connect()
        cursor = conn.cursor(dictionary=True)
        cursor.execute("SELECT * FROM usuarios WHERE username = %s LIMIT 1", (username,))
        user = cursor.fetchone()
        conn.close()
        return user

    def save_user(self, username: str, hashed_password: str) -> None:
        conn = self._connect()
        cursor = conn.cursor()
        cursor.execute(
            "INSERT INTO usuarios (username, password, nombre, apellido, email) VALUES (%s, %s, %s, %s, %s)",
            (
                username,
                hashed_password,
                username,
                username,
                f"{username}@sin-email.local",
            ),
        )
        conn.commit()
        conn.close()


class ImageAnalysisRepository(BaseRepository):
    """Encapsulates image analysis queries."""

    def save_analysis(self, usuario_id: int, nombre_archivo: str | None, descripcion: str, pregunta: str, historia: str) -> None:
        conn = self._connect()
        cursor = conn.cursor()
        cursor.execute(
            "INSERT INTO analisis_imagenes (usuario_id, nombre_archivo, descripcion, pregunta, historia) VALUES (%s, %s, %s, %s, %s)",
            (usuario_id, nombre_archivo, descripcion, pregunta, historia),
        )
        conn.commit()
        conn.close()

    def find_by_user(self, usuario_id: int) -> list[dict]:
        conn = self._connect()
        cursor = conn.cursor(dictionary=True)
        cursor.execute(
            "SELECT id, nombre_archivo, descripcion, pregunta, historia, created_at FROM analisis_imagenes WHERE usuario_id = %s ORDER BY created_at DESC",
            (usuario_id,),
        )
        rows = cursor.fetchall()
        conn.close()
        return rows


class BaseService(ABC):
    """Base service demonstrating inheritance."""

    @abstractmethod
    def execute(self, *args, **kwargs):
        raise NotImplementedError


class TokenManager:
    """Encapsulates JWT creation and validation."""

    def __init__(self, secret: str, expire_minutes: int):
        self._secret = secret
        self._expire_minutes = expire_minutes

    def create_token(self, payload: dict) -> str:
        data = dict(payload)
        data["exp"] = datetime.now(timezone.utc) + timedelta(minutes=self._expire_minutes)
        return jwt.encode(data, self._secret, algorithm="HS256")

    def decode_token(self, token: str) -> dict:
        try:
            return jwt.decode(token, self._secret, algorithms=["HS256"])
        except JWTError as exc:
            raise HTTPException(
                status_code=status.HTTP_401_UNAUTHORIZED,
                detail="Token invalido",
            ) from exc


class AuthService(BaseService):
    """Authentication and registration service."""

    def __init__(self, user_repository: UserRepository, token_manager: TokenManager):
        self._user_repository = user_repository
        self._token_manager = token_manager

    def execute(self, *args, **kwargs):
        return self.authenticate_user(*args, **kwargs)

    def register_user(self, username: str, password: str) -> dict:
        if self._user_repository.find_by_username(username):
            raise HTTPException(status_code=status.HTTP_400_BAD_REQUEST, detail="El usuario ya existe")

        hashed_password = bcrypt.hashpw(password.encode(), bcrypt.gensalt()).decode()
        self._user_repository.save_user(username, hashed_password)
        return {"ok": True, "mensaje": "Usuario registrado correctamente"}

    def authenticate_user(self, username: str, password: str) -> dict:
        user = self._user_repository.find_by_username(username)
        if not user or not self._verify_password(password, user["password"]) or not user.get("activo"):
            raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Credenciales invalidas")

        token_payload = {
            "sub": user["username"],
            "user_id": user["id"],
            "nombre": user["nombre"],
            "apellido": user["apellido"],
            "email": user["email"],
        }
        return {
            "access_token": self._token_manager.create_token(token_payload),
            "token_type": "bearer",
            "user": {
                "id": user["id"],
                "username": user["username"],
                "nombre": user["nombre"],
                "apellido": user["apellido"],
                "email": user["email"],
            },
        }

    def current_user(self, request: Request) -> dict:
        token = self._extract_token(request)
        if not token:
            raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Token faltante")
        return self._token_manager.decode_token(token)

    @staticmethod
    def _verify_password(plain: str, hashed: str) -> bool:
        try:
            return bcrypt.checkpw(plain.encode(), hashed.encode())
        except Exception:
            return False

    @staticmethod
    def _extract_token(request: Request) -> str:
        auth_header = request.headers.get("authorization", "")
        token_header = request.headers.get("x-access-token", "")
        if auth_header.lower().startswith("bearer "):
            return auth_header.split(" ", 1)[1].strip()
        if token_header.lower().startswith("bearer "):
            return token_header.split(" ", 1)[1].strip()
        return token_header.strip()


class ImageAnalysisService(BaseService):
    """Image analysis service and result storage."""

    def __init__(self, analysis_repository: ImageAnalysisRepository, analyzer: AnthropicAnalyzer, token_manager: TokenManager):
        self._analysis_repository = analysis_repository
        self._analyzer = analyzer
        self._token_manager = token_manager

    def execute(self, *args, **kwargs):
        return self.analyze_image(*args, **kwargs)

    def analyze_image(self, imagen_base64: str, nombre_archivo: str | None, request: Request) -> dict:
        usuario = self._get_current_user(request)
        resultado = self._analyzer.analyze(imagen_base64, nombre_archivo)
        self._analysis_repository.save_analysis(
            usuario_id=usuario["user_id"],
            nombre_archivo=nombre_archivo,
            descripcion=resultado["descripcion"],
            pregunta=resultado["pregunta"],
            historia=resultado["historia"],
        )
        return resultado

    def get_user_analyses(self, request: Request) -> list[dict]:
        usuario = self._get_current_user(request)
        return self._analysis_repository.find_by_user(usuario["user_id"])

    def _get_current_user(self, request: Request) -> dict:
        token = AuthService._extract_token(request)
        if not token:
            raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Token faltante")
        return self._token_manager.decode_token(token)


class HealthService:
    """Health service for server metrics."""

    @staticmethod
    def get_status() -> dict:
        return {
            "status": "ok",
            "cpu": psutil.cpu_percent(),
            "ram": psutil.virtual_memory().percent,
            "disk": psutil.disk_usage("/").percent,
        }


# --------- MODELOS ---------

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


# --------- INSTANCIAS ---------

database = Database(AppConfig.db_config())
user_repository = UserRepository(database)
analysis_repository = ImageAnalysisRepository(database)
token_manager = TokenManager(AppConfig.jwt_secret(), AppConfig.jwt_expire_minutes())
auth_service = AuthService(user_repository, token_manager)
image_analysis_service = ImageAnalysisService(analysis_repository, AnthropicAnalyzer(), token_manager)
health_service = HealthService()


# --------- APP ---------

app = FastAPI(title="VPS-POO API", root_path="/api")
app.add_middleware(
    CORSMiddleware,
    allow_origins=AppConfig.cors_origins(),
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.get("/health")
def health():
    return health_service.get_status()


@app.post("/auth/registro")
def registro(payload: RegistroRequest):
    return auth_service.register_user(payload.username, payload.password)


@app.post("/auth/login", response_model=LoginResponse)
def login(payload: LoginRequest):
    return auth_service.authenticate_user(payload.username, payload.password)


@app.get("/auth/me")
def me(request: Request):
    return auth_service.current_user(request)


@app.post("/analizar-imagen")
def analizar_imagen(payload: ImagenRequest, request: Request):
    try:
        return image_analysis_service.analyze_image(payload.imagen_base64, payload.nombre_archivo, request)
    except AnthropicAnalyzerError as exc:
        raise HTTPException(status_code=exc.status_code, detail=exc.message) from exc


@app.get("/mis-analisis")
def mis_analisis(request: Request):
    return image_analysis_service.get_user_analyses(request)
