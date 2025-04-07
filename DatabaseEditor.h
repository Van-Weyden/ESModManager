#pragma once

#include <QWidget>

class ModDatabaseModel;

namespace Ui {
    class DatabaseEditor;
}

class DatabaseEditor : public QWidget
{
    Q_OBJECT

public:
    explicit DatabaseEditor(QWidget *parent = nullptr);
    ~DatabaseEditor();

    void checkModsDisplay();
    void hideAllRows();
    void setModel(ModDatabaseModel *model, const int columnIndex = 0);
    void setModsDisplay(const bool modlistOnly = true);

public slots:
    void filterModsDisplay(const QString &str);
    void removeSelectedMod();
    void show();
    void showSelectedModInfo();
    void eraseDatabase();

protected:
    void changeEvent(QEvent *event);

private slots:
    void adjustRow(const QModelIndex &index);
    void setModsDisplayMode(const int mode);

signals:
    void openModFolder(const QModelIndex &modIndex);

private:
    Ui::DatabaseEditor *ui;

    ModDatabaseModel *m_model = nullptr;
};
