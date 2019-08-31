#ifndef DATABASEEDITOR_H
#define DATABASEEDITOR_H

#include <QWidget>

class DatabaseModel;

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
	void setModel(DatabaseModel *&model, const int &columnIndex = 0);
	void setModsDisplay(const bool &modlistOnly = true);

public slots:
	void filterModsDisplay(const QString &str);
	void removeSelectedMod();
	void saveSelectedModInfo();
	void show();
	void showSelectedModInfo();

protected:
	void changeEvent(QEvent *event);

private slots:
	void adjustRow(const QModelIndex &index);
	void openCurrentModFolder();
	void openWorkshopPage();
	void setModsDisplayMode(const int &mode);

signals:
	void openModFolder(const int &modIndex);

private:
	Ui::DatabaseEditor *ui;

	DatabaseModel *model_ = nullptr;
};

#endif // DATABASEEDITOR_H
