#ifndef MODSDATABASEMODEL_H
#define MODSDATABASEMODEL_H

#include <QAbstractListModel>

#include "ModInfo.h"

class QThread;
class SteamRequester;

class DatabaseModel : public QAbstractListModel
{
	Q_OBJECT

public:
	DatabaseModel();
	~DatabaseModel() = default;

	bool isNameValid(const QString &name) const;

	void appendDatabase(const ModInfo &modInfo);
	inline void clearDatabase();
	inline int columnCount(const QModelIndex &parent = QModelIndex()) const;
	inline int databaseSize() const;
	inline const QString &modFolderName(const int index) const;
	inline const ModInfo &modInfo(const int index) const;
	inline ModInfo &modInfoRef(const int index);
	inline bool modIsEnabled(const int index) const;
	inline bool modIsExists(const int index) const;
	inline const QString &modName(const int index) const;
	inline const QString &modSteamName(const int index) const;

	inline int rowCount(const QModelIndex &parent = QModelIndex()) const;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	void removeFromDatabase(const int index);
	void setCompleteModNames(const bool enabled = true);
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	void setModsExistsState(const bool isExists);
	void setUsingSteamModNames(const bool use = true);
	void sortDatabase();
	inline void updateRow(const int index);
	inline void updateRow(const QModelIndex &index);

public slots:
	void enableMod(const QModelIndex &index);
	void disableMod(const QModelIndex &index);

protected slots:
	void setCompleteModNames(const int mode);
	void setUsingSteamModNames(const int mode);

signals:
	void modCheckStateChanged(const int rowIndex, const bool state);

private:
	QVector<ModInfo> m_database;

	bool m_completeModNames = true;
	bool m_useSteamModNames = true;
};



//public:

inline void DatabaseModel::clearDatabase()
{
	beginResetModel(); m_database.clear(); endResetModel();
}

inline int DatabaseModel::columnCount(const QModelIndex &/*parent*/) const
{
	return 2;
}

inline int DatabaseModel::databaseSize() const
{
	return m_database.size();
}

inline const QString &DatabaseModel::modFolderName(const int index) const
{
	return m_database.at(index).folderName;
}

inline const ModInfo &DatabaseModel::modInfo(const int index) const
{
	return m_database.at(index);
}

inline ModInfo &DatabaseModel::modInfoRef(const int index)
{
	return m_database[index];
}

inline bool DatabaseModel::modIsEnabled(const int index) const
{
	return m_database.at(index).enabled;
}

inline bool DatabaseModel::modIsExists(const int index) const
{
	return m_database.at(index).exists;
}

inline const QString &DatabaseModel::modName(const int index) const
{
	return m_database.at(index).name;
}

inline const QString &DatabaseModel::modSteamName(const int index) const
{
	return m_database.at(index).steamName;
}

inline int DatabaseModel::rowCount(const QModelIndex &/*parent*/) const
{
	return m_database.size();
}

inline void DatabaseModel::updateRow(const int index)
{
	emit dataChanged(this->index(index), this->index(index));
}

inline void DatabaseModel::updateRow(const QModelIndex &index)
{
	emit dataChanged(index, index);
}

#endif // MODSDATABASEMODEL_H
