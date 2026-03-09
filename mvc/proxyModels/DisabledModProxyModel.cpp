#include "../ModDatabaseModel.h"

#include "DisabledModProxyModel.h"

DisabledModProxyModel::DisabledModProxyModel(QObject *parent, QAbstractItemModel *model)
    : ExistsModProxyModel(parent, model)
{}

bool DisabledModProxyModel::filterAccepts(const QModelIndex &sourceModelIndex) const
{
    return ExistsModProxyModel::filterAccepts(sourceModelIndex) &&
           tristateValue(sourceModelIndex, ModDatabaseModel::Role::Enabled) != Qt::Checked;
}
