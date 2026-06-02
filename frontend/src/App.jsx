import { useEffect, useState } from "react";
import { login, me } from "./api.js";

const STORAGE_KEY = "vps_poo_token";

export default function App() {
  const [token, setToken] = useState(localStorage.getItem(STORAGE_KEY));
  const [user, setUser] = useState(null);
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    if (!token) {
      return;
    }

    me(token)
      .then((data) => setUser(data))
      .catch(() => {
        localStorage.removeItem(STORAGE_KEY);
        setToken(null);
      });
  }, [token]);

  const handleSubmit = async (event) => {
    event.preventDefault();
    setError("");
    setLoading(true);

    try {
      const result = await login(username, password);
      localStorage.setItem(STORAGE_KEY, result.access_token);
      setToken(result.access_token);
      setUser(result.user);
    } catch (err) {
      setError(err.message || "No se pudo iniciar sesion");
    } finally {
      setLoading(false);
    }
  };

  const handleLogout = () => {
    localStorage.removeItem(STORAGE_KEY);
    setToken(null);
    setUser(null);
    setUsername("");
    setPassword("");
  };

  const handleCreateAccount = () => {
    alert("Funcionalidad de crear cuenta no implementada.");
  };

  if (token && user) {
    return (
      <main className="page">
        <div className="layout">
          <header className="topbar">
            <span className="brand">VPS-POO</span>
            <button className="logout" onClick={handleLogout}>
              Cerrar sesión
            </button>
          </header>
          <section className="hero card">
            <h1>Bienvenido, {user.nombre || "Usuario"}</h1>
            <p>Acceso correcto. Esta es la portada del sistema.</p>
          </section>
        </div>
      </main>
    );
  }

  return (
    <main className="page">
      <div className="small-screen-notice">
        <div className="notice-card">
          <h2>Interfaz optimizada para escritorio</h2>
          <p>Esta experiencia está diseñada para pantallas grandes. Usa un monitor o amplia la ventana para continuar.</p>
        </div>
      </div>

      <form className="card login-card" onSubmit={handleSubmit}>
        <div className="hero-panel">
          <div className="hero-icon">🎨</div>
          <h1>¡Hola, artista!</h1>
          <p>Bienvenido a la versión horizontal del login. Aquí se combinan el acceso y el registro en una interfaz de escritorio clara y simétrica.</p>
          <div className="hero-info">
            <p>Login y registro alineados en una sola vista.</p>
            <p>Completa tus datos a la derecha y accede rápido.</p>
          </div>
        </div>

        <div className="form-panel">
          <div className="form-header">
            <h2>Ingresar</h2>
            <p>Inicia sesión con tu usuario y contraseña.</p>
          </div>

          <label className="field-label">
            <span>Usuario</span>
            <input
              value={username}
              onChange={(event) => setUsername(event.target.value)}
              autoComplete="username"
              required
            />
          </label>

          <label className="field-label">
            <span>Clave</span>
            <input
              type="password"
              value={password}
              onChange={(event) => setPassword(event.target.value)}
              autoComplete="current-password"
              required
            />
          </label>

          {error ? <p className="error">{error}</p> : null}

          <button className="primary" type="submit" disabled={loading}>
            {loading ? "Ingresando..." : "¡Entrar!"}
          </button>

          <p className="form-help">¿No tenés cuenta? Creala para comenzar a analizar imágenes.</p>
          <button className="secondary" type="button" onClick={handleCreateAccount}>
            ✨ Crear cuenta
          </button>
        </div>
      </form>
    </main>
  );
}
