#include "../ModDatabaseModel.h"

#include "NameFilterProxyModel.h"

NameFilterProxyModel::NameFilterProxyModel(QObject *parent, QAbstractItemModel *model)
    : BaseFilterProxyModel(parent, model)
{}

bool NameFilterProxyModel::filterAccepts(const QModelIndex &sourceModelIndex) const
{
    if (!BaseFilterProxyModel::filterAccepts(sourceModelIndex)) {
        return false;
    }

    if (m_filter.isEmpty()) {
        return true;
    }

    QString itemName = sourceModel()->data(sourceModelIndex, Qt::DisplayRole).toString();
    return itemName.contains(m_filter, Qt::CaseSensitivity::CaseInsensitive);
}

QString NameFilterProxyModel::nameFilter() const
{
    return m_filter;
}

void NameFilterProxyModel::setNameFilter(const QString &filter)
{
    if (m_filter != filter) {
        beginResetModel();
        m_filter = filter;
        endResetModel();
    }
}

Qt::CheckState NameFilterProxyModel::tristateValue(const QModelIndex &sourceModelIndex, int role) const
{
    return sourceModel()->data(sourceModelIndex, role).value<Qt::CheckState>();
}
