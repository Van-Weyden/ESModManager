#pragma once

#include "ExistsModProxyModel.h"

class DisabledModProxyModel : public ExistsModProxyModel
{
public:
    DisabledModProxyModel(QObject *parent = nullptr, QAbstractItemModel *model = nullptr);
    bool filterAccepts(const QModelIndex &sourceModelIndex) const override;
};
