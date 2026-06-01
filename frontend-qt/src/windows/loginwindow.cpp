#include "windows/loginwindow.h"
#include "api/apiclient.h"
#include "uihelpers.h"

#include <QHBoxLayout>
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
    setMinimumSize(420, 540);

    auto *windowLayout = new QVBoxLayout(this);
    windowLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *root = UiHelpers::createAppRoot(this);
    windowLayout->addWidget(root);

    QVBoxLayout *outer = UiHelpers::createOuterLayout(root);
    outer->addStretch(1);

    auto *centerRow = new QHBoxLayout;
    centerRow->addStretch(1);

    auto *card = UiHelpers::createCard(root, UiHelpers::kAuthCardWidth);
    card->setMaximumWidth(UiHelpers::kAuthCardWidth);
    card->setMinimumWidth(320);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(40, 36, 40, 36);
    cardLayout->setSpacing(20);

    cardLayout->addWidget(UiHelpers::createEmoji(QStringLiteral("🎨"), card));
    cardLayout->addWidget(UiHelpers::createTitle(QStringLiteral("¡Hola, artista!"), card));
    cardLayout->addWidget(
        UiHelpers::createSubtitle(QStringLiteral("Ingresá para contar tu dibujo"), card));

    m_usernameEdit = new QLineEdit(card);
    m_usernameEdit->setPlaceholderText(QStringLiteral("Tu usuario"));
    cardLayout->addWidget(
        UiHelpers::createFieldGroup(card, QStringLiteral("👤 Usuario"), m_usernameEdit));

    m_passwordEdit = new QLineEdit(card);
    m_passwordEdit->setPlaceholderText(QStringLiteral("Tu contraseña"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    cardLayout->addWidget(
        UiHelpers::createFieldGroup(card, QStringLiteral("🔒 Clave"), m_passwordEdit));

    cardLayout->addSpacing(8);

    m_ingresarButton = new QPushButton(QStringLiteral("🚀  ¡Entrar!"), card);
    m_registrarseButton = new QPushButton(QStringLiteral("✨  Crear cuenta"), card);
    m_registrarseButton->setObjectName(QStringLiteral("secondaryButton"));

    UiHelpers::addFullWidthButton(cardLayout, m_ingresarButton);
    UiHelpers::addFullWidthButton(cardLayout, m_registrarseButton);

    m_statusLabel = UiHelpers::createStatusLabel(card);
    cardLayout->addWidget(m_statusLabel);

    centerRow->addWidget(card, 0, Qt::AlignCenter);
    centerRow->addStretch(1);
    outer->addLayout(centerRow);
    outer->addStretch(1);

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
    QMessageBox::warning(this, QStringLiteral("Ups, no pudimos entrar"), message);
}
