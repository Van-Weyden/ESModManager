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

    ModDatabaseModel *model = qobject_cast<ModDatabaseModel *>(sourceModel());
    if (model->isCollection(sourceModelIndex)) {
        return model->collection(sourceModelIndex).hasModWithName(m_filter, true);
    }

    QString modName = model->data(sourceModelIndex, Qt::DisplayRole).toString();
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

Qt::CheckState ModFilterProxyModel::tristateValue(const QModelIndex &sourceModelIndex, int role) const
{
    return sourceModel()->data(sourceModelIndex, role).value<Qt::CheckState>();
}



EnabledModsProxyModel::EnabledModsProxyModel(QObject *parent, QAbstractItemModel *model)
    : ModFilterProxyModel(parent, model)
{}

bool EnabledModsProxyModel::filterAccepts(const QModelIndex &sourceModelIndex) const
{
    return ModFilterProxyModel::filterAccepts(sourceModelIndex) &&
           tristateValue(sourceModelIndex, ModDatabaseModel::Role::Exists) != Qt::Unchecked &&
           tristateValue(sourceModelIndex, ModDatabaseModel::Role::Enabled) != Qt::Unchecked;
}



DisabledModsProxyModel::DisabledModsProxyModel(QObject *parent, QAbstractItemModel *model)
    : ModFilterProxyModel(parent, model)
{}

bool DisabledModsProxyModel::filterAccepts(const QModelIndex &sourceModelIndex) const
{
    return ModFilterProxyModel::filterAccepts(sourceModelIndex) &&
           tristateValue(sourceModelIndex, ModDatabaseModel::Role::Exists) != Qt::Unchecked &&
           tristateValue(sourceModelIndex, ModDatabaseModel::Role::Enabled) != Qt::Checked;
}
