#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QSharedMemory>

#include "Logger.h"
#include "MainWindow.h"

//#define ESMM_FILE_LOG

int main(int argc, char *argv[])
{
#if defined(ESMM_FILE_LOG) && !defined(QT_NO_DEBUG) || defined(ESMM_FORCE_FILE_LOG)
    Logger::attach();
#endif

    QApplication app(argc, argv);

    QSharedMemory guard("everlasting-summer-mod-manager-single-app-guard");
    if (!guard.create(1, QSharedMemory::ReadWrite)) {
        QMessageBox::critical(
            nullptr, QObject::tr("Mod manager is running"),
            QObject::tr("Mod manager is already running!"),
            QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton
        );
        return -2;
    }

    MainWindow mw;
    mw.show();

    return app.exec();
}
