#include "domain/storynarrator.h"

#include <QFile>
#include <QProcess>
#include <QTemporaryFile>

StoryNarrator::StoryNarrator(QObject *parent)
    : QObject(parent)
{
}

StoryNarrator::~StoryNarrator()
{
    stop();
}

bool StoryNarrator::isNarrating() const
{
    return m_vozProcess && m_vozProcess->state() != QProcess::NotRunning;
}

void StoryNarrator::narrate(const QString &texto)
{
    stop();

    if (texto.trimmed().isEmpty()) {
        emit narracionError(QStringLiteral("No hay historia para narrar."));
        return;
    }

#ifdef Q_OS_WIN
    m_textoVozFile = new QTemporaryFile(this);
    m_textoVozFile->setAutoRemove(false);
    if (!m_textoVozFile->open()) {
        cleanupNarration();
        emit narracionError(QStringLiteral("No se pudo preparar el texto para narrar."));
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
            &StoryNarrator::onVozFinished);

    m_vozProcess->start(QStringLiteral("powershell"),
                        {QStringLiteral("-NoProfile"),
                         QStringLiteral("-ExecutionPolicy"),
                         QStringLiteral("Bypass"),
                         QStringLiteral("-Command"),
                         script});

    if (!m_vozProcess->waitForStarted(5000)) {
        cleanupNarration();
        emit narracionError(QStringLiteral("No se pudo iniciar la narración de voz en este equipo."));
        return;
    }

    emit narracionStarted();
#else
    Q_UNUSED(texto);
    emit narracionError(QStringLiteral("La narración por voz está disponible en Windows."));
#endif
}

void StoryNarrator::stop()
{
#ifdef Q_OS_WIN
    if (m_vozProcess && m_vozProcess->state() != QProcess::NotRunning) {
        m_vozProcess->kill();
        m_vozProcess->waitForFinished(2000);
    }
#endif
    cleanupNarration();
}

void StoryNarrator::onVozFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(status);

    cleanupNarration();
    emit narracionStopped();
}

void StoryNarrator::cleanupNarration()
{
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
}
