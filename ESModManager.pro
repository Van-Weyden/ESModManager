#-------------------------------------------------
#
# Project created by QtCreator 2019-07-19T17:38:51
#
#-------------------------------------------------

QT       += core gui network svg

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
        EditModCollectionDialog.cpp \
        Logger.cpp \
        MainWindow.cpp \
        ModInfoWidget.cpp \
        ModScanner.cpp \
        SteamRequester.cpp \
        main.cpp \
        mvc/AbstractModDatabaseItem.cpp \
        mvc/AutoExpandTreeView.cpp \
        mvc/ModCollection.cpp \
        mvc/ModDatabaseModel.cpp \
        mvc/ModInfo.cpp \
        mvc/ModNameDelegate.cpp \
        mvc/ModNameEditor.cpp \
        mvc/TreeView.cpp \
        mvc/proxyModels/BaseFilterProxyModel.cpp \
        mvc/proxyModels/CollectionModsProxyModel.cpp \
        mvc/proxyModels/DisabledModProxyModel.cpp \
        mvc/proxyModels/EnabledModProxyModel.cpp \
        mvc/proxyModels/ExcludedAllModsProxyModel.cpp \
        mvc/proxyModels/ExistsModProxyModel.cpp \
        mvc/proxyModels/NameFilterProxyModel.cpp \
        utils/RegExpPatterns.cpp \
        utils/applicationVersion.cpp

HEADERS += \
        DatabaseEditor.h \
        EditModCollectionDialog.h \
        Logger.h \
        MainWindow.h \
        ModInfoWidget.h \
        ModScanner.h \
        SteamRequester.h \
        mvc/AbstractModDatabaseItem.h \
        mvc/AutoExpandTreeView.h \
        mvc/ModCollection.h \
        mvc/ModDatabaseModel.h \
        mvc/ModInfo.h \
        mvc/ModNameDelegate.h \
        mvc/ModNameEditor.h \
        mvc/TreeView.h \
        mvc/proxyModels/BaseFilterProxyModel.h \
        mvc/proxyModels/CollectionModsProxyModel.h \
        mvc/proxyModels/DisabledModProxyModel.h \
        mvc/proxyModels/EnabledModProxyModel.h \
        mvc/proxyModels/ExcludedAllModsProxyModel.h \
        mvc/proxyModels/ExistsModProxyModel.h \
        mvc/proxyModels/NameFilterProxyModel.h \
        utils/RegExpPatterns.h \
        utils/applicationVersion.h

FORMS += \
        DatabaseEditor.ui \
        EditModCollectionDialog.ui \
        MainWindow.ui \
        ModInfoWidget.ui

RC_FILE = icon.rc

RESOURCES = resources.qrc \

TRANSLATIONS += lang_ru.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    ESModManager.rpy \
    esmm_lib.py \
    injection.py
