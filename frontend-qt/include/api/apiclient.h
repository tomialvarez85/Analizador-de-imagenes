#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

class ApiClient : public QObject
{
    Q_OBJECT

public:
    static ApiClient &instance();

    QString token() const;
    void setToken(const QString &token);
    void clearToken();

    void login(const QString &username, const QString &password);
    void registro(const QString &username,
                  const QString &password);
    void analizarImagen(const QString &imagenBase64, const QString &nombreArchivo);

signals:
    void loginSucceeded(const QString &accessToken);
    void loginFailed(const QString &message);

    void registroSucceeded(const QString &message);
    void registroFailed(const QString &message);

    void analisisSucceeded(const QString &descripcion,
                           const QString &pregunta,
                           const QString &historia);
    void analisisFailed(const QString &message);

private:
    explicit ApiClient(QObject *parent = nullptr);

    QNetworkRequest buildJsonRequest(const QString &path, bool authenticated) const;
    void configureSsl(QNetworkReply *reply) const;
    QString parseErrorMessage(const QByteArray &body, int statusCode) const;

    QNetworkAccessManager m_nam;
    QString m_token;
};

#endif // APICLIENT_H
