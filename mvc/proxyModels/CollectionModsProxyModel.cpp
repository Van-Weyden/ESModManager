#include "../ModDatabaseModel.h"

#include "CollectionModsProxyModel.h"

CollectionModsProxyModel::CollectionModsProxyModel(QObject *parent, QAbstractItemModel *model)
    : NameFilterProxyModel(parent, model)
{}
