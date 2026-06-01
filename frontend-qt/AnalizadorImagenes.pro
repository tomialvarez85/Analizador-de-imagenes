QT       += core gui widgets network

CONFIG   += c++17
CONFIG   -= app_bundle

TARGET   = AnalizadorImagenes
TEMPLATE = app

INCLUDEPATH += $$PWD/include

SOURCES += \
    src/main.cpp \
    src/uihelpers.cpp \
    src/api/apiclient.cpp \
    src/windows/loginwindow.cpp \
    src/windows/registrowindow.cpp \
    src/windows/mainwindow.cpp

HEADERS += \
    include/config.h \
    include/styles.h \
    include/uihelpers.h \
    include/api/apiclient.h \
    include/windows/loginwindow.h \
    include/windows/registrowindow.h \
    include/windows/mainwindow.h

win32 {
    RC_ICONS =
}

unix:!macx {
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }
}
