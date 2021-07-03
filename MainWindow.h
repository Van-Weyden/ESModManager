#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class QSettings;
class QTranslator;

class DatabaseEditor;
class DatabaseModel;
class ModScanner;
class SteamRequester;

namespace Ui {
	class MainWindow;
}

static constexpr int applicationVersion(const int major, const int minor = 0, const int micro = 0)
{
	return ((major << 20) | (minor << 10) | micro);
}

inline constexpr int majorApplicationVersion(int version)
{
	return (version >> 20);
}

inline constexpr int minorApplicationVersion(int version)
{
	return ((version  & 1047552) >> 10);
}

inline constexpr int microApplicationVersion(int version)
{
	return (version & 1023);
}

QString applicationVersionToString(const int version);
int applicationVersionFromString(const QString &version);

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	static constexpr const int CurrentApplicationVersion = applicationVersion(1, 1, 13);

	explicit MainWindow(QWidget *parent = nullptr, const bool runCheck = true);
	~MainWindow();

	void checkRowsVisibility();
	void clearSearchField();	//does not call filterModsDisplay slot
	void hideAllRows();
	void loadDatabase();
	void requestSteamModNames();
	void saveDatabase() const;
	void scanMods();

	void setGameFolder(const QString &folderPath);
	bool setLanguage(const QString &lang);

public slots:
	void addShortcutToDesktop() const;
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

	void showAnnouncementMessageBox();
	void showShortcutAddMessageBox();

protected:
	void changeEvent(QEvent *event) override;

private slots:
	void openManagerFolder();
	bool openModFolder(const int modIndex);
	void steamModNameProcessed();

private:
	void checkAnnouncementPopup(const int loadedApplicationVersion);
	bool checkGameMd5(const QString &folderPath);
	void checkOriginLauncherReplacement() const;
	void restoreOriginLauncher() const;
	void moveModFolders(QString *unmovedModsToTempFolder = nullptr, QString *unmovedModsFromTempFolder = nullptr) const;
	void moveModFoldersBack() const;
	void readSettings();
	void saveSettings() const;

	void applyBackwardCompatibilityFixes(const int loadedApplicationVersion);

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
	ModScanner *m_scanner = nullptr;

	QByteArray m_launcherMd5;
	QByteArray m_previousLauncherMd5;

	static constexpr const char *DefaultGameFolderPath	= "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Everlasting Summer\\";
	static constexpr const char *ExecutableExtension	= ".exe";

	static constexpr const char *ManagerFileName		= "ESModManager";
	static constexpr const char *ProcessCheckerFileName = "ProcessChecker";
	static constexpr const char *LauncherFolderPath		= "launcher/";
	static constexpr const char *LauncherFileName		= "ESLauncher";
	static constexpr const char *CleanerFileName		= "ESModManagerCleaner";
	static constexpr const char *GameFileName			= "Everlasting Summer";
	static constexpr const char *OriginGameFileName		= "Everlasting Summer (origin)";
	static constexpr const char *ModifiedGameFileName	= "Everlasting Summer (modified)";

	static QString addExecutableExtension(const QString &fileName);			//inline

	static QString managerFileName(const bool addExtension = true);			//inline
	static QString processCheckerFileName(const bool addExtension = true);	//inline
	static QString launcherFileName(const bool addExtension = true);		//inline
	static QString launcherDataFileName();									//inline
	static QString cleanerFileName(const bool addExtension = true);			//inline
	static QString cleanerDataFileName();									//inline
	static QString gameFileName(const bool addExtension = true);			//inline
	static QString originGameFileName(const bool addExtension = true);		//inline
	static QString modifiedGameFileName(const bool addExtension = true);	//inline

	QString launcherFilePath() const;			//inline
	QString launcherDataFilePath() const;		//inline
	QString gameLauncherDataFilePath() const;	//inline
	QString cleanerFilePath() const;			//inline
	QString cleanerDataFilePath() const;		//inline
	QString gameFilePath() const;				//inline
	QString originGameFilePath() const;			//inline
	QString modifiedGameFilePath() const;		//inline
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

//private:

inline QString MainWindow::addExecutableExtension(const QString &fileName)
{
	return (fileName.endsWith(ExecutableExtension) ? fileName : fileName + ExecutableExtension);
}

inline QString MainWindow::managerFileName(const bool addExtension)
{
	return (addExtension ? addExecutableExtension(ManagerFileName) : ManagerFileName);
}

inline QString MainWindow::processCheckerFileName(const bool addExtension)
{
	return (addExtension ? addExecutableExtension(ProcessCheckerFileName) : ProcessCheckerFileName);
}

inline QString MainWindow::launcherFileName(const bool addExtension)
{
	return (addExtension ? addExecutableExtension(LauncherFileName) : LauncherFileName);
}

inline QString MainWindow::launcherDataFileName()
{
	return "LaunchedProgram.ini";
}

inline QString MainWindow::cleanerFileName(const bool addExtension)
{
	return (addExtension ? addExecutableExtension(CleanerFileName) : CleanerFileName);
}

inline QString MainWindow::cleanerDataFileName()
{
	return "ESModManagerCleaner.dat";
}

inline QString MainWindow::gameFileName(const bool addExtension)
{
	return (addExtension ? addExecutableExtension(GameFileName) : GameFileName);
}

inline QString MainWindow::originGameFileName(const bool addExtension)
{
	return (addExtension ? addExecutableExtension(OriginGameFileName) : OriginGameFileName);
}

inline QString MainWindow::modifiedGameFileName(const bool addExtension)
{
	return (addExtension ? addExecutableExtension(ModifiedGameFileName) : ModifiedGameFileName);
}




inline QString MainWindow::launcherFilePath() const
{
	return LauncherFolderPath + launcherFileName();
}

inline QString MainWindow::launcherDataFilePath() const
{
	return LauncherFolderPath + launcherDataFileName();
}

inline QString MainWindow::gameLauncherDataFilePath() const
{
	return m_gameFolderPath + launcherDataFileName();
}

inline QString MainWindow::cleanerFilePath() const
{
	return m_gameFolderPath + cleanerFileName();
}

inline QString MainWindow::cleanerDataFilePath() const
{
	return m_gameFolderPath + cleanerDataFileName();
}

inline QString MainWindow::gameFilePath() const
{
	return m_gameFolderPath + gameFileName();
}

inline QString MainWindow::originGameFilePath() const
{
	return m_gameFolderPath + originGameFileName();
}

inline QString MainWindow::modifiedGameFilePath() const
{
	return m_gameFolderPath + modifiedGameFileName();
}

#endif // MAINWINDOW_H
