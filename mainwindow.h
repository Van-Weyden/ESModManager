#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class QSettings;
class QTranslator;

class DatabaseEditor;
class DatabaseModel;
class SteamRequester;

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr, const bool &runCheck = true);
	~MainWindow();

	void checkRowsVisibility();
	void clearSearchField();	//does not emit filterModsDisplay slot
	void hideAllRows();
	void loadDatabase();
	void requestSteamModNames();
	void saveDatabase() const;
	void scanMods();

	void setGameFolder(const QString &folderPath);
	bool setLanguage(const QString &lang);

public slots:
	void disableAllMods();
	void enableAllMods();
	void eraseDatabase();
	void filterModsDisplay(const QString &str);
	void refreshModlist();

	void runGame();

	void selectGameFolder();
	void selectModsFolder();
	void selectTempModsFolder();

	void setEnglishLanguage() {setLanguage("en_US");}
	void setRussianLanguage() {setLanguage("ru_RU");}

	void setRowVisibility(const int &rowIndex, const bool &isVisibleInFirstList);

	void showAboutInfo();

protected:
	void changeEvent(QEvent *event);

signals:
	void countOfScannedMods(int count);

private slots:
	bool openModFolder(const int &modIndex);
	void steamModNameProcessed();

private:
	bool checkGameMd5(const QString &folderPath);
	void checkOriginLauncherReplacement() const;
	void moveModFolders(QString *unmovedModsToTempFolder = nullptr, QString *unmovedModsFromTempFolder = nullptr) const;
	void moveModFoldersBack() const;
	void readSettings();
	void saveSettings() const;
	void scanMods(const QString &modsFolderPath);

	Ui::MainWindow *ui = nullptr;
	DatabaseEditor *databaseEditor_ = nullptr;

	QThread *thread_ = nullptr;
	SteamRequester *steamRequester_ = nullptr;
	QSettings *settings_ = nullptr;
	QTranslator *qtTranslator_ = nullptr;
	QTranslator *translator_ = nullptr;

	QString lang_ = "";
	QString gameFolderPath_ = "";
	QString modsFolderPath_ = "";
	QString tempModsFolderPath_ = "";

	DatabaseModel *model_ = nullptr;

	QByteArray launcherMd5_;
	int countOfScannedMods_;
};

#endif // MAINWINDOW_H
