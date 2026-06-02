#include "windows/loginwindow.h"
#include "api/apiclient.h"
#include "uihelpers.h"

#include <QHBoxLayout>
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
    setMinimumSize(820, 540);

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
    card->setMinimumWidth(760);
    auto *cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    QWidget *heroPanel = new QWidget(card);
    heroPanel->setObjectName(QStringLiteral("heroPanel"));
    auto *heroLayout = new QVBoxLayout(heroPanel);
    heroLayout->setContentsMargins(44, 40, 44, 40);
    heroLayout->setSpacing(20);
    heroLayout->addWidget(UiHelpers::createEmoji(QStringLiteral("🎨"), heroPanel));
    heroLayout->addWidget(UiHelpers::createTitle(QStringLiteral("¡Hola, artista!"), heroPanel));
    heroLayout->addWidget(
        UiHelpers::createSubtitle(QStringLiteral("Ingresá para contar tu dibujo"), heroPanel));
    heroLayout->addSpacing(12);
    heroLayout->addWidget(UiHelpers::createSubtitle(QStringLiteral("Diseño horizontal para escritorio."), heroPanel));
    heroLayout->addStretch(1);

    QWidget *formPanel = new QWidget(card);
    auto *formLayout = new QVBoxLayout(formPanel);
    formLayout->setContentsMargins(44, 40, 44, 40);
    formLayout->setSpacing(18);

    auto *sectionLabel = new QLabel(QStringLiteral("Ingreso"), formPanel);
    sectionLabel->setObjectName(QStringLiteral("sectionLabel"));
    formLayout->addWidget(sectionLabel);

    auto *sectionSub = new QLabel(QStringLiteral("Iniciá sesión con tu usuario y clave."), formPanel);
    sectionSub->setWordWrap(true);
    sectionSub->setStyleSheet("color: #475569; font-size: 13pt;");
    formLayout->addWidget(sectionSub);

    m_usernameEdit = new QLineEdit(formPanel);
    m_usernameEdit->setPlaceholderText(QStringLiteral("Tu usuario"));
    formLayout->addWidget(
        UiHelpers::createFieldGroup(formPanel, QStringLiteral("👤 Usuario"), m_usernameEdit));

    m_passwordEdit = new QLineEdit(formPanel);
    m_passwordEdit->setPlaceholderText(QStringLiteral("Tu contraseña"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    formLayout->addWidget(
        UiHelpers::createFieldGroup(formPanel, QStringLiteral("🔒 Clave"), m_passwordEdit));

    formLayout->addSpacing(8);

    m_ingresarButton = new QPushButton(QStringLiteral("🚀  ¡Entrar!"), formPanel);
    m_registrarseButton = new QPushButton(QStringLiteral("✨  Crear cuenta"), formPanel);
    m_registrarseButton->setObjectName(QStringLiteral("secondaryButton"));

    UiHelpers::addFullWidthButton(formLayout, m_ingresarButton);
    UiHelpers::addFullWidthButton(formLayout, m_registrarseButton);

    m_statusLabel = UiHelpers::createStatusLabel(formPanel);
    formLayout->addWidget(m_statusLabel);
    formLayout->addStretch(1);

    heroPanel->setFixedWidth(360);
    cardLayout->addWidget(heroPanel);
    cardLayout->addWidget(formPanel);

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
        m_statusLabel->setVisible(true);
        return;
    }

    m_statusLabel->setText(QStringLiteral("Conectando..."));
    m_statusLabel->setVisible(true);
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
    m_statusLabel->setVisible(false);
    emit loginSuccessful();
}

void LoginWindow::onLoginFailed(const QString &message)
{
    setBusy(false);
    m_statusLabel->setText(message);
    m_statusLabel->setVisible(true);
    QMessageBox::warning(this, QStringLiteral("Ups, no pudimos entrar"), message);
}
