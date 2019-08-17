#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class QSettings;
class QTranslator;
class QTableWidgetItem;

namespace Ui {
	class MainWindow;
}

struct ModInfo
{
	QString name;
	QString folderName;
	bool enabled = true;

	operator QString() const {return folderName;}
};

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

	void fillModlistTable();
	void loadDatabase();
	void saveDatabase() const;
	void scanMods();

	void setGameFolder(const QString &folderPath);
	bool setLanguage(const QString &lang);

public slots:
	void disableAllMods();
	void enableAllMods();
	void eraseDatabase();
	void modInfoChanged(QTableWidgetItem *tableItem);
	void refreshModlist();

	void runGame();

	void selectGameFolder();
	void selectModsFolder();
	void selectTempModsFolder();

	void setEnglishLanguage() {setLanguage("en_US");}
	void setRussianLanguage() {setLanguage("ru_RU");}

	void showAboutInfo();

protected:
	void changeEvent(QEvent *event);

private:
	bool checkGameMd5(const QString &folderPath);
	void readSettings();
	void saveSettings() const;
	void scanMods(const QString &modsFolderPath);

	Ui::MainWindow *ui = nullptr;

	QSettings *settings_ = nullptr;
	QTranslator *qtTranslator_ = nullptr;
	QTranslator *translator_ = nullptr;

	QString lang_ = "";
	QString gameFolderPath_ = "";
	QString modsFolderPath_ = "";
	QString tempModsFolderPath_ = "";

	QVector<int> modlist_;
	QVector<ModInfo> database_;

	QByteArray gameMd5_;
	QByteArray launcherMd5_;
};

#endif // MAINWINDOW_H
