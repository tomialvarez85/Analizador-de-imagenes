#ifndef REGISTROWINDOW_H
#define REGISTROWINDOW_H

#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;

class RegistroWindow : public QWidget
{
    Q_OBJECT

public:
    explicit RegistroWindow(QWidget *parent = nullptr);

signals:
    void backToLoginRequested();

private slots:
    void onRegistrarseClicked();
    void onVolverClicked();
    void onRegistroSucceeded(const QString &message);
    void onRegistroFailed(const QString &message);

private:
    void buildUi();
    void setBusy(bool busy);

    QLineEdit *m_usernameEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QLineEdit *m_nombreEdit = nullptr;
    QLineEdit *m_apellidoEdit = nullptr;
    QLineEdit *m_emailEdit = nullptr;
    QPushButton *m_registrarButton = nullptr;
    QPushButton *m_volverButton = nullptr;
    QLabel *m_statusLabel = nullptr;
};

#endif // REGISTROWINDOW_H
