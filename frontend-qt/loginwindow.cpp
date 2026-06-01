#include "loginwindow.h"
#include "apiclient.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
{
    buildUi();

    connect(&ApiClient::instance(), &ApiClient::loginSucceeded,
            this, &LoginWindow::onLoginSucceeded);
    connect(&ApiClient::instance(), &ApiClient::loginFailed,
            this, &LoginWindow::onLoginFailed);
}

void LoginWindow::buildUi()
{
    setWindowTitle(QStringLiteral("Analizador de Imágenes - Ingresar"));
    setMinimumSize(480, 420);

    auto *title = new QLabel(QStringLiteral("¡Hola! Ingresá a jugar"), this);
    title->setObjectName(QStringLiteral("titleLabel"));
    title->setAlignment(Qt::AlignCenter);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText(QStringLiteral("Usuario"));

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText(QStringLiteral("Contraseña"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_ingresarButton = new QPushButton(QStringLiteral("Ingresar"), this);
    m_registrarseButton = new QPushButton(QStringLiteral("Registrarse"), this);
    m_registrarseButton->setObjectName(QStringLiteral("secondaryButton"));

    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #636E72; font-size: 14pt;"));

    auto *form = new QFormLayout;
    form->setSpacing(16);
    form->addRow(QStringLiteral("Usuario:"), m_usernameEdit);
    form->addRow(QStringLiteral("Contraseña:"), m_passwordEdit);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(20);
    layout->addWidget(title);
    layout->addLayout(form);
    layout->addWidget(m_ingresarButton, 0, Qt::AlignCenter);
    layout->addWidget(m_registrarseButton, 0, Qt::AlignCenter);
    layout->addWidget(m_statusLabel);

    connect(m_ingresarButton, &QPushButton::clicked, this, &LoginWindow::onIngresarClicked);
    connect(m_registrarseButton, &QPushButton::clicked, this, &LoginWindow::onRegistrarseClicked);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginWindow::onIngresarClicked);
}

void LoginWindow::setBusy(bool busy)
{
    m_ingresarButton->setDisabled(busy);
    m_registrarseButton->setDisabled(busy);
    m_usernameEdit->setDisabled(busy);
    m_passwordEdit->setDisabled(busy);
}

void LoginWindow::onIngresarClicked()
{
    const QString username = m_usernameEdit->text().trimmed();
    const QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("Completá usuario y contraseña."));
        return;
    }

    m_statusLabel->setText(QStringLiteral("Conectando..."));
    setBusy(true);
    ApiClient::instance().login(username, password);
}

void LoginWindow::onRegistrarseClicked()
{
    emit openRegistroRequested();
}

void LoginWindow::onLoginSucceeded(const QString & /*accessToken*/)
{
    setBusy(false);
    m_statusLabel->clear();
    emit loginSuccessful();
}

void LoginWindow::onLoginFailed(const QString &message)
{
    setBusy(false);
    m_statusLabel->setText(message);
    QMessageBox::warning(this, QStringLiteral("No se pudo ingresar"), message);
}
