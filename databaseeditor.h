#ifndef DATABASEEDITOR_H
#define DATABASEEDITOR_H

#include <QItemSelection>
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
	void removeSelectedMod();
	void saveSelectedModInfo();
	void show();
	void showSelectedModInfo();

protected:
	void changeEvent(QEvent *event);

protected slots:
	void setModsDisplayMode(const int &mode);
	void adjustRow(const QModelIndex &index);

private:
	Ui::DatabaseEditor *ui;

	DatabaseModel *model_ = nullptr;
};

#endif // DATABASEEDITOR_H
