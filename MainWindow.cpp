#include <QCollator>
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

#include "DatabaseEditor.h"
#include "DatabaseModel.h"
#include "ModScanner.h"
#include "SteamRequester.h"

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QCryptographicHash>
QByteArray fileChecksum(const QString &fileName, QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Md5)
{
	QFile file(fileName);
	if (file.exists() && file.open(QFile::ReadOnly)) {
		QCryptographicHash hash(hashAlgorithm);
		if (hash.addData(&file)) {
			file.close();
			return hash.result();
		}
	}
	return QByteArray();
}

QString applicationVersionToString(const int version)
{
	return QString::number(majorApplicationVersion(version)) + '.' +
		   QString::number(minorApplicationVersion(version)) + '.' +
		   QString::number(microApplicationVersion(version));
}

int applicationVersionFromString(const QString &version)
{
	QStringList subVersions = version.split('.');
	int subVersionsCount = subVersions.count();
	return applicationVersion(subVersionsCount > 0 ? subVersions[0].toInt() : 1,
							  subVersionsCount > 1 ? subVersions[1].toInt() : 0,
							  subVersionsCount > 2 ? subVersions[2].toInt() : 0);
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

//public:

MainWindow::MainWindow(QWidget *parent, const bool runCheck) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	m_model = new DatabaseModel();
	m_steamRequester = new SteamRequester(m_model);
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
	ui->enabledModsList->setModel(m_model);


	ui->disabledModsList->setStyleSheet("QListView::item:!selected:!hover { border-bottom: 1px solid #E5E5E5; }");
	ui->disabledModsList->setModel(m_model);

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
	m_launcherMd5 = fileChecksum(QDir::currentPath() + "/launcher/ESLauncher.exe");

	//connections:

	//Signals from form objects:
	connect(ui->actionGame_folder, SIGNAL(triggered()), this, SLOT(selectGameFolder()));
	connect(ui->actionMods_folder, SIGNAL(triggered()), this, SLOT(selectModsFolder()));
	connect(ui->actionTemp_mods_folder, SIGNAL(triggered()), this, SLOT(selectTempModsFolder()));
	connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAboutInfo()));
	connect(ui->actionAnnouncements, SIGNAL(triggered()), this, SLOT(showAnnouncementMessage()));

	connect(ui->completeNamesCheckBox, SIGNAL(stateChanged(int)),
			m_model, SLOT(setCompleteModNames(int)));
	connect(ui->useSteamModNamesCheckBox, SIGNAL(stateChanged(int)),
			m_model, SLOT(setUsingSteamModNames(int)));

	connect(ui->eraseDatabaseButton, SIGNAL(clicked()), this, SLOT(eraseDatabase()));
	connect(ui->databaseEditorButton, SIGNAL(clicked()), m_databaseEditor, SLOT(show()));

	connect(ui->engLangButton, SIGNAL(clicked()), this, SLOT(setEnglishLanguage()));
	connect(ui->rusLangButton, SIGNAL(clicked()), this, SLOT(setRussianLanguage()));

	connect(ui->disableAllButton, SIGNAL(clicked()), this, SLOT(disableAllMods()));
	connect(ui->enableAllButton, SIGNAL(clicked()), this, SLOT(enableAllMods()));
	connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(refreshModlist()));
	connect(ui->runButton, SIGNAL(clicked()), this, SLOT(runGame()));

	connect(ui->enabledModsList, SIGNAL(clicked(QModelIndex)), m_model, SLOT(disableMod(QModelIndex)));
	connect(ui->disabledModsList, SIGNAL(clicked(QModelIndex)), m_model, SLOT(enableMod(QModelIndex)));

	connect(ui->clearSearchPushButton, SIGNAL(clicked()), ui->searchLineEdit, SLOT(clear()));
	connect(ui->searchLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterModsDisplay(QString)));


	//Other signals:
	connect(m_steamRequester, SIGNAL(finished()), ui->progressLabel, SLOT(hide()));
	connect(m_steamRequester, SIGNAL(finished()), ui->progressBar, SLOT(hide()));
	connect(m_databaseEditor, SIGNAL(openModFolder(int)), this, SLOT(openModFolder(int)));
	connect(m_model, SIGNAL(modCheckStateChanged(int, bool)), this, SLOT(setRowVisibility(int, bool)));

	//this->adjustSize();

	readSettings();

	///Process check block (whether the game / manager is already running)
	if (runCheck)
	{
		QProcess processChecker;

		if (processChecker.execute("ProcessChecker.exe", QStringList("Everlasting Summer")) > 0) {
			QMessageBox::critical(nullptr, tr("Game is running"),
										   tr("Everlasting Summer is running!\n"
											  "Close the game before starting the manager!"),
								  QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton);
			this->setEnabled(false);
			QApplication::exit();
			return;
		}

		if (processChecker.execute("ProcessChecker.exe", QStringList("ESModManager")) > 1) {
			QMessageBox::critical(nullptr, tr("Mod manager is running"),
										   tr("Mod manager is already running!"),
								  QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton);
			this->setEnabled(false);
			QApplication::exit();
			return;
		}
	}
	///End of check block

	loadDatabase();

	if (m_gameFolderPath.isEmpty()) {
		QString gamePath = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Everlasting Summer\\";
		if (QFileInfo::exists(gamePath + "Everlasting Summer.exe")) {
			setGameFolder(gamePath);
		} else {
			gamePath = QDir::currentPath().section('/', 0, -6);
			gamePath.replace('/', '\\');
			gamePath.append("\\common\\Everlasting Summer\\");
			if (QFileInfo::exists(gamePath + "Everlasting Summer.exe")) {
				setGameFolder(gamePath);
			} else {
				m_model->setModsExistsState(false);
				checkRowsVisibility();
			}
		}
	} else {
		ui->gameFolderLineEdit->setText(m_gameFolderPath);
		ui->modsFolderLineEdit->setText(m_modsFolderPath);
		ui->tempModsFolderLineEdit->setText(m_tempModsFolderPath);
		refreshModlist();
	}

	m_databaseEditor->setModel(m_model, 1);
}

MainWindow::~MainWindow()
{
	saveSettings();

	QApplication::removeTranslator(m_translator);
	QApplication::removeTranslator(m_qtTranslator);
	m_steamRequester->deleteLater();
	m_thread->deleteLater();
	delete m_translator;
	delete m_qtTranslator;
	delete m_settings;
	delete m_model;
	delete m_databaseEditor;
	delete m_scanner;

	delete ui;
}

void MainWindow::checkRowsVisibility()
{
	clearSearchField();

	bool enabled, exists;
	int databaseSize = m_model->databaseSize();
	for (int rowIndex = 0; rowIndex < databaseSize; ++rowIndex) {
		enabled = m_model->modIsEnabled(rowIndex);
		exists = m_model->modIsExists(rowIndex);

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

	int rowCount = m_model->databaseSize();
	for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
		ui->enabledModsList->setRowHidden(rowIndex, true);
		ui->disabledModsList->setRowHidden(rowIndex, true);
	}
}

void MainWindow::loadDatabase()
{
	QFile file("mods_database.json");
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

			m_model->appendDatabase(modInfo);
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

	for (int i = 0; i < m_model->databaseSize(); ++i) {
		modInfo = m_model->modInfo(i);
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
	m_model->setModsExistsState(false);

	m_scanner->scanMods(m_modsFolderPath, *m_model);
	m_scanner->scanMods(m_tempModsFolderPath, *m_model);

	m_model->sortDatabase();
}

void MainWindow::setGameFolder(const QString &folderPath)
{
	if (folderPath.isEmpty()) {
		return;
	}

	m_gameFolderPath = folderPath;
	ui->gameFolderLineEdit->setText(m_gameFolderPath);

	if (m_gameFolderPath.contains("common\\Everlasting Summer\\")) {
		//Steam version
		m_modsFolderPath = m_gameFolderPath.section("common\\Everlasting Summer\\", 0, 0);
		m_modsFolderPath.append("workshop\\content\\331470\\");
	}
	else {
		//Non-Steam version
		m_modsFolderPath = m_gameFolderPath;
		m_modsFolderPath.append("\\game\\mods\\");
	}
	ui->modsFolderLineEdit->setText(m_modsFolderPath);

	if (m_tempModsFolderPath != m_gameFolderPath + "mods (temp)\\") {
		if (!m_tempModsFolderPath.isEmpty() && QDir(m_tempModsFolderPath).exists() &&
				QMessageBox::information(this, tr("Delete folder"), tr("Would you like to delete old mods temp folder?") +
								"\n" + m_tempModsFolderPath, QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No) ==
				QMessageBox::StandardButton::Yes)
			QDir(m_tempModsFolderPath).removeRecursively();

		m_tempModsFolderPath = m_gameFolderPath + "mods (temp)\\";
	}
	ui->tempModsFolderLineEdit->setText(m_tempModsFolderPath);

	refreshModlist();
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

void MainWindow::disableAllMods()
{
	m_model->blockSignals(true);

	for (int i = 0; i < m_model->databaseSize(); ++i) {
		if (m_model->modInfo(i).existsAndEnabledCheck(true, true)) {
			m_model->modInfoRef(i).enabled = false;
			ui->enabledModsList->setRowHidden(i, true);
			ui->disabledModsList->setRowHidden(i, false);
		}
	}

	m_model->blockSignals(false);

	ui->searchLineEdit->clear();
}

void MainWindow::enableAllMods()
{
	m_model->blockSignals(true);

	for (int i = 0; i < m_model->databaseSize(); ++i) {
		if (m_model->modInfo(i).existsAndEnabledCheck(true, false)) {
			m_model->modInfoRef(i).enabled = true;
			ui->enabledModsList->setRowHidden(i, false);
			ui->disabledModsList->setRowHidden(i, true);
		}
	}

	m_model->blockSignals(false);

	ui->searchLineEdit->clear();
}

void MainWindow::eraseDatabase()
{
	QFile::remove("mods_database.json");
	m_model->clearDatabase();
	refreshModlist();
}

void MainWindow::filterModsDisplay(const QString &str)
{
	int rowCount = m_model->databaseSize();
	bool enabled;
	for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex)
		if (m_model->modIsExists(rowIndex)) {
			enabled = m_model->modIsEnabled(rowIndex);
			ui->enabledModsList->setRowHidden(rowIndex, !enabled);
			ui->disabledModsList->setRowHidden(rowIndex, enabled);

			if (!m_model->data(m_model->index(rowIndex)).toString().contains(str, Qt::CaseSensitivity::CaseInsensitive)) {
				if (enabled)
					ui->enabledModsList->setRowHidden(rowIndex, true);
				else
					ui->disabledModsList->setRowHidden(rowIndex, true);
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

	int modsCount = QDir(m_modsFolderPath).entryList(QDir::Dirs|QDir::NoDotAndDotDot).count() +
					QDir(m_tempModsFolderPath).entryList(QDir::Dirs|QDir::NoDotAndDotDot).count();
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
	if (m_gameFolderPath.isEmpty()) {
		return;
	}

	if (!checkGameMd5(m_gameFolderPath)) {
		QMessageBox::critical(
			this,
			tr("Wrong game .exe"),
			tr("Game folder doesn't contains origin \n 'Everlasting Summer.exe' \n "
			   "and there is no file \n 'Everlasting Summer (origin).exe'!"),
			QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton
		);

		return;
	}

	if (m_databaseEditor->isVisible())
		m_databaseEditor->close();

	QDir dir(m_tempModsFolderPath);
	if (!dir.exists()) {
		dir.mkdir(m_tempModsFolderPath);
	}

	QString unmovedMods;
	moveModFolders(&unmovedMods);
	if (!unmovedMods.isEmpty()) {
		QMessageBox warningMessage(QMessageBox::Warning, tr("Unmoved mods"),
						tr("Disabled mods in these folders failed to move into temp folder:") + "\n\n" + unmovedMods + "\n" +
						tr("If you press the 'OK' button, the game will load these mods.") + "\n\n" +
						tr("You may move these folders manually from the mods folder:") + "\n\n" + m_modsFolderPath + "\n\n" +
						tr("to the temp mods folder:") + "\n\n" + m_tempModsFolderPath + "\n\n" +
						tr("before pressing the 'OK' button to fix this issue."),
						QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Open | QMessageBox::StandardButton::Cancel,
						this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
		warningMessage.button(QMessageBox::StandardButton::Open)->setText(tr("Open mods folder and temp folder in explorer"));
		warningMessage.setDefaultButton(QMessageBox::StandardButton::Cancel);
		warningMessage.setTextInteractionFlags(Qt::TextSelectableByMouse);
		QMessageBox::StandardButton pressedButton = QMessageBox::StandardButton(warningMessage.exec());

		if (pressedButton == QMessageBox::StandardButton::Open) {
			warningMessage.setStandardButtons(QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Cancel);
			warningMessage.show();	//Must call show() before openUrl(), or exec() will not work properly

			QDesktopServices::openUrl(QUrl("file:///" + m_tempModsFolderPath));
			QDesktopServices::openUrl(QUrl("file:///" + m_modsFolderPath));

			pressedButton = QMessageBox::StandardButton(warningMessage.exec());
		}

		if (pressedButton == QMessageBox::StandardButton::Cancel) {
			moveModFoldersBack();
			return;
		}
	}

	this->hide();

	QProcess gameLauncher;
	gameLauncher.setWorkingDirectory("launcher\\");
	gameLauncher.setProgram("launcher\\ESLauncher.exe");

	QByteArray launcherSettings = m_gameFolderPath.toUtf8() + "\nEverlasting Summer\ntrue";
	rewriteFileIfDataIsDifferent("launcher\\LaunchedProgram.ini", launcherSettings);

	//We must ensure that autoexit flag is up to date because it will be read by our .rpy script
	m_settings->setValue("General/bAutoexit", ui->autoexitCheckBox->isChecked());

	restoreOriginLauncher();

	gameLauncher.start();
	gameLauncher.waitForStarted(-1);
	gameLauncher.waitForFinished(-1);

	checkOriginLauncherReplacement();

	moveModFoldersBack();

	if (ui->autoexitCheckBox->isChecked()) {
		QApplication::exit();
	} else {
		this->show();
	}
}

void MainWindow::selectGameFolder()
{
	QUrl gameFolderUrl = QFileDialog::getExistingDirectory(this, tr("Select Everlasting Summer folder"),
														   m_gameFolderPath, QFileDialog::Option::DontUseNativeDialog);
	if (gameFolderUrl.isValid()) {
		QString folderPath = gameFolderUrl.toString().replace('/', '\\');
		if (QFileInfo::exists(folderPath + "\\Everlasting Summer.exe")) {
			folderPath[0] = folderPath.at(0).toUpper();
			setGameFolder(folderPath + '\\');
		}
		else {
			QMessageBox::critical(this, tr("Wrong game folder"), tr("Game folder doesn't contains \n 'Everlasting Summer.exe'!"),
								  QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton);
		}

	}
}

void MainWindow::selectModsFolder()
{
	QUrl modsFolderUrl = QFileDialog::getExistingDirectory(this, tr("Select folder of Everlasting Summer mods"),
															m_modsFolderPath, QFileDialog::Option::DontUseNativeDialog);
	if (modsFolderUrl.isValid()) {
		QString modsFolderPath = modsFolderUrl.toString().replace('/', '\\');

		m_modsFolderPath = modsFolderPath + '\\';
		m_modsFolderPath[0] = m_modsFolderPath.at(0).toUpper();
		ui->modsFolderLineEdit->setText(m_modsFolderPath);

		refreshModlist();
	}
}

void MainWindow::selectTempModsFolder()
{
	QUrl tempModsFolderUrl = QFileDialog::getExistingDirectory(this, tr("Select temp folder for unused Everlasting Summer mods"),
																m_tempModsFolderPath, QFileDialog::DontUseNativeDialog);
	if (tempModsFolderUrl.isValid()) {
		QString tempModsFolderPath = tempModsFolderUrl.toString().replace('/', '\\');
		if (m_tempModsFolderPath != tempModsFolderPath) {
			if (!m_tempModsFolderPath.isEmpty() && QDir(m_tempModsFolderPath).exists()
				&& QMessageBox::information(this, tr("Delete folder"), tr("Would you like to delete old mods temp folder?") +
											 "\n" + m_tempModsFolderPath,
											 QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No)
				== QMessageBox::StandardButton::Yes)
				QDir(m_tempModsFolderPath).removeRecursively();

			m_tempModsFolderPath = tempModsFolderPath + "\\mods (temp)\\";
			m_tempModsFolderPath[0] = m_tempModsFolderPath.at(0).toUpper();
		}
		ui->tempModsFolderLineEdit->setText(m_tempModsFolderPath);
	}
}

void MainWindow::setRowVisibility(const int rowIndex, const bool isVisibleInFirstList)
{
	ui->enabledModsList->setRowHidden(rowIndex, !isVisibleInFirstList);
	ui->disabledModsList->setRowHidden(rowIndex, isVisibleInFirstList);
}

void MainWindow::showAboutInfo()
{
	QMessageBox messageAbout(QMessageBox::NoIcon, tr("About ") + this->windowTitle(), "",
							 QMessageBox::StandardButton::Close, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	messageAbout.setTextFormat(Qt::TextFormat::RichText);
	messageAbout.setText(
		tr("Everlasting Summer mod manager v.") + applicationVersionToString(CurrentApplicationVersion) + ".<br>" +
		tr("Author:") + " <a href='https://steamcommunity.com/id/van_weyden/'>Slavyan</a><br>" +
		tr("Help in testing:") + " <a href='https://steamcommunity.com/profiles/76561198058938676/'>Hatsune Miku</a>,"
								 " 🔰 <a href='https://steamcommunity.com/id/lena_sova/'>" + tr("Lena") + "</a>🔰 ," +
								 " <a href='https://vk.com/svet_mag'>" + tr("Alexey Golikov") + "</a><br><br>" +
		tr("This program is used to 'fix' conflicts of mods and speed up the launch of the game. "
		   "Before launching the game, all unselected mods are moved to another folder, so the game engine will not load them."));
	messageAbout.setInformativeText(tr("You can leave your questions/suggestions") +
									" <a href='https://steamcommunity.com/sharedfiles/filedetails/?id=1826799366'>" +
									tr("here") + "</a>.");
	messageAbout.exec();
}

void MainWindow::showAnnouncementMessage()
{
	QMessageBox messageAbout(QMessageBox::Icon::Information, tr("Announcement"), "",
							 QMessageBox::StandardButton::Close, nullptr, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	messageAbout.setTextFormat(Qt::TextFormat::RichText);
	messageAbout.setText(
		tr("Currently, a survey is being conducted in which you can leave your "
		   "comments / wishes / suggestions, as well as help with choosing "
		   "a priority direction for future updates!") + "<br>" +
		tr("The survey is anonymous, does not require registration and only takes a few minutes."));
	messageAbout.setInformativeText("<a href='https://docs.google.com/forms/d/e/"
									"1FAIpQLSej0DemqLm1NRJv1eI4vCMz0DAr2d2Nynzwd1VIrwtLYX8r4g/viewform'>" +
									tr("Click here to take the survey") + "</a>");
	messageAbout.exec();
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

bool MainWindow::openModFolder(const int modIndex)
{
	if (QDir().exists(m_modsFolderPath + m_model->modFolderName(modIndex))) {
		return QDesktopServices::openUrl(QUrl("file:///" + m_modsFolderPath + m_model->modFolderName(modIndex)));
	} else if (QDir().exists(m_tempModsFolderPath + m_model->modFolderName(modIndex))) {
		return QDesktopServices::openUrl(QUrl("file:///" + m_tempModsFolderPath + m_model->modFolderName(modIndex)));
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

void MainWindow::checkAnnouncementPopup(const int loadedApplicationVersion)
{
	if (loadedApplicationVersion == CurrentApplicationVersion) {
		return;
	}

	if (loadedApplicationVersion < applicationVersion(1, 1, 9)) {
		showAnnouncementMessage();
	}
}

bool MainWindow::checkGameMd5(const QString &folderPath)
{
	QByteArray gameMd5 = fileChecksum(folderPath + "Everlasting Summer (origin).exe");
	bool isGameMd5Valid = (!gameMd5.isNull() && gameMd5 != m_launcherMd5);

	if (!isGameMd5Valid) {
		gameMd5 = fileChecksum(folderPath + "Everlasting Summer.exe");
		isGameMd5Valid = (!gameMd5.isNull() && gameMd5 != m_launcherMd5);
	}

	return isGameMd5Valid;
}

void MainWindow::checkOriginLauncherReplacement() const
{
	if (m_gameFolderPath.isEmpty()) {
		return;
	}

	QByteArray gameLauncherMd5 = fileChecksum(m_gameFolderPath + "Everlasting Summer.exe");

	if (ui->replaceOriginLauncherCheckBox->isChecked()) {
		if (gameLauncherMd5 != m_launcherMd5) {
			if (QFile(m_gameFolderPath + "Everlasting Summer (origin).exe").exists()) {
				QFile(m_gameFolderPath + "Everlasting Summer.exe").remove();
			} else {
				QFile::rename(m_gameFolderPath + "Everlasting Summer.exe",
							  m_gameFolderPath + "Everlasting Summer (origin).exe");
			}

			if (m_launcherMd5 == fileChecksum(m_gameFolderPath + "Everlasting Summer (modified).exe")) {
				QFile::rename(m_gameFolderPath + "Everlasting Summer (modified).exe",
							  m_gameFolderPath + "Everlasting Summer.exe");
			} else {
				if (QFile(m_gameFolderPath + "Everlasting Summer (modified).exe").exists()) {
					QFile(m_gameFolderPath + "Everlasting Summer (modified).exe").remove();
				}

				QFile::copy("launcher\\ESLauncher.exe", m_gameFolderPath + "Everlasting Summer.exe");
			}
		}

		QByteArray launcherSettings = QDir::currentPath().replace('/', '\\').toUtf8() + "\\\nESModManager\nfalse";
		rewriteFileIfDataIsDifferent(m_gameFolderPath + "LaunchedProgram.ini", launcherSettings);
	} else {
		if (gameLauncherMd5 == fileChecksum(m_gameFolderPath + "Everlasting Summer (origin).exe")) {
			QFile::remove(m_gameFolderPath + "Everlasting Summer (origin).exe");
		} else if (QFile::exists(m_gameFolderPath + "Everlasting Summer (origin).exe")) {
			QFile::remove(m_gameFolderPath + "Everlasting Summer.exe");
			QFile::rename(m_gameFolderPath + "Everlasting Summer (origin).exe",
						  m_gameFolderPath + "Everlasting Summer.exe");
		}

		QFile::remove(m_gameFolderPath + "Everlasting Summer (modified).exe");
		QFile::remove(m_gameFolderPath + "LaunchedProgram.ini");
	}
}

void MainWindow::restoreOriginLauncher() const
{
	QByteArray gameLauncherMd5 = fileChecksum(m_gameFolderPath + "Everlasting Summer.exe");

	if (QFile(m_gameFolderPath + "Everlasting Summer (origin).exe").exists()) {
		// The game launcher may have been changed

		if (gameLauncherMd5 != fileChecksum(m_gameFolderPath + "Everlasting Summer (origin).exe")) {
			if (fileChecksum(m_gameFolderPath + "Everlasting Summer (modified).exe") != m_launcherMd5) {
				QFile(m_gameFolderPath + "Everlasting Summer (modified).exe").remove();

				if (gameLauncherMd5 == m_launcherMd5) {
					QFile::rename(m_gameFolderPath + "Everlasting Summer.exe",
								  m_gameFolderPath + "Everlasting Summer (modified).exe");
				}
			}

			if (QFile(m_gameFolderPath + "Everlasting Summer.exe").exists()) {
				QFile(m_gameFolderPath + "Everlasting Summer.exe").remove();
			}

			QFile::rename(m_gameFolderPath + "Everlasting Summer (origin).exe",
						  m_gameFolderPath + "Everlasting Summer.exe");
		}
	}
}

void MainWindow::moveModFolders(QString *unmovedModsToTempFolder, QString *unmovedModsFromTempFolder) const
{
	QDir dir;
	int databaseSize = m_model->databaseSize();
	for (int i = 0; i < databaseSize; ++i) {
		if (m_model->modInfo(i).exists) {
			if (m_model->modInfo(i).enabled) {
				if (dir.exists(m_tempModsFolderPath + m_model->modFolderName(i)) &&
					!dir.rename(m_tempModsFolderPath + m_model->modFolderName(i), m_modsFolderPath + m_model->modFolderName(i)) &&
					unmovedModsFromTempFolder != nullptr) {
					unmovedModsFromTempFolder->append(m_model->modFolderName(i) + '\n');
				}
			} else if (dir.exists(m_modsFolderPath + m_model->modFolderName(i)) &&
				!dir.rename(m_modsFolderPath + m_model->modFolderName(i), m_tempModsFolderPath + m_model->modFolderName(i)) &&
				unmovedModsToTempFolder != nullptr) {
				unmovedModsToTempFolder->append(m_model->modFolderName(i) + '\n');
			}
		}
	}
}

void MainWindow::moveModFoldersBack() const
{
	if (!ui->moveModsBackCheckBox->isChecked())
		return;

	QDir dir;
	int databaseSize = m_model->databaseSize();
	for (int i = 0; i < databaseSize; ++i) {
		if (m_model->modInfo(i).exists && dir.exists(m_tempModsFolderPath + m_model->modFolderName(i)))
			dir.rename(m_tempModsFolderPath + m_model->modFolderName(i), m_modsFolderPath + m_model->modFolderName(i));
	}
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

	applyBackwardCompatibilityFixes(loadedApplicationVersion);

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

	if (m_settings->contains("General/sTempModsFolder")) {
		value = m_settings->value("General/sTempModsFolder");
		if (value != invalidValue) {
			m_tempModsFolderPath = value.toString();
		}
	}

	if (m_settings->contains("General/bAutoexit")) {
		value = m_settings->value("General/bAutoexit");
		if (value != invalidValue) {
			ui->autoexitCheckBox->setChecked(value.toBool());
		}
	}

	if (m_settings->contains("General/bMoveModsBack")) {
		value = m_settings->value("General/bMoveModsBack");
		if (value != invalidValue) {
			ui->moveModsBackCheckBox->setChecked(value.toBool());
		}
	}

	if (m_settings->contains("General/bReplaceOriginLauncher")) {
		value = m_settings->value("General/bReplaceOriginLauncher");
		if (value != invalidValue) {
			ui->replaceOriginLauncherCheckBox->setChecked(value.toBool());
		}
	}

	if (m_settings->contains("General/bCompleteModNames")) {
		value = m_settings->value("General/bCompleteModNames");
		if (value != invalidValue) {
			ui->completeNamesCheckBox->setChecked(value.toBool());
		}
	}

	if (m_settings->contains("General/bUseSteamModNames")) {
		value = m_settings->value("General/bUseSteamModNames");
		if (value != invalidValue) {
			ui->useSteamModNamesCheckBox->setChecked(value.toBool());
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

	checkAnnouncementPopup(loadedApplicationVersion);
}

void MainWindow::saveSettings() const
{
	m_settings->clear();
	m_settings->setValue("General/sAppVersion", applicationVersionToString(CurrentApplicationVersion));
	m_settings->setValue("General/sLang", m_lang);
	m_settings->setValue("General/bMaximized", this->isMaximized());
	m_settings->setValue("General/qsSize", this->size());
	m_settings->setValue("General/sGameFolder", m_gameFolderPath);
	m_settings->setValue("General/sModsFolder", m_modsFolderPath);
	m_settings->setValue("General/sTempModsFolder", m_tempModsFolderPath);
	m_settings->setValue("General/bAutoexit", ui->autoexitCheckBox->isChecked());
	m_settings->setValue("General/bMoveModsBack", ui->moveModsBackCheckBox->isChecked());
	m_settings->setValue("General/bReplaceOriginLauncher", ui->replaceOriginLauncherCheckBox->isChecked());
	m_settings->setValue("General/bCompleteModNames", ui->completeNamesCheckBox->isChecked());
	m_settings->setValue("General/bUseSteamModNames", ui->useSteamModNamesCheckBox->isChecked());
	m_settings->setValue("Editor/bMaximized", m_databaseEditor->isMaximized());
	m_settings->setValue("Editor/qsSize", m_databaseEditor->size());

	moveModFoldersBack();
	checkOriginLauncherReplacement();
	m_model->sortDatabase();
	saveDatabase();
}

void MainWindow::applyBackwardCompatibilityFixes(const int loadedApplicationVersion)
{
	if (loadedApplicationVersion < applicationVersion(1, 1, 9)) {
		///Fix bug from old versions when game folder path contained "Everlasting Summer.exe"
		m_gameFolderPath.remove("Everlasting Summer.exe");
		m_modsFolderPath.remove("Everlasting Summer.exe");
		m_tempModsFolderPath.remove("Everlasting Summer.exe");

		///Condition for backward compatibility
		///Reset the checkbox for replacing game files for older versions, as defaults changed
		if (m_settings->contains("General/bMaximized")) {
			m_settings->setValue("General/bReplaceOriginLauncher", true);
		}

		///Conversion from old version of DB
		{
			QFile file("mods_database.dat");
			QJsonDocument database;
			QJsonArray mods;
			QJsonObject mod;

			if (file.exists()) {
				QString name, folderName, enabled;
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
				}
				while (file.canReadLine());
				file.close();
				file.remove();

				database.setArray(mods);
				rewriteFileIfDataIsDifferent("mods_database.json", database.toJson());
			}
		}
	}
}
