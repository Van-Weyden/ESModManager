#pragma once

#include <QDialog>

class QStringListModel;

class NameFilterProxyModel;
class ModCollection;
class ModDatabaseModel;
class ModInfo;

namespace Ui {
class EditModCollectionDialog;
}

class EditModCollectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditModCollectionDialog(ModDatabaseModel *model, QWidget *parent = nullptr);
    ~EditModCollectionDialog();

    void setCollection(ModCollection *collection);
    ModCollection *collection() const;

public slots:
    void open() override;
    int exec() override;
    void accept() override;
    void reject() override;
    void done(int resultCode) override;

private slots:
    bool apply();
    void addSelectedMods();
    void removeSelectedMods();

private:
    void onOpen();

private:
    Ui::EditModCollectionDialog *ui;
    QPushButton *m_applyButton = nullptr;
    ModDatabaseModel *m_databaseModel = nullptr;
    NameFilterProxyModel *m_databaseModListModel = nullptr;
    NameFilterProxyModel *m_collectionListModel = nullptr;
    ModCollection *m_collection = nullptr;
    QVector<ModInfo *> m_collectionMods;
};

