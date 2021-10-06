#ifndef MODSDATABASEMODEL_H
#define MODSDATABASEMODEL_H

#include <QAbstractListModel>

#include "ModInfo.h"

class QThread;
class SteamRequester;

class ModDatabaseModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ModDatabaseModel();
    ~ModDatabaseModel() = default;

    bool isNameValid(const QString &name) const;

    void add(const ModInfo &modInfo);
    inline void clear();
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

signals:
    void modCheckStateChanged(const int rowIndex, const bool state);

protected slots:
    void setCompleteModNames(const int mode);
    void setUsingSteamModNames(const int mode);

private:
    QVector<ModInfo> m_database;

    bool m_completeModNames;
    bool m_useSteamModNames;
};



//public:

inline void ModDatabaseModel::clear()
{
    beginResetModel(); m_database.clear(); endResetModel();
}

inline int ModDatabaseModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 2;
}

inline int ModDatabaseModel::databaseSize() const
{
    return m_database.size();
}

inline const QString &ModDatabaseModel::modFolderName(const int index) const
{
    return m_database.at(index).folderName;
}

inline const ModInfo &ModDatabaseModel::modInfo(const int index) const
{
    return m_database.at(index);
}

inline ModInfo &ModDatabaseModel::modInfoRef(const int index)
{
    return m_database[index];
}

inline bool ModDatabaseModel::modIsEnabled(const int index) const
{
    return m_database.at(index).enabled;
}

inline bool ModDatabaseModel::modIsExists(const int index) const
{
    return m_database.at(index).exists;
}

inline const QString &ModDatabaseModel::modName(const int index) const
{
    return m_database.at(index).name;
}

inline const QString &ModDatabaseModel::modSteamName(const int index) const
{
    return m_database.at(index).steamName;
}

inline int ModDatabaseModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_database.size();
}

inline void ModDatabaseModel::updateRow(const int index)
{
    emit dataChanged(this->index(index), this->index(index));
}

inline void ModDatabaseModel::updateRow(const QModelIndex &index)
{
    emit dataChanged(index, index);
}

#endif // MODSDATABASEMODEL_H
