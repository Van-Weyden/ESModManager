#include "../ModDatabaseModel.h"

#include "ExistsModProxyModel.h"

ExistsModProxyModel::ExistsModProxyModel(QObject *parent, QAbstractItemModel *model)
    : ExcludedAllModsProxyModel(parent, model)
{}

bool ExistsModProxyModel::filterAccepts(const QModelIndex &sourceModelIndex) const
{
    return ExcludedAllModsProxyModel::filterAccepts(sourceModelIndex) &&
           tristateValue(sourceModelIndex, ModDatabaseModel::Role::Exists) == Qt::Checked;
}
