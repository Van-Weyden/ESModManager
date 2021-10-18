#include <QApplication>
#include <QDebug>

#include "Logger.h"
#include "MainWindow.h"

//#define ESMM_FILE_LOG

int main(int argc, char *argv[])
{
#if defined(ESMM_FILE_LOG) && !defined(QT_NO_DEBUG) || defined(ESMM_FORCE_FILE_LOG)
    Logger::attach();
#endif

    QApplication app(argc, argv);

    MainWindow mw;

    if (mw.isEnabled()) {
        mw.show();
    } else {
        return 0;
    }

    return app.exec();
}
