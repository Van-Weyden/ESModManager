#-------------------------------------------------
#
# Project created by QtCreator 2019-07-19T17:38:51
#
#-------------------------------------------------

QT       += core gui network

INCLUDEPATH += "framelesswindow"

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ESModManager
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        DatabaseEditor.cpp \
        Logger.cpp \
        MainWindow.cpp \
        ModDatabaseModel.cpp \
        ModInfo.cpp \
        ModScanner.cpp \
        RegExpPatterns.cpp \
        SteamRequester.cpp \
        applicationVersion.cpp \
        main.cpp

HEADERS += \
        DatabaseEditor.h \
        Logger.h \
        MainWindow.h \
        ModDatabaseModel.h \
        ModInfo.h \
        ModScanner.h \
        RegExpPatterns.h \
        SteamRequester.h \
        applicationVersion.h

FORMS += \
        DatabaseEditor.ui \
        MainWindow.ui

RC_FILE = icon.rc

RESOURCES = resources.qrc \

TRANSLATIONS += lang_ru.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
