#include <QCollator>
#include <QCryptographicHash>
#include <QDebug>
#include <QDesktopServices>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QSettings>
#include <QThread>
#include <QTranslator>

#include "applicationVersion.h"
#include "DatabaseEditor.h"
#include "ModDatabaseModel.h"
#include "ModScanner.h"
#include "SteamRequester.h"

#include "MainWindow.h"
#include "ui_MainWindow.h"

inline constexpr const char *gameFileName(const bool is64BitOs);
QByteArray fileChecksumHex(const QString &fileName,
                           QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Md5);
void rewriteFileIfDataIsDifferent(const QString &fileName, const QByteArray &newData);

class WaitingLauncherArgs
{
public:
    WaitingLauncherArgs(const QString &programFolderPath, const QString &programName);

    void setProgramFolderPath(const QString &programFolderPath);
    void setProgramName(const QString &programName);

    void setIsMonitoringNeeded(const bool isMonitoringNeeded);
    void setDontLaunchIfAlreadyLaunched(const bool dontLaunchIfAlreadyLaunched);
    void setLaunchProgramAfterClose(const bool launchProgramAfterClose);

    void setProgramOnErrorFolderPath(const QString &programOnErrorFolderPath);
    void setProgramOnErrorName(const QString &programOnErrorName);
    void setProgramAfterCloseFolderPath(const QString &programAfterCloseFolderPath);
    void setProgramAfterCloseName(const QString &programAfterCloseName);

    QString toString() const;
    QByteArray toUtf8() const;

private:
    QString m_programFolderPath;
    QString m_programName;
    bool m_isMonitoringNeeded               = false;
    bool m_dontLaunchIfAlreadyLaunched      = false;
    bool m_launchProgramAfterClose          = false;
    QString m_programOnErrorFolderPath      = QString();
    QString m_programOnErrorName            = QString();
    QString m_programAfterCloseFolderPath   = QString();
    QString m_programAfterCloseName         = QString();
};

///class MainWindow:

const bool MainWindow::Is64BitOs = QSysInfo::currentCpuArchitecture().contains(QLatin1String("64"));

//public:

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    m_modDatabaseModel = new ModDatabaseModel();
    m_steamRequester = new SteamRequester(m_modDatabaseModel);
    m_thread = new QThread;
    connect(m_thread, SIGNAL(started()), m_steamRequester, SLOT(requestModNames()));
    connect(m_steamRequester, SIGNAL(modProcessed()), this, SLOT(steamModNameProcessed()), Qt::BlockingQueuedConnection);
    connect(m_steamRequester, SIGNAL(finished()), m_thread, SLOT(quit()));
    m_steamRequester->moveToThread(m_thread);

    m_scanner = new ModScanner(this);

    ui->setupUi(this);

    ui->engLangButton->setIcon(QIcon(":/images/Flag-United-States.ico"));
    ui->rusLangButton->setIcon(QIcon(":/images/Flag-Russia.ico"));

    ui->enabledModsList->setStyleSheet("QListView::item:!selected:!hover { border-bottom: 1px solid #E5E5E5; }");
    ui->enabledModsList->setModel(m_modDatabaseModel);


    ui->disabledModsList->setStyleSheet("QListView::item:!selected:!hover { border-bottom: 1px solid #E5E5E5; }");
    ui->disabledModsList->setModel(m_modDatabaseModel);

    //A small crutch for highlighting when you hover the mouse, but you couldn't select the line
    connect(ui->enabledModsList->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            ui->enabledModsList, SLOT(clearSelection()));
    connect(ui->disabledModsList->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            ui->disabledModsList, SLOT(clearSelection()));

    ui->progressLabel->hide();
    ui->progressBar->hide();

    m_databaseEditor = new DatabaseEditor();

    m_settings = new QSettings(QString("settings.ini"), QSettings::IniFormat, this);
    m_qtTranslator = new QTranslator();
    m_translator = new QTranslator();

    //connections:

    //Signals from form objects:
    connect(ui->actionAddShortcutToDesktop, SIGNAL(triggered()), this, SLOT(addShortcutToDesktop()));
    connect(ui->actionOpenManagerFolder, SIGNAL(triggered()), this, SLOT(openManagerFolder()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAboutInfo()));
    connect(ui->actionAnnouncements, SIGNAL(triggered()), this, SLOT(showAnnouncementMessageBox()));

    connect(ui->actionCompleteModNames, SIGNAL(toggled(bool)),
            m_modDatabaseModel, SLOT(setCompleteModNames(bool)));
    connect(ui->actionUseSteamModNames, SIGNAL(toggled(bool)),
            m_modDatabaseModel, SLOT(setUsingSteamModNames(bool)));

    connect(ui->actionOpenDatabaseEditor, SIGNAL(triggered()), m_databaseEditor, SLOT(show()));

    connect(ui->engLangButton, SIGNAL(clicked()), this, SLOT(setEnglishLanguage()));
    connect(ui->rusLangButton, SIGNAL(clicked()), this, SLOT(setRussianLanguage()));

    connect(ui->disableAllButton, SIGNAL(clicked()), this, SLOT(disableAllMods()));
    connect(ui->enableAllButton, SIGNAL(clicked()), this, SLOT(enableAllMods()));
    connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(refreshModlist()));
    connect(ui->runButton, SIGNAL(clicked()), this, SLOT(runGame()));

    connect(ui->enabledModsList, SIGNAL(clicked(QModelIndex)), m_modDatabaseModel, SLOT(disableMod(QModelIndex)));
    connect(ui->disabledModsList, SIGNAL(clicked(QModelIndex)), m_modDatabaseModel, SLOT(enableMod(QModelIndex)));

    connect(ui->clearSearchPushButton, SIGNAL(clicked()), ui->searchLineEdit, SLOT(clear()));
    connect(ui->searchLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterModsDisplay(QString)));


    //Other signals:
    connect(m_steamRequester, SIGNAL(finished()), ui->progressLabel, SLOT(hide()));
    connect(m_steamRequester, SIGNAL(finished()), ui->progressBar, SLOT(hide()));
    connect(m_databaseEditor, SIGNAL(openModFolder(int)), this, SLOT(openModFolder(int)));
    connect(m_modDatabaseModel, SIGNAL(modCheckStateChanged(int, bool)), this, SLOT(setRowVisibility(int, bool)));

    //this->adjustSize();

    readSettings();
    loadDatabase();

    if (!isGameFolderValid(m_gameFolderPath)) {
        QString gamePath = DefaultGameFolderPath;
        if (isGameFolderValid(gamePath)) {
            m_gameFolderPath = gamePath;
        } else {
            gamePath = QCoreApplication::applicationDirPath().section('/', 0, -6);
            gamePath.replace('/', '\\');
            gamePath.append("\\common\\Everlasting Summer\\");
            if (isGameFolderValid(gamePath)) {
                m_gameFolderPath = gamePath;
            } else {
                m_modDatabaseModel->setModsExistsState(false);
                checkRowsVisibility();
            }
    }

        QDir dir = QCoreApplication::applicationDirPath();
        dir.cd("../../");
        m_modsFolderPath = dir.absolutePath();
    }

    refreshModlist();
    m_databaseEditor->setModel(m_modDatabaseModel, 1);
}

MainWindow::~MainWindow()
{
    if (isEnabled()) {
        saveSettings();

        m_modDatabaseModel->sortDatabase();
        saveDatabase();
    }

    QApplication::removeTranslator(m_translator);
    QApplication::removeTranslator(m_qtTranslator);
    m_steamRequester->deleteLater();
    m_thread->deleteLater();
    delete m_translator;
    delete m_qtTranslator;
    delete m_settings;
    delete m_modDatabaseModel;
    delete m_databaseEditor;
    delete m_scanner;

    delete ui;
}

void MainWindow::checkRowsVisibility()
{
    clearSearchField();

    bool enabled, exists;
    int databaseSize = m_modDatabaseModel->databaseSize();
    for (int rowIndex = 0; rowIndex < databaseSize; ++rowIndex) {
        enabled = m_modDatabaseModel->modIsEnabled(rowIndex);
        exists = m_modDatabaseModel->modIsExists(rowIndex);

        ui->enabledModsList->setRowHidden(rowIndex, !exists || !enabled);
        ui->disabledModsList->setRowHidden(rowIndex, !exists || enabled);
    }
}

void MainWindow::clearSearchField()
{
    ui->searchLineEdit->blockSignals(true);
    ui->searchLineEdit->clear();
    ui->searchLineEdit->blockSignals(false);
}

void MainWindow::hideAllRows()
{
    clearSearchField();

    int rowCount = m_modDatabaseModel->databaseSize();
    for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
        ui->enabledModsList->setRowHidden(rowIndex, true);
        ui->disabledModsList->setRowHidden(rowIndex, true);
    }
}

void MainWindow::loadDatabase()
{
    QFile file(modDatabaseFileName());
    if (file.exists()) {
        QJsonDocument database;
        //QJsonParseError err;
        QJsonArray mods;
        QJsonObject mod;
        ModInfo modInfo;

        file.open(QFile::ReadOnly);
        database = QJsonDocument::fromJson(file.readAll()/*, &err*/);
        file.close();

        mods = database.array();
        for (int i = 0; i < mods.size(); i++) {
            mod = mods.at(i).toObject();
            modInfo.name = mod["Name"].toString();
            modInfo.folderName = mod["Folder name"].toString();
            modInfo.steamName = mod["Steam name"].toString();
            modInfo.enabled = mod["Is enabled"].toBool();

            m_modDatabaseModel->add(modInfo);
        }
    }
}

void MainWindow::requestSteamModNames()
{
    if (!m_steamRequester->isRunning()) {
        m_thread->start();
    }
}

void MainWindow::saveDatabase() const
{
    QJsonDocument database;
    QJsonArray mods;
    QJsonObject mod;
    ModInfo modInfo;

    for (int i = 0; i < m_modDatabaseModel->databaseSize(); ++i) {
        modInfo = m_modDatabaseModel->modInfo(i);
        mod["Name"] = modInfo.name;
        mod["Folder name"] = modInfo.folderName;
        mod["Steam name"] = modInfo.steamName;
        mod["Is enabled"] = modInfo.enabled;
        mods.append(mod);
    }
    database.setArray(mods);
    rewriteFileIfDataIsDifferent("mods_database.json", database.toJson());
}

void MainWindow::scanMods()
{
    m_modDatabaseModel->setModsExistsState(false);

    m_scanner->scanMods(m_modsFolderPath, *m_modDatabaseModel);

    m_modDatabaseModel->sortDatabase();
}

bool MainWindow::setLanguage(const QString &lang)
{
    m_lang = lang;
    bool isTranslationLoaded = m_translator->load(QString(":/lang/lang_") + m_lang, ":/lang/");
    QApplication::installTranslator(m_translator);
    m_qtTranslator->load(QString(":/lang/qtbase_") + m_lang, ":/lang/");
    QApplication::installTranslator(m_qtTranslator);

    return isTranslationLoaded;
}

//public slots:

void MainWindow::addShortcutToDesktop() const
{
    QFile::link(QCoreApplication::applicationDirPath() + '/' + managerFileName(),
                QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + windowTitle() +".lnk");
}

void MainWindow::disableAllMods()
{
    m_modDatabaseModel->blockSignals(true);

    for (int i = 0; i < m_modDatabaseModel->databaseSize(); ++i) {
        if (m_modDatabaseModel->modInfo(i).existsAndEnabledCheck(true, true)) {
            m_modDatabaseModel->modInfoRef(i).enabled = false;
            ui->enabledModsList->setRowHidden(i, true);
            ui->disabledModsList->setRowHidden(i, false);
        }
    }

    m_modDatabaseModel->blockSignals(false);

    ui->searchLineEdit->clear();
}

void MainWindow::enableAllMods()
{
    m_modDatabaseModel->blockSignals(true);

    for (int i = 0; i < m_modDatabaseModel->databaseSize(); ++i) {
        if (m_modDatabaseModel->modInfo(i).existsAndEnabledCheck(true, false)) {
            m_modDatabaseModel->modInfoRef(i).enabled = true;
            ui->enabledModsList->setRowHidden(i, false);
            ui->disabledModsList->setRowHidden(i, true);
        }
    }

    m_modDatabaseModel->blockSignals(false);

    ui->searchLineEdit->clear();
}

void MainWindow::filterModsDisplay(const QString &str)
{
    int rowCount = m_modDatabaseModel->databaseSize();
    for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
        if (m_modDatabaseModel->modIsExists(rowIndex)) {
            QString modName = m_modDatabaseModel->data(m_modDatabaseModel->index(rowIndex)).toString();
            if (!modName.contains(str, Qt::CaseSensitivity::CaseInsensitive)) {
                bool enabled = m_modDatabaseModel->modIsEnabled(rowIndex);
                (enabled ? ui->enabledModsList : ui->disabledModsList)->setRowHidden(rowIndex, true);
            }
        }
    }
}

void MainWindow::refreshModlist()
{
    if (m_modsFolderPath.isEmpty()) {
        return;
    }

    bool wasOpened = this->isVisible();
    bool editorWasOpened = m_databaseEditor->isVisible();

    if (editorWasOpened) {
        m_databaseEditor->close();
    }

    if (wasOpened) {
        this->hide();
    }

    int modsCount = QDir(m_modsFolderPath).entryList(QDir::Dirs|QDir::NoDotAndDotDot).count();
    QProgressDialog progressDialog(tr("Mod Manager: scanning installed mods..."), "", 0, modsCount);
    progressDialog.setCancelButton(nullptr);
    connect(m_scanner, SIGNAL(modScanned(int)), &progressDialog, SLOT(setValue(int)));
    progressDialog.show();

    ui->progressBar->setMaximum(modsCount);
    ui->progressBar->setValue(0);

    scanMods();

    ui->progressLabel->show();
    ui->progressBar->show();
    requestSteamModNames();
    checkRowsVisibility();

    if (wasOpened) {
        this->show();
    }

    if (editorWasOpened) {
        m_databaseEditor->show();
    }
}

void MainWindow::runGame()
{
    if (!isGameFolderValid(m_gameFolderPath)) {
        QMessageBox::critical(
            this,
            tr("Wrong game %1").arg(ExecutableExtension),
            tr("Game folder doesn't contains '%1'!").arg(gameFileName()),
            QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton
        );

        return;
    }

    if (m_databaseEditor->isVisible()) {
        m_databaseEditor->close();
    }

    updateDisabledModsFile();

    QProcess gameLauncher;
    gameLauncher.setWorkingDirectory(QDir(m_gameFolderPath).absolutePath());
    gameLauncher.setProgram(QFileInfo(gameFilePath()).absoluteFilePath());

    //We must ensure that autoexit flag is up to date because it will be read by our .rpy script
    m_settings->setValue("General/bAutoexit", ui->actionAutoexit->isChecked());
    m_settings->sync();

    gameLauncher.start();
    gameLauncher.waitForStarted(-1);

    if (ui->actionAutoexit->isChecked()) {
        QApplication::exit();
    }
}

void MainWindow::showAboutInfo()
{
    QMessageBox messageAbout(
        QMessageBox::NoIcon,
        tr("About ") + this->windowTitle(), "",
        QMessageBox::StandardButton::Close, this,
        Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint
    );
    messageAbout.setTextFormat(Qt::TextFormat::RichText);
    messageAbout.setText(
        tr("Everlasting Summer mod manager v.") + applicationVersionToString() + ".<br>" +
        tr("Author:") + " <a href='https://steamcommunity.com/id/van_weyden/'>Slavyan</a><br>" +
        tr("Help in testing:") +
        " <a href='https://steamcommunity.com/profiles/76561198058938676/'>Hatsune Miku</a>,"
        " 🔰 <a href='https://steamcommunity.com/id/lena_sova/'>" + tr("Lena") + "</a>🔰 ,"
        " <a href='https://vk.com/svet_mag'>" + tr("Alexey Golikov") + "</a>"
        "<br><br>" +
        tr("This program is used to 'fix' conflicts of mods and speed up the launch of the game. "
           "Before launching the game, all unselected mods are moved to another folder, "
           "so the game engine will not load them.") +
        "<br><br>" + rkkOrionMessage()
    );
    messageAbout.setInformativeText(
        tr("You can leave your questions/suggestions") +
        " <a href='https://steamcommunity.com/sharedfiles/filedetails/?id=1826799366'>" +
        tr("in the Steam Workshop") + "</a> " +
        tr("or") +
        " <a href='https://discord.gg/d2qYfsc'>" +
        tr("on the Discord server") + "</a>."
    );
    messageAbout.exec();
}

void MainWindow::showAnnouncementMessageBox()
{
    QMessageBox messageAbout(QMessageBox::Icon::Information, tr("Announcement"), "",
                             QMessageBox::StandardButton::Close,
                             nullptr, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    messageAbout.setTextFormat(Qt::TextFormat::RichText);
    messageAbout.setText(rkkOrionMessage());

    messageAbout.exec();
}

void MainWindow::showShortcutAddMessageBox()
{
    QMessageBox messageAbout(QMessageBox::Icon::Warning, tr("Warning"), "",
                             QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                             nullptr, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    messageAbout.setText(
        tr("This manager modifies the game files in such a way that when the game is started via Steam, "
           "the manager will be launched first.\n\n"
           "However, due to periodic checks of the game files integrity by Steam, "
           "the original files can be restored. In this case, the manager will also start before the game, "
           "but only if the game can load all installed mods.\n\n In other words, "
           "if there are too many mods, and Steam will restore the original game launcher, "
           "the manager will need to be launched manually from its folder.\n\n"
           "You can now add a shortcut to the desktop to make it easier to launch the manager in such situations.")
    );
    messageAbout.setInformativeText(tr("Do you want to add a manager shortcut to your desktop?"));
    messageAbout.setAcceptDrops(true);

    if (messageAbout.exec() == QMessageBox::StandardButton::Yes) {
        addShortcutToDesktop();
    }
}

//protected:

void MainWindow::changeEvent(QEvent *event)
{
   if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
   } else {
        QMainWindow::changeEvent(event);
   }
}

//private slots:

void MainWindow::openManagerFolder()
{
    QDesktopServices::openUrl(QUrl("file:///" + QCoreApplication::applicationDirPath()));
}

bool MainWindow::openModFolder(const int modIndex)
{
    if (QDir().exists(m_modsFolderPath + m_modDatabaseModel->modFolderName(modIndex))) {
        return QDesktopServices::openUrl(
            QUrl("file:///" + m_modsFolderPath + m_modDatabaseModel->modFolderName(modIndex))
        );
    }

    return false;
}

void MainWindow::steamModNameProcessed()
{
    ui->progressBar->setValue(ui->progressBar->value() + 1);
    if (ui->progressBar->value() >= ui->progressBar->maximum()) {
        ui->progressLabel->hide();
        ui->progressBar->hide();
    }
}

//private:

bool MainWindow::isGameFolderValid(const QString &folderPath)
{
    //In case of different versions of the game, it is necessary to double-check the name of the game launcher
    m_gameFileName = ::gameFileName(Is64BitOs);

    bool isFolderValid = QFileInfo::exists(folderPath + gameFileName());

    if (!isFolderValid && !Is64BitOs) {
        //The user may have chosen an older version of the game without a separate 32-bit launcher
        m_gameFileName = ::gameFileName(true);
        isFolderValid = QFileInfo::exists(folderPath + gameFileName());

        if (!isFolderValid) {
            //Definitely wrong folder, restore the name of the game launcher
            m_gameFileName = ::gameFileName(false);
        }
    }

    return isFolderValid;
}

void MainWindow::checkAnnouncementPopup(const int loadedApplicationVersion)
{
    if (loadedApplicationVersion == currentApplicationVersion()) {
        return;
    }

    if (loadedApplicationVersion < applicationVersion(1, 1, 12)) {
        showShortcutAddMessageBox();
        showAnnouncementMessageBox();
    }
}

void MainWindow::updateDisabledModsFile()
{
    QString disabledMods;
    int databaseSize = m_modDatabaseModel->databaseSize();
    for (int i = 0; i < databaseSize; ++i) {
        const auto &modInfo = m_modDatabaseModel->modInfo(i);
        if (modInfo.exists && !modInfo.enabled) {
            disabledMods.append(modInfo.folderName + "\n");
        }
    }
    rewriteFileIfDataIsDifferent("disabled_mods.txt", disabledMods.toUtf8());
}

void MainWindow::readSettings()
{
    QVariant value;
    QVariant invalidValue;
    int loadedApplicationVersion = applicationVersion(1, 0, 0);

    if (m_settings->contains("General/sAppVersion")) {
        value = m_settings->value("General/sAppVersion");
        if (value != invalidValue) {
            loadedApplicationVersion = applicationVersionFromString(value.toString());
        }
    }

    if (m_settings->contains("General/sLang")) {
        value = m_settings->value("General/sLang");
        if (value != invalidValue) {
            m_lang = value.toString();
        }
    }

    if (m_lang.isEmpty()) {
        if (m_translator->load(QString(":/lang/lang_") + QLocale::system().name(), ":/lang/")) {
            QApplication::installTranslator(m_translator);
            m_qtTranslator->load(QString(":/lang/qtbase_") + QLocale::system().name(), ":/lang/");
            QApplication::installTranslator(m_qtTranslator);
            m_lang = QLocale::system().name();
        } else {
            m_lang = "en_US";
        }
    } else {
        setLanguage(m_lang);
    }

    if (m_settings->contains("General/bMaximized")) {
        value = m_settings->value("General/bMaximized");
        if (value != invalidValue && value.toBool() == true) {
            this->setWindowState(Qt::WindowMaximized);
        } else if (m_settings->contains("General/qsSize")) {
            value = m_settings->value("General/qsSize");
            if (value != invalidValue) {
                this->resize(value.toSize());
            }
        }
    }

    if (m_settings->contains("General/sGameFolder")) {
        value = m_settings->value("General/sGameFolder");
        if (value != invalidValue) {
            m_gameFolderPath = value.toString();
        }
    }

    if (m_settings->contains("General/sModsFolder")) {
        value = m_settings->value("General/sModsFolder");
        if (value != invalidValue) {
            m_modsFolderPath = value.toString();
        }
    }

    if (m_settings->contains("General/bAutoexit")) {
        value = m_settings->value("General/bAutoexit");
        if (value != invalidValue) {
            ui->actionAutoexit->setChecked(value.toBool());
        }
    }

    if (m_settings->contains("General/bLaunchBeforeGame")) {
        value = m_settings->value("General/bLaunchBeforeGame");
        if (value != invalidValue) {
            ui->actionLaunchBeforeGame->setChecked(value.toBool());
        }
    }

    if (m_settings->contains("General/bCompleteModNames")) {
        value = m_settings->value("General/bCompleteModNames");
        if (value != invalidValue) {
            ui->actionCompleteModNames->setChecked(value.toBool());
        }
    }

    if (m_settings->contains("General/bUseSteamModNames")) {
        value = m_settings->value("General/bUseSteamModNames");
        if (value != invalidValue) {
            ui->actionUseSteamModNames->setChecked(value.toBool());
        }
    }

    if (m_settings->contains("Editor/bMaximized")) {
        value = m_settings->value("Editor/bMaximized");
        if (value != invalidValue && value.toBool() == true) {
            m_databaseEditor->setWindowState(Qt::WindowMaximized);
        } else if (m_settings->contains("Editor/qsSize")) {
            value = m_settings->value("Editor/qsSize");
            if (value != invalidValue) {
                m_databaseEditor->resize(value.toSize());
            }
        }
    }

    applyBackwardCompatibilityFixes(loadedApplicationVersion);
    checkAnnouncementPopup(loadedApplicationVersion);
}

void MainWindow::saveSettings() const
{
    m_settings->clear();
    m_settings->setValue("General/sAppVersion", applicationVersionToString());
    m_settings->setValue("General/sLang", m_lang);
    m_settings->setValue("General/bMaximized", this->isMaximized());
    m_settings->setValue("General/qsSize", this->size());
    m_settings->setValue("General/sGameFolder", m_gameFolderPath);
    m_settings->setValue("General/sModsFolder", m_modsFolderPath);
    m_settings->setValue("General/bAutoexit", ui->actionAutoexit->isChecked());
    m_settings->setValue("General/bLaunchBeforeGame", ui->actionLaunchBeforeGame->isChecked());
    m_settings->setValue("General/bCompleteModNames", ui->actionCompleteModNames->isChecked());
    m_settings->setValue("General/bUseSteamModNames", ui->actionUseSteamModNames->isChecked());
    m_settings->setValue("Editor/bMaximized", m_databaseEditor->isMaximized());
    m_settings->setValue("Editor/qsSize", m_databaseEditor->size());
}

void MainWindow::applyBackwardCompatibilityFixes(const int loadedApplicationVersion)
{
    if (loadedApplicationVersion == currentApplicationVersion()) {
        return;
    }

    //FIXME: move mods from default temp folder
}

QString MainWindow::rkkOrionMessage()
{
    return tr("If you need similar functionality on your Android device, "
              "you can use the RKK Orion client from A&A Creative Team:") + "<br>"
              "<a href='https://play.google.com/store/apps/details?id=com.alativity.esorion.installer'>" +
              tr("RKK Orion in Google Play Market") + "</a>" + "<br>"
              "<a href='https://fl.alativity.com/ZCvBg'>" + tr("RKK Orion Project Discord") + "</a>";
}


///class WaitingLauncherArgs:

inline WaitingLauncherArgs::WaitingLauncherArgs(const QString &programFolderPath, const QString &programName) :
    m_programFolderPath(programFolderPath),
    m_programName(programName)
{}

inline void WaitingLauncherArgs::setProgramFolderPath(const QString &programFolderPath)
{
    m_programFolderPath = programFolderPath;
}

inline void WaitingLauncherArgs::setProgramName(const QString &programName)
{
    m_programName = programName;
}

inline void WaitingLauncherArgs::setIsMonitoringNeeded(const bool isMonitoringNeeded)
{
    m_isMonitoringNeeded = isMonitoringNeeded;
}

inline void WaitingLauncherArgs::setDontLaunchIfAlreadyLaunched(const bool dontLaunchIfAlreadyLaunched)
{
    m_dontLaunchIfAlreadyLaunched = dontLaunchIfAlreadyLaunched;
}

inline void WaitingLauncherArgs::setLaunchProgramAfterClose(const bool launchProgramAfterClose)
{
    m_launchProgramAfterClose = launchProgramAfterClose;
}

inline void WaitingLauncherArgs::setProgramOnErrorFolderPath(const QString &programOnErrorFolderPath)
{
    m_programOnErrorFolderPath = programOnErrorFolderPath;
}

inline void WaitingLauncherArgs::setProgramOnErrorName(const QString &programOnErrorName)
{
    m_programOnErrorName = programOnErrorName;
}

inline void WaitingLauncherArgs::setProgramAfterCloseFolderPath(const QString &programAfterCloseFolderPath)
{
    m_programAfterCloseFolderPath = programAfterCloseFolderPath;
}

inline void WaitingLauncherArgs::setProgramAfterCloseName(const QString &programAfterCloseName)
{
    m_programAfterCloseName = programAfterCloseName;
}

QString WaitingLauncherArgs::toString() const
{
    QString args = "\"" + m_programFolderPath + "\" " +
                   "\"" + m_programName + "\"" +
                   (m_isMonitoringNeeded           ? " -IsMonitoringNeeded"            : "") +
                   (m_dontLaunchIfAlreadyLaunched  ? " -DontLaunchIfAlreadyLaunched"   : "") +
                   (m_launchProgramAfterClose      ? " -LaunchProgramAfterClose"       : "");

    if (!m_programOnErrorFolderPath.isEmpty() && !m_programOnErrorName.isEmpty()) {
        args += " -ProgramOnError \"" + m_programOnErrorFolderPath + "\" " +
                                 "\"" + m_programOnErrorName + "\"";
    }

    if (!m_programAfterCloseFolderPath.isEmpty() && !m_programAfterCloseName.isEmpty()) {
        args += " -ProgramAfterClose \"" + m_programAfterCloseFolderPath + "\" " +
                                    "\"" + m_programAfterCloseName + "\"";
    }

    return args;
}

inline QByteArray WaitingLauncherArgs::toUtf8() const
{
    return toString().toUtf8();
}

///Additional functions:

inline constexpr const char *gameFileName(const bool is64BitOs)
{
    return (is64BitOs ? "Everlasting Summer" : "Everlasting Summer-32");
}

QByteArray fileChecksumHex(const QString &fileName, QCryptographicHash::Algorithm hashAlgorithm)
{
    QFile file(fileName);
    if (file.exists() && file.open(QFile::ReadOnly)) {
        QCryptographicHash hash(hashAlgorithm);
        if (hash.addData(&file)) {
            file.close();
            return hash.result().toHex();
        }
    }
    return QByteArray();
}

void rewriteFileIfDataIsDifferent(const QString &fileName, const QByteArray &newData)
{
    QFile file(fileName);

    if (QFile::exists(fileName)) {
        if (file.open(QFile::ReadOnly)) {
            QByteArray currentData = file.readAll();
            file.close();

            if (currentData == newData) {
                return;
            }
        } else {
            return;
        }
    }

    if (file.open(QFile::WriteOnly)) {
        file.write(newData);
        file.close();
    }
}
