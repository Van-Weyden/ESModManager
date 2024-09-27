#include "ModDatabaseModel.h"

#include "proxyModels.h"

ModFilterProxyModel::ModFilterProxyModel(QObject *parent, QAbstractItemModel *model)
    : QSortFilterProxyModel(parent)
{
    if (model) {
        setSourceModel(model);
    }
}

bool ModFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!index.isValid()) {
        return false;
    }

    return filterAccepts(index);
}

bool ModFilterProxyModel::filterAccepts(const QModelIndex &sourceModelIndex) const
{
    if (m_filter.isEmpty()) {
        return true;
    }

    QString modName = sourceModel()->data(sourceModelIndex, Qt::DisplayRole).toString();
    return modName.contains(m_filter, Qt::CaseSensitivity::CaseInsensitive);
}

QString ModFilterProxyModel::filter() const
{
    return m_filter;
}

void ModFilterProxyModel::setFilter(const QString &filter)
{
    if (m_filter != filter) {
        beginResetModel();
        m_filter = filter;
        endResetModel();
    }
}



EnabledModsProxyModel::EnabledModsProxyModel(QObject *parent, QAbstractItemModel *model)
    : ModFilterProxyModel(parent, model)
{}

bool EnabledModsProxyModel::filterAccepts(const QModelIndex &sourceModelIndex) const
{
    return ModFilterProxyModel::filterAccepts(sourceModelIndex) &&
           sourceModel()->data(sourceModelIndex, ModDatabaseModel::ModRole::Exists).toBool() &&
           sourceModel()->data(sourceModelIndex, ModDatabaseModel::ModRole::Enabled).toBool();
}



DisabledModsProxyModel::DisabledModsProxyModel(QObject *parent, QAbstractItemModel *model)
    : ModFilterProxyModel(parent, model)
{}

bool DisabledModsProxyModel::filterAccepts(const QModelIndex &sourceModelIndex) const
{
    return ModFilterProxyModel::filterAccepts(sourceModelIndex) &&
           sourceModel()->data(sourceModelIndex, ModDatabaseModel::ModRole::Exists).toBool() &&
           !sourceModel()->data(sourceModelIndex, ModDatabaseModel::ModRole::Enabled).toBool();
}
