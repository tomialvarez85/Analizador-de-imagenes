#include "mainwindow.h"
#include "apiclient.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QPushButton>
#include <QTemporaryFile>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

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

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("Analizador de Imágenes"));
    setMinimumSize(900, 700);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *title = new QLabel(QStringLiteral("Contame tu dibujo"), central);
    title->setObjectName(QStringLiteral("titleLabel"));
    title->setAlignment(Qt::AlignCenter);

    m_imageLabel = new QLabel(central);
    m_imageLabel->setObjectName(QStringLiteral("imagePreview"));
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setMinimumSize(360, 280);
    m_imageLabel->setText(QStringLiteral("Acá va tu imagen"));

    m_cargarButton = new QPushButton(QStringLiteral("Cargar imagen"), central);
    m_cargarButton->setObjectName(QStringLiteral("secondaryButton"));

    m_analizarButton = new QPushButton(QStringLiteral("Analizar"), central);
    m_analizarButton->setDisabled(true);

    m_reproducirButton = new QPushButton(QStringLiteral("Reproducir historia"), central);
    m_reproducirButton->setObjectName(QStringLiteral("successButton"));
    m_reproducirButton->setDisabled(true);

    m_resultEdit = new QTextEdit(central);
    m_resultEdit->setReadOnly(true);
    m_resultEdit->setPlaceholderText(
        QStringLiteral("La descripción, la pregunta y la historia aparecerán acá."));

    auto *buttonsRow = new QHBoxLayout;
    buttonsRow->setSpacing(16);
    buttonsRow->addWidget(m_cargarButton);
    buttonsRow->addWidget(m_analizarButton);
    buttonsRow->addWidget(m_reproducirButton);

    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(18);
    layout->addWidget(title);
    layout->addWidget(m_imageLabel, 1);
    layout->addLayout(buttonsRow);
    layout->addWidget(m_resultEdit, 1);

    connect(m_cargarButton, &QPushButton::clicked,
            this, &MainWindow::onCargarImagenClicked);
    connect(m_analizarButton, &QPushButton::clicked,
            this, &MainWindow::onAnalizarClicked);
    connect(m_reproducirButton, &QPushButton::clicked,
            this, &MainWindow::onReproducirClicked);
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

    QPixmap pixmap;
    if (!pixmap.loadFromData(m_imageBytes)) {
        QMessageBox::warning(this,
                             QStringLiteral("Error"),
                             QStringLiteral("Formato de imagen no válido."));
        m_imageBytes.clear();
        return;
    }

    m_imagePath = path;
    const QPixmap scaled = pixmap.scaled(m_imageLabel->size(),
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
    m_imageLabel->setPixmap(scaled);
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
    m_resultEdit->setPlainText(QStringLiteral("Analizando tu dibujo..."));
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

    m_reproducirButton->setText(QStringLiteral("Detener"));
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
    m_reproducirButton->setText(QStringLiteral("Reproducir historia"));
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

    m_reproducirButton->setText(QStringLiteral("Reproducir historia"));
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
                              "DESCRIPCIÓN:\n%1\n\n"
                              "PREGUNTA:\n%2\n\n"
                              "HISTORIA:\n%3")
                              .arg(descripcion, pregunta, historia);
    m_resultEdit->setPlainText(texto);
}
