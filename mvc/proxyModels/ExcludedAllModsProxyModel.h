#pragma once

#include "NameFilterProxyModel.h"

class ExcludedAllModsProxyModel : public NameFilterProxyModel
{
public:
    ExcludedAllModsProxyModel(QObject *parent = nullptr, QAbstractItemModel *model = nullptr);
    bool filterAccepts(const QModelIndex &sourceModelIndex) const override;
};
