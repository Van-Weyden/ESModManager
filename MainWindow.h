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
	static constexpr const int CurrentApplicationVersion = applicationVersion(1, 1, 11);

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

	void showAnnouncementMessage();

protected:
	void changeEvent(QEvent *event) override;

private slots:
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
