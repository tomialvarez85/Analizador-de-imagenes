#include "windows/registrowindow.h"
#include "api/apiclient.h"
#include "uihelpers.h"

#include <QHBoxLayout>
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
    cardLayout->setContentsMargins(40, 32, 40, 32);
    cardLayout->setSpacing(16);

    cardLayout->addWidget(UiHelpers::createEmoji(QStringLiteral("🌟"), card));
    cardLayout->addWidget(UiHelpers::createTitle(QStringLiteral("¡Creá tu cuenta!"), card));
    cardLayout->addWidget(
        UiHelpers::createSubtitle(QStringLiteral("Solo necesitás usuario y clave"), card));

    m_usernameEdit = new QLineEdit(card);
    m_usernameEdit->setPlaceholderText(QStringLiteral("Usuario"));
    cardLayout->addWidget(
        UiHelpers::createFieldGroup(card, QStringLiteral("👤 Usuario"), m_usernameEdit));

    m_passwordEdit = new QLineEdit(card);
    m_passwordEdit->setPlaceholderText(QStringLiteral("Contraseña"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    cardLayout->addWidget(
        UiHelpers::createFieldGroup(card, QStringLiteral("🔒 Clave"), m_passwordEdit));

    cardLayout->addSpacing(8);

    m_registrarButton = new QPushButton(QStringLiteral("🎉  Registrarme"), card);
    m_registrarButton->setObjectName(QStringLiteral("successButton"));

    m_volverButton = new QPushButton(QStringLiteral("⬅️  Volver al login"), card);
    m_volverButton->setObjectName(QStringLiteral("secondaryButton"));

    UiHelpers::addFullWidthButton(cardLayout, m_registrarButton);
    UiHelpers::addFullWidthButton(cardLayout, m_volverButton);

    m_statusLabel = UiHelpers::createStatusLabel(card);
    cardLayout->addWidget(m_statusLabel);

    centerRow->addWidget(card, 0, Qt::AlignCenter);
    centerRow->addStretch(1);
    outer->addLayout(centerRow);
    outer->addStretch(1);

    connect(m_registrarButton, &QPushButton::clicked, this, &RegistroWindow::onRegistrarseClicked);
    connect(m_volverButton, &QPushButton::clicked, this, &RegistroWindow::onVolverClicked);
}

void RegistroWindow::setBusy(bool busy)
{
    m_registrarButton->setDisabled(busy);
    m_volverButton->setDisabled(busy);
    m_usernameEdit->setDisabled(busy);
    m_passwordEdit->setDisabled(busy);
}

void RegistroWindow::onRegistrarseClicked()
{
    const QString username = m_usernameEdit->text().trimmed();
    const QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("Completá usuario y contraseña."));
        return;
    }

    m_statusLabel->setText(QStringLiteral("Registrando..."));
    setBusy(true);
    ApiClient::instance().registro(username, password);
}

void RegistroWindow::onVolverClicked()
{
    emit backToLoginRequested();
}

void RegistroWindow::onRegistroSucceeded(const QString &message)
{
    setBusy(false);
    m_statusLabel->setText(message);
    QMessageBox::information(this, QStringLiteral("¡Genial!"), message);
    emit backToLoginRequested();
}

void RegistroWindow::onRegistroFailed(const QString &message)
{
    setBusy(false);
    m_statusLabel->setText(message);
    QMessageBox::warning(this, QStringLiteral("No se pudo registrar"), message);
}
