#include "apiclient.h"
#include "config.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QSslSocket>

ApiClient &ApiClient::instance()
{
    static ApiClient client;
    return client;
}

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
{
    QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(ssl);
}

QString ApiClient::token() const
{
    return m_token;
}

void ApiClient::setToken(const QString &token)
{
    m_token = token;
}

void ApiClient::clearToken()
{
    m_token.clear();
}

QNetworkRequest ApiClient::buildJsonRequest(const QString &path, bool authenticated) const
{
    const QUrl url(QString::fromUtf8(AppConfig::apiBaseUrl()) + path);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    if (authenticated && !m_token.isEmpty()) {
        request.setRawHeader("x-access-token",
                             QByteArray("Bearer ") + m_token.toUtf8());
    }

    return request;
}

void ApiClient::configureSsl(QNetworkReply *reply) const
{
    if (!reply) {
        return;
    }
    reply->ignoreSslErrors();
}

QString ApiClient::parseErrorMessage(const QByteArray &body, int statusCode) const
{
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        const QJsonObject obj = doc.object();
        if (obj.contains(QStringLiteral("detail"))) {
            const QJsonValue detail = obj.value(QStringLiteral("detail"));
            if (detail.isString()) {
                return detail.toString();
            }
            if (detail.isArray() && !detail.toArray().isEmpty()) {
                return detail.toArray().first().toObject()
                    .value(QStringLiteral("msg"))
                    .toString(QStringLiteral("Error del servidor"));
            }
        }
        if (obj.contains(QStringLiteral("mensaje"))) {
            return obj.value(QStringLiteral("mensaje")).toString();
        }
    }
    return QStringLiteral("Error del servidor (código %1)").arg(statusCode);
}

void ApiClient::login(const QString &username, const QString &password)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("username"), username);
    payload.insert(QStringLiteral("password"), password);

    QNetworkRequest request = buildJsonRequest(QStringLiteral("/auth/login"), false);
    QNetworkReply *reply = m_nam.post(request, QJsonDocument(payload).toJson());
    configureSsl(reply);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || status >= 400) {
            emit loginFailed(parseErrorMessage(reply->readAll(), status));
            return;
        }

        QJsonParseError parseError{};
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit loginFailed(QStringLiteral("Respuesta de login inválida"));
            return;
        }

        const QString accessToken = doc.object()
                                        .value(QStringLiteral("access_token"))
                                        .toString();
        if (accessToken.isEmpty()) {
            emit loginFailed(QStringLiteral("No se recibió el token de acceso"));
            return;
        }

        m_token = accessToken;
        emit loginSucceeded(accessToken);
    });
}

void ApiClient::registro(const QString &username,
                         const QString &password,
                         const QString &nombre,
                         const QString &apellido,
                         const QString &email)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("username"), username);
    payload.insert(QStringLiteral("password"), password);
    payload.insert(QStringLiteral("nombre"), nombre);
    payload.insert(QStringLiteral("apellido"), apellido);
    payload.insert(QStringLiteral("email"), email);

    QNetworkRequest request = buildJsonRequest(QStringLiteral("/auth/registro"), false);
    QNetworkReply *reply = m_nam.post(request, QJsonDocument(payload).toJson());
    configureSsl(reply);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();

        if (reply->error() != QNetworkReply::NoError || status >= 400) {
            emit registroFailed(parseErrorMessage(body, status));
            return;
        }

        QJsonParseError parseError{};
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        QString mensaje = QStringLiteral("Usuario registrado correctamente");
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            mensaje = doc.object().value(QStringLiteral("mensaje")).toString(mensaje);
        }
        emit registroSucceeded(mensaje);
    });
}

void ApiClient::analizarImagen(const QString &imagenBase64, const QString &nombreArchivo)
{
    if (m_token.isEmpty()) {
        emit analisisFailed(QStringLiteral("Debés iniciar sesión primero"));
        return;
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("imagen_base64"), imagenBase64);
    payload.insert(QStringLiteral("nombre_archivo"), nombreArchivo);

    QNetworkRequest request = buildJsonRequest(QStringLiteral("/analizar-imagen"), true);
    QNetworkReply *reply = m_nam.post(request, QJsonDocument(payload).toJson());
    configureSsl(reply);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();

        if (reply->error() != QNetworkReply::NoError || status >= 400) {
            emit analisisFailed(parseErrorMessage(body, status));
            return;
        }

        QJsonParseError parseError{};
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit analisisFailed(QStringLiteral("Respuesta de análisis inválida"));
            return;
        }

        const QJsonObject obj = doc.object();
        emit analisisSucceeded(
            obj.value(QStringLiteral("descripcion")).toString(),
            obj.value(QStringLiteral("pregunta")).toString(),
            obj.value(QStringLiteral("historia")).toString());
    });
}
