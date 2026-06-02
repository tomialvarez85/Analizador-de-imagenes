#ifndef ANALYSISSERVICE_H
#define ANALYSISSERVICE_H

#include <QObject>
#include <QString>

class ImageDocument;

class AnalysisService : public QObject
{
    Q_OBJECT

public:
    explicit AnalysisService(QObject *parent = nullptr);
    void analyzeImage(const ImageDocument &image);

signals:
    void analysisStarted();
    void analysisSucceeded(const QString &descripcion,
                           const QString &pregunta,
                           const QString &historia);
    void analysisFailed(const QString &message);

private slots:
    void onApiAnalysisSucceeded(const QString &descripcion,
                                const QString &pregunta,
                                const QString &historia);
    void onApiAnalysisFailed(const QString &message);

private:
    void configureConnections();
};

#endif // ANALYSISSERVICE_H
