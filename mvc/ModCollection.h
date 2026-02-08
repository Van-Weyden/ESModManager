#pragma once

#include "AbstractModDatabaseItem.h"

class ModInfo;

class ModCollection : public AbstractModDatabaseItem
{
public:
    using SortComparator = std::function<bool(const ModInfo *i, const ModInfo *j)>;
    enum SortType
    {
        ByDisplayedName,
        ByFolderName
    };

public:
    ModCollection(const QString &name = QString());
    ModCollection(const QJsonObject &object);
    ~ModCollection() override = default;

    static SortComparator comparator();

    Type type() const override;
    Qt::CheckState exists() const override;
    Qt::CheckState enabled() const override;
    Qt::CheckState locked() const override;
    bool expanded() const;

    bool setExists(const bool exists) override;
    bool setEnabled(const bool enabled, const bool force = false) override;
    bool setLocked(const bool locked) override;
    bool setExpanded(bool expanded);

    // Don't save/load mod list; model handles this
    void fromJsonObject(const QJsonObject &object) override;
    void toJsonObject(QJsonObject &object) const override;

    int size() const;
    bool isEmpty() const;
    bool hasModWithName(const QString &name, bool fuzzyMode) const;
    const QVector<ModInfo *> &mods() const;
    QVector<ModInfo *> enabledMods() const;
    QVector<ModInfo *> disabledMods() const;

    static void setSortSettings(SortType sortType, Qt::SortOrder order = Qt::AscendingOrder);
    static void sort(QVector<ModInfo *> &mods);
    void sort();

    void clear();
    int addMod(ModInfo *mod, bool updateLinks = true);
    void insertMod(int index, ModInfo *mod, bool updateLinks = true);
    bool contains(ModInfo *mod) const;
    int modIndex(ModInfo *mod) const;
    bool removeMod(int pos, bool updateLinks = true);
    void setId(int id);

private:
    static SortComparator sortComparator(SortType sortType, Qt::SortOrder order = Qt::AscendingOrder);

private:
    QVector<ModInfo *> m_mods;
    static SortComparator Comparator;
    bool m_expanded = true;
};

