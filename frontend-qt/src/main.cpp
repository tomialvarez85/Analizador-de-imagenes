#include "windows/loginwindow.h"
#include "windows/mainwindow.h"
#include "windows/registrowindow.h"
#include "styles.h"

#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("AnalizadorImagenes"));
    app.setApplicationDisplayName(QStringLiteral("Analizador de Imágenes"));

    QFont appFont(QStringLiteral("Comic Sans MS"), 12);
    app.setFont(appFont);
    app.setStyleSheet(AppStyles::applicationStyleSheet());

    auto *loginWindow = new LoginWindow;
    auto *registroWindow = new RegistroWindow;
    MainWindow *mainWindow = nullptr;

    QObject::connect(loginWindow, &LoginWindow::openRegistroRequested, [&]() {
        loginWindow->hide();
        registroWindow->show();
        registroWindow->raise();
        registroWindow->activateWindow();
    });

    QObject::connect(registroWindow, &RegistroWindow::backToLoginRequested, [&]() {
        registroWindow->hide();
        loginWindow->show();
        loginWindow->raise();
        loginWindow->activateWindow();
    });

    QObject::connect(loginWindow, &LoginWindow::loginSuccessful, [&]() {
        if (!mainWindow) {
            mainWindow = new MainWindow;
        }
        loginWindow->hide();
        mainWindow->show();
        mainWindow->raise();
        mainWindow->activateWindow();
    });

    loginWindow->show();

    const int code = app.exec();

    delete mainWindow;
    delete registroWindow;
    delete loginWindow;

    return code;
}
