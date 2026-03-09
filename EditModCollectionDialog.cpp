#include <QDebug>
#include <QMessageBox>
#include <QStringListModel>

#include "mvc/ModDatabaseModel.h"
#include "mvc/proxyModels/NameFilterProxyModel.h"

#include "EditModCollectionDialog.h"
#include "ui_EditModCollectionDialog.h"

EditModCollectionDialog::EditModCollectionDialog(ModDatabaseModel *model, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EditModCollectionDialog)
    , m_databaseModel(model)
    , m_databaseModListModel(new NameFilterProxyModel(this, model))
    , m_collectionListModel(new NameFilterProxyModel(this, model))
{
    ui->setupUi(this);
    m_applyButton = ui->buttonBox->button(QDialogButtonBox::StandardButton::Apply);

    m_databaseModListModel->addFilter(
        [this](QAbstractItemModel */*model*/, const QModelIndex &sourceModelIndex)
        {
            if (m_databaseModel->isCollection(sourceModelIndex)) {
                return true;
            }

            return !m_collectionMods.contains(&m_databaseModel->modInfoRef(sourceModelIndex));
        }
    );
    ui->listView_databaseMods->setModel(m_databaseModListModel);
    ui->listView_databaseMods->setRootIndex(m_databaseModListModel->index(0, 0));

    m_collectionListModel->addFilter(
        [this](QAbstractItemModel */*model*/, const QModelIndex &sourceModelIndex)
        {
            if (m_databaseModel->isCollection(sourceModelIndex)) {
                return true;
            }

            return m_collectionMods.contains(&m_databaseModel->modInfoRef(sourceModelIndex));
        }
    );
    ui->listView_collectionMods->setModel(m_collectionListModel);
    ui->listView_collectionMods->setRootIndex(m_collectionListModel->index(0, 0));

    // TODO: implement
//    connect(ui->toolButton_setLocked, &QToolButton::clicked, this, &EditModCollectionDialog::lockSelectedMods);
//    connect(ui->toolButton_setUnlocked, &QToolButton::clicked, this, &EditModCollectionDialog::unlockSelectedMods);
//    connect(ui->toolButton_openFolder, &QToolButton::clicked, this, &EditModCollectionDialog::openModFolder);
//    connect(ui->toolButton_openSteamPage, &QToolButton::clicked, this, &EditModCollectionDialog::openModSteamPage);
    connect(ui->toolButton_addMods, &QToolButton::clicked, this, &EditModCollectionDialog::addSelectedMods);
    connect(ui->toolButton_removeMods, &QToolButton::clicked, this, &EditModCollectionDialog::removeSelectedMods);

    connect(m_applyButton, &QPushButton::clicked, this, &EditModCollectionDialog::apply);

    ui->buttonBox->layout()->setSpacing(1);

    // TODO: reject on close?
}

EditModCollectionDialog::~EditModCollectionDialog()
{
    delete ui;
}

void EditModCollectionDialog::setCollection(ModCollection *collection)
{
    m_collection = collection;
}

ModCollection *EditModCollectionDialog::collection() const
{
    return m_collection;
}

void EditModCollectionDialog::open()
{
    onOpen();
    QDialog::open();
}

int EditModCollectionDialog::exec()
{
    onOpen();
    return QDialog::exec();
}

void EditModCollectionDialog::accept()
{
    if (apply()) {
        QDialog::accept();
    }
}

void EditModCollectionDialog::reject()
{
    QDialog::reject();
}

void EditModCollectionDialog::done(int resultCode)
{
    QDialog::done(resultCode);
}

bool EditModCollectionDialog::apply()
{
    if (ui->lineEdit_collectionName->text().isEmpty()) {
        QMessageBox::critical(this, tr("Collection name is not set"), tr("Collection name can't be empty!"));
        return false;
    }

    if (m_collection) {
        m_collection->clear();
    } else {
        m_collection = new ModCollection(ui->lineEdit_collectionName->text());
    }

    for (ModInfo *mod : qAsConst(m_collectionMods)) {
        m_collection->addMod(mod);
    }

    return true;
}

void EditModCollectionDialog::addSelectedMods()
{
    QItemSelectionModel *selectionModel = ui->listView_databaseMods->selectionModel();
    if (!selectionModel) {
        return;
    }

    QItemSelection selection = selectionModel->selection();
    if (selection.isEmpty()) {
        return;
    }

    QModelIndexList indexes = m_databaseModListModel->mapSelectionToSource(selection).indexes();
    if (indexes.isEmpty()) {
        return;
    }

    for (const auto &index : qAsConst(indexes)) {
        m_collectionMods.append(&m_databaseModel->modInfoRef(index));
    }

    m_databaseModListModel->invalidate();
    m_collectionListModel->invalidate();
}

void EditModCollectionDialog::removeSelectedMods()
{
    QItemSelectionModel *selectionModel = ui->listView_collectionMods->selectionModel();
    if (!selectionModel) {
        return;
    }

    QItemSelection selection = selectionModel->selection();
    if (selection.isEmpty()) {
        return;
    }

    QModelIndexList indexes = m_collectionListModel->mapSelectionToSource(selection).indexes();
    if (indexes.isEmpty()) {
        return;
    }

    for (const auto &index : qAsConst(indexes)) {
        m_collectionMods.removeOne(&m_databaseModel->modInfoRef(index));
    }

    m_databaseModListModel->invalidate();
    m_collectionListModel->invalidate();
}

void EditModCollectionDialog::onOpen()
{
    ui->lineEdit_search->clear();
    if (m_collection) {
        ui->lineEdit_collectionName->setText(m_collection->name());
        m_applyButton->setVisible(true);
        m_collectionMods = m_collection->mods();
    } else {
        ui->lineEdit_collectionName->clear();
        m_applyButton->setVisible(false);
        m_collectionMods.clear();
    }
}
