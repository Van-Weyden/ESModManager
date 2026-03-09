#pragma once

#include "ExistsModProxyModel.h"

class EnabledModProxyModel : public ExistsModProxyModel
{
public:
    EnabledModProxyModel(QObject *parent = nullptr, QAbstractItemModel *model = nullptr);
    bool filterAccepts(const QModelIndex &sourceModelIndex) const override;
};
