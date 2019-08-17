#include <QCollator>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QTranslator>

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

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	ui->engLangButton->setIcon(QIcon(":/images/Flag-United-States.ico"));
	ui->rusLangButton->setIcon(QIcon(":/images/Flag-Russia.ico"));
	ui->modlistTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	ui->modlistTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
	ui->modlistTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);

	settings_ = new QSettings(QString("settings.ini"), QSettings::IniFormat, this);
	qtTranslator_ = new QTranslator();
	translator_ = new QTranslator();
	launcherMd5_ = fileChecksum(QDir::currentPath() + "/launcher/ESLauncher.exe", QCryptographicHash::Md5);
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
		ui->gameFolderLineEdit->setText(gameFolderPath_);
		ui->modsFolderLineEdit->setText(modsFolderPath_);
		ui->tempModsFolderLineEdit->setText(tempModsFolderPath_);
		scanMods();
		fillModlistTable();
	}

	//connections:

	connect(ui->actionGame_folder, SIGNAL(triggered()), this, SLOT(selectGameFolder()));
	connect(ui->actionMods_folder, SIGNAL(triggered()), this, SLOT(selectModsFolder()));
	connect(ui->actionTemp_mods_folder, SIGNAL(triggered()), this, SLOT(selectTempModsFolder()));
	connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAboutInfo()));

	connect(ui->eraseDatabaseButton, SIGNAL(clicked()), this, SLOT(eraseDatabase()));

	connect(ui->engLangButton, SIGNAL(clicked()), this, SLOT(setEnglishLanguage()));
	connect(ui->rusLangButton, SIGNAL(clicked()), this, SLOT(setRussianLanguage()));

	connect(ui->disableAllButton, SIGNAL(clicked()), this, SLOT(disableAllMods()));
	connect(ui->enableAllButton, SIGNAL(clicked()), this, SLOT(enableAllMods()));
	connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(refreshModlist()));
	connect(ui->runButton, SIGNAL(clicked()), this, SLOT(runGame()));

	connect(ui->modlistTableWidget, SIGNAL(itemChanged(QTableWidgetItem *)),
			this, SLOT(modInfoChanged(QTableWidgetItem *)));
	connect(ui->modlistTableWidget, SIGNAL(cellChanged(int,int)),
			ui->modlistTableWidget, SLOT(resizeRowsToContents()));
	connect(ui->modlistTableWidget->horizontalHeader(), SIGNAL(sectionResized(int,int,int)),
			ui->modlistTableWidget, SLOT(resizeRowsToContents()));

	this->adjustSize();
}

MainWindow::~MainWindow()
{
	//Сортировка имён, чтобы порядок сответвовал тому, что в проводнике
	QCollator collator;
	collator.setNumericMode(true);
	std::sort(database_.begin(), database_.end(), collator);

	saveSettings();

	QApplication::removeTranslator(translator_);
	QApplication::removeTranslator(qtTranslator_);
	delete translator_;
	delete qtTranslator_;
	delete settings_;

	delete ui;
}

void MainWindow::fillModlistTable()
{
	ui->modlistTableWidget->blockSignals(true);
	ui->modlistTableWidget->setRowCount(0);

	for (int indexOfLastRow = 0; indexOfLastRow < modlist_.size(); indexOfLastRow++) {
		ui->modlistTableWidget->insertRow(indexOfLastRow);
		ui->modlistTableWidget->setItem(indexOfLastRow, 0, new QTableWidgetItem);
		ui->modlistTableWidget->setItem(indexOfLastRow, 1, new QTableWidgetItem);
//		ui->modlistTableWidget->item(indexOfLastRow, 0)->setText(database_.at(modlist_.at(indexOfLastRow)).name);
		ui->modlistTableWidget->item(indexOfLastRow, 0)->setData(Qt::DisplayRole, database_.at(modlist_.at(indexOfLastRow)).name);
		ui->modlistTableWidget->item(indexOfLastRow, 1)->setText(database_.at(modlist_.at(indexOfLastRow)).folderName);
		ui->modlistTableWidget->item(indexOfLastRow, 0)->setFlags(Qt::ItemFlag::ItemIsEnabled |
																  Qt::ItemFlag::ItemIsEditable |
																  Qt::ItemFlag::ItemIsSelectable |
																  Qt::ItemFlag::ItemIsUserCheckable);
		if (database_.at(modlist_.at(indexOfLastRow)).enabled)
			ui->modlistTableWidget->item(indexOfLastRow, 0)->setCheckState(Qt::CheckState::Checked);
		else
			ui->modlistTableWidget->item(indexOfLastRow, 0)->setCheckState(Qt::CheckState::Unchecked);
		ui->modlistTableWidget->item(indexOfLastRow, 1)->setFlags(Qt::ItemFlag::NoItemFlags);
	}
	ui->modlistTableWidget->blockSignals(false);

	QApplication::processEvents();
	ui->modlistTableWidget->resizeRowsToContents();
}

void MainWindow::loadDatabase()
{
	QFile file;
	//Подчищаем старую версию БД
	if (QFile::exists("mods_database.dat")) {
		file.setFileName("mods_database.dat");

		file.open(QFile::ReadOnly);
		QString name, folderName, enabled;
		ModInfo modInfo;
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
				database_.append(modInfo);
			}
		}
		while (file.canReadLine());
		file.close();

		file.remove();
	}
	else if (QFile::exists("mods_database.json")) {
		file.setFileName("mods_database.json");
		file.open(QFile::ReadOnly);

		QJsonDocument database;
		QJsonArray mods;
		QJsonObject mod;
		ModInfo modInfo;

		//QJsonParseError err;
		database = QJsonDocument::fromJson(file.readAll()/*, &err*/);
		file.close();
		mods = database.array();

		for (int i = 0; i < mods.size(); i++) {
			mod = mods.at(i).toObject();
			modInfo.name = mod["Name"].toString();
			modInfo.folderName = mod["Folder name"].toString();
			modInfo.enabled = mod["Is enabled"].toBool();

			database_.append(modInfo);
		}
	}
}

void MainWindow::saveDatabase() const
{
	QJsonDocument database;
	QJsonArray mods;
	QJsonObject mod;

	for (int i = 0; i < database_.size(); ++i) {
		mod["Name"] = database_.at(i).name;
		mod["Folder name"] = database_.at(i).folderName;
		mod["Is enabled"] = database_.at(i).enabled;
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
	modlist_.clear();

	scanMods(modsFolderPath_);
	scanMods(tempModsFolderPath_);

	//Сортировка имён, чтобы порядок сответвовал тому, что в проводнике
	QCollator collator;
	collator.setNumericMode(true);

	std::sort(modlist_.begin(), modlist_.end(),
			[&](int i, int j) {return collator(database_.at(i), database_.at(j));});
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
	ui->modlistTableWidget->blockSignals(true);
	for (int i = 0; i < modlist_.size(); ++i) {
		database_[modlist_.at(i)].enabled = false;
		ui->modlistTableWidget->item(i,0)->setCheckState(Qt::CheckState::Unchecked);
	}
	ui->modlistTableWidget->blockSignals(false);
}

void MainWindow::enableAllMods()
{
	ui->modlistTableWidget->blockSignals(true);
	for (int i = 0; i < modlist_.size(); ++i) {
		database_[modlist_.at(i)].enabled = true;
		ui->modlistTableWidget->item(i,0)->setCheckState(Qt::CheckState::Checked);
	}
	ui->modlistTableWidget->blockSignals(false);
}

void MainWindow::eraseDatabase()
{
	QFile::remove("mods_database.json");
	database_.clear();
	refreshModlist();
}

void MainWindow::modInfoChanged(QTableWidgetItem *tableItem)
{
	database_[modlist_.at(tableItem->row())].name = tableItem->text();
	database_[modlist_.at(tableItem->row())].enabled = (tableItem->checkState() == Qt::CheckState::Checked);
}

void MainWindow::refreshModlist()
{
	ui->modlistTableWidget->blockSignals(true);
	ui->modlistTableWidget->setRowCount(0);
	ui->modlistTableWidget->blockSignals(false);
	QApplication::processEvents();
	scanMods();
	fillModlistTable();
}

void MainWindow::runGame()
{
	if (!checkGameMd5(gameFolderPath_))
		return;

	this->hide();
	QDir dir(tempModsFolderPath_);
	if (!dir.exists())
		dir.mkdir(tempModsFolderPath_);

	for (int i = 0; i < modlist_.size(); ++i) {
		if (!database_.at(modlist_.at(i)).enabled)
			dir.rename(modsFolderPath_ + database_.at(modlist_.at(i)).folderName,
					   tempModsFolderPath_ + database_.at(modlist_.at(i)).folderName);
	}

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
		QFile::rename(gameFolderPath_ + "Everlasting Summer.exe", gameFolderPath_ + "Everlasting Summer (modified).exe");
		QFile::rename(gameFolderPath_ + "Everlasting Summer (origin).exe", gameFolderPath_ + "Everlasting Summer.exe");
	}

	gameLauncher.start();
	gameLauncher.waitForStarted(-1);
	gameLauncher.waitForFinished(-1);

	if (ui->replaceOriginLauncherCheckBox->isChecked()) {
		QFile::rename(gameFolderPath_ + "Everlasting Summer.exe", gameFolderPath_ + "Everlasting Summer (origin).exe");
		if (QFileInfo::exists(gameFolderPath_ + "Everlasting Summer (modified).exe"))
			QFile::rename(gameFolderPath_ + "Everlasting Summer (modified).exe", gameFolderPath_ + "Everlasting Summer.exe");
		else
			QFile::copy("launcher\\ESLauncher.exe", gameFolderPath_ + "Everlasting Summer.exe");

		launcherSettings = QDir::currentPath().replace('/', '\\').toUtf8() + "\\\nESModManager\nfalse";
		launcherSettingsFile.setFileName(gameFolderPath_ + "LaunchedProgram.ini");
		launcherSettingsFile.open(QFile::ReadOnly);
		if (launcherSettingsFile.readAll() != launcherSettings) {
			launcherSettingsFile.close();
			launcherSettingsFile.open(QFile::WriteOnly);
			launcherSettingsFile.write(launcherSettings);
		}
		launcherSettingsFile.close();
	}
	else {
		if (QFileInfo::exists(gameFolderPath_ + "Everlasting Summer (modified).exe")) {
			QFile::remove(gameFolderPath_ + "Everlasting Summer (modified).exe");
			QFile::remove(gameFolderPath_ + "LaunchedProgram.ini");
		}
		else if (QFileInfo::exists(gameFolderPath_ + "Everlasting Summer (origin).exe")) {
			QFile::remove(gameFolderPath_ + "Everlasting Summer.exe");
			QFile::remove(gameFolderPath_ + "LaunchedProgram.ini");
			QFile::rename(gameFolderPath_ + "Everlasting Summer (origin).exe", gameFolderPath_ + "Everlasting Summer.exe");
		}
	}

	if (ui->moveModsBackCheckBox->isChecked()) {
		for (int i = 0; i < modlist_.size(); ++i) {
			if (!database_.at(modlist_.at(i)).enabled)
				dir.rename(tempModsFolderPath_ + database_.at(modlist_.at(i)).folderName,
						   modsFolderPath_ + database_.at(modlist_.at(i)).folderName);
		}
	}

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

void MainWindow::showAboutInfo()
{
	QMessageBox messageAbout(QMessageBox::NoIcon, tr("About ") + this->windowTitle(), "",
							 QMessageBox::StandardButton::Close, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	messageAbout.setText(
		tr("Everlasting Summer mod manager v.1.0.\n"
		   "Author: Slavyan\n\n"
		   "This program is used to 'fix' conflicts of mods and speed up the launch of the game. "
		   "Before launching the game, all unselected mods are moved to another folder, so the game engine will not load them."));
	messageAbout.setInformativeText(tr("You can leave your questions/suggestions") +
									" <a href='https://steamcommunity.com/sharedfiles/itemedittext/?id=1826799366'>" + tr("here") + "</a>.");
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

//private:

bool MainWindow::checkGameMd5(const QString &folderPath)
{
	gameMd5_ = fileChecksum(folderPath + "Everlasting Summer.exe", QCryptographicHash::Md5);

	if (gameMd5_ == launcherMd5_ || gameMd5_.isNull())
		gameMd5_ = fileChecksum(folderPath + "Everlasting Summer (origin).exe", QCryptographicHash::Md5);

	if (gameMd5_.isNull()) {
		QMessageBox::critical(this, tr("Wrong game .exe"), tr("Game folder doesn't contains origin \n 'Everlasting Summer.exe' \n and there is no file \n 'Everlasting Summer (origin).exe'!"),
							  QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::NoButton);
		return false;
	}
	return true;
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

	if (settings_->contains("General/sGameFolder")) {
		value = settings_->value("General/sGameFolder");
		if (value != invalidValue) {
			gameFolderPath_ = value.toString();
			//Исправляем старый баг, когда в путь попадал "Everlasting Summer.exe"
			if (gameFolderPath_.contains("Everlasting Summer.exe"))
				gameFolderPath_.remove("Everlasting Summer.exe");
		}
	}

	if (settings_->contains("General/sModsFolder")) {
		value = settings_->value("General/sModsFolder");
		if (value != invalidValue) {
			modsFolderPath_ = value.toString();
			//Исправляем старый баг, когда в путь попадал "Everlasting Summer.exe"
			if (modsFolderPath_.contains("Everlasting Summer.exe"))
				modsFolderPath_.remove("Everlasting Summer.exe");
		}
	}

	if (settings_->contains("General/sTempModsFolder")) {
		value = settings_->value("General/sTempModsFolder");
		if (value != invalidValue) {
			tempModsFolderPath_ = value.toString();
			//Исправляем старый баг, когда в путь попадал "Everlasting Summer.exe"
			if (tempModsFolderPath_.contains("Everlasting Summer.exe"))
				tempModsFolderPath_.remove("Everlasting Summer.exe");
		}
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

	if (settings_->contains("General/bReplaceOriginLauncher")) {
		value = settings_->value("General/bReplaceOriginLauncher");
		if (value != invalidValue)
			ui->replaceOriginLauncherCheckBox->setChecked(value.toBool());
	}

	loadDatabase();
}

void MainWindow::saveSettings() const
{
	settings_->clear();

	settings_->setValue("General/sLang", lang_);

	settings_->setValue("General/sGameFolder", gameFolderPath_);

	settings_->setValue("General/sModsFolder", modsFolderPath_);

	settings_->setValue("General/sTempModsFolder", tempModsFolderPath_);

	settings_->setValue("General/bAutoexit", ui->autoexitCheckBox->isChecked());

	settings_->setValue("General/bMoveModsBack", ui->moveModsBackCheckBox->isChecked());
	if (ui->moveModsBackCheckBox->isChecked()) {
		QDir dir(tempModsFolderPath_);
		if (dir.exists()) {
			for (int i = 0; i < modlist_.size(); ++i) {
				if (QDir(tempModsFolderPath_ + database_.at(modlist_.at(i)).folderName).exists())
					dir.rename(tempModsFolderPath_ + database_.at(modlist_.at(i)).folderName,
							   modsFolderPath_ + database_.at(modlist_.at(i)).folderName);
			}
		}
	}

	settings_->setValue("General/bReplaceOriginLauncher", ui->replaceOriginLauncherCheckBox->isChecked());
	if (ui->replaceOriginLauncherCheckBox->isChecked()) {
		QFile::rename(gameFolderPath_ + "Everlasting Summer.exe", gameFolderPath_ + "Everlasting Summer (origin).exe");
		if (QFileInfo::exists(gameFolderPath_ + "Everlasting Summer (modified).exe")) {
			QFile::rename(gameFolderPath_ + "Everlasting Summer (modified).exe", gameFolderPath_ + "Everlasting Summer.exe");
		}
		else {
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
		if (QFileInfo::exists(gameFolderPath_ + "Everlasting Summer (modified).exe")) {
			QFile::remove(gameFolderPath_ + "Everlasting Summer (modified).exe");
			QFile::remove(gameFolderPath_ + "LaunchedProgram.ini");
		}
		else if (QFileInfo::exists(gameFolderPath_ + "Everlasting Summer (origin).exe")) {
			QFile::remove(gameFolderPath_ + "Everlasting Summer.exe");
			QFile::remove(gameFolderPath_ + "LaunchedProgram.ini");
			QFile::rename(gameFolderPath_ + "Everlasting Summer (origin).exe", gameFolderPath_ + "Everlasting Summer.exe");
		}
	}

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

	int oldDatabaseSize = database_.size();
	int indexInDatabase, i;
	while (!modsFolders.isEmpty()) {
		modInfo.folderName = modsFolders.takeFirst();
		if (currentPath.contains(modsFolderPath + modInfo.folderName))
			continue;

		indexInDatabase = -1;
		for (i = 0; i < oldDatabaseSize; ++i)
			if (modInfo.folderName == database_.at(i).folderName) {
				if (database_.at(i).name.isEmpty() ||
					database_.at(i).name.contains(tr("WARNING: unknown mod name. Set the name manually."))) {
						indexInDatabase = i;
				}
				else {
					modInfo.folderName.clear();
					modlist_.append(i);
				}
				break;
			}

		if (modInfo.folderName.isEmpty())
			continue;

		initMap.clear();
		prioryModName.clear();
		prioryModNameKey.clear();
		isModTagsFounded = false;
		dir.setPath(modsFolderPath + modInfo.folderName);

		QDirIterator it(dir.path(), extensionFilter, fileFilters, QDirIterator::Subdirectories);
		while (it.hasNext()) {
			file.setFileName(it.next());
			file.open(QFile::ReadOnly);
			//QFile::canReadLine() может выдавать false в некоторых случаях, поэтому проверяем конец файла через QFile::atEnd()
			while (!file.atEnd()) {
				tmp = file.readLine();
				if (!tmp.contains(QRegExp("(\\[|\\{)[^\\]\\}]*#[^\\]\\}]*(\\]|\\})")))
					tmp.remove(QRegExp("#.*$"));	//Delete Python comments
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
			modInfo.name = tr("WARNING: unknown mod name. Set the name manually.");
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

		if (indexInDatabase == -1) {
			modlist_.append(database_.size());
			database_.append(modInfo);
		}
		else {
			modlist_.append(indexInDatabase);
			database_[indexInDatabase].name = modInfo.name;
		}
	}
}
