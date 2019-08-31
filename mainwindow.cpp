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

#include "databaseeditor.h"
#include "databasemodel.h"
#include "steamrequester.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCryptographicHash>
QByteArray fileChecksum(const QString &fileName, QCryptographicHash::Algorithm hashAlgorithm)
{
	QFile f(fileName);
	if (f.open(QFile::ReadOnly)) {
		QCryptographicHash hash(hashAlgorithm);
		if (hash.addData(&f)) {
			f.close();
			return hash.result();
		}
	}
	return QByteArray();
}

//public:

MainWindow::MainWindow(QWidget *parent, const bool &runCheck) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	model_ = new DatabaseModel();
	steamRequester_ = new SteamRequester(model_);
	thread_ = new QThread;
	connect(thread_, SIGNAL(started()), steamRequester_, SLOT(requestModNames()));
	connect(steamRequester_, SIGNAL(modProcessed()), this, SLOT(steamModNameProcessed()), Qt::BlockingQueuedConnection);
	connect(steamRequester_, SIGNAL(finished()), thread_, SLOT(quit()));
	steamRequester_->moveToThread(thread_);

	ui->setupUi(this);

	ui->engLangButton->setIcon(QIcon(":/images/Flag-United-States.ico"));
	ui->rusLangButton->setIcon(QIcon(":/images/Flag-Russia.ico"));

	ui->enabledModsList->setStyleSheet("QListView::item:!selected:!hover { border-bottom: 1px solid #E5E5E5; }");
	ui->enabledModsList->setModel(model_);


	ui->disabledModsList->setStyleSheet("QListView::item:!selected:!hover { border-bottom: 1px solid #E5E5E5; }");
	ui->disabledModsList->setModel(model_);

	//A small crutch for highlighting when you hover the mouse, but you couldn't select the line
	connect(ui->enabledModsList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			ui->enabledModsList, SLOT(clearSelection()));
	connect(ui->disabledModsList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			ui->disabledModsList, SLOT(clearSelection()));

	ui->progressLabel->hide();
	ui->progressBar->hide();

	databaseEditor_ = new DatabaseEditor();

	settings_ = new QSettings(QString("settings.ini"), QSettings::IniFormat, this);
	qtTranslator_ = new QTranslator();
	translator_ = new QTranslator();
	launcherMd5_ = fileChecksum(QDir::currentPath() + "/launcher/ESLauncher.exe", QCryptographicHash::Md5);

	//connections:

	//Signals from form objects:
	connect(ui->actionGame_folder, SIGNAL(triggered()), this, SLOT(selectGameFolder()));
	connect(ui->actionMods_folder, SIGNAL(triggered()), this, SLOT(selectModsFolder()));
	connect(ui->actionTemp_mods_folder, SIGNAL(triggered()), this, SLOT(selectTempModsFolder()));
	connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAboutInfo()));

	connect(ui->completeNamesCheckBox, SIGNAL(stateChanged(int)),
			model_, SLOT(setCompleteModNames(const int &)));
	connect(ui->useSteamModNamesCheckBox, SIGNAL(stateChanged(int)),
			model_, SLOT(setUsingSteamModNames(const int &)));

	connect(ui->eraseDatabaseButton, SIGNAL(clicked()), this, SLOT(eraseDatabase()));
	connect(ui->databaseEditorButton, SIGNAL(clicked()), databaseEditor_, SLOT(show()));

	connect(ui->engLangButton, SIGNAL(clicked()), this, SLOT(setEnglishLanguage()));
	connect(ui->rusLangButton, SIGNAL(clicked()), this, SLOT(setRussianLanguage()));

	connect(ui->disableAllButton, SIGNAL(clicked()), this, SLOT(disableAllMods()));
	connect(ui->enableAllButton, SIGNAL(clicked()), this, SLOT(enableAllMods()));
	connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(refreshModlist()));
	connect(ui->runButton, SIGNAL(clicked()), this, SLOT(runGame()));

	connect(ui->enabledModsList, SIGNAL(clicked(const QModelIndex &)), model_, SLOT(disableMod(const QModelIndex &)));
	connect(ui->disabledModsList, SIGNAL(clicked(const QModelIndex &)), model_, SLOT(enableMod(const QModelIndex &)));

	connect(ui->clearSearchPushButton, SIGNAL(clicked()), ui->searchLineEdit, SLOT(clear()));
	connect(ui->searchLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterModsDisplay(const QString &)));


	//Other signals:
	connect(steamRequester_, SIGNAL(finished()), ui->progressLabel, SLOT(hide()));
	connect(steamRequester_, SIGNAL(finished()), ui->progressBar, SLOT(hide()));
	connect(databaseEditor_, SIGNAL(openModFolder(const int &)), this, SLOT(openModFolder(const int &)));
	connect(model_, SIGNAL(modCheckStateChanged(const int &, const bool &)),
			this, SLOT(setRowVisibility(const int &, const bool &)));

	//this->adjustSize();

	readSettings();

	if (lang_.isEmpty()) {
		if (translator_->load(QString(":/lang/lang_") + QLocale::system().name(), ":/lang/")) {
			QApplication::installTranslator(translator_);
			qtTranslator_->load(QString(":/lang/qtbase_") + QLocale::system().name(), ":/lang/");
			QApplication::installTranslator(qtTranslator_);
			lang_ = QLocale::system().name();
		}
		else {
			lang_ = "en_US";
		}
	}
	else {
		setLanguage(lang_);
	}

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

	if (gameFolderPath_.isEmpty()) {
		QString gamePath = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Everlasting Summer\\";
		if (QFileInfo::exists(gamePath + "Everlasting Summer.exe")) {
			setGameFolder(gamePath);
		}
		else {
			gamePath = QDir::currentPath().section('/', 0, -6);
			gamePath.replace('/', '\\');
			gamePath.append("\\common\\Everlasting Summer\\");
			if (QFileInfo::exists(gamePath + "Everlasting Summer.exe"))
				setGameFolder(gamePath);
		}
	}
	else {
		///Begin of backward compatibility block
		///Fix bug from old versions when game folder path contained "Everlasting Summer.exe"
		{
			if (gameFolderPath_.contains("Everlasting Summer.exe"))
				gameFolderPath_.remove("Everlasting Summer.exe");
			if (modsFolderPath_.contains("Everlasting Summer.exe"))
				modsFolderPath_.remove("Everlasting Summer.exe");
			if (tempModsFolderPath_.contains("Everlasting Summer.exe"))
				tempModsFolderPath_.remove("Everlasting Summer.exe");
		}
		///End of backward compatibility block

		ui->gameFolderLineEdit->setText(gameFolderPath_);
		ui->modsFolderLineEdit->setText(modsFolderPath_);
		ui->tempModsFolderLineEdit->setText(tempModsFolderPath_);
		loadDatabase();
		refreshModlist();
	}

	databaseEditor_->setModel(model_, 1);
}

MainWindow::~MainWindow()
{
	model_->sortDatabase();

	saveSettings();

	QApplication::removeTranslator(translator_);
	QApplication::removeTranslator(qtTranslator_);
	steamRequester_->deleteLater();
	thread_->deleteLater();
	delete translator_;
	delete qtTranslator_;
	delete settings_;
	delete model_;
	delete databaseEditor_;

	delete ui;
}

void MainWindow::checkRowsVisibility()
{
	clearSearchField();

	bool enabled, exists;
	int databaseSize = model_->databaseSize();
	for (int rowIndex = 0; rowIndex < databaseSize; ++rowIndex) {
		enabled = model_->modIsEnabled(rowIndex);
		exists = model_->modIsExists(rowIndex);

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

	int rowCount = model_->databaseSize();
	for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
		ui->enabledModsList->setRowHidden(rowIndex, true);
		ui->disabledModsList->setRowHidden(rowIndex, true);
	}
}

void MainWindow::loadDatabase()
{
	QFile file;

	///Begin of backward compatibility block
	///Conversion from old version of DB
	{
		file.setFileName("mods_database.dat");
		if (file.exists()) {
			QString name, folderName, enabled;
			ModInfo modInfo;

			file.open(QFile::ReadOnly);
			do {
				modInfo.name = file.readLine();
				if (!file.canReadLine())
					break;
				modInfo.folderName = file.readLine();
				if (!file.canReadLine())
					break;
				enabled = file.readLine();
				modInfo.name.remove(QRegExp("[\\n\\r]"));
				modInfo.folderName.remove(QRegExp("[\\n\\r]"));
				enabled.remove(QRegExp("[\\n\\r]"));
				if (enabled.isEmpty()) {
					continue;
				}
				else {
					modInfo.enabled = enabled.toInt();
					model_->appendDatabase(modInfo);
				}
			}
			while (file.canReadLine());
			file.close();

			file.remove();
			return;
		}
	}
	///End of backward compatibility block

	file.setFileName("mods_database.json");
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

			model_->appendDatabase(modInfo);
		}
	}
}

void MainWindow::requestSteamModNames()
{
	if (!steamRequester_->isRunning())
		thread_->start();
}

void MainWindow::saveDatabase() const
{
	QJsonDocument database;
	QJsonArray mods;
	QJsonObject mod;
	ModInfo modInfo;

	for (int i = 0; i < model_->databaseSize(); ++i) {
		modInfo = model_->modInfo(i);
		mod["Name"] = modInfo.name;
		mod["Folder name"] = modInfo.folderName;
		mod["Steam name"] = modInfo.steamName;
		mod["Is enabled"] = modInfo.enabled;
		mods.append(mod);
	}
	database.setArray(mods);

	QByteArray rawData = database.toJson();
	QFile file("mods_database.json");
	if (file.exists("mods_database.json")) {
		file.open(QFile::ReadOnly);
		if (file.readAll() != rawData) {
			file.close();
			file.open(QFile::WriteOnly);
			file.write(rawData);
		}
		file.close();
	}
	else {
		file.open(QFile::WriteOnly);
		file.write(rawData);
		file.close();
	}
}

void MainWindow::scanMods()
{
	model_->setModsExistsState(false);

	scanMods(modsFolderPath_);
	scanMods(tempModsFolderPath_);

	model_->sortDatabase();
}

void MainWindow::setGameFolder(const QString &folderPath)
{
	gameFolderPath_ = folderPath;
	ui->gameFolderLineEdit->setText(gameFolderPath_);

	if (gameFolderPath_.contains("common\\Everlasting Summer\\")) {
		//Steam version
		modsFolderPath_ = gameFolderPath_.section("common\\Everlasting Summer\\", 0, 0);
		modsFolderPath_.append("workshop\\content\\331470\\");
	}
	else {
		//Non-Steam version
		modsFolderPath_ = gameFolderPath_;
		modsFolderPath_.append("\\game\\mods\\");
	}
	ui->modsFolderLineEdit->setText(modsFolderPath_);

	if (tempModsFolderPath_ != gameFolderPath_ + "mods (temp)\\") {
		if (!tempModsFolderPath_.isEmpty() && QDir(tempModsFolderPath_).exists() &&
				QMessageBox::information(this, tr("Delete folder"), tr("Would you like to delete old mods temp folder?") +
								"\n" + tempModsFolderPath_, QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No) ==
				QMessageBox::StandardButton::Yes)
			QDir(tempModsFolderPath_).removeRecursively();

		tempModsFolderPath_ = gameFolderPath_ + "mods (temp)\\";
	}
	ui->tempModsFolderLineEdit->setText(tempModsFolderPath_);

	refreshModlist();
}

bool MainWindow::setLanguage(const QString &lang)
{
	lang_ = lang;
	bool isTranslationLoaded = translator_->load(QString(":/lang/lang_") + lang_, ":/lang/");
	QApplication::installTranslator(translator_);
	qtTranslator_->load(QString(":/lang/qtbase_") + lang_, ":/lang/");
	QApplication::installTranslator(qtTranslator_);

	return isTranslationLoaded;
}

//public slots:

void MainWindow::disableAllMods()
{
	model_->blockSignals(true);

	for (int i = 0; i < model_->databaseSize(); ++i) {
		if (model_->modInfo(i).existsAndEnabledCheck(true, true)) {
			model_->modInfoRef(i).enabled = false;
			ui->enabledModsList->setRowHidden(i, true);
			ui->disabledModsList->setRowHidden(i, false);
		}
	}

	model_->blockSignals(false);

	ui->searchLineEdit->clear();
}

void MainWindow::enableAllMods()
{
	model_->blockSignals(true);

	for (int i = 0; i < model_->databaseSize(); ++i) {
		if (model_->modInfo(i).existsAndEnabledCheck(true, false)) {
			model_->modInfoRef(i).enabled = true;
			ui->enabledModsList->setRowHidden(i, false);
			ui->disabledModsList->setRowHidden(i, true);
		}
	}

	model_->blockSignals(false);

	ui->searchLineEdit->clear();
}

void MainWindow::eraseDatabase()
{
	QFile::remove("mods_database.json");
	model_->clearDatabase();
	refreshModlist();
}

void MainWindow::filterModsDisplay(const QString &str)
{
	int rowCount = model_->databaseSize();
	bool enabled;
	for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex)
		if (model_->modIsExists(rowIndex)) {
			enabled = model_->modIsEnabled(rowIndex);
			ui->enabledModsList->setRowHidden(rowIndex, !enabled);
			ui->disabledModsList->setRowHidden(rowIndex, enabled);

			if (!model_->data(model_->index(rowIndex)).toString().contains(str, Qt::CaseSensitivity::CaseInsensitive)) {
				if (enabled)
					ui->enabledModsList->setRowHidden(rowIndex, true);
				else
					ui->disabledModsList->setRowHidden(rowIndex, true);
			}
		}
}

void MainWindow::refreshModlist()
{
	bool wasOpened = this->isVisible();
	bool editorWasOpened = databaseEditor_->isVisible();
	if (editorWasOpened)
		databaseEditor_->close();
	if (wasOpened)
		this->hide();

	int modsCount = QDir(modsFolderPath_).entryList(QDir::Dirs|QDir::NoDotAndDotDot).count() +
					QDir(tempModsFolderPath_).entryList(QDir::Dirs|QDir::NoDotAndDotDot).count();
	QProgressDialog progressDialog(tr("Mod Manager: scanning installed mods..."), "", 0, modsCount);
	progressDialog.setCancelButton(nullptr);
	connect(this, SIGNAL(countOfScannedMods(int)), &progressDialog, SLOT(setValue(int)));
	progressDialog.show();
	countOfScannedMods_ = 0;

	ui->progressBar->setMaximum(modsCount);
	ui->progressBar->setValue(0);

	scanMods();

	ui->progressLabel->show();
	ui->progressBar->show();
	requestSteamModNames();
	checkRowsVisibility();

	if (wasOpened)
		this->show();
	if (editorWasOpened)
		databaseEditor_->show();
}

void MainWindow::runGame()
{
	if (!checkGameMd5(gameFolderPath_))
		return;

	if (databaseEditor_->isVisible())
		databaseEditor_->close();
	this->hide();
	QDir dir(tempModsFolderPath_);
	if (!dir.exists())
		dir.mkdir(tempModsFolderPath_);

	QString unmovedMods;
	moveModFolders(&unmovedMods);
	if (!unmovedMods.isEmpty())
		QMessageBox::warning(this, tr("Unmoved mods"),
							 tr("Disabled mods in these folders failed to move into temp folder:") +
								"\n\n" + unmovedMods + "\n" +
							 tr("Please move these folders manually from the mods folder:") + "\n\n" + modsFolderPath_ + "\n\n" +
							 tr("to the temp mods folder:") + "\n\n" + tempModsFolderPath_ + "\n\n" +
							 tr("before closing this message box, otherwise the game will load them."),
								QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton);

	QProcess gameLauncher;
	gameLauncher.setWorkingDirectory("launcher\\");
	gameLauncher.setProgram("launcher\\ESLauncher.exe");

	QFile launcherSettingsFile("launcher\\LaunchedProgram.ini");
	QByteArray launcherSettings = (gameFolderPath_).toUtf8() + "\nEverlasting Summer\ntrue";
	launcherSettingsFile.open(QFile::ReadOnly);
	QByteArray tmp = launcherSettingsFile.readAll();
	if (tmp != launcherSettings) {
		launcherSettingsFile.close();
		launcherSettingsFile.open(QFile::WriteOnly);
		launcherSettingsFile.write(launcherSettings);
	}
	launcherSettingsFile.close();

	if (fileChecksum(gameFolderPath_ + "Everlasting Summer.exe", QCryptographicHash::Md5) == launcherMd5_) {
		//Remove "extra" files
		if (QFile(gameFolderPath_ + "Everlasting Summer (modified).exe").exists())
			QFile(gameFolderPath_ + "Everlasting Summer (modified).exe").remove();

		QFile::rename(gameFolderPath_ + "Everlasting Summer.exe", gameFolderPath_ + "Everlasting Summer (modified).exe");
		QFile::rename(gameFolderPath_ + "Everlasting Summer (origin).exe", gameFolderPath_ + "Everlasting Summer.exe");
	}

	gameLauncher.start();
	gameLauncher.waitForStarted(-1);
	gameLauncher.waitForFinished(-1);

	checkOriginLauncherReplacement();

	moveModFoldersBack();

	if (ui->autoexitCheckBox->isChecked())
		QApplication::exit();
	else
		this->show();
}

void MainWindow::selectGameFolder()
{
	QUrl gameFolderUrl = QFileDialog::getExistingDirectory(this, tr("Select Everlasting Summer folder"),
														   gameFolderPath_, QFileDialog::Option::DontUseNativeDialog);
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
															modsFolderPath_, QFileDialog::Option::DontUseNativeDialog);
	if (modsFolderUrl.isValid()) {
		QString modsFolderPath = modsFolderUrl.toString().replace('/', '\\');

		modsFolderPath_ = modsFolderPath + '\\';
		modsFolderPath_[0] = modsFolderPath_.at(0).toUpper();
		ui->modsFolderLineEdit->setText(modsFolderPath_);

		refreshModlist();
	}
}

void MainWindow::selectTempModsFolder()
{
	QUrl tempModsFolderUrl = QFileDialog::getExistingDirectory(this, tr("Select temp folder for unused Everlasting Summer mods"),
																tempModsFolderPath_, QFileDialog::DontUseNativeDialog);
	if (tempModsFolderUrl.isValid()) {
		QString tempModsFolderPath = tempModsFolderUrl.toString().replace('/', '\\');
		if (tempModsFolderPath_ != tempModsFolderPath) {
			if (!tempModsFolderPath_.isEmpty() && QDir(tempModsFolderPath_).exists() &&
					QMessageBox::information(this, tr("Delete folder"), tr("Would you like to delete old mods temp folder?") +
											 "\n" + tempModsFolderPath_, QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No) ==
					QMessageBox::StandardButton::Yes)
				QDir(tempModsFolderPath_).removeRecursively();

			tempModsFolderPath_ = tempModsFolderPath + "\\mods (temp)\\";
			tempModsFolderPath_[0] = tempModsFolderPath_.at(0).toUpper();
		}
		ui->tempModsFolderLineEdit->setText(tempModsFolderPath_);
	}
}

void MainWindow::setRowVisibility(const int &rowIndex, const bool &isVisibleInFirstList)
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
		tr("Everlasting Summer mod manager v.1.1.") + "<br>" +
		tr("Author:") + " <a href='https://steamcommunity.com/id/van_weyden/'>Slavyan</a><br>" +
		tr("Help in testing:") + " <a href='https://steamcommunity.com/profiles/76561198058938676/'>Hatsune Miku</a>,"
								 " 🔰 <a href='https://steamcommunity.com/id/lena_sova/'>" + tr("Lena") + "</a>🔰 ," +
								 " <a href='https://vk.com/svet_mag'>" + tr("Alexey Golikov") + "</a><br><br>" +
		tr("This program is used to 'fix' conflicts of mods and speed up the launch of the game. "
		   "Before launching the game, all unselected mods are moved to another folder, so the game engine will not load them."));
	messageAbout.setInformativeText(tr("You can leave your questions/suggestions") +
									" <a href='https://steamcommunity.com/sharedfiles/filedetails/?id=1826799366'>" + tr("here") + "</a>.");
	messageAbout.exec();
}

//protected:
void MainWindow::changeEvent(QEvent *event)
{
   if (event->type() == QEvent::LanguageChange)
		ui->retranslateUi(this);
   else
		QMainWindow::changeEvent(event);
}

//private slots:
bool MainWindow::openModFolder(const int &modIndex)
{
	if (QDir().exists(modsFolderPath_ + model_->modFolderName(modIndex)))
		return QDesktopServices::openUrl(QUrl("file:///" + modsFolderPath_ + model_->modFolderName(modIndex)));
	else if (QDir().exists(tempModsFolderPath_ + model_->modFolderName(modIndex)))
		return QDesktopServices::openUrl(QUrl("file:///" + tempModsFolderPath_ + model_->modFolderName(modIndex)));

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

bool MainWindow::checkGameMd5(const QString &folderPath)
{
	QByteArray gameMd5 = fileChecksum(folderPath + "Everlasting Summer.exe", QCryptographicHash::Md5);

	if (gameMd5 == launcherMd5_ || gameMd5.isNull())
		gameMd5 = fileChecksum(folderPath + "Everlasting Summer (origin).exe", QCryptographicHash::Md5);

	if (gameMd5.isNull()) {
		QMessageBox::critical(this, tr("Wrong game .exe"), tr("Game folder doesn't contains origin \n 'Everlasting Summer.exe' \n and there is no file \n 'Everlasting Summer (origin).exe'!"),
							  QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton);
		return false;
	}
	return true;
}

void MainWindow::checkOriginLauncherReplacement() const
{
	if (ui->replaceOriginLauncherCheckBox) {
		if (fileChecksum(gameFolderPath_ + "Everlasting Summer.exe", QCryptographicHash::Md5) != launcherMd5_) {
			//Remove "extra" files
			if (QFile(gameFolderPath_ + "Everlasting Summer (origin).exe").exists())
				QFile(gameFolderPath_ + "Everlasting Summer (origin).exe").remove();

			QFile::rename(gameFolderPath_ + "Everlasting Summer.exe", gameFolderPath_ + "Everlasting Summer (origin).exe");

			if (QFileInfo::exists(gameFolderPath_ + "Everlasting Summer (modified).exe"))
				QFile::rename(gameFolderPath_ + "Everlasting Summer (modified).exe", gameFolderPath_ + "Everlasting Summer.exe");
			else
				QFile::copy("launcher\\ESLauncher.exe", gameFolderPath_ + "Everlasting Summer.exe");
		}

		QByteArray launcherSettings = QDir::currentPath().replace('/', '\\').toUtf8() + "\\\nESModManager\nfalse";
		QFile launcherSettingsFile(gameFolderPath_ + "LaunchedProgram.ini");
		launcherSettingsFile.open(QFile::ReadOnly);
		if (launcherSettingsFile.readAll() != launcherSettings) {
			launcherSettingsFile.close();
			launcherSettingsFile.open(QFile::WriteOnly);
			launcherSettingsFile.write(launcherSettings);
		}
		launcherSettingsFile.close();
	}
	else {
		if (fileChecksum(gameFolderPath_ + "Everlasting Summer.exe", QCryptographicHash::Md5) == launcherMd5_) {
			QFile::remove(gameFolderPath_ + "Everlasting Summer.exe");
			QFile::rename(gameFolderPath_ + "Everlasting Summer (origin).exe", gameFolderPath_ + "Everlasting Summer.exe");
		}
		else
			QFile::remove(gameFolderPath_ + "Everlasting Summer (origin).exe");

		QFile::remove(gameFolderPath_ + "Everlasting Summer (modified).exe");
		QFile::remove(gameFolderPath_ + "LaunchedProgram.ini");
	}
}

void MainWindow::moveModFolders(QString *unmovedModsToTempFolder, QString *unmovedModsFromTempFolder) const
{
	QDir dir;
	int databaseSize = model_->databaseSize();
	for (int i = 0; i < databaseSize; ++i) {
		if (model_->modInfo(i).exists) {
			if (model_->modInfo(i).enabled) {
				if (dir.exists(tempModsFolderPath_ + model_->modFolderName(i)) &&
					!dir.rename(tempModsFolderPath_ + model_->modFolderName(i), modsFolderPath_ + model_->modFolderName(i)) &&
					unmovedModsFromTempFolder != nullptr) {
					unmovedModsFromTempFolder->append(model_->modFolderName(i) + '\n');
				}
			}
			else {
				if (dir.exists(modsFolderPath_ + model_->modFolderName(i)) &&
					!dir.rename(modsFolderPath_ + model_->modFolderName(i), tempModsFolderPath_ + model_->modFolderName(i)) &&
					unmovedModsToTempFolder != nullptr) {
					unmovedModsToTempFolder->append(model_->modFolderName(i) + '\n');
				}
			}
		}
	}
}

void MainWindow::moveModFoldersBack() const
{
	if (!ui->moveModsBackCheckBox->isChecked())
		return;

	QDir dir;
	int databaseSize = model_->databaseSize();
	for (int i = 0; i < databaseSize; ++i) {
		if (model_->modInfo(i).exists && dir.exists(tempModsFolderPath_ + model_->modFolderName(i)))
			dir.rename(tempModsFolderPath_ + model_->modFolderName(i), modsFolderPath_ + model_->modFolderName(i));
	}
}

void MainWindow::readSettings()
{
	QVariant value;
	QVariant invalidValue;

	if (settings_->contains("General/sLang")) {
		value = settings_->value("General/sLang");
		if (value != invalidValue)
			lang_ = value.toString();
	}

	if (settings_->contains("General/bMaximized")) {
		value = settings_->value("General/bMaximized");
		if (value != invalidValue && value.toBool() == true) {
			this->setWindowState(Qt::WindowMaximized);
		}
		else if (settings_->contains("General/qsSize")) {
			value = settings_->value("General/qsSize");
			if (value != invalidValue)
				this->resize(value.toSize());
		}
	}

	if (settings_->contains("General/sGameFolder")) {
		value = settings_->value("General/sGameFolder");
		if (value != invalidValue)
			gameFolderPath_ = value.toString();
	}

	if (settings_->contains("General/sModsFolder")) {
		value = settings_->value("General/sModsFolder");
		if (value != invalidValue)
			modsFolderPath_ = value.toString();
	}

	if (settings_->contains("General/sTempModsFolder")) {
		value = settings_->value("General/sTempModsFolder");
		if (value != invalidValue)
			tempModsFolderPath_ = value.toString();
	}

	if (settings_->contains("General/bAutoexit")) {
		value = settings_->value("General/bAutoexit");
		if (value != invalidValue)
			ui->autoexitCheckBox->setChecked(value.toBool());
	}

	if (settings_->contains("General/bMoveModsBack")) {
		value = settings_->value("General/bMoveModsBack");
		if (value != invalidValue)
			ui->moveModsBackCheckBox->setChecked(value.toBool());
	}

	///Condition for backward compatibility
	///Reset the checkbox for replacing game files for older versions, as defaults changed
	if (settings_->contains("General/bMaximized"))
		if (settings_->contains("General/bReplaceOriginLauncher")) {
			value = settings_->value("General/bReplaceOriginLauncher");
			if (value != invalidValue)
				ui->replaceOriginLauncherCheckBox->setChecked(value.toBool());
		}


	if (settings_->contains("General/bCompleteModNames")) {
		value = settings_->value("General/bCompleteModNames");
		if (value != invalidValue)
			ui->completeNamesCheckBox->setChecked(value.toBool());
	}

	if (settings_->contains("General/bUseSteamModNames")) {
		value = settings_->value("General/bUseSteamModNames");
		if (value != invalidValue)
			ui->useSteamModNamesCheckBox->setChecked(value.toBool());
	}

	if (settings_->contains("Editor/bMaximized")) {
		value = settings_->value("Editor/bMaximized");
		if (value != invalidValue && value.toBool() == true) {
			databaseEditor_->setWindowState(Qt::WindowMaximized);
		}
		else if (settings_->contains("Editor/qsSize")) {
			value = settings_->value("Editor/qsSize");
			if (value != invalidValue)
				databaseEditor_->resize(value.toSize());
		}
	}
}

void MainWindow::saveSettings() const
{
	settings_->clear();

	settings_->setValue("General/sLang", lang_);

	settings_->setValue("General/bMaximized", this->isMaximized());

	settings_->setValue("General/qsSize", this->size());

	settings_->setValue("General/sGameFolder", gameFolderPath_);

	settings_->setValue("General/sModsFolder", modsFolderPath_);

	settings_->setValue("General/sTempModsFolder", tempModsFolderPath_);

	settings_->setValue("General/bAutoexit", ui->autoexitCheckBox->isChecked());

	settings_->setValue("General/bMoveModsBack", ui->moveModsBackCheckBox->isChecked());
	moveModFoldersBack();

	settings_->setValue("General/bReplaceOriginLauncher", ui->replaceOriginLauncherCheckBox->isChecked());
	checkOriginLauncherReplacement();

	settings_->setValue("General/bCompleteModNames", ui->completeNamesCheckBox->isChecked());

	settings_->setValue("General/bUseSteamModNames", ui->useSteamModNamesCheckBox->isChecked());

	settings_->setValue("Editor/bMaximized", databaseEditor_->isMaximized());

	settings_->setValue("Editor/qsSize", databaseEditor_->size());

	saveDatabase();
}

void MainWindow::scanMods(const QString &modsFolderPath)
{
	QString currentPath = QDir::currentPath().replace('/', '\\');
	QDir dir(modsFolderPath);
	QStringList extensionFilter("*.rpy");
	QDir::Filters fileFilters = QDir::Files;
	QDir::Filters dirFilters = QDir::Dirs|QDir::NoDotAndDotDot;
							   //  \$?[ \n\r]*((mods)|(filters))[ \n\r]*\[((\"[^\"]*\")|(\'[^\"]*\'))\][ \n\r]*=[^\"]*
	QRegExp initRegExp = QRegExp("\\$?[ \\n\\r]*((mods)|(filters))[ \\n\\r]*\\[((\"[^\"]*\")|(\'[^\"]*\'))\\][ \\n\\r]*=[^\"]*");
	QRegExp initKeyRegExp = QRegExp("\\[((\"[^\"]*\")|(\'[^\"]*\'))\\]");
	QRegExp modInitRegExp = QRegExp("\\$?[ \\n\\r]*(mods)[ \\n\\r]*\\[((\"[^\"]*\")|(\'[^\"]*\'))\\][ \\n\\r]*=[^\"]*");
	QRegExp filterInitRegExp = QRegExp("\\$?[ \\n\\r]*(filters)[ \\n\\r]*\\[((\"[^\"]*\")|(\'[^\"]*\'))\\][ \\n\\r]*=[^\"]*");
	QRegExp modTagsRegExp = QRegExp("\\$?[ \\n\\r]*(mod_tags)[ \\n\\r]*\\[\"[^\"]*\"\\][ \\n\\r]*=[^\"]*");
	QRegExp bracesRegExp = QRegExp("\\{[^\\{\\}]*\\}");
	QStringList modsFolders = dir.entryList(dirFilters);

	QMap<QString, QString> initMap;
	QString prioryModName, prioryModNameKey, tmp, initKey, initValue, out;
	ModInfo modInfo;
	bool isModTagsFounded;
	QFile file;

	int oldDatabaseSize = model_->databaseSize();
	int indexInDatabase, i;

	while (!modsFolders.isEmpty()) {
		modInfo.folderName = modsFolders.takeFirst();
		if (currentPath.contains(modsFolderPath + modInfo.folderName)) {
			emit countOfScannedMods(++countOfScannedMods_);
			steamModNameProcessed();
			QApplication::processEvents();
			continue;
		}

		indexInDatabase = -1;
		for (i = 0; i < oldDatabaseSize; ++i)
			if (modInfo.folderName == model_->modFolderName(i)) {
				model_->modInfoRef(i).exists = true;

				if (!model_->isValidName(model_->modName(i)))
					indexInDatabase = i;
				else
					modInfo.folderName.clear();

				break;
			}

		if (modInfo.folderName.isEmpty()) {
			emit countOfScannedMods(++countOfScannedMods_);
			QApplication::processEvents();
			continue;
		}

		initMap.clear();
		prioryModName.clear();
		prioryModNameKey.clear();
		isModTagsFounded = false;
		dir.setPath(modsFolderPath + modInfo.folderName);

		QDirIterator it(dir.path(), extensionFilter, fileFilters, QDirIterator::Subdirectories);
		while (it.hasNext()) {
			file.setFileName(it.next());
			file.open(QFile::ReadOnly);
			//'QFile::canReadLine()' may return false in some cases, so check the end of the file with 'QFile::atEnd()'
			while (!file.atEnd()) {
				tmp = file.readLine();
				if (!tmp.contains(QRegExp("(\\[|\\{)[^\\]\\}]*#[^\\]\\}]*(\\]|\\})")))
					tmp.remove(QRegExp("#.*$"));	//Removing Python comments
				isModTagsFounded = tmp.contains(modTagsRegExp);
				if (tmp.contains(initRegExp)) {
					initKeyRegExp.indexIn(tmp);
					initKey = initKeyRegExp.cap();
					initKey.remove(QRegExp("(^[^\"']*(\"|'))|((\"|')[^\"']*$)"));

					initValue = tmp;
					initValue.remove(initRegExp);
					initValue.remove(QRegExp("(^[^\"']*(\"|'))|((\"|')[^\"']*$)"));
					while (initValue.contains(bracesRegExp))
						initValue.replace(bracesRegExp, "");
					if (!initValue.isEmpty()) {
						if (tmp.contains(filterInitRegExp))
							initValue.append(tr(" [filter]"));
						initMap[initKey] = initValue;

						if (isModTagsFounded && tmp.contains(modInitRegExp)) {
							prioryModName = initValue;
							prioryModNameKey = initKey;
						}
					}
				}
			}
			isModTagsFounded = false;
			file.close();
		}

		if (!prioryModName.isEmpty())
			initMap[prioryModNameKey] = prioryModName;

		i = 1;
		modInfo.name.clear();
		out = '/' + QString::number(initMap.count()) + "]: ";
		if (initMap.isEmpty()) {
			modInfo.name = tr("WARNING: couldn't get the name of the mod. Set the name manually.");
		}
		else {
			if (initMap.count() == 1) {
				modInfo.name = initMap[initKey];
			}
			else foreach (QString initKey, initMap.keys()) {
				if (modInfo.name.isEmpty()) {
					modInfo.name = "[1" + out + initMap[initKey];
				}
				else {
					modInfo.name.append("\n[" + QString::number(i) + out + initMap[initKey]);
				}
				++i;
			}
		}

		if (indexInDatabase == -1)
			model_->appendDatabase(modInfo);
		else
			model_->modInfoRef(indexInDatabase).name = modInfo.name;

		emit countOfScannedMods(++countOfScannedMods_);
		QApplication::processEvents();
	}
}
