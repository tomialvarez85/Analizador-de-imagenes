#include "registrowindow.h"
#include "apiclient.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

RegistroWindow::RegistroWindow(QWidget *parent)
    : QWidget(parent)
{
    buildUi();

    connect(&ApiClient::instance(), &ApiClient::registroSucceeded,
            this, &RegistroWindow::onRegistroSucceeded);
    connect(&ApiClient::instance(), &ApiClient::registroFailed,
            this, &RegistroWindow::onRegistroFailed);
}

void RegistroWindow::buildUi()
{
    setWindowTitle(QStringLiteral("Analizador de Imágenes - Registro"));
    setMinimumSize(520, 560);

    auto *title = new QLabel(QStringLiteral("Crear cuenta nueva"), this);
    title->setObjectName(QStringLiteral("titleLabel"));
    title->setAlignment(Qt::AlignCenter);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText(QStringLiteral("Usuario"));

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText(QStringLiteral("Contraseña"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_nombreEdit = new QLineEdit(this);
    m_nombreEdit->setPlaceholderText(QStringLiteral("Nombre"));

    m_apellidoEdit = new QLineEdit(this);
    m_apellidoEdit->setPlaceholderText(QStringLiteral("Apellido"));

    m_emailEdit = new QLineEdit(this);
    m_emailEdit->setPlaceholderText(QStringLiteral("correo@ejemplo.com"));

    m_registrarButton = new QPushButton(QStringLiteral("Registrarse"), this);
    m_registrarButton->setObjectName(QStringLiteral("successButton"));

    m_volverButton = new QPushButton(QStringLiteral("Volver al login"), this);
    m_volverButton->setObjectName(QStringLiteral("secondaryButton"));

    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #636E72; font-size: 14pt;"));

    auto *form = new QFormLayout;
    form->setSpacing(14);
    form->addRow(QStringLiteral("Usuario:"), m_usernameEdit);
    form->addRow(QStringLiteral("Contraseña:"), m_passwordEdit);
    form->addRow(QStringLiteral("Nombre:"), m_nombreEdit);
    form->addRow(QStringLiteral("Apellido:"), m_apellidoEdit);
    form->addRow(QStringLiteral("Email:"), m_emailEdit);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(18);
    layout->addWidget(title);
    layout->addLayout(form);
    layout->addWidget(m_registrarButton, 0, Qt::AlignCenter);
    layout->addWidget(m_volverButton, 0, Qt::AlignCenter);
    layout->addWidget(m_statusLabel);

    connect(m_registrarButton, &QPushButton::clicked, this, &RegistroWindow::onRegistrarseClicked);
    connect(m_volverButton, &QPushButton::clicked, this, &RegistroWindow::onVolverClicked);
}

void RegistroWindow::setBusy(bool busy)
{
    m_registrarButton->setDisabled(busy);
    m_volverButton->setDisabled(busy);
    m_usernameEdit->setDisabled(busy);
    m_passwordEdit->setDisabled(busy);
    m_nombreEdit->setDisabled(busy);
    m_apellidoEdit->setDisabled(busy);
    m_emailEdit->setDisabled(busy);
}

void RegistroWindow::onRegistrarseClicked()
{
    const QString username = m_usernameEdit->text().trimmed();
    const QString password = m_passwordEdit->text();
    const QString nombre = m_nombreEdit->text().trimmed();
    const QString apellido = m_apellidoEdit->text().trimmed();
    const QString email = m_emailEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty() || nombre.isEmpty()
        || apellido.isEmpty() || email.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("Completá todos los campos."));
        return;
    }

    m_statusLabel->setText(QStringLiteral("Registrando..."));
    setBusy(true);
    ApiClient::instance().registro(username, password, nombre, apellido, email);
}

void RegistroWindow::onVolverClicked()
{
    emit backToLoginRequested();
}

void RegistroWindow::onRegistroSucceeded(const QString &message)
{
    setBusy(false);
    m_statusLabel->setText(message);
    QMessageBox::information(this, QStringLiteral("¡Listo!"), message);
    emit backToLoginRequested();
}

void RegistroWindow::onRegistroFailed(const QString &message)
{
    setBusy(false);
    m_statusLabel->setText(message);
    QMessageBox::warning(this, QStringLiteral("No se pudo registrar"), message);
}
