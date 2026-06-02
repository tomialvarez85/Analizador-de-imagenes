#ifndef REGISTROWINDOW_H
#define REGISTROWINDOW_H

#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;
class QBoxLayout;
class QResizeEvent;

class RegistroWindow : public QWidget
{
    Q_OBJECT

public:
    explicit RegistroWindow(QWidget *parent = nullptr);

signals:
    void backToLoginRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onRegistrarseClicked();
    void onVolverClicked();
    void onRegistroSucceeded(const QString &message);
    void onRegistroFailed(const QString &message);

private:
    void buildUi();
    void setBusy(bool busy);
    void ajustarLayoutResponsivo();

    QLineEdit *m_usernameEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QPushButton *m_registrarButton = nullptr;
    QPushButton *m_volverButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QBoxLayout *m_cardLayout = nullptr;
    QWidget *m_heroPanel = nullptr;
};

#endif // REGISTROWINDOW_H
