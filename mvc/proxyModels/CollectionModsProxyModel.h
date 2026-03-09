#pragma once

#include "NameFilterProxyModel.h"

class ModCollection;

class CollectionModsProxyModel : public NameFilterProxyModel
{
public:
    explicit CollectionModsProxyModel(QObject *parent = nullptr, QAbstractItemModel *model = nullptr);

private:
    ModCollection *m_collection = nullptr;
};

