#include <QDesktopServices>
#include <QUrl>

#include "mvc/ModDatabaseModel.h"

#include "DatabaseEditor.h"
#include "ui_DatabaseEditor.h"

//public:

DatabaseEditor::DatabaseEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DatabaseEditor)
{
    ui->setupUi(this);

    connect(ui->modInfo, &ModInfoWidget::openFolder, this, &DatabaseEditor::openModFolder);
    connect(ui->removeModPushButton, &QPushButton::clicked, this, &DatabaseEditor::removeSelectedMod);

    connect(ui->searchLineEdit, &QLineEdit::textChanged, this, &DatabaseEditor::filterModsDisplay);
    connect(ui->clearSearchPushButton, &QPushButton::clicked, ui->searchLineEdit, &QLineEdit::clear);
    connect(ui->eraseDatabaseButton, &QPushButton::clicked, this, &DatabaseEditor::eraseDatabase);
    connect(ui->showAllModsCheckBox, &QCheckBox::stateChanged, this, &DatabaseEditor::setModsDisplayMode);

    connect(ui->databaseView->horizontalHeader(), &QHeaderView::geometriesChanged,
            ui->databaseView, &QTableView::resizeRowsToContents);
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
    if (m_model == nullptr) {
        return;
    }

    ui->searchLineEdit->blockSignals(true);
    ui->searchLineEdit->clear();
    ui->searchLineEdit->blockSignals(false);

    int rowCount = m_model->size();
    for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
        ui->databaseView->setRowHidden(rowIndex, true);
    }
}

void DatabaseEditor::setModel(ModDatabaseModel *model, const int columnIndex)
{
    if (model == m_model || model == nullptr) {
        return;
    }

    if (m_model != nullptr) {
        disconnect(ui->databaseView->selectionModel(), &QItemSelectionModel::currentChanged,
                   this, &DatabaseEditor::showSelectedModInfo);
        disconnect(m_model, &ModDatabaseModel::dataChanged, this, &DatabaseEditor::adjustRow);
    }

    m_model = model;
    ui->modInfo->setModel(m_model);
    ui->databaseView->setModel(m_model);
    for (int i = 0; i < m_model->columnCount(); ++i) {
        ui->databaseView->hideColumn(i);
    }
    ui->databaseView->showColumn(columnIndex);
    ui->databaseView->horizontalHeader()->setSectionResizeMode(columnIndex, QHeaderView::Stretch);

    setModsDisplay(!ui->showAllModsCheckBox->isChecked());
    connect(ui->databaseView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &DatabaseEditor::showSelectedModInfo);
    connect(m_model, &ModDatabaseModel::dataChanged, this, &DatabaseEditor::adjustRow);
}

void DatabaseEditor::setModsDisplay(const bool modlistOnly)
{
    if (m_model == nullptr) {
        return;
    }

    hideAllRows();
    if (this->isVisible()) {
        QApplication::processEvents();
    }

    int databaseSize = m_model->size();
    if (modlistOnly) {
        for (int row = 0; row < databaseSize; ++row) {
            if (m_model->modInfo(row).exists()) {
                ui->databaseView->setRowHidden(row, false);
            }
        }
    } else {
        for (int modIndex = 0; modIndex < databaseSize; ++modIndex) {
            ui->databaseView->setRowHidden(modIndex, false);
        }
    }
}

//public slots:

void DatabaseEditor::filterModsDisplay(const QString &str)
{
    if (m_model == nullptr) {
        return;
    }

    int rowCount = m_model->size();
    for (int row = 0; row < rowCount; ++row) {
        if (ui->showAllModsCheckBox->isChecked() || m_model->modInfo(row).exists()) {
            ui->databaseView->setRowHidden(row, false);
            QString modName = m_model->modInfo(row).displayedName();
            if (!modName.contains(str, Qt::CaseSensitivity::CaseInsensitive)) {
                ui->databaseView->setRowHidden(row, true);
            }
        }
    }
}

void DatabaseEditor::removeSelectedMod()
{
    QModelIndex modIndex = ui->databaseView->selectionModel()->currentIndex();
    if (!modIndex.isValid()) {
        return;
    }

    m_model->removeItem(modIndex);
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
    if (modIndex.isValid()) {
        ui->modInfo->setIndex(modIndex);
    } else {
        ui->modInfo->clear();
    }
    ui->modInfo->setEnabled(modIndex.isValid());
}

void DatabaseEditor::eraseDatabase()
{
    m_model->reset();
}

//protected:

void DatabaseEditor::changeEvent(QEvent *event)
{
   if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
   } else {
        QWidget::changeEvent(event);
   }
}

//private slots:

void DatabaseEditor::adjustRow(const QModelIndex &index)
{
    if (index.isValid() && index.row() < m_model->size()) {
        ui->databaseView->resizeRowToContents(index.row());
    }
}

void DatabaseEditor::setModsDisplayMode(const int mode)
{
    if (mode == Qt::CheckState::Checked) {
        setModsDisplay(false);
    } else if (mode == Qt::CheckState::Unchecked) {
        setModsDisplay(true);
    }
}
