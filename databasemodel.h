#ifndef MODSDATABASEMODEL_H
#define MODSDATABASEMODEL_H

#include <QAbstractListModel>

#include "modinfo.h"

class QThread;
class SteamRequester;

class DatabaseModel : public QAbstractListModel
{
	Q_OBJECT

public:
	DatabaseModel();
	~DatabaseModel();

	inline bool isValidName(const QString &name) const {return !(name.isEmpty() ||
																 name.contains(tr("WARNING: unknown mod name. Set the name manually.")) ||
																 name.contains(tr("WARNING: couldn't get the name of the mod. Set the name manually."))
																 /* || name == tr("Waiting for Steam mod name response...")*/
																 );}

	inline void appendDatabase(const ModInfo &modInfo)
	{
		emit beginInsertRows(QModelIndex(), database_.size(), database_.size());
		database_.append(modInfo);
		if (database_.last().steamName.isEmpty())
			database_.last().steamName = tr("WARNING: couldn't get the name of the mod. Set the name manually.");
			//database_.last().steamName = tr("Waiting for Steam mod name response...");

		emit endInsertRows();
	}
	inline void clearDatabase() {beginResetModel(); database_.clear(); endResetModel();}
	inline int columnCount(const QModelIndex &/*parent*/ = QModelIndex()) const {return 2;}
	inline int databaseSize() const {return database_.size();}
	inline const QString &modFolderName(const int &index) const {return database_.at(index).folderName;}
	inline const ModInfo &modInfo(const int &index) const {return database_.at(index);}
	inline ModInfo &modInfoRef(const int &index) {return database_[index];}
	inline const bool &modIsEnabled(const int &index) const {return database_.at(index).enabled;}
	inline const bool &modIsExists(const int &index) const {return database_.at(index).exists;}
	inline const QString &modName(const int &index) const {return database_.at(index).name;}
	inline const QString &modSteamName(const int &index) const {return database_.at(index).steamName;}

	inline int rowCount(const QModelIndex &/*parent = QModelIndex()*/) const {return database_.size();}

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	void removeFromDatabase(const int index);
	void setCompleteModNames(const bool &enabled = true);
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	void setModsExistsState(const bool &isExists);
	void setUsingSteamModNames(const bool &use = true);
	void sortDatabase();
	inline void updateRow(const int &index) {emit dataChanged(this->index(index), this->index(index));}
	inline void updateRow(const QModelIndex &index) {emit dataChanged(index, index);}

protected slots:
	void setCompleteModNames(const int &mode);
	void setUsingSteamModNames(const int &mode);

signals:
	void modCheckStateChanged(const int &rowIndex, const bool &state);

private:
	QVector<ModInfo> database_;

	bool completeModNames_ = true;
	bool useSteamModNames_ = true;
};

#endif // MODSDATABASEMODEL_H
