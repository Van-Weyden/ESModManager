#include <QDesktopServices>
#include <QUrl>

#include "databasemodel.h"

#include "databaseeditor.h"
#include "ui_databaseeditor.h"

//public:

DatabaseEditor::DatabaseEditor(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::DatabaseEditor)
{
	ui->setupUi(this);

	connect(ui->openModFolderPushButton, SIGNAL(clicked()),
			this, SLOT(openCurrentModFolder()));
	connect(ui->openWorkshopPushButton, SIGNAL(clicked()),
			this, SLOT(openWorkshopPage()));
	connect(ui->saveModInfoPushButton, SIGNAL(clicked()),
			this, SLOT(saveSelectedModInfo()));
	connect(ui->removeModPushButton, SIGNAL(clicked()),
			this, SLOT(removeSelectedMod()));

	connect(ui->clearSearchPushButton, SIGNAL(clicked()),
			ui->searchLineEdit, SLOT(clear()));
	connect(ui->searchLineEdit, SIGNAL(textChanged(const QString &)),
			this, SLOT(filterModsDisplay(const QString &)));

	connect(ui->showAllModsCheckBox, SIGNAL(stateChanged(int)),
			this, SLOT(setModsDisplayMode(const int &)));

	connect(ui->databaseView->horizontalHeader(), SIGNAL(geometriesChanged()),
			ui->databaseView, SLOT(resizeRowsToContents()));
}

DatabaseEditor::~DatabaseEditor()
{
	delete ui;
}

void DatabaseEditor::checkModsDisplay()
{
	setModsDisplay(!ui->showAllModsCheckBox->isChecked());
}

void DatabaseEditor::hideAllRows()
{
	if (model_ == nullptr)
		return;

	ui->searchLineEdit->blockSignals(true);
	ui->searchLineEdit->clear();
	ui->searchLineEdit->blockSignals(false);

	int rowCount = model_->databaseSize();
	for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex)
		ui->databaseView->setRowHidden(rowIndex, true);
}

void DatabaseEditor::setModel(DatabaseModel *&model, const int &columnIndex)
{
	if (model == model_ || model == nullptr)
		return;

	if (model_ != nullptr) {
		disconnect(ui->databaseView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
				this, SLOT(showSelectedModInfo(/*const QItemSelection &*/)));
		disconnect(model_, SIGNAL(dataChanged(QModelIndex &, QModelIndex &, QVector<int>)),
				this, SLOT(adjustRow(QModelIndex &)));
	}

	model_ = model;
	ui->databaseView->setModel(model_);
	for (int i = 0; i < model_->columnCount(); ++i)
		ui->databaseView->hideColumn(i);
	ui->databaseView->showColumn(columnIndex);
	ui->databaseView->horizontalHeader()->setSectionResizeMode(columnIndex, QHeaderView::Stretch);

	setModsDisplay(!ui->showAllModsCheckBox->isChecked());
	connect(ui->databaseView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
			this, SLOT(showSelectedModInfo(/*const QItemSelection &*/)));
	connect(model_, SIGNAL(dataChanged(QModelIndex, QModelIndex)),
			this, SLOT(adjustRow(QModelIndex)));
}

void DatabaseEditor::setModsDisplay(const bool &modlistOnly)
{
	if (model_ == nullptr)
		return;

	hideAllRows();
	if (this->isVisible())
		QApplication::processEvents();

	int databaseSize = model_->databaseSize();
	if (modlistOnly) {
		for (int modIndex = 0; modIndex < databaseSize; ++modIndex)
			if (model_->modIsExists(modIndex))
				ui->databaseView->setRowHidden(modIndex, false);
	}
	else {
		for (int modIndex = 0; modIndex < databaseSize; ++modIndex)
			ui->databaseView->setRowHidden(modIndex, false);
	}
}

//public slots:

void DatabaseEditor::filterModsDisplay(const QString &str)
{
	if (model_ == nullptr)
		return;

	int rowCount = model_->databaseSize();
	for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex)
		if (ui->showAllModsCheckBox->isChecked() || model_->modIsExists(rowIndex)) {
			ui->databaseView->setRowHidden(rowIndex, false);
			if (!model_->data(model_->index(rowIndex)).toString().contains(str, Qt::CaseSensitivity::CaseInsensitive))
				ui->databaseView->setRowHidden(rowIndex, true);
		}
}

void DatabaseEditor::removeSelectedMod()
{
	QModelIndex modIndex = ui->databaseView->selectionModel()->currentIndex();
	if (!modIndex.isValid())
		return;

	model_->removeFromDatabase(modIndex.row());
}

void DatabaseEditor::saveSelectedModInfo()
{
	QModelIndex modIndex = ui->databaseView->selectionModel()->currentIndex();
	if (!modIndex.isValid())
		return;

	ModInfo modInfo = model_->modInfo(modIndex.row());
	QString text;

	text = ui->modNameLineEdit->text();
	if (!text.isEmpty() && text != modInfo.name) {
		model_->modInfoRef(modIndex.row()).name = text;
		model_->updateRow(modIndex);
	}

	text = ui->steamModNameLineEdit->text();
	if (!text.isEmpty() && text != modInfo.steamName) {
		model_->modInfoRef(modIndex.row()).steamName = text;
		model_->updateRow(modIndex);
	}
}

void DatabaseEditor::show()
{
	setModsDisplay(!ui->showAllModsCheckBox->isChecked());
	showSelectedModInfo();
	QWidget::show();
}

void DatabaseEditor::showSelectedModInfo()
{
	QModelIndex modIndex = ui->databaseView->selectionModel()->currentIndex();
	if(modIndex.isValid()) {
		ModInfo modInfo = model_->modInfo(modIndex.row());

		ui->modNameLineEdit->setText(modInfo.name);
		ui->modFolderNameLineEdit->setText(modInfo.folderName);
		ui->steamModNameLineEdit->setText(modInfo.steamName);

		ui->saveModInfoPushButton->setEnabled(true);
		ui->removeModPushButton->setEnabled(ui->showAllModsCheckBox->isChecked() && !modInfo.exists);
		ui->openModFolderPushButton->setEnabled(!ui->showAllModsCheckBox->isChecked() || modInfo.exists);
		ui->openWorkshopPushButton->setEnabled(ModInfo::isSteamId(modInfo.folderName));
	}
	else {
		ui->saveModInfoPushButton->setEnabled(false);
		ui->openModFolderPushButton->setEnabled(false);
		ui->openWorkshopPushButton->setEnabled(false);
		ui->removeModPushButton->setEnabled(false);
		ui->modNameLineEdit->clear();
		ui->modFolderNameLineEdit->clear();
		ui->steamModNameLineEdit->clear();
	}
}

//protected:

void DatabaseEditor::changeEvent(QEvent *event)
{
   if (event->type() == QEvent::LanguageChange)
		ui->retranslateUi(this);
   else
		QWidget::changeEvent(event);
}

//private slots:

void DatabaseEditor::adjustRow(const QModelIndex &index)
{
	if (index.isValid() && index.row() < model_->databaseSize())
		ui->databaseView->resizeRowToContents(index.row());
}

void DatabaseEditor::openCurrentModFolder()
{
	emit openModFolder(ui->databaseView->selectionModel()->currentIndex().row());
}

void DatabaseEditor::openWorkshopPage()
{
	if (ui->openWorkshopWithSteamCheckBox->isChecked())
		QDesktopServices::openUrl(QUrl("steam://url/CommunityFilePage/" +
									   model_->modFolderName(ui->databaseView->selectionModel()->currentIndex().row())));
	else
		QDesktopServices::openUrl(QUrl("https://steamcommunity.com/sharedfiles/filedetails/?id=" +
									   model_->modFolderName(ui->databaseView->selectionModel()->currentIndex().row())));
}

void DatabaseEditor::setModsDisplayMode(const int &mode)
{
	if (mode == Qt::CheckState::Checked)
		setModsDisplay(false);
	else if (mode == Qt::CheckState::Unchecked)
		setModsDisplay(true);
}
