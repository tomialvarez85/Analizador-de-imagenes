QT       += core gui widgets network

CONFIG   += c++17
CONFIG   -= app_bundle

TARGET   = AnalizadorImagenes
TEMPLATE = app

SOURCES += \
    main.cpp \
    apiclient.cpp \
    loginwindow.cpp \
    registrowindow.cpp \
    mainwindow.cpp

HEADERS += \
    config.h \
    styles.h \
    apiclient.h \
    loginwindow.h \
    registrowindow.h \
    mainwindow.h

# Qt 6: descomentar si qmake no detecta multimedia automáticamente
# greaterThan(QT_MAJOR_VERSION, 5): QT += multimedia

win32 {
    RC_ICONS =
}

unix:!macx {
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }
}
