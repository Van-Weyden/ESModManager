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
    enum ModRole
    {
        Enabled = Qt::UserRole,
        Exists,
        Marked,
        Locked
    };

public:
    ModDatabaseModel();
    ~ModDatabaseModel() = default;

    bool isNameValid(const QString &name) const;

    void add(const ModInfo &modInfo);
    inline void clear();
    inline int databaseSize() const;
    inline const QString &modFolderName(const QModelIndex &index) const;
    inline const ModInfo &modInfo(const QModelIndex &index) const;
    inline ModInfo &modInfoRef(const QModelIndex &index);
    inline bool modIsEnabled(const QModelIndex &index) const;
    inline bool modIsExists(const QModelIndex &index) const;
    inline const QString &modName(const QModelIndex &index) const;
    inline const QString &modSteamName(const QModelIndex &index) const;

    QString displayedModName(const ModInfo &modInfo) const;

    inline int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    inline int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    void sort(int column = 0, Qt::SortOrder order = Qt::AscendingOrder) override;

    void removeFromDatabase(const QModelIndex &index);
    void setModsExistsState(const bool isExists);
    inline void updateRow(const QModelIndex &index);

    void reset(std::function<void()> actions);

public slots:
    void enableMod(const QModelIndex &index);
    void disableMod(const QModelIndex &index);

    void setCompleteModNames(const bool enabled = true);
    void setUsingSteamModNames(const bool use = true);

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
    beginResetModel();
    m_database.clear();
    endResetModel();
}

inline int ModDatabaseModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 1;
}

inline int ModDatabaseModel::databaseSize() const
{
    return m_database.size();
}

inline const QString &ModDatabaseModel::modFolderName(const QModelIndex &index) const
{
    return m_database[index.row()].folderName;
}

inline const ModInfo &ModDatabaseModel::modInfo(const QModelIndex &index) const
{
    return m_database[index.row()];
}

inline ModInfo &ModDatabaseModel::modInfoRef(const QModelIndex &index)
{
    return m_database[index.row()];
}

inline bool ModDatabaseModel::modIsEnabled(const QModelIndex &index) const
{
    return m_database[index.row()].enabled();
}

inline bool ModDatabaseModel::modIsExists(const QModelIndex &index) const
{
    return m_database[index.row()].exists();
}

inline const QString &ModDatabaseModel::modName(const QModelIndex &index) const
{
    return m_database[index.row()].name;
}

inline const QString &ModDatabaseModel::modSteamName(const QModelIndex &index) const
{
    return m_database[index.row()].steamName;
}

inline int ModDatabaseModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_database.size();
}

inline void ModDatabaseModel::updateRow(const QModelIndex &index)
{
    emit dataChanged(index, index);
}

#endif // MODSDATABASEMODEL_H
