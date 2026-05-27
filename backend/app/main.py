from datetime import datetime, timedelta, timezone
import os
import psutil
from pydantic import BaseModel
from fastapi import FastAPI, HTTPException, Request, status
from fastapi.middleware.cors import CORSMiddleware
from jose import JWTError, jwt
import bcrypt
import mysql.connector

app = FastAPI(title="VPS-POO API", root_path="/api")

JWT_SECRET = os.getenv("JWT_SECRET", "dev_secret_change_me")
JWT_EXPIRE_MINUTES = int(os.getenv("JWT_EXPIRE_MINUTES", "60"))

DB_CONFIG = {
    "host": os.getenv("DB_HOST", "db"),
    "port": int(os.getenv("DB_PORT", "3306")),
    "user": os.getenv("DB_USER", "poo_user"),
    "password": os.getenv("DB_PASSWORD", "poo_pass"),
    "database": os.getenv("DB_NAME", "vps-poo"),
}

cors_origins = [o.strip() for o in os.getenv("CORS_ORIGINS", "").split(",") if o.strip()]
if not cors_origins:
    cors_origins = ["*"]

app.add_middleware(
    CORSMiddleware,
    allow_origins=cors_origins,
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)

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
    nombre: str
    apellido: str
    email: str

class ImagenRequest(BaseModel):
    imagen_base64: str
    nombre_archivo: str | None = None

# --------- DB ---------

def get_db():
    return mysql.connector.connect(**DB_CONFIG)

# --------- HELPERS ---------

def get_user_by_username(username: str):
    conn = get_db()
    cursor = conn.cursor(dictionary=True)
    cursor.execute(
        "SELECT * FROM usuarios WHERE username = %s LIMIT 1",
        (username,),
    )
    user = cursor.fetchone()
    conn.close()
    return user

def verify_password(plain, hashed):
    try:
        return bcrypt.checkpw(plain.encode(), hashed.encode())
    except:
        return False

def create_access_token(payload: dict):
    exp = datetime.now(timezone.utc) + timedelta(minutes=JWT_EXPIRE_MINUTES)
    payload.update({"exp": exp})
    return jwt.encode(payload, JWT_SECRET, algorithm="HS256")

def decode_token(token: str):
    try:
        return jwt.decode(token, JWT_SECRET, algorithms=["HS256"])
    except JWTError as exc:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Token invalido",
        ) from exc

def get_current_user(request: Request):
    auth_header = request.headers.get("authorization", "")
    token_header = request.headers.get("x-access-token", "")

    token = ""
    if auth_header.lower().startswith("bearer "):
        token = auth_header.split(" ", 1)[1].strip()
    elif token_header.lower().startswith("bearer "):
        token = token_header.split(" ", 1)[1].strip()
    else:
        token = token_header.strip()

    if not token:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Token faltante")

    return decode_token(token)

# --------- HEALTH ---------

@app.get("/health")
def health():
    return {
        "status": "ok",
        "cpu": psutil.cpu_percent(),
        "ram": psutil.virtual_memory().percent,
        "disk": psutil.disk_usage('/').percent
    }

# --------- AUTH ---------

@app.post("/auth/registro")
def registro(payload: RegistroRequest):
    if get_user_by_username(payload.username):
        raise HTTPException(status_code=400, detail="El usuario ya existe")

    hashed = bcrypt.hashpw(payload.password.encode(), bcrypt.gensalt()).decode()

    conn = get_db()
    cursor = conn.cursor()
    cursor.execute(
        "INSERT INTO usuarios (username, password, nombre, apellido, email) VALUES (%s, %s, %s, %s, %s)",
        (payload.username, hashed, payload.nombre, payload.apellido, payload.email),
    )
    conn.commit()
    conn.close()

    return {"ok": True, "mensaje": "Usuario registrado correctamente"}

@app.post("/auth/login", response_model=LoginResponse)
def login(payload: LoginRequest):
    user = get_user_by_username(payload.username)
    if not user or not verify_password(payload.password, user["password"]) or not user["activo"]:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Credenciales invalidas")

    token_payload = {
        "sub": user["username"],
        "user_id": user["id"],
        "nombre": user["nombre"],
        "apellido": user["apellido"],
        "email": user["email"],
    }
    access_token = create_access_token(token_payload)

    return {
        "access_token": access_token,
        "token_type": "bearer",
        "user": {
            "id": user["id"],
            "username": user["username"],
            "nombre": user["nombre"],
            "apellido": user["apellido"],
            "email": user["email"],
        },
    }

@app.get("/auth/me")
def me(request: Request):
    payload = get_current_user(request)
    return {
        "username": payload.get("sub"),
        "nombre": payload.get("nombre"),
        "apellido": payload.get("apellido"),
        "email": payload.get("email"),
    }

# --------- IMAGENES ---------

@app.post("/analizar-imagen")
def analizar_imagen(payload: ImagenRequest, request: Request):
    usuario = get_current_user(request)
    usuario_id = usuario.get("user_id")

    # TODO: reemplazar con llamada real a OpenAI cuando tengamos la API key
    descripcion = "Veo un dibujo muy colorido con figuras interesantes."
    pregunta = "¿Podés contarme qué quisiste dibujar en esta imagen?"
    historia = "Había una vez un dibujo mágico que cobró vida y empezó a explorar el mundo..."

    conn = get_db()
    cursor = conn.cursor()
    cursor.execute(
        "INSERT INTO analisis_imagenes (usuario_id, nombre_archivo, descripcion, pregunta, historia) VALUES (%s, %s, %s, %s, %s)",
        (usuario_id, payload.nombre_archivo, descripcion, pregunta, historia),
    )
    conn.commit()
    conn.close()

    return {
        "descripcion": descripcion,
        "pregunta": pregunta,
        "historia": historia,
    }

@app.get("/mis-analisis")
def mis_analisis(request: Request):
    usuario = get_current_user(request)
    usuario_id = usuario.get("user_id")

    conn = get_db()
    cursor = conn.cursor(dictionary=True)
    cursor.execute(
        "SELECT id, nombre_archivo, descripcion, pregunta, historia, created_at FROM analisis_imagenes WHERE usuario_id = %s ORDER BY created_at DESC",
        (usuario_id,),
    )
    data = cursor.fetchall()
    conn.close()

    return data