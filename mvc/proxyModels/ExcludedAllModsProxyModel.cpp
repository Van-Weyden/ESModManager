#include "../ModDatabaseModel.h"

#include "ExcludedAllModsProxyModel.h"

ExcludedAllModsProxyModel::ExcludedAllModsProxyModel(QObject *parent, QAbstractItemModel *model)
    : NameFilterProxyModel(parent, model)
{}

bool ExcludedAllModsProxyModel::filterAccepts(const QModelIndex &sourceModelIndex) const
{
    if (sourceModelIndex.parent().isValid() && sourceModelIndex.parent().row() == 0) {
        return false;
    } else if (!sourceModelIndex.parent().isValid() && sourceModelIndex.row() == 0) {
        return false;
    }

    ModDatabaseModel *model = qobject_cast<ModDatabaseModel *>(sourceModel());
    if (model->isCollection(sourceModelIndex)) {
        return model->collection(sourceModelIndex).hasModWithName(nameFilter(), true);
    }

    return NameFilterProxyModel::filterAccepts(sourceModelIndex);
}
