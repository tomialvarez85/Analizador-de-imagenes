#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;

class LoginWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);

signals:
    void loginSuccessful();
    void openRegistroRequested();

private slots:
    void onIngresarClicked();
    void onRegistrarseClicked();
    void onLoginSucceeded(const QString &accessToken);
    void onLoginFailed(const QString &message);

private:
    void buildUi();
    void setBusy(bool busy);

    QLineEdit *m_usernameEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QPushButton *m_ingresarButton = nullptr;
    QPushButton *m_registrarseButton = nullptr;
    QLabel *m_statusLabel = nullptr;
};

#endif // LOGINWINDOW_H
