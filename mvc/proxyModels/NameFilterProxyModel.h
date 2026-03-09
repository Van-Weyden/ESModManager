#pragma once

#include "BaseFilterProxyModel.h"

class NameFilterProxyModel : public BaseFilterProxyModel
{
    Q_OBJECT
public:
    NameFilterProxyModel(QObject *parent = nullptr, QAbstractItemModel *model = nullptr);
    ~NameFilterProxyModel() override = default;

    bool filterAccepts(const QModelIndex &sourceModelIndex) const override;

    QString nameFilter() const;

public slots:
    void setNameFilter(const QString &filter);

protected:
    Qt::CheckState tristateValue(const QModelIndex &sourceModelIndex, int role) const;

private:
    QString m_filter = "";
};
