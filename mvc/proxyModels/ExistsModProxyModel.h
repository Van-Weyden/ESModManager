#pragma once

#include "ExcludedAllModsProxyModel.h"

class ExistsModProxyModel : public ExcludedAllModsProxyModel
{
public:
    ExistsModProxyModel(QObject *parent = nullptr, QAbstractItemModel *model = nullptr);
    bool filterAccepts(const QModelIndex &sourceModelIndex) const override;
};
