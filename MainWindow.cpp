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
#include <QTimer>
#include <QThread>
#include <QTranslator>

#include "mvc/ModDatabaseModel.h"
#include "mvc/ModNameDelegate.h"
#include "mvc/proxyModels.h"
#include "utils/applicationVersion.h"

#include "DatabaseEditor.h"
#include "ModScanner.h"
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

    connect(ui->enabledModsView->header(), &QHeaderView::sectionResized,
            ui->enabledModsView, &QTreeView::doItemsLayout);
    connect(ui->disabledModsView->header(), &QHeaderView::sectionResized,
            ui->disabledModsView, &QTreeView::doItemsLayout);

    m_enabledModsModel = new EnabledModsProxyModel(this, m_model);
    m_disabledModsModel = new DisabledModsProxyModel(this, m_model);

    ui->enabledModsView->setModel(m_enabledModsModel);
    ui->disabledModsView->setModel(m_disabledModsModel);

    m_modNameDelegate = new ModNameDelegate(this);
    ui->enabledModsView->setItemDelegate(m_modNameDelegate);
    ui->disabledModsView->setItemDelegate(m_modNameDelegate);

    ui->progressLabel->hide();
    ui->progressBar->hide();

    m_databaseEditor = new DatabaseEditor();

#if 1
    if (!QFileInfo::exists(settingsFilePath()) && QFileInfo::exists(QString("../") + SettingsFileName))
    {
        /// Move files from the old folder for the backward compatibility
        QFile::rename(QString("../") + SettingsFileName, settingsFilePath());
        QFile::rename(QString("../") + ModDatabaseFileName, modDatabaseFilePath());
    }
#else
    //FIXME: remove
    QFile::remove(settingsFilePath());
    QFile::remove(modDatabaseFilePath());
    if (!QFileInfo::exists(settingsFilePath()) && QFileInfo::exists(QString("../") + SettingsFileName))
    {
        /// Move files from the old folder for the backward compatibility
        QFile::copy(QString("../") + SettingsFileName, settingsFilePath());
        QFile::copy(QString("../") + ModDatabaseFileName, modDatabaseFilePath());
    }
#endif
    qDebug() << settingsFilePath();
    qDebug() << QDir(settingsFilePath()).absolutePath();
    m_settings = new QSettings(settingsFilePath(), QSettings::IniFormat, this);
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
//    connect(ui->searchLineEdit, &QLineEdit::textChanged, m_enabledModsModel, &ModFilterProxyModel::setFilter);
    connect(ui->searchLineEdit, &QLineEdit::textChanged, m_disabledModsModel, &ModFilterProxyModel::setFilter);

    connect(ui->enabledModsView, &QTreeView::expanded, this, [this](const QModelIndex &index) {
        QModelIndex sourceIndex = m_enabledModsModel->mapToSource(index);
        m_model->setData(sourceIndex, true, ModDatabaseModel::Role::Expanded);
    });
    connect(ui->enabledModsView, &QTreeView::collapsed, this, [this](const QModelIndex &index) {
        QModelIndex sourceIndex = m_enabledModsModel->mapToSource(index);
        m_model->setData(sourceIndex, false, ModDatabaseModel::Role::Expanded);
    });
    connect(ui->disabledModsView, &QTreeView::expanded, this, [this](const QModelIndex &index) {
        QModelIndex sourceIndex = m_disabledModsModel->mapToSource(index);
        m_model->setData(sourceIndex, true, ModDatabaseModel::Role::Expanded);
    });
    connect(ui->disabledModsView, &QTreeView::collapsed, this, [this](const QModelIndex &index) {
        QModelIndex sourceIndex = m_disabledModsModel->mapToSource(index);
        m_model->setData(sourceIndex, false, ModDatabaseModel::Role::Expanded);
    });

    ui->enabledModsView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->disabledModsView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->enabledModsView, &QTreeView::customContextMenuRequested,
            this, &MainWindow::showModContextMenu);
    connect(ui->disabledModsView, &QTreeView::customContextMenuRequested,
            this, &MainWindow::showModContextMenu);

    connect(ui->enabledModsView, &QTreeView::pressed, this, [this](){
        ui->disabledModsView->clearSelection();
        closeViewEditor();
    });
    connect(ui->disabledModsView, &QTreeView::pressed, this, [this](){
        ui->enabledModsView->clearSelection();
        closeViewEditor();
    });

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
    updateDisabledModsFile();

    m_model->sort(1);
    saveDatabase();

    if (ui->actionLaunchBeforeGame->isChecked()) {
        QFile file(disableAutolaunchFilePath());
        file.remove();
    }

    QApplication::removeTranslator(m_translator);
    QApplication::removeTranslator(m_qtTranslator);

    m_requesterThread->requestInterruption();
    m_requesterThread->quit();
    m_requesterThread->deleteLater();
    m_steamRequester->deleteLater();

    m_scannerThread->requestInterruption();
    m_scannerThread->quit();
    m_scannerThread->deleteLater();
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
    QFile file(modDatabaseFilePath());
    if (file.exists()) {
        QJsonDocument database;
        //QJsonParseError err;

        file.open(QFile::ReadOnly);
        database = QJsonDocument::fromJson(file.readAll()/*, &err*/);
        file.close();

        m_model->fromJson(database.object());
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
    QJsonDocument database(m_model->toJson());
    rewriteFileIfDataIsDifferent(modDatabaseFilePath(), database.toJson());
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
//    QVector<QModelIndex> expandedCollections;
//    for (int row = 0; row < m_enabledModsModel->rowCount(); ++row)
//    {
//        QModelIndex index = m_enabledModsModel->index(row, 0);
//        if (ui->enabledModsView->isExpanded(index))
//        {
//            expandedCollections.append(m_enabledModsModel->mapToSource(index));
//        }
//    }

    ui->searchLineEdit->clear();
    m_model->reset([this]() {
        for (ModInfo *modInfo : qAsConst(m_model->modListRef())) {
            modInfo->setEnabled(false);
        }
    });

//    for (const QModelIndex &index : expandedCollections)
//    {
//        ui->disabledModsView->expand(m_disabledModsModel->mapFromSource(index));
//    }
}

void MainWindow::enableAllMods()
{
//    QVector<QModelIndex> expandedCollections;
//    for (int row = 0; row < m_disabledModsModel->rowCount(); ++row)
//    {
//        QModelIndex index = m_disabledModsModel->index(row, 0);
//        if (ui->disabledModsView->isExpanded(index))
//        {
//            expandedCollections.append(m_disabledModsModel->mapToSource(index));
//        }
//    }

    ui->searchLineEdit->clear();
    m_model->reset([this]() {
        for (ModInfo *modInfo : qAsConst(m_model->modListRef())) {
            modInfo->setEnabled(true);
        }
    });

//    for (const QModelIndex &index : expandedCollections)
//    {
//        ui->enabledModsView->expand(m_enabledModsModel->mapFromSource(index));
//    }
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
    connect(&progressDialog, &QProgressDialog::canceled, m_scannerThread, [this](){
        m_scannerThread->requestInterruption();
        m_scannerThread->quit();
    });

    ui->progressBar->setMaximum(modsCount);
    ui->progressBar->setValue(0);

    m_model->setModsExistsState(false);
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

    ui->runButton->setEnabled(false);

    if (m_databaseEditor->isVisible()) {
        m_databaseEditor->close();
    }

    updateDisabledModsFile();

    QProcess *gameLauncher = new QProcess();
    gameLauncher->setWorkingDirectory(QDir(m_gameFolderPath).absolutePath());
    gameLauncher->setProgram(QFileInfo(gameFilePath()).absoluteFilePath());
    connect(gameLauncher, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](){
        if (ui->actionAutoexit->isChecked()) {
            QApplication::exit();
        } else {
            ui->runButton->setEnabled(true);
        }
    });
    connect(gameLauncher, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            gameLauncher, &QProcess::deleteLater);

    //We must ensure that autoexit flag is up to date because it will be read by our .rpy script
    m_settings->setValue("General/bAutoexit", ui->actionAutoexit->isChecked());
    m_settings->sync();
    rewriteFileIfDataIsDifferent(disableAutolaunchFilePath(), "");

    gameLauncher->start();
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
           "It allows to disable mods so the game engine will not load them.") +
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
        tr("Due to updates of the game, current mod load system might be broken."
           "In this case, the manager will also start, "
           "but only if the game can load all installed mods.\n\n In other words, "
           "if there are too many mods, the manager will need to be launched manually from its folder.\n\n"
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

    QModelIndex index = modList->indexAt(pos); model->mapToSource(modList->indexAt(pos));
    if (index.isValid()) {
        setupItemContextMenu(model, index);
        m_itemContextMenu->exec(modList->viewport()->mapToGlobal(pos));
    }
}

void MainWindow::performOnSelectedMods(
    std::function<bool(QModelIndex index, QModelIndex proxyIndex)> action
)
{
    QTreeView *view = selectedView();
    ModFilterProxyModel *model = proxyModel(view);
    const QItemSelection& selection = this->selection(view);
    if (!action || !view || !model || selection.isEmpty()) {
        return;
    }

    const QModelIndexList& proxyIndexes = selection.indexes();
    const QModelIndexList& indexes = model->mapSelectionToSource(selection).indexes();
//    QSet<QModelIndex> collectionsToExpand;
    bool ok = true;
    for (int i = 0; ok && i < indexes.size(); ++i) {
        ok = action(indexes[i], proxyIndexes[i]);

//        QModelIndex collectionIndex = m_model->collectionIndex(indexes[i]);
//        const ModCollection &collection = m_model->modCollection(collectionIndex);
//        if (collection.expanded() && collection.size()) {
//            collectionsToExpand.insert(collectionIndex);
//        }
    }

//    QTreeView *otherView = (view == ui->enabledModsView ? ui->disabledModsView : ui->enabledModsView);
//    ModFilterProxyModel *othermodel = proxyModel(otherView);
//    for (const QModelIndex &collectionIndex : collectionsToExpand) {
//        view->expand(model->mapFromSource(collectionIndex));
//        otherView->expand(othermodel->mapFromSource(collectionIndex));
//    }
}

void MainWindow::performOnSelectedMods(std::function<void(QModelIndexList)> action)
{
    QTreeView *view = selectedView();
    ModFilterProxyModel *model = proxyModel(view);
    const QItemSelection& selection = this->selection(view);
    if (!action || !view || !model || selection.isEmpty()) {
        return;
    }

    QModelIndexList indexes = model->mapSelectionToSource(selection).indexes();
//    QSet<QModelIndex> collectionsToExpand;
//    for (int i = 0; i < indexes.size(); ++i) {
//        QModelIndex collectionIndex = m_model->collectionIndex(indexes[i]);
//        const ModCollection &collection = m_model->modCollection(collectionIndex);
//        if (collection.expanded()) {
//            collectionsToExpand.insert(collectionIndex);
//        }
//    }

    action(std::move(indexes));

//    QTreeView *otherView = (view == ui->enabledModsView ? ui->disabledModsView : ui->enabledModsView);
//    ModFilterProxyModel *othermodel = proxyModel(otherView);
//    for (const QModelIndex &collectionIndex : collectionsToExpand) {
//        if (m_model->modCollection(collectionIndex).size() > 0) {
//            view->expand(model->mapFromSource(collectionIndex));
//            otherView->expand(othermodel->mapFromSource(collectionIndex));
//        }
//    }
}

//private:

QTreeView *MainWindow::selectedView() const
{
    if (ui->disabledModsView->selectionModel()->hasSelection()) {
        return ui->enabledModsView->selectionModel()->hasSelection() ? nullptr : ui->disabledModsView;
    } else {
        return ui->enabledModsView->selectionModel()->hasSelection() ? ui->enabledModsView : nullptr;
    }
}

QItemSelection MainWindow::selection(QTreeView *view) const
{
    return view ? view->selectionModel()->selection() : QItemSelection();
}

ModFilterProxyModel* MainWindow::proxyModel(QTreeView *view) const
{
    return view ? qobject_cast<ModFilterProxyModel *>(view->model()) : nullptr;
}

void MainWindow::closeViewEditor()
{
    if (m_viewEditorData.first) {
        emit m_modNameDelegate->closeEditor(
            m_viewEditorData.first->indexWidget(m_viewEditorData.second),
            QAbstractItemDelegate::SubmitModelCache
        );
        m_viewEditorData = {nullptr, QModelIndex()};
    }
}

void MainWindow::initActions()
{
    m_enableAction = new QAction(tr("Enable"), m_itemContextMenu);
    m_disableAction = new QAction(tr("Disable"), m_itemContextMenu);
    m_setLockedAction = new QAction(tr("Lock"), m_itemContextMenu);
    m_setUnlockedAction = new QAction(tr("Unlock"), m_itemContextMenu);
    m_toggleFavoritesAction = new QAction(tr("Add/remove favorites"), m_itemContextMenu);
    m_openFolderAction = new QAction(tr("Open folder in the explorer"), m_itemContextMenu);
    m_openSteamPageAction = new QAction(tr("Open the Steam Workshop page"), m_itemContextMenu);
    m_renameAction = new QAction(tr("Rename"), m_itemContextMenu);

    m_itemContextMenu->addAction(m_enableAction);
    m_itemContextMenu->addAction(m_disableAction);
    m_itemContextMenu->addAction(m_setLockedAction);
    m_itemContextMenu->addAction(m_setUnlockedAction);
    m_itemContextMenu->addAction(m_toggleFavoritesAction);
    m_itemContextMenu->addAction(m_openFolderAction);
    m_itemContextMenu->addAction(m_openSteamPageAction);
    m_itemContextMenu->addAction(m_renameAction);
    //TODO: add submenu to add in collection

    connect(ui->setLockedToolButton, &QPushButton::clicked, m_setLockedAction, &QAction::trigger);
    connect(ui->setUnlockedToolButton, &QPushButton::clicked, m_setUnlockedAction, &QAction::trigger);
    connect(ui->toggleFavoritesToolButton, &QPushButton::clicked, m_toggleFavoritesAction, &QAction::trigger);
    connect(ui->openFolderToolButton, &QPushButton::clicked, m_openFolderAction, &QAction::trigger);
    connect(ui->openSteamPageToolButton, &QPushButton::clicked, m_openSteamPageAction, &QAction::trigger);
    connect(ui->renameToolButton, &QPushButton::clicked, m_renameAction, &QAction::trigger);
    connect(ui->setEnabledToolButton, &QPushButton::clicked, m_enableAction, &QAction::trigger);
    connect(ui->setDisabledToolButton, &QPushButton::clicked, m_disableAction, &QAction::trigger);

    connect(m_openSteamPageAction, &QAction::triggered, this, [this](bool /*enabled*/) {
        performOnSelectedMods([this](QModelIndex index, QModelIndex /*proxy*/) {
            QDesktopServices::openUrl(QUrl(
                "steam://url/CommunityFilePage/" + m_model->modFolderName(index)
            ));
            return false; // Can't open several pages via Steam client
        });
    });

    connect(m_openFolderAction, &QAction::triggered, this, [this]() {
        performOnSelectedMods([this](QModelIndex index, QModelIndex /*proxy*/) {
            openModFolder(index);
            return true;
        });
    });

    connect(m_renameAction, &QAction::triggered, this, [this]() {
        QTreeView *view = selectedView();
        if (!view) {
            return;
        }

        const QModelIndexList &selectedIndexes = view->selectionModel()->selectedIndexes();
        if (selectedIndexes.size() > 0) {
            QModelIndex index = selectedIndexes.first();
            qDebug() << index;
            view->setCurrentIndex(index);
            view->edit(index);
            m_viewEditorData = {view, index};
        }
    });

    connect(m_setLockedAction, &QAction::triggered, this, [this]() {
        performOnSelectedMods([this](QModelIndex index, QModelIndex /*proxy*/) {
            m_model->setData(index, Qt::Checked, ModDatabaseModel::Role::Locked);
            return true;
        });
    });

    connect(m_setUnlockedAction, &QAction::triggered, this, [this]() {
        performOnSelectedMods([this](QModelIndex index, QModelIndex /*proxy*/) {
            m_model->setData(index, Qt::Unchecked, ModDatabaseModel::Role::Locked);
            return true;
        });
    });

    connect(m_toggleFavoritesAction, &QAction::triggered, this, [this]() {
        bool hasNonFavorite = false;
        Qt::CheckState enabledFilter = selectedView() == ui->enabledModsView ? Qt::Checked : Qt::Unchecked;

        //If at least one mod is not favorite, all should set favorite
        performOnSelectedMods([this, &hasNonFavorite](QModelIndex index, QModelIndex /*proxy*/) {
            hasNonFavorite = !m_model->isFavorite(index);
            return !hasNonFavorite; //Stop search if found
        });

        performOnSelectedMods([this, &hasNonFavorite, &enabledFilter](QModelIndexList indexes) {
            hasNonFavorite ? m_model->addFavorite(indexes, enabledFilter)
                           : m_model->removeFavorite(indexes, enabledFilter);
            return true;
        });
    });

    connect(m_enableAction, &QAction::triggered, this, [this]() {
        performOnSelectedMods([this](QModelIndex index, QModelIndex /*proxy*/) {
            m_model->setData(index, Qt::Checked, ModDatabaseModel::Role::Enabled);
            return true;
        });
    });

    connect(m_disableAction, &QAction::triggered, this, [this]() {
        performOnSelectedMods([this](QModelIndex index, QModelIndex /*proxy*/) {
            m_model->setData(index, Qt::Unchecked, ModDatabaseModel::Role::Enabled);
            return true;
        });
    });
}

void MainWindow::setupItemContextMenu(QSortFilterProxyModel *model, QModelIndex index) const
{
    const AbstractModDatabaseItem &item = m_model->item(model->mapToSource(index));
    bool exists = item.exists() != Qt::Unchecked;
    bool locked = item.locked() == Qt::Checked;
    bool unlocked = item.locked() == Qt::Unchecked;

    m_enableAction->setVisible(exists && !locked && item.enabled() != Qt::Checked);
    m_disableAction->setVisible(exists && !locked && item.enabled() != Qt::Unchecked);
    m_setLockedAction->setVisible(exists && !locked);
    m_setUnlockedAction->setVisible(exists && !unlocked);
    m_toggleFavoritesAction->setVisible(exists);
    m_openFolderAction->setVisible(exists);
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
    for (const ModInfo *modInfo : qAsConst(m_model->modList())) {
        if (modInfo->exists() && !modInfo->enabled()) {
            disabledMods.append(modInfo->folderName + "\n");
        }
    }
    rewriteFileIfDataIsDifferent(disabledModsFilePath(), disabledMods.toUtf8());
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

    QString tempModsFolderPath = m_gameFolderPath + "mods (temp)\\";
    if (m_settings->contains("General/sTempModsFolder")) {
        QVariant value = m_settings->value("General/sTempModsFolder");
        if (value != QVariant()) {
            tempModsFolderPath = value.toString();
        }
    }

    if (loadedApplicationVersion < applicationVersion(1, 1, 9)) {
        ///Fix bug from old versions when game folder path contained "Everlasting Summer.exe"
        m_gameFolderPath.remove("Everlasting Summer.exe");
        m_modsFolderPath.remove("Everlasting Summer.exe");
        tempModsFolderPath.remove("Everlasting Summer.exe");

        ///Conversion from old version of DB
        {
            QFile file("mods_database.dat");

            if (file.exists()) {
                QJsonDocument database;
                QJsonArray mods;
                QJsonObject mod;
                ModInfo modInfo;

                file.open(QFile::ReadOnly);
                do {
                    mod["Name"] = QString(file.readLine()).remove(QRegExp("[\\n\\r]"));
                    if (!file.canReadLine()) {
                        break;
                    }

                    mod["Folder name"] = QString(file.readLine()).remove(QRegExp("[\\n\\r]"));
                    if (!file.canReadLine()) {
                        break;
                    }

                    mod["Is enabled"] = bool(QString(file.readLine()).remove(QRegExp("[\\n\\r]")).toInt());
                    mods.append(mod);
                } while (file.canReadLine());
                file.close();
                file.remove();

                database.setArray(mods);
                rewriteFileIfDataIsDifferent(modDatabaseFilePath(), database.toJson());
            }
        }
    }

    if (loadedApplicationVersion < applicationVersion(2, 0, 0)) {
        QDir tempModsFolder(tempModsFolderPath);

        if (tempModsFolder.exists()) {
            QDirIterator it(tempModsFolder.path(), QDir::Dirs);
            while (it.hasNext()) {
                QFileInfo modFolder(it.next());
                QDir().rename(
                    modFolder.path(),
                    m_modsFolderPath + modFolder.baseName()
                );
            }

            if (!QDir().rmdir(tempModsFolderPath)) {
                QMessageBox warningMessage(
                    QMessageBox::Warning, tr("Unmoved mods"),
                    tr("Failed to move back some of mods from the temp folder.\n"
                       "Probably Steam re-download some of disabled mods.") + "\n\n" +
                    tr("You can move these folders manually from the folder:") + "\n\n" +
                    tempModsFolderPath + "\n\n" + tr("to the mods folder:") + "\n\n" + m_modsFolderPath + "\n\n" +
                    tr("with the files replacement or just delete the temp mod folder if you don't need it."),
                    QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Open,
                    this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint
                );

                warningMessage.button(QMessageBox::StandardButton::Open)->setText(
                    tr("Open mods folder and temp folder in explorer")
                );
                warningMessage.setDefaultButton(QMessageBox::StandardButton::Ok);
                auto pressedButton = QMessageBox::StandardButton(warningMessage.exec());

                while (pressedButton == QMessageBox::StandardButton::Open) {
                    warningMessage.show(); //Must call show() before openUrl(), or exec() will not work properly

                    QDesktopServices::openUrl(QUrl("file:///" + tempModsFolderPath));
                    QDesktopServices::openUrl(QUrl("file:///" + m_modsFolderPath));

                    pressedButton = QMessageBox::StandardButton(warningMessage.exec());
                }
            }
        }

        ///Conversion from old version of DB
        {
            QFile file(QString("../") + ModDatabaseFileName);
            if (file.exists()) {
                QJsonDocument database;
                //QJsonParseError err;

                file.open(QFile::ReadOnly);
                database = QJsonDocument::fromJson(file.readAll()/*, &err*/);
                file.close();
                // File will remove automatically after workshop update

                QJsonArray mods = database.array();
                for (int i = 0; i < mods.size(); i++) {
                    QJsonObject mod = mods[i].toObject();
                    ModInfo modInfo;
                    modInfo.setName(mod["Name"].toString());
                    modInfo.folderName = mod["Folder name"].toString();
                    modInfo.steamName = mod["Steam name"].toString();
                    modInfo.setEnabled(mod["Is enabled"].toBool());
                    m_model->add(modInfo);
                }
            }
        }
    }
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
