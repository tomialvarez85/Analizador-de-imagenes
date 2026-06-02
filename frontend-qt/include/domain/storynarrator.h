#ifndef STORYNARRATOR_H
#define STORYNARRATOR_H

#include <QObject>
#include <QProcess>
#include <QString>

class QTemporaryFile;

class StoryNarrator : public QObject
{
    Q_OBJECT

public:
    explicit StoryNarrator(QObject *parent = nullptr);
    ~StoryNarrator() override;

    bool isNarrating() const;
    void narrate(const QString &texto);
    void stop();

signals:
    void narracionStarted();
    void narracionStopped();
    void narracionError(const QString &message);

private slots:
    void onVozFinished(int exitCode, QProcess::ExitStatus status);

private:
    void cleanupNarration();

    QProcess *m_vozProcess = nullptr;
    QTemporaryFile *m_textoVozFile = nullptr;
};

#endif // STORYNARRATOR_H
