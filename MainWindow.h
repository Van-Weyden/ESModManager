#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <functional>

#include <QMainWindow>
#include <QString>

class QItemSelectionModel;
class QSettings;
class QTranslator;

class DatabaseEditor;
class ModDatabaseModel;
class ModFilterProxyModel;
class ModInfo;
class ModScanner;
class SteamRequester;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void checkRowsVisibility();
    void clearSearchField();    //does not call filterModsDisplay slot
    void loadDatabase();
    void requestSteamModNames();
    void saveDatabase() const;

    bool setLanguage(const QString &lang);

public slots:
    void addShortcutToDesktop() const;
    void disableAllMods();
    void enableAllMods();
    void refreshModlist();

    void runGame();

    void setEnglishLanguage(); //inline
    void setRussianLanguage(); //inline

    void showAboutInfo();

    void showAnnouncementMessageBox();
    void showShortcutAddMessageBox();

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void openManagerFolder();
    bool openModFolder(const QModelIndex &index);
    void steamModNameProcessed();
    void showModContextMenu(const QPoint &pos);

    /**
     * @brief Perform function 'action' on the each mod in the selection.
     * Action may return false to stop mod processing.
     */
    void performOnSelectedMods(std::function<bool(ModDatabaseModel *model, QModelIndex index)> action);

private:
    QItemSelectionModel *selectedMods() const;
    void initActions();
    void setupItemContextMenu(const ModInfo &item) const;
    bool isGameFolderValid(const QString &folderPath);
    void checkAnnouncementPopup(const int loadedApplicationVersion);

    void updateDisabledModsFile();

    void readSettings();
    void saveSettings() const;

    void applyBackwardCompatibilityFixes(const int loadedApplicationVersion);
    QString rkkOrionMessage();

    QString gameFileName(const bool addExtension = true) const;             //inline
    QString gameFilePath() const;                                           //inline

    static QString addExecutableExtension(const QString &fileName);         //inline
    static QString managerFileName(const bool addExtension = true);         //inline
    static QString modDatabaseFileName();                                   //inline

private:
    Ui::MainWindow *ui = nullptr;
    DatabaseEditor *m_databaseEditor = nullptr;

    QThread *m_requesterThread = nullptr;
    QThread *m_scannerThread = nullptr;
    SteamRequester *m_steamRequester = nullptr;
    QSettings *m_settings = nullptr;
    QTranslator *m_qtTranslator = nullptr;
    QTranslator *m_translator = nullptr;

    QString m_lang = "";
    QString m_gameFolderPath = "";
    QString m_modsFolderPath = "";

    ModDatabaseModel *m_model = nullptr;
    ModFilterProxyModel *m_enabledModsModel = nullptr;
    ModFilterProxyModel *m_disabledModsModel = nullptr;
    ModScanner *m_scanner = nullptr;

    QMenu *m_itemContextMenu = nullptr;
    QAction *m_setEnabledAction = nullptr;
    QAction *m_setLockedAction = nullptr;
    QAction *m_setUnlockedAction = nullptr;
    QAction *m_setMarkedAction = nullptr;
    QAction *m_openFolderAction = nullptr;
    QAction *m_renameAction = nullptr;
    QAction *m_openSteamPageAction = nullptr;

    static constexpr const char *DefaultGameFolderPath =
        "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Everlasting Summer\\";
    static constexpr const char *ManagerFileName        = "ESModManager";
    static constexpr const char *ExecutableExtension    = ".exe";


    static const bool Is64BitOs;
    QString m_gameFileName;
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

inline QString MainWindow::gameFileName(const bool addExtension) const
{
    return (addExtension ? addExecutableExtension(m_gameFileName) : m_gameFileName);
}

inline QString MainWindow::gameFilePath() const
{
    return m_gameFolderPath + gameFileName();
}

inline QString MainWindow::addExecutableExtension(const QString &fileName)
{
    return (fileName.endsWith(ExecutableExtension) ? fileName : fileName + ExecutableExtension);
}

inline QString MainWindow::managerFileName(const bool addExtension)
{
    return (addExtension ? addExecutableExtension(ManagerFileName) : ManagerFileName);
}

inline QString MainWindow::modDatabaseFileName()
{
    return "mods_database.json";
}

#endif // MAINWINDOW_H
