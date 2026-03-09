#include "BaseFilterProxyModel.h"

BaseFilterProxyModel::BaseFilterProxyModel(QObject *parent, QAbstractItemModel *model)
    : QSortFilterProxyModel(parent)
{
    if (model) {
        setSourceModel(model);
    }
}

bool BaseFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!index.isValid()) {
        return false;
    }

    return filterAccepts(index);
}

bool BaseFilterProxyModel::filterAccepts(const QModelIndex &sourceModelIndex) const
{
    for (const FilterType &filter : qAsConst(m_filters)) {
        if (!filter(sourceModel(), sourceModelIndex)) {
            return false;
        }
    }

    return true;
}

int BaseFilterProxyModel::addFilter(const FilterType &filter)
{
    beginResetModel();
    m_filters.append(filter);
    endResetModel();
    return m_filters.size() - 1;
}

void BaseFilterProxyModel::removeFilter(int index)
{
    m_filters.removeAt(index);
}
