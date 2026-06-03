#include <microhttpd.h>
#include <mysql/mysql.h>
#include <curl/curl.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// Alias para facilitar el uso de JSON en todo el archivo.
using json = nlohmann::json;

// Funciones utilitarias de bajo nivel utilizadas por varios componentes.
static std::string to_lower(const std::string& s) {
    std::string out = s;
    for (auto& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

static std::string random_string(size_t length) {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<size_t> distribution(0, sizeof(charset) - 2);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[distribution(generator)];
    }
    return result;
}

static std::string hex_encode(const std::string& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : data) {
        oss << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
}

static std::string sha256_hex(const std::string& data) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int length = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, data.data(), data.size());
    EVP_DigestFinal_ex(ctx, hash, &length);
    EVP_MD_CTX_free(ctx);
    return hex_encode(std::string(reinterpret_cast<char*>(hash), length));
}

static std::string base64url_encode(const std::string& data) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    BIO_write(bio, data.data(), static_cast<int>(data.size()));
    BIO_flush(bio);
    BUF_MEM* buffer;
    BIO_get_mem_ptr(bio, &buffer);
    std::string encoded(buffer->data, buffer->length);
    BIO_free_all(bio);
    for (auto& c : encoded) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!encoded.empty() && encoded.back() == '=') encoded.pop_back();
    return encoded;
}

static std::string base64url_decode(const std::string& input) {
    std::string encoded = input;
    for (auto& c : encoded) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    while (encoded.size() % 4 != 0) encoded += '=';
    BIO* bio = BIO_new_mem_buf(encoded.data(), static_cast<int>(encoded.size()));
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    std::string decoded(encoded.size(), '\0');
    int length = BIO_read(bio, decoded.data(), static_cast<int>(decoded.size()));
    BIO_free_all(bio);
    if (length < 0) return {};
    decoded.resize(length);
    return decoded;
}

static std::string hmac_sha256(const std::string& key, const std::string& data) {
    unsigned char* result = HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
                                reinterpret_cast<const unsigned char*>(data.data()), static_cast<int>(data.size()), nullptr, nullptr);
    return std::string(reinterpret_cast<char*>(result), SHA256_DIGEST_LENGTH);
}

// Funciones auxiliares para crear y validar tokens JWT personalizados.
static json create_jwt_payload(const json& payload, const std::string& secret, int expire_minutes) {
    auto now = std::chrono::system_clock::now();
    auto exp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() + expire_minutes * 60;
    json full_payload = payload;
    full_payload["exp"] = exp;
    json header = { {"alg", "HS256"}, {"typ", "JWT"} };
    std::string header_encoded = base64url_encode(header.dump());
    std::string payload_encoded = base64url_encode(full_payload.dump());
    std::string signature = hmac_sha256(secret, header_encoded + "." + payload_encoded);
    std::string jwt = header_encoded + "." + payload_encoded + "." + base64url_encode(signature);
    return { {"token", jwt}, {"payload", full_payload} };
}

static json decode_jwt(const std::string& token, const std::string& secret) {
    auto parts = std::vector<std::string>{};
    std::string part;
    std::istringstream ss(token);
    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }
    if (parts.size() != 3) {
        throw std::runtime_error("Token invalido");
    }
    std::string header = base64url_decode(parts[0]);
    std::string payload = base64url_decode(parts[1]);
    std::string signature = base64url_decode(parts[2]);
    std::string expected = hmac_sha256(secret, parts[0] + "." + parts[1]);
    if (signature != expected) {
        throw std::runtime_error("Firma de token invalida");
    }
    json data = json::parse(payload);
    long long exp = data.value("exp", 0LL);
    long long now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (exp < now) {
        throw std::runtime_error("Token expirado");
    }
    return data;
}

// Normaliza la cadena base64 de imagen eliminando metadatos y espacios.
static std::string normalize_base64(const std::string& image_base64) {
    std::string data = image_base64;
    auto pos = data.find(',');
    if (pos != std::string::npos) {
        data = data.substr(pos + 1);
    }
    data.erase(std::remove_if(data.begin(), data.end(), [](unsigned char c){ return std::isspace(c); }), data.end());
    return data;
}

static std::string media_type_from_filename(const std::string& filename) {
    auto ext = std::filesystem::path(filename).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    if (ext == ".webp") return "image/webp";
    return "image/jpeg";
}

// Buffer global utilizado por la llamada a cURL para recopilar la respuesta HTTP.
static std::vector<char> curl_response_buffer;

// Callback que recibe fragmentos de datos devueltos por cURL.
static size_t curl_write_cb(char* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    auto* buffer = static_cast<std::vector<char>*>(userp);
    buffer->insert(buffer->end(), contents, contents + total_size);
    return total_size;
}

// Configuración de conexión a la base de datos MySQL.
struct DbConfig {
    std::string host;
    int port;
    std::string user;
    std::string password;
    std::string database;
};

// Clase responsable de abrir conexiones MySQL según una configuración.
class Database {
public:
    explicit Database(const DbConfig& config) : config_(config) {}

    MYSQL* connect() const {
        MYSQL* conn = mysql_init(nullptr);
        if (!conn) {
            throw std::runtime_error("No se pudo inicializar MySQL");
        }
        if (!mysql_real_connect(conn, config_.host.c_str(), config_.user.c_str(), config_.password.c_str(), config_.database.c_str(), config_.port, nullptr, 0)) {
            std::string error = mysql_error(conn);
            mysql_close(conn);
            throw std::runtime_error("MySQL connection failed: " + error);
        }
        return conn;
    }

private:
    DbConfig config_;
};

// Clase base que mantiene la dependencia hacia Database para repositorios.
class BaseRepository {
public:
    explicit BaseRepository(const Database& database) : db_(&database) {}
    virtual ~BaseRepository() = default;

protected:
    const Database* db_;
};

// Interfaz para analizadores de imagen que devuelven JSON.
class IAnalyzer {
public:
    virtual ~IAnalyzer() = default;
    virtual json analyze(const std::string& image_base64, const std::string& filename) const = 0;
};

// Interfaz genérica para servicios de negocio que reciben un JSON y devuelven JSON.
class BaseService {
public:
    virtual ~BaseService() = default;
    virtual json execute(const json& payload) = 0;
};

// Clase auxiliar para crear y validar tokens JWT simples.
class TokenManager {
public:
    TokenManager(const std::string& secret, int expire_minutes)
        : secret_(secret), expire_minutes_(expire_minutes) {}

    std::string create_token(const json& payload) const {
        json data = payload;
        data["exp"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() + expire_minutes_ * 60;
        json header = {{"alg", "HS256"}, {"typ", "JWT"}};
        std::string header_encoded = base64url_encode(header.dump());
        std::string payload_encoded = base64url_encode(data.dump());
        std::string signature = hmac_sha256(secret_, header_encoded + "." + payload_encoded);
        return header_encoded + "." + payload_encoded + "." + base64url_encode(signature);
    }

    json decode_token(const std::string& token) const {
        if (token.empty()) {
            throw std::runtime_error("Token faltante");
        }
        auto parts = std::vector<std::string>{};
        std::string part;
        std::istringstream ss(token);
        while (std::getline(ss, part, '.')) {
            parts.push_back(part);
        }
        if (parts.size() != 3) {
            throw std::runtime_error("Token invalido");
        }
        std::string payload = base64url_decode(parts[1]);
        std::string signature = base64url_decode(parts[2]);
        std::string expected = hmac_sha256(secret_, parts[0] + "." + parts[1]);
        if (signature != expected) {
            throw std::runtime_error("Firma de token invalida");
        }
        json data = json::parse(payload);
        long long exp = data.value("exp", 0LL);
        long long now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (exp < now) {
            throw std::runtime_error("Token expirado");
        }
        return data;
    }

private:
    std::string secret_;
    int expire_minutes_;
};

// Repositorio responsable de operaciones de usuarios en MySQL.
class UserRepository : public BaseRepository {
public:
    explicit UserRepository(const Database& database) : BaseRepository(database) {}

    json find_by_username(const std::string& username) const {
        MYSQL* conn = db_->connect();
        std::string query = "SELECT id, username, password, nombre, apellido, email, COALESCE(activo, 1) AS activo FROM usuarios WHERE username = '" + mysql_real_escape_string_quote(conn, username) + "' LIMIT 1";
        if (mysql_query(conn, query.c_str())) {
            std::string error = mysql_error(conn);
            mysql_close(conn);
            throw std::runtime_error("MySQL query error: " + error);
        }
        MYSQL_RES* res = mysql_store_result(conn);
        json user;
        if (res && mysql_num_rows(res) == 1) {
            MYSQL_ROW row = mysql_fetch_row(res);
            user = {
                {"id", std::stoi(row[0])},
                {"username", row[1] ? row[1] : ""},
                {"password", row[2] ? row[2] : ""},
                {"nombre", row[3] ? row[3] : ""},
                {"apellido", row[4] ? row[4] : ""},
                {"email", row[5] ? row[5] : ""},
                {"activo", row[6] ? std::string(row[6]) == "1" : true}
            };
        }
        if (res) mysql_free_result(res);
        mysql_close(conn);
        return user;
    }

    void save_user(const std::string& username, const std::string& hashed_password) const {
        MYSQL* conn = db_->connect();
        std::string escaped_username = mysql_real_escape_string_quote(conn, username);
        std::string escaped_password = mysql_real_escape_string_quote(conn, hashed_password);
        std::string email = escaped_username + "@sin-email.local";
        std::string query = "INSERT INTO usuarios (username, password, nombre, apellido, email) VALUES ('" + escaped_username + "', '" + escaped_password + "', '" + escaped_username + "', '" + escaped_username + "', '" + email + "')";
        if (mysql_query(conn, query.c_str())) {
            std::string error = mysql_error(conn);
            mysql_close(conn);
            throw std::runtime_error("MySQL insert error: " + error);
        }
        mysql_close(conn);
    }

private:
    static std::string mysql_real_escape_string_quote(MYSQL* conn, const std::string& input) {
        std::string buffer(input.size() * 2 + 1, '\0');
        unsigned long length = mysql_real_escape_string(conn, buffer.data(), input.c_str(), static_cast<unsigned long>(input.size()));
        buffer.resize(length);
        return buffer;
    }
};

// Repositorio responsable de almacenar y recuperar análisis de imágenes.
class ImageAnalysisRepository : public BaseRepository {
public:
    explicit ImageAnalysisRepository(const Database& database) : BaseRepository(database) {}

    void save_analysis(int user_id, const std::string& filename, const std::string& descripcion,
                       const std::string& pregunta, const std::string& historia) const {
        MYSQL* conn = db_->connect();
        std::string qfilename = escape(conn, filename);
        std::string qdescripcion = escape(conn, descripcion);
        std::string qpregunta = escape(conn, pregunta);
        std::string qhistoria = escape(conn, historia);
        std::string query = "INSERT INTO analisis_imagenes (usuario_id, nombre_archivo, descripcion, pregunta, historia) VALUES (" + std::to_string(user_id) + ", '" + qfilename + "', '" + qdescripcion + "', '" + qpregunta + "', '" + qhistoria + "')";
        if (mysql_query(conn, query.c_str())) {
            std::string error = mysql_error(conn);
            mysql_close(conn);
            throw std::runtime_error("MySQL insert error: " + error);
        }
        mysql_close(conn);
    }

    json find_by_user(int user_id) const {
        MYSQL* conn = db_->connect();
        std::string query = "SELECT id, nombre_archivo, descripcion, pregunta, historia, created_at FROM analisis_imagenes WHERE usuario_id = " + std::to_string(user_id) + " ORDER BY created_at DESC";
        if (mysql_query(conn, query.c_str())) {
            std::string error = mysql_error(conn);
            mysql_close(conn);
            throw std::runtime_error("MySQL query error: " + error);
        }
        MYSQL_RES* res = mysql_store_result(conn);
        json results = json::array();
        if (res) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                results.push_back({
                    {"id", row[0] ? std::stoi(row[0]) : 0},
                    {"nombre_archivo", row[1] ? row[1] : ""},
                    {"descripcion", row[2] ? row[2] : ""},
                    {"pregunta", row[3] ? row[3] : ""},
                    {"historia", row[4] ? row[4] : ""},
                    {"created_at", row[5] ? row[5] : ""}
                });
            }
            mysql_free_result(res);
        }
        mysql_close(conn);
        return results;
    }

private:
    static std::string escape(MYSQL* conn, const std::string& value) {
        std::string buffer(value.size() * 2 + 1, '\0');
        unsigned long len = mysql_real_escape_string(conn, buffer.data(), value.c_str(), static_cast<unsigned long>(value.size()));
        buffer.resize(len);
        return buffer;
    }

};

// Servicio de negocio que ejecuta el análisis de imagen combinando autenticación y análisis.
class ImageAnalysisService : public BaseService {
public:
    ImageAnalysisService(const ImageAnalysisRepository& analysis_repository,
                         std::shared_ptr<IAnalyzer> analyzer,
                         const TokenManager& token_manager)
        : analysis_repository_(analysis_repository), analyzer_(std::move(analyzer)), token_manager_(token_manager) {}

    json execute(const json& payload) override {
        std::string token = payload.value("token", "");
        return analyze_image(payload.value("imagen_base64", ""), payload.value("nombre_archivo", ""), token);
    }

    json analyze_image(const std::string& imagen_base64, const std::string& nombre_archivo, const std::string& token) const {
        json user = get_current_user(token);
        json result = analyzer_->analyze(imagen_base64, nombre_archivo);
        analysis_repository_.save_analysis(
            user.value("user_id", 0),
            nombre_archivo,
            result["descripcion"].get<std::string>(),
            result["pregunta"].get<std::string>(),
            result["historia"].get<std::string>()
        );
        return result;
    }

private:
    json get_current_user(const std::string& token) const {
        return token_manager_.decode_token(token);
    }

    const ImageAnalysisRepository& analysis_repository_;
    std::shared_ptr<IAnalyzer> analyzer_;
    const TokenManager& token_manager_;
};

// Servicio de negocio para registro, login y validación de token.
class AuthService : public BaseService {
public:
    AuthService(const UserRepository& user_repo, const TokenManager& token_manager)
        : user_repository_(user_repo), token_manager_(token_manager) {}

    json register_user(const std::string& username, const std::string& password) const {
        if (!username.size() || !password.size()) {
            throw std::runtime_error("username y password son requeridos");
        }
        if (!user_repository_.find_by_username(username).is_null()) {
            throw std::runtime_error("El usuario ya existe");
        }
        std::string hashed = hash_password(password);
        user_repository_.save_user(username, hashed);
        return { {"ok", true}, {"mensaje", "Usuario registrado correctamente"} };
    }

    json authenticate_user(const std::string& username, const std::string& password) const {
        json user = user_repository_.find_by_username(username);
        if (user.is_null() || !verify_password(password, user["password"].get<std::string>()) || !user.value("activo", true)) {
            throw std::runtime_error("Credenciales invalidas");
        }
        json payload = {
            {"sub", user["username"]},
            {"user_id", user["id"]},
            {"nombre", user["nombre"]},
            {"apellido", user["apellido"]},
            {"email", user["email"]}
        };
        return {
            {"access_token", token_manager_.create_token(payload)},
            {"token_type", "bearer"},
            {"user", {
                {"id", user["id"]},
                {"username", user["username"]},
                {"nombre", user["nombre"]},
                {"apellido", user["apellido"]},
                {"email", user["email"]}
            }}
        };
    }

    json current_user(const std::string& token) const {
        return token_manager_.decode_token(token);
    }

    json execute(const json& payload) override {
        return authenticate_user(payload.value("username", ""), payload.value("password", ""));
    }

private:
    std::string hash_password(const std::string& password) const {
        std::string salt = random_string(16);
        std::string digest = sha256_hex(salt + password);
        return salt + "$" + digest;
    }

    bool verify_password(const std::string& plain, const std::string& hashed) const {
        auto pos = hashed.find('$');
        if (pos == std::string::npos) return false;
        std::string salt = hashed.substr(0, pos);
        std::string expected = hashed.substr(pos + 1);
        return sha256_hex(salt + plain) == expected;
    }

    const UserRepository& user_repository_;
    const TokenManager& token_manager_;
};

// Implementación concreta de IAnalyzer que llama al servicio Anthropic.
class AnthropicAnalyzer : public IAnalyzer {
public:
    AnthropicAnalyzer(const std::string& api_key, const std::string& model, int max_tokens)
        : api_key_(api_key), model_(model), max_tokens_(max_tokens) {}

    json analyze(const std::string& image_base64, const std::string& filename) const {
        if (api_key_.empty()) {
            throw std::runtime_error("ANTHROPIC_API_KEY no configurada en el servidor");
        }
        std::string image_data = normalize_base64(image_base64);
        std::string media_type = media_type_from_filename(filename);

        json payload = {
            {"model", model_},
            {"max_tokens", max_tokens_},
            {"system", prompt_sistema()},
            {"messages", json::array({
                {
                    {"role", "user"},
                    {"content", json::array({
                        {
                            {"type", "image"},
                            {"source", {
                                {"type", "base64"},
                                {"media_type", media_type},
                                {"data", image_data}
                            }}
                        },
                        {
                            {"type", "text"},
                            {"text", "Analizá esta imagen y devolvé el JSON pedido."}
                        }
                    })}
                }
            })}
        };

        std::string response_text = call_anthropic(payload.dump());
        json response_json = json::parse(response_text);
        std::string text = extract_text_from_response(response_json);
        return parse_analysis_json(text);
    }

private:
    static std::string prompt_sistema() {
        return "Sos un tutor visual parlante para un centro educativo infantil.\n"
               "Mira la imagen (dibujo o foto de un niño) y genera contenido en español rioplatense, simple y cálido.\n\n"
               "Responde ÚNICAMENTE con un objeto JSON válido (sin markdown, sin texto extra) con estas claves:\n"
               "- \"descripcion\": qué ves en la imagen, 2 o 3 oraciones cortas.\n"
               "- \"pregunta\": una pregunta abierta y amable para que el niño hable de su dibujo.\n"
               "- \"historia\": un cuento corto de 4 a 6 oraciones inspirado en la imagen, apto para niños pequeños.";
    }

    std::string call_anthropic(const std::string& payload_json) const {
        curl_response_buffer.clear();
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("Error al inicializar cURL");
        }

        struct curl_slist* headers = nullptr;
        std::string api_key_header = "x-api-key: " + api_key_;
        headers = curl_slist_append(headers, api_key_header.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.anthropic.com/v1/messages");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_json.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_response_buffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            throw std::runtime_error(std::string("Error al llamar a Anthropic: ") + curl_easy_strerror(res));
        }

        return std::string(curl_response_buffer.begin(), curl_response_buffer.end());
    }

    json extract_text_from_response(const json& response) const {
        if (response.contains("completion") && response["completion"].is_string()) {
            return response["completion"].get<std::string>();
        }
        if (response.contains("message") && response["message"].contains("content")) {
            return response["message"]["content"].dump();
        }
        if (response.contains("choices") && response["choices"].is_array() && !response["choices"].empty()) {
            auto& first = response["choices"][0];
            if (first.contains("message") && first["message"].contains("content")) {
                return first["message"]["content"];
            }
            if (first.contains("text")) {
                return first["text"];
            }
        }
        throw std::runtime_error("Anthropic no devolvió texto en la respuesta");
    }

    json parse_analysis_json(const std::string& text) const {
        std::string clean = text;
        if (!clean.empty() && clean.front() == '`') {
            while (!clean.empty() && clean.front() == '`') clean.erase(clean.begin());
            while (!clean.empty() && clean.back() == '`') clean.pop_back();
        }
        json data = json::parse(clean);
        for (auto key : {"descripcion", "pregunta", "historia"}) {
            if (!data.contains(key) || !data[key].is_string() || data[key].get<std::string>().empty()) {
                throw std::runtime_error(std::string("Falta o es invalida la clave '") + key + "' en la respuesta");
            }
        }
        return {
            {"descripcion", data["descripcion"].get<std::string>()},
            {"pregunta", data["pregunta"].get<std::string>()},
            {"historia", data["historia"].get<std::string>()}
        };
    }

    std::string api_key_;
    std::string model_;
    int max_tokens_;
};

// Servicio de salud para exponer métricas básicas del sistema.
class HealthService {
public:
    json get_status() const {
        struct sysinfo info;
        if (sysinfo(&info) != 0) {
            throw std::runtime_error("No se pudo leer el estado del sistema");
        }
        double ram_percent = 100.0 * (info.totalram - info.freeram) / info.totalram;
        int cores = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
        double load_avg = 0.0;
        if (getloadavg(&load_avg, 1) != -1) {
            load_avg = (load_avg / std::max(1, cores)) * 100.0;
        }
        struct statvfs fsinfo;
        if (statvfs("/", &fsinfo) != 0) {
            throw std::runtime_error("No se pudo leer el uso de disco");
        }
        double disk_percent = 100.0 * (fsinfo.f_blocks - fsinfo.f_bfree) / fsinfo.f_blocks;
        return {
            {"status", "ok"},
            {"cpu", std::round(load_avg)},
            {"ram", std::round(ram_percent)},
            {"disk", std::round(disk_percent)}
        };
    }
};

struct ConnectionInfo {
    std::string body;
};

// Iterador de cabeceras HTTP usado por libmicrohttpd para leer los headers entrantes.
static int header_iterator(void* cls, enum MHD_ValueKind kind, const char* key, const char* value) {
    if (kind != MHD_HEADER_KIND) return MHD_YES;
    auto headers = static_cast<std::unordered_map<std::string, std::string>*>(cls);
    (*headers)[to_lower(key)] = value ? value : "";
    return MHD_YES;
}

// Añade cabeceras CORS para permitir solicitudes desde el frontend.
static int add_cors_headers(MHD_Response* response) {
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization, X-Access-Token");
    return MHD_YES;
}

static int send_json_response(struct MHD_Connection* connection, const json& payload, int status_code) {
    std::string body = payload.dump();
    struct MHD_Response* response = MHD_create_response_from_buffer(body.size(), const_cast<char*>(body.c_str()), MHD_RESPMEM_MUST_COPY);
    if (!response) return MHD_NO;
    MHD_add_response_header(response, "Content-Type", "application/json");
    add_cors_headers(response);
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

static int send_error(struct MHD_Connection* connection, const std::string& message, int status_code) {
    return send_json_response(connection, json({{"detail", message}}), status_code);
}

// Clase principal del servidor API que enruta peticiones HTTP hacia servicios y repositorios.
class ApiServer {
public:
    ApiServer(const DbConfig& db_config, const std::string& jwt_secret, int jwt_expire_minutes,
              const std::string& anthropic_api_key, const std::string& anthropic_model, int anthropic_max_tokens)
        : database_(db_config),
          user_repository_(database_),
          analysis_repository_(database_),
          token_manager_(jwt_secret, jwt_expire_minutes),
          auth_service_(user_repository_, token_manager_),
          image_analysis_service_(analysis_repository_, std::make_shared<AnthropicAnalyzer>(anthropic_api_key, anthropic_model, anthropic_max_tokens), token_manager_) {}

    int handle_request(struct MHD_Connection* connection, const char* url, const char* method,
                       const std::string& body, const std::unordered_map<std::string, std::string>& headers, json& output, int& status_code) {
        try {
            if (strcmp(method, "OPTIONS") == 0) {
                output = json::object();
                status_code = MHD_HTTP_OK;
                return MHD_YES;
            }
            if (strcmp(method, "GET") == 0 && std::string(url) == "/api/health") {
                output = HealthService().get_status();
                status_code = MHD_HTTP_OK;
                return MHD_YES;
            }
            if (strcmp(method, "POST") == 0 && std::string(url) == "/api/auth/registro") {
                json payload = json::parse(body);
                output = auth_service_.register_user(payload.value("username", ""), payload.value("password", ""));
                status_code = MHD_HTTP_OK;
                return MHD_YES;
            }
            if (strcmp(method, "POST") == 0 && std::string(url) == "/api/auth/login") {
                json payload = json::parse(body);
                output = auth_service_.authenticate_user(payload.value("username", ""), payload.value("password", ""));
                status_code = MHD_HTTP_OK;
                return MHD_YES;
            }
            if (strcmp(method, "GET") == 0 && std::string(url) == "/api/auth/me") {
                output = auth_service_.current_user(extract_token(headers));
                status_code = MHD_HTTP_OK;
                return MHD_YES;
            }
            if (strcmp(method, "POST") == 0 && std::string(url) == "/api/analizar-imagen") {
                json payload = json::parse(body);
                std::string token = extract_token(headers);
                payload["token"] = token;
                json result = image_analysis_service_.execute(payload);
                output = result;
                status_code = MHD_HTTP_OK;
                return MHD_YES;
            }
            if (strcmp(method, "GET") == 0 && std::string(url) == "/api/mis-analisis") {
                json user = auth_service_.current_user(extract_token(headers));
                output = analysis_repository_.find_by_user(user.value("user_id", 0));
                status_code = MHD_HTTP_OK;
                return MHD_YES;
            }
            return MHD_NO;
        } catch (const std::exception& ex) {
            status_code = MHD_HTTP_BAD_REQUEST;
            output = json({{"detail", ex.what()}});
            return MHD_YES;
        }
    }

private:
    std::string extract_token(const std::unordered_map<std::string, std::string>& headers) const {
        auto it = headers.find("authorization");
        if (it != headers.end() && to_lower(it->second).rfind("bearer ", 0) == 0) {
            return it->second.substr(7);
        }
        it = headers.find("x-access-token");
        if (it != headers.end() && to_lower(it->second).rfind("bearer ", 0) == 0) {
            return it->second.substr(7);
        }
        return it != headers.end() ? it->second : "";
    }

    Database database_;
    UserRepository user_repository_;
    ImageAnalysisRepository analysis_repository_;
    TokenManager token_manager_;
    AuthService auth_service_;
    ImageAnalysisService image_analysis_service_;
};

// Callback principal de libmicrohttpd que construye la petición y llama a ApiServer.
static int answer_to_connection(void* cls, struct MHD_Connection* connection,
                                const char* url, const char* method, const char* version,
                                const char* upload_data, size_t* upload_data_size, void** con_cls) {
    if (*con_cls == nullptr) {
        *con_cls = new ConnectionInfo();
        return MHD_YES;
    }
    auto* info = static_cast<ConnectionInfo*>(*con_cls);
    if (strcmp(method, "POST") == 0 && *upload_data_size > 0) {
        info->body.append(upload_data, *upload_data_size);
        *upload_data_size = 0;
        return MHD_YES;
    }

    std::unordered_map<std::string, std::string> headers;
    MHD_get_connection_values(connection, MHD_HEADER_KIND, header_iterator, &headers);
    json response_body;
    int status_code = MHD_HTTP_OK;
    auto* server = static_cast<ApiServer*>(cls);
    if (server->handle_request(connection, url, method, info->body, headers, response_body, status_code) == MHD_YES) {
        int result = send_json_response(connection, response_body, status_code);
        return result;
    }
    return send_error(connection, "Endpoint no encontrado", MHD_HTTP_NOT_FOUND);
}

// Libera la información asociada a la conexión cuando la petición finaliza.
static void request_completed(void* cls, struct MHD_Connection* connection, void** con_cls, enum MHD_RequestTerminationCode toe) {
    if (*con_cls) {
        delete static_cast<ConnectionInfo*>(*con_cls);
        *con_cls = nullptr;
    }
}

// Punto de entrada del servidor. Lee variables de entorno, crea el servidor y arranca el daemon HTTP.
int main() {
    DbConfig db_config{
        std::getenv("DB_HOST") ? std::getenv("DB_HOST") : "db",
        std::getenv("DB_PORT") ? std::stoi(std::getenv("DB_PORT")) : 3306,
        std::getenv("DB_USER") ? std::getenv("DB_USER") : "poo_user",
        std::getenv("DB_PASSWORD") ? std::getenv("DB_PASSWORD") : "poo_pass",
        std::getenv("DB_NAME") ? std::getenv("DB_NAME") : "vps-poo"
    };

    std::string jwt_secret = std::getenv("JWT_SECRET") ? std::getenv("JWT_SECRET") : "dev_secret_change_me";
    int jwt_expire_minutes = std::getenv("JWT_EXPIRE_MINUTES") ? std::stoi(std::getenv("JWT_EXPIRE_MINUTES")) : 60;
    std::string anthropic_api_key = std::getenv("ANTHROPIC_API_KEY") ? std::getenv("ANTHROPIC_API_KEY") : "";
    std::string anthropic_model = std::getenv("ANTHROPIC_MODEL") ? std::getenv("ANTHROPIC_MODEL") : "claude-sonnet-4-6";
    int anthropic_max_tokens = std::getenv("ANTHROPIC_MAX_TOKENS") ? std::stoi(std::getenv("ANTHROPIC_MAX_TOKENS")) : 1024;

    ApiServer server(db_config, jwt_secret, jwt_expire_minutes, anthropic_api_key, anthropic_model, anthropic_max_tokens);

    struct MHD_Daemon* daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, 8000, nullptr, nullptr,
                                                 &answer_to_connection, &server,
                                                 MHD_OPTION_NOTIFY_COMPLETED, request_completed, nullptr,
                                                 MHD_OPTION_END);
    if (!daemon) {
        std::cerr << "No se pudo iniciar el servidor HTTP" << std::endl;
        return 1;
    }

    std::cout << "Backend C++ iniciado en http://0.0.0.0:8000" << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(24));
    }
    MHD_stop_daemon(daemon);
    return 0;
}
