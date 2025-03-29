#include <QCollator>
#include <QCryptographicHash>
#include <QDebug>
#include <QDesktopServices>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QItemEditorFactory>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QThread>
#include <QTranslator>

#include "applicationVersion.h"
#include "DatabaseEditor.h"
#include "ModDatabaseModel.h"
#include "ModScanner.h"
#include "proxyModels.h"
#include "SteamRequester.h"

#include "MainWindow.h"
#include "ui_MainWindow.h"

inline constexpr const char *gameFileName(const bool is64BitOs);
void rewriteFileIfDataIsDifferent(const QString &fileName, const QByteArray &newData);

///class MainWindow:

const bool MainWindow::Is64BitOs = QSysInfo::currentCpuArchitecture().contains(QLatin1String("64"));

//public:

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    m_model = new ModDatabaseModel();
    m_steamRequester = new SteamRequester(m_model);
    m_requesterThread = new QThread;
    connect(m_requesterThread, &QThread::started, m_steamRequester, &SteamRequester::requestModNames);
    connect(m_steamRequester, &SteamRequester::modProcessed,
            this, &MainWindow::steamModNameProcessed, Qt::BlockingQueuedConnection);
    connect(m_steamRequester, &SteamRequester::finished, m_requesterThread, &QThread::quit);
    m_steamRequester->moveToThread(m_requesterThread);

    m_scanner = new ModScanner;
    m_scanner->setModel(m_model);
    m_scannerThread = new QThread;
    connect(m_scannerThread, &QThread::started, m_scanner, &ModScanner::scanMods);
    connect(m_scanner, &ModScanner::modsScanned, m_scannerThread, &QThread::quit);
    m_scanner->moveToThread(m_scannerThread);

    ui->setupUi(this);

    ui->engLangButton->setIcon(QIcon(":/images/Flag-United-States.ico"));
    ui->rusLangButton->setIcon(QIcon(":/images/Flag-Russia.ico"));

    m_enabledModsModel = new EnabledModsProxyModel(this, m_model);
    m_disabledModsModel = new DisabledModsProxyModel(this, m_model);

    ui->enabledModsList->setStyleSheet("QTreeView::item:!selected:!hover { border-bottom: 1px solid #E5E5E5; }");
    ui->enabledModsList->setModel(m_enabledModsModel);

    ui->disabledModsList->setStyleSheet("QTreeView::item:!selected:!hover { border-bottom: 1px solid #E5E5E5; }");
    ui->disabledModsList->setModel(m_disabledModsModel);

    ui->progressLabel->hide();
    ui->progressBar->hide();

    m_databaseEditor = new DatabaseEditor();

    m_settings = new QSettings(QString("settings.ini"), QSettings::IniFormat, this);
    m_qtTranslator = new QTranslator();
    m_translator = new QTranslator();

    m_itemContextMenu = new QMenu(this);
    initActions();

    //connections:

    //Signals from form objects:
    connect(ui->actionAddShortcutToDesktop, &QAction::triggered, this, &MainWindow::addShortcutToDesktop);
    connect(ui->actionOpenManagerFolder, &QAction::triggered, this, &MainWindow::openManagerFolder);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAboutInfo);
    connect(ui->actionAnnouncements, &QAction::triggered, this, &MainWindow::showAnnouncementMessageBox);

    connect(ui->actionCompleteModNames, &QAction::toggled, m_model, &ModDatabaseModel::setCompleteModNames);
    connect(ui->actionUseSteamModNames, &QAction::toggled, m_model, &ModDatabaseModel::setUsingSteamModNames);

    connect(ui->actionOpenDatabaseEditor, &QAction::triggered, m_databaseEditor, &DatabaseEditor::show);

    connect(ui->engLangButton, &QPushButton::clicked, this, &MainWindow::setEnglishLanguage);
    connect(ui->rusLangButton, &QPushButton::clicked, this, &MainWindow::setRussianLanguage);

    connect(ui->disableAllButton, &QPushButton::clicked, this, &MainWindow::disableAllMods);
    connect(ui->enableAllButton, &QPushButton::clicked, this, &MainWindow::enableAllMods);
    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::refreshModlist);
    connect(ui->runButton, &QPushButton::clicked, this, &MainWindow::runGame);

    connect(ui->clearSearchPushButton, &QPushButton::clicked, ui->searchLineEdit, &QLineEdit::clear);
    connect(ui->searchLineEdit, &QLineEdit::textChanged, m_enabledModsModel, &ModFilterProxyModel::setFilter);
    connect(ui->searchLineEdit, &QLineEdit::textChanged, m_disabledModsModel, &ModFilterProxyModel::setFilter);

    connect(ui->enabledModsList, &QTreeView::doubleClicked, this, [this](const QModelIndex &i) {
        m_enabledModsModel->setData(i, false, ModDatabaseModel::ModRole::Enabled);
    });

    connect(ui->disabledModsList, &QTreeView::doubleClicked, this, [this](const QModelIndex &i) {
        m_disabledModsModel->setData(i, true, ModDatabaseModel::ModRole::Enabled);
    });

    ui->enabledModsList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->disabledModsList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->enabledModsList, &QTreeView::customContextMenuRequested,
            this, &MainWindow::showModContextMenu);
    connect(ui->disabledModsList, &QTreeView::customContextMenuRequested,
            this, &MainWindow::showModContextMenu);

    connect(ui->enabledModsList, &QTreeView::pressed, ui->disabledModsList, &QTreeView::clearSelection);
    connect(ui->disabledModsList, &QTreeView::pressed, ui->enabledModsList, &QTreeView::clearSelection);

    //Other signals:
    connect(m_steamRequester, &SteamRequester::finished, ui->progressLabel, &QLabel::hide);
    connect(m_steamRequester, &SteamRequester::finished, ui->progressBar, &QLabel::hide);
    connect(m_databaseEditor, &DatabaseEditor::openModFolder, this, &MainWindow::openModFolder);

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
                m_model->setModsExistsState(false);
            }
        }

        m_modsFolderPath = m_gameFolderPath.section("common\\Everlasting Summer\\", 0, 0);
        m_modsFolderPath.append("workshop\\content\\331470\\");
    }

    refreshModlist();
    m_databaseEditor->setModel(m_model);
}

MainWindow::~MainWindow()
{
    saveSettings();

        m_model->sort(1);
        saveDatabase();

    QApplication::removeTranslator(m_translator);
    QApplication::removeTranslator(m_qtTranslator);

    m_requesterThread->requestInterruption();
    m_scannerThread->requestInterruption();
    m_requesterThread->deleteLater();
    m_scannerThread->deleteLater();
    m_steamRequester->deleteLater();
    m_scanner->deleteLater();

    delete m_translator;
    delete m_qtTranslator;
    delete m_settings;
    delete m_model;
    delete m_databaseEditor;

    delete ui;
}

void MainWindow::clearSearchField()
{
    ui->searchLineEdit->blockSignals(true);
    ui->searchLineEdit->clear();
    ui->searchLineEdit->blockSignals(false);
}

void MainWindow::loadDatabase()
{
    QFile file(modDatabaseFileName());
    if (file.exists()) {
        QJsonDocument database;
        //QJsonParseError err;

        file.open(QFile::ReadOnly);
        database = QJsonDocument::fromJson(file.readAll()/*, &err*/);
        file.close();

        QJsonArray mods = database.array();
        for (int i = 0; i < mods.size(); i++) {
            m_model->add(ModInfo(mods[i].toObject()));
        }
    }
}

void MainWindow::requestSteamModNames()
{
    if (!m_steamRequester->isRunning()) {
        m_requesterThread->start();
    }
}

void MainWindow::saveDatabase() const
{
    QJsonArray mods;
    for (const ModInfo &modInfo : qAsConst(m_model->modList())) {
        mods.append(modInfo.toJsonObject());
    }
    QJsonDocument database(mods);
    rewriteFileIfDataIsDifferent("mods_database.json", database.toJson());
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
    ui->searchLineEdit->clear();
    m_model->reset([this]() {
        for (ModInfo &modInfo : m_model->modListRef()) {
            modInfo.setEnabled(false);
        }
    });
}

void MainWindow::enableAllMods()
{
    ui->searchLineEdit->clear();
    m_model->reset([this]() {
        for (ModInfo &modInfo : m_model->modListRef()) {
            modInfo.setEnabled(true);
        }
    });
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
    connect(m_scanner, &ModScanner::modScanned, &progressDialog, &QProgressDialog::setValue);
    connect(m_scanner, &ModScanner::modsScanned, &progressDialog, &QProgressDialog::accept);

    ui->progressBar->setMaximum(modsCount);
    ui->progressBar->setValue(0);

    m_scanner->setModsFolderPath(m_modsFolderPath);
    m_scannerThread->start();
    progressDialog.exec();

    m_model->sort();

    ui->progressLabel->show();
    ui->progressBar->show();
    requestSteamModNames();

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

bool MainWindow::openModFolder(const QModelIndex &index)
{
    if (QDir().exists(m_modsFolderPath + m_model->modFolderName(index))) {
        return QDesktopServices::openUrl(
            QUrl("file:///" + m_modsFolderPath + m_model->modFolderName(index))
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

void MainWindow::showModContextMenu(const QPoint &pos)
{
    QTreeView *modList = qobject_cast<QTreeView *>(sender());
    QSortFilterProxyModel *model = modList ? qobject_cast<QSortFilterProxyModel *>(modList->model()) : nullptr;
    if (!modList || !model) {
        return;
    }

    QModelIndex index = model->mapToSource(modList->indexAt(pos));
    if (index.isValid()) {
        setupItemContextMenu(m_model->modInfo(index));
        m_itemContextMenu->exec(modList->viewport()->mapToGlobal(pos));
    }
}

void MainWindow::performOnSelectedMods(std::function<bool(ModDatabaseModel *model, QModelIndex index)> action)
{
    QItemSelectionModel *selection = selectedMods();
    ModFilterProxyModel *model = selection ? qobject_cast<ModFilterProxyModel *>(selection->model()) : nullptr;
    if (!selection || !model) {
        return;
    }

    const QModelIndexList &selectedIndexes = selection->selectedIndexes();
    for (auto index : selectedIndexes) {
        if (!action(m_model, model->mapToSource(index))) {
            break;
        }
    }
}


//private:

QItemSelectionModel *MainWindow::selectedMods() const
{
    if (ui->disabledModsList->selectionModel()->hasSelection()) {
        return ui->enabledModsList->selectionModel()->hasSelection() ? nullptr : ui->disabledModsList->selectionModel();
    } else {
        return ui->enabledModsList->selectionModel()->hasSelection() ? ui->enabledModsList->selectionModel() : nullptr;
    }
}

void MainWindow::initActions()
{
    m_setEnabledAction = new QAction(m_itemContextMenu);
    m_setLockedAction = new QAction(tr("Lock"), m_itemContextMenu);
    m_setUnlockedAction = new QAction(tr("Unlock"), m_itemContextMenu);
    m_setMarkedAction = new QAction(tr("Toggle mark"), m_itemContextMenu);
    m_setMarkedAction->setCheckable(true);
    m_openFolderAction = new QAction(tr("Open folder in the explorer"), m_itemContextMenu);
    m_openSteamPageAction = new QAction(tr("Open the Steam Workshop page"), m_itemContextMenu);
    m_renameAction = new QAction(tr("Rename"), m_itemContextMenu);

    m_itemContextMenu->addAction(m_setEnabledAction);
    m_itemContextMenu->addAction(m_setLockedAction);
    m_itemContextMenu->addAction(m_setUnlockedAction);
    m_itemContextMenu->addAction(m_setMarkedAction);
    m_itemContextMenu->addAction(m_openFolderAction);
    m_itemContextMenu->addAction(m_openSteamPageAction);
    m_itemContextMenu->addAction(m_renameAction);

    connect(ui->setLockedToolButton, &QPushButton::clicked, m_setLockedAction, &QAction::trigger);
    connect(ui->setUnlockedToolButton, &QPushButton::clicked, m_setUnlockedAction, &QAction::trigger);
    connect(ui->setMarkedToolButton, &QPushButton::clicked, m_setMarkedAction, &QAction::trigger);
    connect(ui->openFolderToolButton, &QPushButton::clicked, m_openFolderAction, &QAction::trigger);
    connect(ui->openSteamPageToolButton, &QPushButton::clicked, m_openSteamPageAction, &QAction::trigger);
    connect(ui->renameToolButton, &QPushButton::clicked, m_renameAction, &QAction::trigger);
    connect(ui->setDisabledToolButton, &QPushButton::clicked, m_setEnabledAction, &QAction::trigger);
    connect(ui->setEnabledToolButton, &QPushButton::clicked, m_setEnabledAction, &QAction::trigger);

    connect(m_openSteamPageAction, &QAction::triggered, this, [this](bool /*enabled*/) {
        performOnSelectedMods([this](ModDatabaseModel */*model*/, QModelIndex index) {
            QDesktopServices::openUrl(QUrl(
                "steam://url/CommunityFilePage/" + m_model->modFolderName(index)
            ));
            return false;
        });
    });

    connect(m_openFolderAction, &QAction::triggered, this, [this]() {
        performOnSelectedMods([this](ModDatabaseModel */*model*/, QModelIndex index) {
            openModFolder(index);
            return false;
        });
    });

    connect(m_setLockedAction, &QAction::triggered, this, [this]() {
        performOnSelectedMods([](ModDatabaseModel *model, QModelIndex index) {
            model->setData(index, true, ModDatabaseModel::ModRole::Locked);
            return true;
        });
    });

    connect(m_setUnlockedAction, &QAction::triggered, this, [this]() {
        performOnSelectedMods([](ModDatabaseModel *model, QModelIndex index) {
            model->setData(index, false, ModDatabaseModel::ModRole::Locked);
            return true;
        });
    });

    connect(m_setMarkedAction, &QAction::triggered, this, [this]() {
        bool marked = false;

        //If at least one mod is marked, all should set marked
        performOnSelectedMods([&marked](ModDatabaseModel *model, QModelIndex index) {
            marked = !model->data(index, ModDatabaseModel::ModRole::Marked).toBool();
            return !marked; //Stop search
        });

        performOnSelectedMods([&marked](ModDatabaseModel *model, QModelIndex index) {
            model->setData(index, marked, ModDatabaseModel::ModRole::Marked);
            return true;
        });
    });

    connect(m_setEnabledAction, &QAction::triggered, this, [this]() {
        performOnSelectedMods([](ModDatabaseModel *model, QModelIndex index) {
            bool enabled = model->data(index, ModDatabaseModel::ModRole::Enabled).toBool();
            model->setData(index, !enabled, ModDatabaseModel::ModRole::Enabled);
            return true;
        });
    });
}

void MainWindow::setupItemContextMenu(const ModInfo &item) const
{
    m_setEnabledAction->setText(item.enabled() ? tr("Disable") : tr("Enable"));
    m_setEnabledAction->setVisible(item.exists() && !item.locked());
    m_setLockedAction->setVisible(item.exists() && !item.locked());
    m_setUnlockedAction->setVisible(item.exists() && item.locked());
    m_setMarkedAction->setVisible(item.exists());
    m_setMarkedAction->setChecked(item.marked());
    m_openFolderAction->setVisible(item.exists());
}

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
    for (const ModInfo &modInfo : qAsConst(m_model->modList())) {
        if (modInfo.exists() && !modInfo.enabled()) {
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



///Additional functions:

inline constexpr const char *gameFileName(const bool is64BitOs)
{
    return (is64BitOs ? "Everlasting Summer" : "Everlasting Summer-32");
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
