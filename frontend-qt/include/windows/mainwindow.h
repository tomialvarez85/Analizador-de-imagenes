#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>

class AnalysisService;
class ImageDocument;
class QLabel;
class QBoxLayout;
class QFrame;
class QPushButton;
class QTextEdit;
class StoryNarrator;

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

    ImageDocument *m_imageDocument = nullptr;
    AnalysisService *m_analysisService = nullptr;
    StoryNarrator *m_storyNarrator = nullptr;
    QString m_historiaText;
};

#endif // MAINWINDOW_H
