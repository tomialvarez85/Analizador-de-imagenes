#include "domain/analysisservice.h"
#include "api/apiclient.h"
#include "domain/imagedocument.h"

#include <QFileInfo>

AnalysisService::AnalysisService(QObject *parent)
    : QObject(parent)
{
    configureConnections();
}

void AnalysisService::configureConnections()
{
    connect(&ApiClient::instance(), &ApiClient::analisisSucceeded,
            this, &AnalysisService::onApiAnalysisSucceeded);
    connect(&ApiClient::instance(), &ApiClient::analisisFailed,
            this, &AnalysisService::onApiAnalysisFailed);
}

void AnalysisService::analyzeImage(const ImageDocument &image)
{
    if (!image.isValid()) {
        emit analysisFailed(QStringLiteral("No hay imagen válida para analizar."));
        return;
    }

    emit analysisStarted();

    const QString imagenBase64 = QString::fromLatin1(image.bytes().toBase64());
    const QFileInfo info(image.path());
    ApiClient::instance().analizarImagen(imagenBase64, info.fileName());
}

void AnalysisService::onApiAnalysisSucceeded(const QString &descripcion,
                                             const QString &pregunta,
                                             const QString &historia)
{
    emit analysisSucceeded(descripcion, pregunta, historia);
}

void AnalysisService::onApiAnalysisFailed(const QString &message)
{
    emit analysisFailed(message);
}
