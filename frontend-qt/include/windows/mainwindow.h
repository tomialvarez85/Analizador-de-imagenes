#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QProcess>

class QLabel;
class QTextEdit;
class QPushButton;
class QTemporaryFile;
class QBoxLayout;
class QFrame;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void resizeEvent(QResizeEvent *event) override;

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
    void refrescarVistaImagen();
    void ajustarLayoutResponsivo();

    QLabel *m_imageLabel = nullptr;
    QTextEdit *m_resultEdit = nullptr;
    QPushButton *m_cargarButton = nullptr;
    QPushButton *m_analizarButton = nullptr;
    QPushButton *m_reproducirButton = nullptr;
    QBoxLayout *m_contentLayout = nullptr;
    QFrame *m_leftCard = nullptr;
    QFrame *m_rightCard = nullptr;

    QString m_imagePath;
    QByteArray m_imageBytes;
    QPixmap m_pixmapOriginal;
    QString m_historiaText;

    QProcess *m_vozProcess = nullptr;
    QTemporaryFile *m_textoVozFile = nullptr;
};

#endif // MAINWINDOW_H
