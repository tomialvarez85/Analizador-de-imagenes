#include "windows/registrowindow.h"
#include "api/apiclient.h"
#include "uihelpers.h"

#include <QBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
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
    setMinimumSize(680, 520);

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
    card->setMinimumWidth(560);
    m_cardLayout = new QBoxLayout(QBoxLayout::LeftToRight, card);
    m_cardLayout->setContentsMargins(0, 0, 0, 0);
    m_cardLayout->setSpacing(0);

    m_heroPanel = new QWidget(card);
    m_heroPanel->setObjectName(QStringLiteral("heroPanel"));
    auto *heroLayout = new QVBoxLayout(m_heroPanel);
    heroLayout->setContentsMargins(44, 40, 44, 40);
    heroLayout->setSpacing(20);
    heroLayout->addWidget(UiHelpers::createEmoji(QStringLiteral("🌟"), m_heroPanel));
    heroLayout->addWidget(UiHelpers::createTitle(QStringLiteral("¡Creá tu cuenta!"), m_heroPanel));
    heroLayout->addWidget(
        UiHelpers::createSubtitle(QStringLiteral("Solo necesitás usuario y clave"), m_heroPanel));
    heroLayout->addSpacing(12);
    heroLayout->addWidget(UiHelpers::createSubtitle(QStringLiteral("Registro rápido en un solo paso."), m_heroPanel));
    heroLayout->addStretch(1);

    QWidget *formPanel = new QWidget(card);
    formPanel->setObjectName(QStringLiteral("formPanel"));
    auto *formLayout = new QVBoxLayout(formPanel);
    formLayout->setContentsMargins(44, 40, 44, 40);
    formLayout->setSpacing(18);

    auto *sectionLabel = new QLabel(QStringLiteral("Registro"), formPanel);
    sectionLabel->setObjectName(QStringLiteral("sectionLabel"));
    formLayout->addWidget(sectionLabel);

    auto *sectionSub = new QLabel(QStringLiteral("Completá tus datos para crear tu cuenta."), formPanel);
    sectionSub->setWordWrap(true);
    sectionSub->setStyleSheet("color: #475569; font-size: 13pt;");
    formLayout->addWidget(sectionSub);

    m_usernameEdit = new QLineEdit(formPanel);
    m_usernameEdit->setPlaceholderText(QStringLiteral("Usuario"));
    formLayout->addWidget(
        UiHelpers::createFieldGroup(formPanel, QStringLiteral("👤 Usuario"), m_usernameEdit));

    m_passwordEdit = new QLineEdit(formPanel);
    m_passwordEdit->setPlaceholderText(QStringLiteral("Contraseña"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    formLayout->addWidget(
        UiHelpers::createFieldGroup(formPanel, QStringLiteral("🔒 Clave"), m_passwordEdit));

    formLayout->addSpacing(8);

    m_registrarButton = new QPushButton(QStringLiteral("🎉  Registrarme"), formPanel);
    m_registrarButton->setObjectName(QStringLiteral("successButton"));

    m_volverButton = new QPushButton(QStringLiteral("⬅️  Volver al login"), formPanel);
    m_volverButton->setObjectName(QStringLiteral("secondaryButton"));

    UiHelpers::addFullWidthButton(formLayout, m_registrarButton);
    UiHelpers::addFullWidthButton(formLayout, m_volverButton);

    m_statusLabel = UiHelpers::createStatusLabel(formPanel);
    formLayout->addWidget(m_statusLabel);
    formLayout->addStretch(1);

    m_heroPanel->setFixedWidth(360);
    m_cardLayout->addWidget(m_heroPanel);
    m_cardLayout->addWidget(formPanel);

    centerRow->addWidget(card, 0, Qt::AlignCenter);
    centerRow->addStretch(1);
    outer->addLayout(centerRow);
    outer->addStretch(1);

    connect(m_registrarButton, &QPushButton::clicked, this, &RegistroWindow::onRegistrarseClicked);
    connect(m_volverButton, &QPushButton::clicked, this, &RegistroWindow::onVolverClicked);

    ajustarLayoutResponsivo();
}

void RegistroWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    ajustarLayoutResponsivo();
}

void RegistroWindow::ajustarLayoutResponsivo()
{
    if (!m_cardLayout || !m_heroPanel) {
        return;
    }

    if (width() < 820) {
        m_cardLayout->setDirection(QBoxLayout::TopToBottom);
        m_heroPanel->setMinimumWidth(0);
        m_heroPanel->setMaximumWidth(QWIDGETSIZE_MAX);
    } else {
        m_cardLayout->setDirection(QBoxLayout::LeftToRight);
        m_heroPanel->setFixedWidth(360);
    }
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
        m_statusLabel->setVisible(true);
        return;
    }

    m_statusLabel->setText(QStringLiteral("Registrando..."));
    m_statusLabel->setVisible(true);
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
    m_statusLabel->setVisible(true);
    QMessageBox::information(this, QStringLiteral("¡Genial!"), message);
    emit backToLoginRequested();
}

void RegistroWindow::onRegistroFailed(const QString &message)
{
    setBusy(false);
    m_statusLabel->setText(message);
    m_statusLabel->setVisible(true);
    QMessageBox::warning(this, QStringLiteral("No se pudo registrar"), message);
}
