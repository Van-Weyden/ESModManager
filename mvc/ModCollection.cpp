#include <QCollator>

#include "ModInfo.h"

#include "ModCollection.h"

ModCollection::SortComparator ModCollection::Comparator = ModCollection::sortComparator(
    ModCollection::SortType::ByDisplayedName
);

ModCollection::ModCollection(const QString &name)
    : AbstractModDatabaseItem(name)
{}

ModCollection::ModCollection(const QJsonObject &object)
{
    ModCollection::fromJsonObject(object);
}

ModCollection::SortComparator ModCollection::comparator()
{
    return Comparator;
}

AbstractModDatabaseItem::Type ModCollection::type() const
{
    return Type::Collection;
}

Qt::CheckState ModCollection::exists() const
{
    if (m_mods.empty()) {
        return Qt::Unchecked;
    }

    Qt::CheckState res = m_mods.first()->exists();
    for (int i = 0; i < m_mods.size(); ++i) {
        if (m_mods[i]->exists() != res) {
            return Qt::PartiallyChecked;
        }
    }
    return res;
}

Qt::CheckState ModCollection::enabled() const
{
    if (m_mods.empty()) {
        return Qt::Unchecked;
    }

    Qt::CheckState res = m_mods.first()->enabled();
    for (int i = 0; i < m_mods.size(); ++i) {
        if (m_mods[i]->enabled() != res) {
            return Qt::PartiallyChecked;
        }
    }
    return res;
}

Qt::CheckState ModCollection::locked() const
{
    if (m_mods.empty()) {
        return Qt::Unchecked;
    }

    Qt::CheckState res = m_mods.first()->locked();
    for (int i = 0; i < m_mods.size(); ++i) {
        if (m_mods[i]->locked() != res) {
            return Qt::PartiallyChecked;
        }
    }
    return res;
}

bool ModCollection::expanded() const
{
    return m_expanded;
}

bool ModCollection::setEnabled(const bool enabled, const bool force)
{
    bool changed = false;
    if (force || locked() != Qt::Checked) {
        for (ModInfo *mod : qAsConst(m_mods)) {
            changed |= mod->setEnabled(enabled);
        }
    }
    return changed;
}

bool ModCollection::setExists(const bool exists)
{
    bool changed = false;
    for (ModInfo *mod : qAsConst(m_mods)) {
        changed |= mod->setExists(exists);
    }
    return changed;
}

bool ModCollection::setLocked(const bool locked)
{
    bool changed = false;
    for (ModInfo *mod : qAsConst(m_mods)) {
        changed |= mod->setLocked(locked);
    }
    return changed;
}

bool ModCollection::setExpanded(bool expanded)
{
    bool changed = (m_expanded != expanded);
    m_expanded = expanded;
    return changed;
}

void ModCollection::fromJsonObject(const QJsonObject &object)
{
    AbstractModDatabaseItem::fromJsonObject(object);
}

void ModCollection::toJsonObject(QJsonObject &object) const
{
    AbstractModDatabaseItem::toJsonObject(object);
}

int ModCollection::size() const
{
    return m_mods.size();
}

bool ModCollection::isEmpty() const
{
    return m_mods.isEmpty();
}

bool ModCollection::hasModWithName(const QString &name, bool fuzzyMode) const
{
    bool contains = false;
    for (const auto& mod : qAsConst(m_mods)) {
        if (fuzzyMode) {
            contains = mod->displayedName().contains(name, Qt::CaseSensitivity::CaseInsensitive);
        } else {
            contains = (mod->displayedName() == name);
        }

        if (contains) {
            return true;
        }
    }

    return false;
}

const QVector<ModInfo *> &ModCollection::mods() const
{
    return m_mods;
}

QVector<ModInfo *> ModCollection::enabledMods() const
{
    QVector<ModInfo *> mods;
    std::copy_if(
        m_mods.begin(),
        m_mods.end(),
        std::back_inserter(mods),
        [](ModInfo *mod) {
            return mod->enabled();
        }
    );
    return mods;
}

QVector<ModInfo *> ModCollection::disabledMods() const
{
    QVector<ModInfo *> mods;
    std::copy_if(
        m_mods.begin(),
        m_mods.end(),
        std::back_inserter(mods),
        [](ModInfo *mod) {
            return !mod->enabled();
        }
    );
    return mods;
}

void ModCollection::setSortSettings(SortType sortType, Qt::SortOrder order)
{
    Comparator = sortComparator(sortType, order);
}

void ModCollection::sort(QVector<ModInfo *> &mods)
{
    std::sort(mods.begin(), mods.end(), Comparator);
}

void ModCollection::sort()
{
    sort(m_mods);
}

void ModCollection::clear()
{
    m_mods.clear();
}

int ModCollection::addMod(ModInfo *mod, bool updateLinks)
{
    auto it = std::upper_bound(m_mods.begin(), m_mods.end(), mod, Comparator);
    int index = it - m_mods.begin();
    insertMod(index, mod, updateLinks);
    return index;
}

void ModCollection::insertMod(int index, ModInfo *mod, bool updateLinks)
{
    m_mods.insert(index, mod);
    if (updateLinks) {
        mod->collectionsRef().insert(this);
    }
}

bool ModCollection::contains(ModInfo *mod) const
{
    return m_mods.contains(mod);
}

int ModCollection::modIndex(ModInfo *mod) const
{
    return m_mods.indexOf(mod);
}

bool ModCollection::removeMod(int pos, bool updateLinks)
{
    int oldSize = size();
    ModInfo *mod = m_mods.takeAt(pos);
    if (updateLinks) {
        mod->collectionsRef().remove(this);
    }
    return oldSize > size();
}

ModCollection::SortComparator ModCollection::sortComparator(SortType sortType, Qt::SortOrder order)
{
    QCollator collator;
    collator.setNumericMode(true);
    switch (sortType) {
        case ByDisplayedName:
            return [collator, order](const ModInfo *i, const ModInfo *j) {
                return ((i->exists() ^ j->exists()) ? i->exists() :
                    collator(i->displayedName(), j->displayedName()) ^ (order == Qt::DescendingOrder));
            };
        break;

        case ByFolderName:
            return [collator, order](const ModInfo *i, const ModInfo *j) {
                return ((i->exists() ^ j->exists()) ? i->exists() :
                    collator(i->folderName, j->folderName) ^ (order == Qt::DescendingOrder));
            };
        break;
    }

    return nullptr;
}
