#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class QSettings;
class QTranslator;

class DatabaseEditor;
class ModDatabaseModel;
class ModScanner;
class SteamRequester;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr, const bool runCheck = true);
    ~MainWindow();

    void checkRowsVisibility();
    void clearSearchField();    //does not call filterModsDisplay slot
    void hideAllRows();
    void loadDatabase();
    void requestSteamModNames();
    void saveDatabase() const;
    void scanMods();

    void setGameFolder(const QString &folderPath, const bool refreshModlist = true);
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
    bool isGameFolderValid(const QString &folderPath);
    bool isWaitingLauncher(const QByteArray &fileChecksum) const; //inline
    void checkAnnouncementPopup(const int loadedApplicationVersion);
    bool checkOriginGameLauncher(const QString &folderPath) const;
    void checkOriginLauncherReplacement();
    void restoreOriginLauncher() const;
    void moveModFolders(QString *unmovedModsToTempFolder = nullptr,
                        QString *unmovedModsFromTempFolder = nullptr) const;
    void moveModFoldersBack() const;
    void readSettings();
    void saveSettings() const;

    void applyBackwardCompatibilityFixes(const int loadedApplicationVersion);
    QString rkkOrionMessage();

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

    ModDatabaseModel *m_modDatabaseModel = nullptr;
    ModScanner *m_scanner = nullptr;

    /**
     * @brief All WaitingLauncher versions hashes, from newest to oldest.
     */
    QList<QByteArray> m_launcherChecksumDatabase;
    QByteArray m_launcherChecksum;

    static constexpr const char *DefaultGameFolderPath =
        "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Everlasting Summer\\";
    static constexpr const char *ExecutableExtension    = ".exe";

    static constexpr const char *ManagerFileName        = "ESModManager";
    static constexpr const char *ProcessCheckerFileName = "ProcessChecker";
    static constexpr const char *LauncherFolderPath     = "launcher/";
    static constexpr const char *LauncherFileName       = "ESLauncher";
    static constexpr const char *CleanerFileName        = "ESModManagerCleaner";

    static const bool Is64BitOs;
    QString m_gameFileName;

    static QString addExecutableExtension(const QString &fileName);         //inline

    static QString managerFileName(const bool addExtension = true);         //inline
    static QString processCheckerFileName(const bool addExtension = true);  //inline
    static QString launcherFileName(const bool addExtension = true);        //inline
    static QString launcherDataFileName();                                  //inline
    static QString modDatabaseFileName();                                   //inline
    static QString launcherChecksumDatabaseFileName();                      //inline
    static QString cleanerFileName(const bool addExtension = true);         //inline
    static QString cleanerDataFileName();                                   //inline
    QString gameFileName(const bool addExtension = true) const;             //inline
    QString originGameFileName(const bool addExtension = true) const;       //inline
    QString modifiedGameFileName(const bool addExtension = true) const;     //inline

    QString launcherFilePath() const;                   //inline
    static QString launcherDataFilePath();              //inline
    static QString launcherChecksumDatabaseFilePath();  //inline
    QString gameLauncherDataFilePath() const;           //inline
    QString cleanerFilePath() const;                    //inline
    QString cleanerDataFilePath() const;                //inline
    QString gameFilePath() const;                       //inline
    QString originGameFilePath() const;                 //inline
    QString modifiedGameFilePath() const;               //inline
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

inline bool MainWindow::isWaitingLauncher(const QByteArray &fileChecksum) const
{
    return m_launcherChecksumDatabase.contains(fileChecksum);
}

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

inline QString MainWindow::modDatabaseFileName()
{
    return "mods_database.json";
}

inline QString MainWindow::launcherChecksumDatabaseFileName()
{
    return "ChecksumDatabase.dat";
}

inline QString MainWindow::cleanerFileName(const bool addExtension)
{
    return (addExtension ? addExecutableExtension(CleanerFileName) : CleanerFileName);
}

inline QString MainWindow::cleanerDataFileName()
{
    return "ESModManagerCleaner.dat";
}

inline QString MainWindow::gameFileName(const bool addExtension) const
{
    return (addExtension ? addExecutableExtension(m_gameFileName) : m_gameFileName);
}

inline QString MainWindow::originGameFileName(const bool addExtension) const
{
    return (addExtension ? addExecutableExtension(m_gameFileName + " (origin)") : m_gameFileName + " (origin)");
}

inline QString MainWindow::modifiedGameFileName(const bool addExtension) const
{
    return (addExtension ? addExecutableExtension(m_gameFileName + " (modified)") : m_gameFileName + " (modified)");
}

inline QString MainWindow::launcherFilePath() const
{
    return LauncherFolderPath + launcherFileName();
}

inline QString MainWindow::launcherDataFilePath()
{
    return LauncherFolderPath + launcherDataFileName();
}

inline QString MainWindow::launcherChecksumDatabaseFilePath()
{
    return LauncherFolderPath + launcherChecksumDatabaseFileName();
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
