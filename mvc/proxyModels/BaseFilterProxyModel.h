#pragma once

#include <functional>

#include <QSortFilterProxyModel>

class BaseFilterProxyModel : public QSortFilterProxyModel
{
public:
    using FilterType = std::function<bool(QAbstractItemModel *, const QModelIndex &)>;
public:
    explicit BaseFilterProxyModel(QObject *parent = nullptr, QAbstractItemModel *model = nullptr);
    ~BaseFilterProxyModel() override = default;

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    virtual bool filterAccepts(const QModelIndex &sourceModelIndex) const;

public slots:
    int addFilter(const FilterType &filter);
    void removeFilter(int index);

private:
    QList<FilterType> m_filters;
};

