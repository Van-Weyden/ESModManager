#include "../ModDatabaseModel.h"

#include "EnabledModProxyModel.h"

EnabledModProxyModel::EnabledModProxyModel(QObject *parent, QAbstractItemModel *model)
    : ExistsModProxyModel(parent, model)
{}

bool EnabledModProxyModel::filterAccepts(const QModelIndex &sourceModelIndex) const
{
    return ExistsModProxyModel::filterAccepts(sourceModelIndex) &&
           tristateValue(sourceModelIndex, ModDatabaseModel::Role::Enabled) != Qt::Unchecked;
}
