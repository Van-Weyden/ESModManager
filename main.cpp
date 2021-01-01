#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	MainWindow mw;

	if (mw.isEnabled()) {
		mw.show();
	} else {
		return 0;
	}


	return app.exec();

}
