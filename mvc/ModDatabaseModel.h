#pragma once

#include <QAbstractItemModel>

#include "ModCollection.h"
#include "ModInfo.h"

class QThread;
class SteamRequester;

class ModDatabaseModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Role
    {
        Type = Qt::UserRole,
        Enabled,
        Exists,
        Locked,
        Expanded
    };

    using ItemType = AbstractModDatabaseItem::Type;

public:
    ModDatabaseModel();
    ~ModDatabaseModel();

    inline QVector<ModInfo *> &modListRef();
    inline const QVector<ModInfo *> &modList() const;
    QVector<ModInfo *> modList(const QModelIndexList &indexes) const;
    QVector<ModCollection *> userCollectionList() const;
    QVector<ModCollection *> userCollectionList(const QModelIndex &modIndex) const;
    QVector<ModCollection *> userCollectionList(const QModelIndexList &indexes) const;

    void addMod(const ModInfo &modInfo);
    void addUserCollection(ModCollection *collection);
    void removeUserCollection(ModCollection *collection);
    inline int size() const;

    bool isFavorite(const QModelIndex &index) const;
    bool isUncategorized(const QModelIndex &index) const;

    void addFavorite(const QModelIndexList &indexes, Qt::CheckState enabledFilter = Qt::PartiallyChecked);
    void removeFavorite(const QModelIndexList &indexes, Qt::CheckState enabledFilter = Qt::PartiallyChecked);

    void addToCollection(const QModelIndex &index, const QModelIndex &collectionIndex,
                         Qt::CheckState enabledFilter = Qt::PartiallyChecked);
    void removeFromCollection(const QModelIndex &index, const QModelIndex &collectionIndex,
                              Qt::CheckState enabledFilter = Qt::PartiallyChecked);

    void addToCollection(const QModelIndexList &indexes, const QModelIndex &collectionIndex,
                         Qt::CheckState enabledFilter = Qt::PartiallyChecked);
    void removeFromCollection(const QModelIndexList &indexes, const QModelIndex &collectionIndex,
                              Qt::CheckState enabledFilter = Qt::PartiallyChecked);

    void addToCollection(const QModelIndexList &indexes, ModCollection *collection,
                         Qt::CheckState enabledFilter = Qt::PartiallyChecked);
    void removeFromCollection(const QModelIndexList &indexes, ModCollection *collection,
                              Qt::CheckState enabledFilter = Qt::PartiallyChecked);

    inline const AbstractModDatabaseItem& item(const QModelIndex &index) const;
    inline AbstractModDatabaseItem& itemRef(const QModelIndex &index);

    inline const QString &modFolderName(const QModelIndex &index) const;
    inline const ModInfo &modInfo(const QModelIndex &index) const;
    inline const ModInfo &modInfo(int index) const;
    inline ModInfo &modInfoRef(const QModelIndex &index);
    inline ModInfo &modInfoRef(int index) const;
    inline bool modIsEnabled(const QModelIndex &index) const;
    inline bool modIsExists(const QModelIndex &index) const;
    inline const QString &modName(const QModelIndex &index) const;
    inline const QString &modSteamName(const QModelIndex &index) const;

    void setSteamName(const QString &modFolder, const QString &name);

    bool isCollection(const QModelIndex &index) const;
    bool isUserCollection(const QModelIndex &index) const;
    inline const ModCollection &collection(const QModelIndex &index) const;
    inline ModCollection &collectionRef(const QModelIndex &index) const;

    QModelIndex collectionIndex(const QModelIndex &index) const;

    inline int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex	index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex	parent(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    void sort(int column = 0, Qt::SortOrder order = Qt::AscendingOrder) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    void removeItem(const QModelIndex &index);
    void setModsExistsState(const bool isExists);
    inline void updateRow(const QModelIndex &index);
    void onModInfoUpdated(int index, const QVector<int> &roles);

    void reset();
    void reset(std::function<void ()> actions);

    bool submit() override;
    void revert() override;

    void fromJson(const QJsonObject &json);
    QJsonObject toJson() const;

public slots:
    void setCompleteModNames(const bool enabled = true);
    void setUsingSteamModNames(const bool use = true);

private:
    void clear(bool removeBuiltInCollections = false);
    QModelIndex collectionIndex(ModCollection *collection, int column = 0) const;
    QModelIndex	modInfoIndex(const QModelIndex &collectionIndex, int modIndex) const;
    QModelIndex	modInfoIndex(ModCollection *collection, ModInfo *modInfo) const;
    QModelIndex	modInfoIndex(ModCollection *collection, int modIndex) const;
    QModelIndex	modInfoIndex(int collectionIndex, int modIndex) const;
    AbstractModDatabaseItem *itemPtr(const QModelIndex &index) const;
    ModInfo *modInfoPtr(const QModelIndex &index) const;
    ModCollection *collectionPtr(const QModelIndex &index) const;

    bool setData(ModCollection *collection, const QVariant &value, int role);
    bool setData(ModInfo *mod, const QVariant &value, int role);

    /**
     * @brief Preprocessor method for the addToCollection(QVector<ModInfo *>, ModCollection *)
     * @note Collection may contain passed mods and all mods must be arranged in sort order
     */
    void addToCollection(QVector<ModInfo *> mods, const QModelIndex &collectionIndex);

    /**
     * @brief Preprocessor method for the removeFromCollection(QVector<ModInfo *>, ModCollection *)
     * @note Collection may contain passed mods and all mods must be arranged in sort order
     */
    void removeFromCollection(QVector<ModInfo *> mods, const QModelIndex &collectionIndex);

    /**
     * @note Collection must NOT contain passed mods and all mods must be arranged in sort order
     */
    void addToCollection(const QVector<ModInfo *> &mods, ModCollection *collection);

    /**
     * @note Collection must contain passed mods and all mods must be arranged in sort order
     */
    void removeFromCollection(const QVector<ModInfo *> &mods, ModCollection *collection);

    bool hasUserCollections() const;
    int firstUserCollectionIndex() const;
    int lastUserCollectionIndex() const;
    ModCollection *allModsCollection() const;
    ModCollection *favoriteCollection() const;
    ModCollection *uncategorizedCollection() const;
    void removeBuiltInCollections(QVector<ModCollection *> &collections, bool sort) const;
    int newCollectionIndex(ModCollection *collection) const;

private:
    QVector<ModInfo *> m_database;
    QVector<ModCollection *> m_collections;
    QMap<QModelIndex, QString> m_editingName;

    bool m_completeModNames;
    bool m_useSteamModNames;
};



//public:

inline QVector<ModInfo *> &ModDatabaseModel::modListRef()
{
    return m_database;
}

inline const QVector<ModInfo *> &ModDatabaseModel::modList() const
{
    return m_database;
}

inline int ModDatabaseModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 1;
}

inline int ModDatabaseModel::size() const
{
    return m_database.size();
}

inline const AbstractModDatabaseItem& ModDatabaseModel::item(const QModelIndex &index) const
{
    auto* item = itemPtr(index);
    return *item;
}

inline AbstractModDatabaseItem& ModDatabaseModel::itemRef(const QModelIndex &index)
{
    auto* item = itemPtr(index);
    return *item;
}

inline const QString &ModDatabaseModel::modFolderName(const QModelIndex &index) const
{
    return modInfo(index).folderName;
}

inline const ModInfo &ModDatabaseModel::modInfo(const QModelIndex &index) const
{
    return *modInfoPtr(index);
}

inline const ModInfo &ModDatabaseModel::modInfo(int index) const
{
    return *m_database[index];
}

inline ModInfo &ModDatabaseModel::modInfoRef(const QModelIndex &index)
{
    return *modInfoPtr(index);
}

inline ModInfo &ModDatabaseModel::modInfoRef(int index) const
{
    return *m_database[index];
}

inline bool ModDatabaseModel::modIsEnabled(const QModelIndex &index) const
{
    return modInfo(index).enabled();
}

inline bool ModDatabaseModel::modIsExists(const QModelIndex &index) const
{
    return modInfo(index).exists();
}

inline const QString &ModDatabaseModel::modName(const QModelIndex &index) const
{
    return modInfo(index).sourcesName;
}

inline const QString &ModDatabaseModel::modSteamName(const QModelIndex &index) const
{
    return modInfo(index).steamName;
}

inline const ModCollection &ModDatabaseModel::collection(const QModelIndex &index) const
{
    return *collectionPtr(index);
}

inline ModCollection &ModDatabaseModel::collectionRef(const QModelIndex &index) const
{
    return *collectionPtr(index);
}

inline void ModDatabaseModel::updateRow(const QModelIndex &index)
{
    emit dataChanged(index, index);
}
