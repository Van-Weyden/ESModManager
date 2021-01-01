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
	static constexpr const char *ApplicationVersion = "1.1";

	explicit MainWindow(QWidget *parent = nullptr, const bool runCheck = true);
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

	void setEnglishLanguage(); //inline
	void setRussianLanguage(); //inline

	void setRowVisibility(const int rowIndex, const bool isVisibleInFirstList);

	void showAboutInfo();

protected:
	void changeEvent(QEvent *event);

signals:
	void countOfScannedMods(int count);

private slots:
	bool openModFolder(const int modIndex);
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
	DatabaseEditor *m_databaseEditor = nullptr;

	QThread *m_thread = nullptr;
	SteamRequester *m_steamRequester = nullptr;
	QSettings *m_settings = nullptr;
	QTranslator *m_qtTranslator = nullptr;
	QTranslator *m_translator = nullptr;

	QString m_lang = "";
	QString m_gameFolderPath = "";
	QString m_modsFolderPath = "";
	QString m_tempModsFolderPath = "";

	DatabaseModel *m_model = nullptr;

	QByteArray m_launcherMd5;
	int m_countOfScannedMods;
};



//public:

inline void MainWindow::setEnglishLanguage()
{
	setLanguage("en_US");
}

inline void MainWindow::setRussianLanguage()
{
	setLanguage("ru_RU");
}

#endif // MAINWINDOW_H
