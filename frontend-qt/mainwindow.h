#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>

class QLabel;
class QTextEdit;
class QPushButton;
class QTemporaryFile;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onCargarImagenClicked();
    void onAnalizarClicked();
    void onReproducirClicked();
    void onAnalisisSucceeded(const QString &descripcion,
                             const QString &pregunta,
                             const QString &historia);
    void onAnalisisFailed(const QString &message);
    void onVozFinished(int exitCode, QProcess::ExitStatus status);

private:
    void buildUi();
    void setAnalisisBusy(bool busy);
    void updateResultText(const QString &descripcion,
                          const QString &pregunta,
                          const QString &historia);
    void narrarHistoria(const QString &texto);
    void detenerNarracion();
    void actualizarBotonReproducir();

    QLabel *m_imageLabel = nullptr;
    QTextEdit *m_resultEdit = nullptr;
    QPushButton *m_cargarButton = nullptr;
    QPushButton *m_analizarButton = nullptr;
    QPushButton *m_reproducirButton = nullptr;

    QString m_imagePath;
    QByteArray m_imageBytes;
    QString m_historiaText;

    QProcess *m_vozProcess = nullptr;
    QTemporaryFile *m_textoVozFile = nullptr;
};

#endif // MAINWINDOW_H
