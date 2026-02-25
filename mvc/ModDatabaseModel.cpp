#include <functional>

#include <QCollator>
#include <QColor>
#include <QDebug>
#include <QFont>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QThread>

#include "utils/RegExpPatterns.h"
#include "SteamRequester.h"

#include "ModDatabaseModel.h"

//public:

ModDatabaseModel::ModDatabaseModel()
{
    m_completeModNames = true;
    m_useSteamModNames = true;

    m_collections.append(new ModCollection(tr("Favorites")));
    m_collections.append(new ModCollection(tr("Uncategorized")));

    qRegisterMetaType<QVector<int>>("QVector<int>");
}

ModDatabaseModel::~ModDatabaseModel()
{
    clear(true);
}

const QVector<ModCollection *> ModDatabaseModel::userCollectionList() const
{
    QVector<ModCollection *> collections = m_collections;
    collections.removeOne(favoriteCollection());
    collections.removeOne(uncategorizedCollection());
    return collections;
}

const QVector<ModCollection *> ModDatabaseModel::userCollectionList(const QModelIndex &modIndex) const
{
    Q_ASSERT_X(!isCollection(modIndex), "userCollectionList(QModelIndex)", "not a mod index");

    QVector<ModCollection *> collections = modInfo(modIndex).collections().values().toVector();
    collections.removeOne(favoriteCollection());
    collections.removeOne(uncategorizedCollection());

    if (!collections.isEmpty()) {
        QCollator collator;
        collator.setNumericMode(true);
        std::sort(collections.begin(), collections.end(), [collator](ModCollection *i, ModCollection *j) {
            return collator.compare(i->displayedName(), j->displayedName());
        });
    }

    return collections;
}

const QVector<ModCollection *> ModDatabaseModel::userCollectionList(const QModelIndexList &indexes) const
{
    QSet<ModCollection *> collectionSet;
    for (const QModelIndex &index : indexes) {
        if (!index.isValid() || isCollection(index)) {
            continue;
        }

        collectionSet |= modInfo(index).collections();
    }

    QVector<ModCollection *> collections = collectionSet.values().toVector();
    collections.removeOne(favoriteCollection());
    collections.removeOne(uncategorizedCollection());

    if (!collections.isEmpty()) {
        QCollator collator;
        collator.setNumericMode(true);
        std::sort(collections.begin(), collections.end(), [collator](ModCollection *i, ModCollection *j) {
            return collator.compare(i->displayedName(), j->displayedName());
        });
    }

    return collections;
}

void ModDatabaseModel::addMod(const ModInfo &modInfo)
{
    ModCollection *collection = uncategorizedCollection();
    auto it = std::upper_bound(collection->mods().begin(), collection->mods().end(), &modInfo, ModCollection::comparator());
    int index = it - collection->mods().begin();

    beginInsertRows(collectionIndex(collection), index, index);
    m_database.append(new ModInfo(modInfo));
    if (m_database.last()->steamName.isEmpty()) {
        m_database.last()->steamName = ModInfo::generateFailedToGetNameStub();
        //m_database.last().steamName = ModInfo::generateWaitingForSteamResponseStub();
    }
    collection->insertMod(index, m_database.last());
    endInsertRows();
}

void ModDatabaseModel::addUserCollection(ModCollection *collection)
{
    Q_ASSERT_X(!m_collections.contains(collection), "add(ModCollection)", "already have this collection added");

    int index = newCollectionIndex(collection);
    beginInsertRows(QModelIndex(), index, index);
    m_collections.insert(index, collection);
    endInsertRows();
}

bool ModDatabaseModel::isFavorite(const QModelIndex &index) const
{
    return index.isValid() ? collectionPtr(index) == favoriteCollection() : false;
}

bool ModDatabaseModel::isUncategorized(const QModelIndex &index) const
{
    return index.isValid() ? collectionPtr(index) == uncategorizedCollection() : false;
}

void ModDatabaseModel::addFavorite(const QModelIndexList &indexes, Qt::CheckState enabledFilter)
{
    addToCollection(indexes, collectionIndex(favoriteCollection()), enabledFilter);
}

void ModDatabaseModel::removeFavorite(const QModelIndexList &indexes, Qt::CheckState enabledFilter)
{
    removeFromCollection(indexes, collectionIndex(favoriteCollection()), enabledFilter);
}

void ModDatabaseModel::addToCollection(
        const QModelIndex &index,
        const QModelIndex &collectionIndex,
        Qt::CheckState enabledFilter
)
{
    if (!index.isValid() || !collectionIndex.isValid() || !isCollection(collectionIndex)) {
        return;
    }

    QVector<ModInfo *> mods;

    if (isCollection(index)) {
        switch (enabledFilter) {
            case Qt::Checked:
                mods.append(collectionPtr(index)->enabledMods());
            break;

            case Qt::Unchecked:
                mods.append(collectionPtr(index)->disabledMods());
            break;

            case Qt::PartiallyChecked:
                mods.append(collectionPtr(index)->mods());
            break;
        }
    } else {
        mods.append(modInfoPtr(index));
    }

    addToCollection(std::move(mods), collectionIndex);
}

void ModDatabaseModel::removeFromCollection(
        const QModelIndex &index,
        const QModelIndex &collectionIndex,
        Qt::CheckState enabledFilter
)
{
    if (!index.isValid() || !collectionIndex.isValid() || !isCollection(collectionIndex)) {
        return;
    }

    QVector<ModInfo *> mods;

    if (isCollection(index)) {
        switch (enabledFilter) {
            case Qt::Checked:
                mods.append(collectionPtr(index)->enabledMods());
            break;

            case Qt::Unchecked:
                mods.append(collectionPtr(index)->disabledMods());
            break;

            case Qt::PartiallyChecked:
                mods.append(collectionPtr(index)->mods());
            break;
        }
    } else {
        mods.append(modInfoPtr(index));
    }

    removeFromCollection(std::move(mods), collectionIndex);
}

void ModDatabaseModel::addToCollection(
        const QModelIndexList &indexes,
        const QModelIndex &collectionIndex,
        Qt::CheckState enabledFilter
)
{
    if (indexes.isEmpty() || !collectionIndex.isValid() || !isCollection(collectionIndex)) {
        return;
    }

    QVector<ModInfo *> mods;

    for (const QModelIndex &index : indexes) {
        if (!index.isValid()) {
            continue;
        }

        if (isCollection(index)) {
            switch (enabledFilter) {
                case Qt::Checked:
                    mods.append(collectionPtr(index)->enabledMods());
                break;

                case Qt::Unchecked:
                    mods.append(collectionPtr(index)->disabledMods());
                break;

                case Qt::PartiallyChecked:
                    mods.append(collectionPtr(index)->mods());
                break;
            }
        } else {
            mods.append(modInfoPtr(index));
        }
    }

    ModCollection::sort(mods);
    mods.erase(std::unique(mods.begin(), mods.end()), mods.end());
    addToCollection(std::move(mods), collectionIndex);
}

void ModDatabaseModel::removeFromCollection(
        const QModelIndexList &indexes,
        const QModelIndex &collectionIndex,
        Qt::CheckState enabledFilter
)
{
    if (indexes.isEmpty() || !collectionIndex.isValid() || !isCollection(collectionIndex)) {
        return;
    }

    QVector<ModInfo *> mods;

    for (const QModelIndex &index : indexes) {
        if (!index.isValid()) {
            continue;
        }

        if (isCollection(index)) {
            switch (enabledFilter) {
                case Qt::Checked:
                    mods.append(collectionPtr(index)->enabledMods());
                break;

                case Qt::Unchecked:
                    mods.append(collectionPtr(index)->disabledMods());
                break;

                case Qt::PartiallyChecked:
                    mods.append(collectionPtr(index)->mods());
                break;
            }
        } else {
            mods.append(modInfoPtr(index));
        }
    }

    ModCollection::sort(mods);
    mods.erase(std::unique(mods.begin(), mods.end()), mods.end());
    removeFromCollection(std::move(mods), collectionIndex);
}

void ModDatabaseModel::addToCollection(
    const QModelIndexList &indexes,
    ModCollection *collection,
    Qt::CheckState enabledFilter
)
{
    addToCollection(indexes, collectionIndex(collection), enabledFilter);
}

void ModDatabaseModel::removeFromCollection(
    const QModelIndexList &indexes,
    ModCollection *collection,
    Qt::CheckState enabledFilter
)
{
    removeFromCollection(indexes, collectionIndex(collection), enabledFilter);
}

void ModDatabaseModel::setSteamName(const QString &modFolder, const QString &name)
{
    if (name.isEmpty()) {
        return;
    }

    auto it = std::find_if(
        m_database.begin(),
        m_database.end(),
        [&modFolder](ModInfo *mod) {
            return mod->folderName == modFolder;
        }
    );

    ModInfo* mod = it == m_database.end() ? nullptr : *it;

    // Should also skip if name is already set
    if (!mod || ModInfo::isNameValid(mod->steamName)) {
        return;
    }

    mod->steamName = name;
    for (ModCollection *collection : qAsConst(mod->collections())) {
        int oldIndex = collection->modIndex(mod);
        auto it = std::upper_bound(collection->mods().begin(), collection->mods().end(), mod, ModCollection::comparator());
        int newIndex = it - collection->mods().begin();
        if (newIndex > oldIndex) {
            --newIndex;
        }

        if (oldIndex != newIndex) {
            QModelIndex parentIndex = collectionIndex(collection);

            beginMoveRows(parentIndex, oldIndex, oldIndex, parentIndex, newIndex);
            collection->removeMod(oldIndex, false);
            collection->insertMod(newIndex, mod, false);
            endMoveRows();
        }
    }
}

bool ModDatabaseModel::isCollection(const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());
    return !index.internalPointer();
}

QModelIndex ModDatabaseModel::collectionIndex(const QModelIndex &index) const
{
    return index.parent().isValid() ? index.parent() : index;
}

int ModDatabaseModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_collections.size();
    }

    return isCollection(parent) ? collectionPtr(parent)->size() : 0;
}

QModelIndex	ModDatabaseModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    return createIndex(row, column, collectionPtr(parent));
}

QModelIndex	ModDatabaseModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || isCollection(index)) {
        return QModelIndex();
    }

    return collectionIndex(collectionPtr(index), index.column());
}

QVariant ModDatabaseModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    switch (role) {
        case Qt::EditRole:
            if (m_editingName.contains(index)) {
                return m_editingName[index];
            }
            Q_FALLTHROUGH();
        case Qt::DisplayRole:
            return itemPtr(index)->displayedName();
        break;

        case Qt::ForegroundRole:
            if (itemPtr(index)->exists() == Qt::Unchecked) {
                return QColor(Qt::gray);
            }
        break;

        case Qt::FontRole:
            if (itemPtr(index)->locked() == Qt::Checked) {
                QFont font;
                font.setBold(true);
                return font;
            }
        break;

        case Qt::DecorationRole:
            if (isCollection(index)) {
                return QIcon(QString(":/images/ui_outline/%1.svg").arg(
                    collectionPtr(index)->expanded() ? "collapse" : "expand"
                ));
            } else if (itemPtr(index)->locked() == Qt::Checked) {
                return QIcon(":/images/ui_outline/lock.svg");
            }
        break;

        case Role::Type:
            return itemPtr(index)->type();
        break;

        case Role::Enabled:
            return itemPtr(index)->enabled();
        break;

        case Role::Exists:
            return itemPtr(index)->exists();
        break;

        case Role::Locked:
            return itemPtr(index)->locked();
        break;

        case Role::Expanded:
            return collectionPtr(index)->expanded();
        break;

        default:
        break;
    }

    return QVariant();
}

Qt::ItemFlags ModDatabaseModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

QVariant ModDatabaseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            return tr("Mod name");
        } else {
            return QString::number(section + 1);
        }
    }

    return QVariant();
}

void ModDatabaseModel::removeItem(QModelIndex index)
{
    removeRows(index.row(), 1, index.parent());
}

bool ModDatabaseModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    bool isDataChanged = false;
    switch (role) {
        case Qt::EditRole:
            m_editingName[index] = value.toString();
            isDataChanged = true;
        break;

        default:
            if (isCollection(index)) {
                isDataChanged = setData(collectionPtr(index), value, role);
            } else {
                isDataChanged = setData(modInfoPtr(index), value, role);
            }
        break;
    }

    if (isDataChanged) {
        emit dataChanged(index, index, {role});
    }

    return isDataChanged;
}

void ModDatabaseModel::sort(int column, Qt::SortOrder order)
{
    ModCollection::SortType sortType;
    switch (column) {
        case 0:
            sortType = ModCollection::SortType::ByDisplayedName;
        break;

        case 1:
            sortType = ModCollection::SortType::ByFolderName;
        break;

        default:
        return;
    }

    beginResetModel();
    ModCollection::setSortSettings(sortType, order);
    for (ModCollection *collection : qAsConst(m_collections))
    {
        collection->sort();
    }
    endResetModel();
}

bool ModDatabaseModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (!count) {
        return false;
    }

    for (const int end = row + count; row < end; ++row) {
        QModelIndex entry = index(row, 0, parent);
        if (isCollection(entry)) {
            beginRemoveRows(entry.parent(), entry.row(), entry.row());
            delete m_collections.takeAt(entry.row());
            endRemoveRows();
        } else {
            ModInfo *modInfo = modInfoPtr(entry);
            for (int i = 0; i < m_collections.size(); ++i) {
                entry = index(m_collections[i]->modIndex(modInfo), 0, index(i));
                if (entry.isValid()) {
                    beginRemoveRows(entry.parent(), entry.row(), entry.row());
                    m_collections[i]->removeMod(entry.row());
                    endRemoveRows();
                }
            }
            m_database.removeOne(modInfo);
            delete modInfo;
        }
    }

    return true;
}

void ModDatabaseModel::setModsExistsState(const bool isExists)
{
    for (int i = 0; i < m_database.size(); ++i) {
        m_database[i]->setExists(isExists);
    }
}

void ModDatabaseModel::onModInfoUpdated(int index, const QVector<int> &roles)
{
    for (const auto& collection : qAsConst(m_database[index]->collections())) {
        const QModelIndex &row = modInfoIndex(collection, index);
        emit dataChanged(row, row, roles);
    }
}

void ModDatabaseModel::reset()
{
    beginResetModel();
    clear();
    endResetModel();
}

void ModDatabaseModel::reset(std::function<void()> actions)
{
    beginResetModel();
    actions();
    endResetModel();
}

bool ModDatabaseModel::submit()
{
    if (m_editingName.isEmpty()) {
        return false;
    }

    const QList<QModelIndex> &keys = m_editingName.keys();
    for (const QModelIndex &index : keys) {
        itemRef(index).setName(m_editingName[index]);
    }
    m_editingName.clear();

    return true;
}

void ModDatabaseModel::revert()
{
    m_editingName.clear();
}

void ModDatabaseModel::fromJson(const QJsonObject &json)
{
    beginResetModel();
    clear(true);
    QJsonArray mods = json["mods"].toArray();
    for (int i = 0; i < mods.size(); i++) {
        m_database.append(new ModInfo(mods[i].toObject()));
    }

    QJsonArray collections = json["collections"].toArray();
    for (int i = 0; i < collections.size(); i++) {
        QJsonObject json = collections[i].toObject();
        m_collections.append(new ModCollection(json));
        QStringList mods = json["mods"].toString().split(';', Qt::SkipEmptyParts);
        for (const QString &range : qAsConst(mods)) {
            int first;
            int last;
            if (range.contains('-')) {
                QStringList parts = range.split('-', Qt::SkipEmptyParts);
                first = parts[0].toInt();
                last = parts[1].toInt();
            } else {
                first = last = range.toInt();
            }

            for (int i = first; i <= last; ++i) {
                m_collections.last()->addMod(m_database[i]);
            }
        }
    }
    endResetModel();
}

QJsonObject ModDatabaseModel::toJson() const
{
    QJsonArray mods;
    for (const ModInfo *modInfo : qAsConst(m_database)) {
        mods.append(modInfo->toNewJsonObject());
    }

    QJsonArray collections;
    for (const ModCollection *collection : qAsConst(m_collections)) {
        QJsonObject json = collection->toNewJsonObject();
        QString mods;
        int first = -1;
        int last = -1;
        for (ModInfo *modInfo : collection->mods()) {
            int index = m_database.indexOf(modInfo);
            if (first < 0) {
                first = last = index;
            } else if (index == last + 1) {
                last += 1;
            } else {
                mods += QString::number(first);
                if (last > first) {
                    mods += '-' + QString::number(last);
                }
                mods += ';';
                first = last = index;
            }
        }

        if (first >= 0) {
            mods += QString::number(first);
            if (last > first) {
                mods += '-' + QString::number(last);
            }
        }

        json["mods"] = mods;
        collections.append(std::move(json));
    }

    QJsonObject database;
    database["mods"] = mods;
    database["collections"] = collections;
    return database;
}

//public slots:

void ModDatabaseModel::setCompleteModNames(const bool enabled)
{
    if (m_completeModNames == enabled) {
        return;
    }

    m_completeModNames = enabled;
    emit dataChanged(index(0), index(m_database.size() - 1));
}

void ModDatabaseModel::setUsingSteamModNames(const bool use)
{
    if (m_useSteamModNames == use) {
        return;
    }

    m_useSteamModNames = use;
    emit dataChanged(index(0), index(m_database.size() - 1));
}

//private:

void ModDatabaseModel::clear(bool removeBuiltInCollections)
{
    for (ModCollection *collection : qAsConst(m_collections)) {
        collection->clear();
    }

    QVector<ModCollection *>::iterator begin = m_collections.begin();
    QVector<ModCollection *>::iterator end = m_collections.end();
    if (!removeBuiltInCollections) {
        ++begin;
        --end;
    }

    qDeleteAll(begin, end);
    m_collections.erase(begin, end);

    qDeleteAll(m_database);
    m_database.clear();
}

QModelIndex ModDatabaseModel::collectionIndex(ModCollection *collection, int column) const
{
    return index(m_collections.indexOf(collection), column);
}

QModelIndex	ModDatabaseModel::modInfoIndex(const QModelIndex &collectionIndex, int modIndex) const
{
    return index(modIndex, 0, collectionIndex);
}

QModelIndex	ModDatabaseModel::modInfoIndex(ModCollection *collection, ModInfo *modInfo) const
{
    return modInfoIndex(collection, collection->modIndex(modInfo));
}

QModelIndex	ModDatabaseModel::modInfoIndex(ModCollection *collection, int modIndex) const
{
    return index(modIndex, 0, collectionIndex(collection));
}

QModelIndex	ModDatabaseModel::modInfoIndex(int collectionIndex, int modIndex) const
{
    return modInfoIndex(index(collectionIndex), modIndex);
}

AbstractModDatabaseItem *ModDatabaseModel::itemPtr(const QModelIndex &index) const
{
    AbstractModDatabaseItem *item = nullptr;
    ModCollection *collection = collectionPtr(index);
    if (isCollection(index)) {
        item = collection;
    } else {
        item = collection->mods()[index.row()];
    }

    Q_ASSERT_X(
        item,
        "itemPtr",
        QString("can't get item info by %1 index (%2, %3); parent (%4, %5) is %6")
                .arg(index.isValid() ? "valid" : "invalid")
                .arg(index.row())
                .arg(index.column())
                .arg(index.parent().isValid() ? "valid" : "invalid")
                .arg(index.parent().row())
                .arg(index.parent().column()).toUtf8()
    );

    return item;
}

ModInfo *ModDatabaseModel::modInfoPtr(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return nullptr;
    }

    if (isCollection(index))
    {
        Q_ASSERT_X(!isCollection(index), "modPtr", "can't get mod by category index");

    }

    Q_ASSERT_X(!isCollection(index), "modPtr", "can't get mod by category index");
    return collectionPtr(index)->mods()[index.row()];
}

ModCollection *ModDatabaseModel::collectionPtr(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return nullptr;
    }

    if (isCollection(index)) {
        return m_collections[index.row()];
    } else {
        return static_cast<ModCollection *>(index.internalPointer());
    }
}

bool ModDatabaseModel::setData(ModCollection *collection, const QVariant &value, int role)
{
    bool isDataChanged = false;
    switch (role) {
        case Role::Expanded:
            isDataChanged = collection->setExpanded(value.toBool());
        break;

        case Role::Enabled:
            Q_FALLTHROUGH();
        case Role::Exists:
            Q_FALLTHROUGH();
        case Role::Locked:
            for (int i = 0; i < collection->size(); ++i) {
                setData(modInfoIndex(collection, i), value, role);
            }
        break;

        default:
        break;
    }

    return isDataChanged;
}

bool ModDatabaseModel::setData(ModInfo *item, const QVariant &value, int role)
{
    bool isDataChanged = false;
    bool isChangingState = (role >= Role::Enabled && role <= Role::Locked);
    Qt::CheckState newState = isChangingState ? value.value<Qt::CheckState>() : Qt::Checked;
    Q_ASSERT(newState != Qt::PartiallyChecked);

    QVector<QPair<ModCollection *, Qt::CheckState>> collectionsData;
    switch (role) {
        case Role::Enabled:
            if (item->enabled() != newState) {
                for (const auto &collection : qAsConst(item->collections())) {
                    collectionsData.append({collection, collection->enabled()});
                }
                isDataChanged = item->setEnabled(newState == Qt::Checked);
                if (isDataChanged) {
                    for (auto it = collectionsData.begin(); it != collectionsData.end();) {
                        if (it->first->enabled() == it->second) {
                            it = collectionsData.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            }
        break;

        case Role::Exists:
            if (item->exists() != newState) {
                for (const auto &collection : qAsConst(item->collections())) {
                    collectionsData.append({collection, collection->exists()});
                }
                isDataChanged = item->setExists(newState == Qt::Checked);
                if (isDataChanged) {
                    for (auto it = collectionsData.begin(); it != collectionsData.end();) {
                        if (it->first->exists() == it->second) {
                            it = collectionsData.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            }
        break;

        case Role::Locked:
            if (item->locked() != newState) {
                for (const auto &collection : qAsConst(item->collections())) {
                    collectionsData.append({collection, collection->locked()});
                }
                isDataChanged = item->setLocked(newState == Qt::Checked);
                if (isDataChanged) {
                    for (auto it = collectionsData.begin(); it != collectionsData.end();) {
                        if (it->first->locked() == it->second) {
                            it = collectionsData.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            }
        break;

        default:
        break;
    }

    if (isDataChanged) {
        for (const auto &collection : qAsConst(collectionsData)) {
            const QModelIndex &collectionIndex = this->collectionIndex(collection.first);
            emit dataChanged(collectionIndex, collectionIndex, {role});
        }
    }

    return isDataChanged;
}

void ModDatabaseModel::addToCollection(QVector<ModInfo *> mods, const QModelIndex &collectionIndex)
{
    ModCollection *collection = collectionPtr(collectionIndex);
    for (int i = 0; i < mods.size(); ++i) {
        if (collection->contains(mods[i])) {
            mods.removeAt(i);
            --i;
        }
    }

    if (mods.isEmpty()) {
        return;
    }


    addToCollection(mods, collection);

    ModCollection *uncategorized = uncategorizedCollection();
    if (collection != uncategorized)
    {
        for (int i = 0; i < mods.size(); ++i) {
            if (!mods[i]->collections().contains(uncategorized)) {
                mods.removeAt(i);
                --i;
            }
        }

        if (!mods.isEmpty()) {
            removeFromCollection(mods, uncategorized);
        }
    }
}

void ModDatabaseModel::removeFromCollection(QVector<ModInfo *> mods, const QModelIndex &collectionIndex)
{
    ModCollection *collection = collectionPtr(collectionIndex);
    for (int i = 0; i < mods.size(); ++i) {
        if (!collection->contains(mods[i])) {
            mods.removeAt(i);
            --i;
        }
    }

    if (mods.isEmpty()) {
        return;
    }

    removeFromCollection(mods, collection);

    ModCollection *uncategorized = uncategorizedCollection();
    if (collection != uncategorized)
    {
        for (int i = 0; i < mods.size(); ++i) {
            if (!mods[i]->collections().isEmpty()) {
                mods.removeAt(i);
                --i;
            }
        }

        if (!mods.isEmpty()) {
            addToCollection(mods, uncategorized);
        }
    }
}

void ModDatabaseModel::addToCollection(const QVector<ModInfo *> &mods, ModCollection *collection)
{
    QVector<QPair<int, int>> rowRanges;
    QPair<int, int> range = {-1, -1};
    QVector<ModInfo *> collectionMods = collection->mods();
    for (ModInfo *mod : mods) {
        auto it = std::upper_bound(collectionMods.begin(), collectionMods.end(), mod, ModCollection::comparator());
        int index = it - collectionMods.begin();
        collectionMods.insert(index, mod);

        if (range.first == -1) {
            range.first = index;
            range.second = index;
        } else if (index == range.second + 1) {
            range.second = index;
        } else {
            rowRanges.append(range);
            range.first = index;
            range.second = index;
        }
    }

    if (range.first != -1) {
        rowRanges.append(range);
    }

    int i = 0;
    QModelIndex parentIndex = collectionIndex(collection);
    for (const auto &range : rowRanges) {
        beginInsertRows(parentIndex, range.first, range.second);
        for (int row = range.first; row <= range.second; ++row) {
            collection->insertMod(row, mods[i]);
            ++i;
        }
        endInsertRows();
    }
    emit dataChanged(parentIndex, parentIndex, {Qt::DisplayRole});
}

void ModDatabaseModel::removeFromCollection(const QVector<ModInfo *> &mods, ModCollection *collection)
{
    QVector<QPair<int, int>> rowRanges;
    QPair<int, int> range = {-1, -1};
    for (ModInfo *mod : mods) {
        int index = collection->modIndex(mod);
        Q_ASSERT_X(index >= 0, "removeFromCollection", "collection doesn't contain mod");

        if (range.first == -1) {
            range.first = index;
            range.second = index;
        } else if (index == range.second + 1) {
            range.second = index;
        } else {
            rowRanges.prepend(range);
            range.first = index;
            range.second = index;
        }
    }

    if (range.first != -1) {
        rowRanges.prepend(range);
    }

    QModelIndex parentIndex = collectionIndex(collection);
    for (const auto &range : rowRanges) {
        beginRemoveRows(parentIndex, range.first, range.second);
        for (int row = range.second; row >= range.first; --row) {
            collection->removeMod(row);
        }
        endRemoveRows();
    }
    emit dataChanged(parentIndex, parentIndex, {Qt::DisplayRole});
}

bool ModDatabaseModel::hasUserCollections() const
{
    return m_collections.size() > 2;
}

ModCollection *ModDatabaseModel::favoriteCollection() const
{
    return m_collections.first();
}

ModCollection *ModDatabaseModel::uncategorizedCollection() const
{
    return m_collections.last();
}

int ModDatabaseModel::newCollectionIndex(ModCollection *collection) const
{
    if (!hasUserCollections())
    {
        return 1;
    }

    QCollator collator;
    collator.setNumericMode(true);
    int i = 1;
    while (i < m_collections.size() - 1 && collator(m_collections[i]->name(), collection->name())) {
        ++i;
    }

    return i;
}
