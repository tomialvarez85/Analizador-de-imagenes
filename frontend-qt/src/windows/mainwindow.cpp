#include "windows/mainwindow.h"
#include "api/apiclient.h"
#include "uihelpers.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QPushButton>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QTemporaryFile>
#include <QTextEdit>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();

    connect(&ApiClient::instance(), &ApiClient::analisisSucceeded,
            this, &MainWindow::onAnalisisSucceeded);
    connect(&ApiClient::instance(), &ApiClient::analisisFailed,
            this, &MainWindow::onAnalisisFailed);
}

MainWindow::~MainWindow()
{
    detenerNarracion();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    ajustarLayoutResponsivo();
    refrescarVistaImagen();
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("Analizador de Imágenes"));
    setMinimumSize(760, 560);

    auto *root = UiHelpers::createAppRoot(nullptr);
    setCentralWidget(root);

    auto *layout = new QVBoxLayout(root);
    layout->setContentsMargins(32, 28, 32, 32);
    layout->setSpacing(18);
    layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    layout->addWidget(UiHelpers::createEmoji(QStringLiteral("🖼️"), root), 0, Qt::AlignCenter);
    layout->addWidget(UiHelpers::createTitle(QStringLiteral("Contame tu dibujo"), root), 0, Qt::AlignCenter);
    layout->addWidget(UiHelpers::createSubtitle(
        QStringLiteral("Cargá una imagen, analizala y escuchá tu cuento"), root), 0, Qt::AlignCenter);

    m_contentLayout = new QHBoxLayout;
    m_contentLayout->setSpacing(28);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);

    m_leftCard = UiHelpers::createCard(root);
    m_leftCard->setMinimumWidth(380);
    m_leftCard->setMinimumHeight(360);
    m_leftCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *leftLayout = new QVBoxLayout(m_leftCard);
    leftLayout->setContentsMargins(24, 24, 24, 24);
    leftLayout->setSpacing(16);

    auto *imgSection = new QLabel(QStringLiteral("📷 Tu imagen"), m_leftCard);
    imgSection->setObjectName(QStringLiteral("sectionLabel"));

    auto *imageFrame = new QFrame(m_leftCard);
    imageFrame->setObjectName(QStringLiteral("imageFrame"));
    auto *imageFrameLayout = new QVBoxLayout(imageFrame);
    imageFrameLayout->setContentsMargins(12, 12, 12, 12);

    m_imageLabel = new QLabel(imageFrame);
    m_imageLabel->setObjectName(QStringLiteral("imagePreview"));
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setMinimumHeight(340);
    m_imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_imageLabel->setText(QStringLiteral("Tocá «Cargar imagen»"));
    imageFrameLayout->addWidget(m_imageLabel);

    auto *leftButtons = new QHBoxLayout;
    leftButtons->setSpacing(12);

    m_cargarButton = new QPushButton(QStringLiteral("📁 Cargar"), m_leftCard);
    m_cargarButton->setObjectName(QStringLiteral("secondaryButton"));

    m_analizarButton = new QPushButton(QStringLiteral("🔍 Analizar"), m_leftCard);
    m_analizarButton->setObjectName(QStringLiteral("accentButton"));
    m_analizarButton->setDisabled(true);

    m_cargarButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_analizarButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_cargarButton->setMinimumHeight(54);
    m_analizarButton->setMinimumHeight(54);
    m_cargarButton->setMinimumWidth(0);
    m_analizarButton->setMinimumWidth(0);

    leftButtons->addWidget(m_cargarButton, 1);
    leftButtons->addWidget(m_analizarButton, 1);

    leftLayout->addWidget(imgSection);
    leftLayout->addWidget(imageFrame, 1);
    leftLayout->addLayout(leftButtons);

    m_rightCard = UiHelpers::createCard(root);
    m_rightCard->setMinimumWidth(380);
    m_rightCard->setMinimumHeight(360);
    m_rightCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *rightLayout = new QVBoxLayout(m_rightCard);
    rightLayout->setContentsMargins(24, 24, 24, 24);
    rightLayout->setSpacing(16);

    auto *storySection = new QLabel(QStringLiteral("📖 Tu cuento"), m_rightCard);
    storySection->setObjectName(QStringLiteral("sectionLabel"));

    m_resultEdit = new QTextEdit(m_rightCard);
    m_resultEdit->setReadOnly(true);
    m_resultEdit->setMinimumHeight(240);
    m_resultEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_resultEdit->setPlaceholderText(
        QStringLiteral("Acá aparece la descripción, la pregunta y la historia..."));

    m_reproducirButton = new QPushButton(QStringLiteral("🔊 Reproducir historia"), m_rightCard);
    m_reproducirButton->setObjectName(QStringLiteral("successButton"));
    m_reproducirButton->setDisabled(true);

    rightLayout->addWidget(storySection);
    rightLayout->addWidget(m_resultEdit, 1);
    UiHelpers::addFullWidthButton(rightLayout, m_reproducirButton);

    m_contentLayout->addWidget(m_leftCard, 1);
    m_contentLayout->addWidget(m_rightCard, 1);
    layout->addLayout(m_contentLayout, 1);
    layout->setStretchFactor(m_contentLayout, 1);
    ajustarLayoutResponsivo();

    connect(m_cargarButton, &QPushButton::clicked,
            this, &MainWindow::onCargarImagenClicked);
    connect(m_analizarButton, &QPushButton::clicked,
            this, &MainWindow::onAnalizarClicked);
    connect(m_reproducirButton, &QPushButton::clicked,
            this, &MainWindow::onReproducirClicked);
}

void MainWindow::ajustarLayoutResponsivo()
{
    if (!m_contentLayout) {
        return;
    }
    if (width() < 820) {
        m_contentLayout->setDirection(QBoxLayout::TopToBottom);
    } else {
        m_contentLayout->setDirection(QBoxLayout::LeftToRight);
    }
}

void MainWindow::refrescarVistaImagen()
{
    if (m_pixmapOriginal.isNull() || !m_imageLabel) {
        return;
    }
    const QPixmap scaled = m_pixmapOriginal.scaled(m_imageLabel->size(),
                                                   Qt::KeepAspectRatio,
                                                   Qt::SmoothTransformation);
    m_imageLabel->setPixmap(scaled);
}

void MainWindow::onCargarImagenClicked()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Elegí una imagen"),
        QString(),
        QStringLiteral("Imágenes (*.jpg *.jpeg *.png);;Todos los archivos (*)"));

    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this,
                             QStringLiteral("Error"),
                             QStringLiteral("No se pudo abrir la imagen."));
        return;
    }

    m_imageBytes = file.readAll();
    file.close();

    if (m_imageBytes.isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("Error"),
                             QStringLiteral("El archivo está vacío."));
        return;
    }

    if (!m_pixmapOriginal.loadFromData(m_imageBytes)) {
        QMessageBox::warning(this,
                             QStringLiteral("Error"),
                             QStringLiteral("Formato de imagen no válido."));
        m_imageBytes.clear();
        m_pixmapOriginal = QPixmap();
        return;
    }

    m_imagePath = path;
    refrescarVistaImagen();
    m_analizarButton->setEnabled(true);
    m_reproducirButton->setEnabled(false);
    m_historiaText.clear();
    detenerNarracion();
    m_resultEdit->clear();
}

void MainWindow::onAnalizarClicked()
{
    if (m_imageBytes.isEmpty()) {
        QMessageBox::information(this,
                               QStringLiteral("Sin imagen"),
                               QStringLiteral("Primero cargá una imagen."));
        return;
    }

    const QFileInfo info(m_imagePath);
    const QString nombreArchivo = info.fileName();
    const QString imagenBase64 = QString::fromLatin1(m_imageBytes.toBase64());

    setAnalisisBusy(true);
    m_resultEdit->setPlainText(QStringLiteral("✨ Analizando tu dibujo..."));
    ApiClient::instance().analizarImagen(imagenBase64, nombreArchivo);
}

void MainWindow::onReproducirClicked()
{
    if (m_historiaText.trimmed().isEmpty()) {
        QMessageBox::information(
            this,
            QStringLiteral("Sin historia"),
            QStringLiteral("Primero analizá una imagen para obtener la historia."));
        return;
    }

    if (m_vozProcess && m_vozProcess->state() != QProcess::NotRunning) {
        detenerNarracion();
        return;
    }

    narrarHistoria(m_historiaText);
}

void MainWindow::onAnalisisSucceeded(const QString &descripcion,
                                     const QString &pregunta,
                                     const QString &historia)
{
    setAnalisisBusy(false);
    updateResultText(descripcion, pregunta, historia);

    m_historiaText = historia.trimmed();
    detenerNarracion();
    actualizarBotonReproducir();
}

void MainWindow::onAnalisisFailed(const QString &message)
{
    setAnalisisBusy(false);
    m_resultEdit->setPlainText(message);
    QMessageBox::warning(this, QStringLiteral("Error al analizar"), message);
}

void MainWindow::narrarHistoria(const QString &texto)
{
#ifdef Q_OS_WIN
    detenerNarracion();

    m_textoVozFile = new QTemporaryFile(this);
    m_textoVozFile->setAutoRemove(false);
    if (!m_textoVozFile->open()) {
        QMessageBox::warning(this,
                             QStringLiteral("Error"),
                             QStringLiteral("No se pudo preparar el texto para narrar."));
        return;
    }
    m_textoVozFile->write(texto.toUtf8());
    m_textoVozFile->flush();

    const QString ruta = m_textoVozFile->fileName();
    const QString script = QStringLiteral(
        "Add-Type -AssemblyName System.Speech; "
        "$s = New-Object System.Speech.Synthesis.SpeechSynthesizer; "
        "$s.Rate = -1; "
        "foreach ($v in $s.GetInstalledVoices()) { "
        "  if ($v.VoiceInfo.Culture.Name -like 'es*') { "
        "    $s.SelectVoice($v.VoiceInfo.Name); break "
        "  } "
        "} "
        "$t = Get-Content -LiteralPath '%1' -Encoding UTF8 -Raw; "
        "$s.Speak($t)")
                               .arg(QString(ruta).replace(QLatin1Char('\''), QStringLiteral("''")));

    m_vozProcess = new QProcess(this);
    connect(m_vozProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &MainWindow::onVozFinished);

    m_vozProcess->start(QStringLiteral("powershell"),
                        {QStringLiteral("-NoProfile"),
                         QStringLiteral("-ExecutionPolicy"),
                         QStringLiteral("Bypass"),
                         QStringLiteral("-Command"),
                         script});

    if (!m_vozProcess->waitForStarted(5000)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Error"),
            QStringLiteral("No se pudo iniciar la narración de voz en este equipo."));
        detenerNarracion();
        return;
    }

    m_reproducirButton->setText(QStringLiteral("⏹️ Detener"));
    m_reproducirButton->setEnabled(true);
#else
    Q_UNUSED(texto);
    QMessageBox::information(
        this,
        QStringLiteral("No disponible"),
        QStringLiteral("La narración por voz está disponible en Windows."));
#endif
}

void MainWindow::detenerNarracion()
{
    if (m_vozProcess) {
        if (m_vozProcess->state() != QProcess::NotRunning) {
            m_vozProcess->kill();
            m_vozProcess->waitForFinished(2000);
        }
        m_vozProcess->deleteLater();
        m_vozProcess = nullptr;
    }
    if (m_textoVozFile) {
        const QString path = m_textoVozFile->fileName();
        m_textoVozFile->close();
        m_textoVozFile->deleteLater();
        m_textoVozFile = nullptr;
        QFile::remove(path);
    }
    m_reproducirButton->setText(QStringLiteral("🔊 Reproducir historia"));
    actualizarBotonReproducir();
}

void MainWindow::onVozFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(status);

    if (m_vozProcess) {
        m_vozProcess->deleteLater();
        m_vozProcess = nullptr;
    }
    if (m_textoVozFile) {
        const QString path = m_textoVozFile->fileName();
        m_textoVozFile->close();
        m_textoVozFile->deleteLater();
        m_textoVozFile = nullptr;
        QFile::remove(path);
    }

    m_reproducirButton->setText(QStringLiteral("🔊 Reproducir historia"));
    actualizarBotonReproducir();
}

void MainWindow::setAnalisisBusy(bool busy)
{
    m_cargarButton->setDisabled(busy);
    m_analizarButton->setDisabled(busy);
    if (busy) {
        m_reproducirButton->setDisabled(true);
    } else {
        actualizarBotonReproducir();
    }
}

void MainWindow::actualizarBotonReproducir()
{
    const bool narrando = m_vozProcess && m_vozProcess->state() != QProcess::NotRunning;
    m_reproducirButton->setEnabled(!m_historiaText.isEmpty() || narrando);
}

void MainWindow::updateResultText(const QString &descripcion,
                                  const QString &pregunta,
                                  const QString &historia)
{
    const QString texto = QStringLiteral(
                              "🌈 DESCRIPCIÓN\n%1\n\n"
                              "❓ PREGUNTA\n%2\n\n"
                              "📚 HISTORIA\n%3")
                              .arg(descripcion, pregunta, historia);
    m_resultEdit->setPlainText(texto);
}
