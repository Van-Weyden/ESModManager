#pragma once

#include <QSortFilterProxyModel>

class ModFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    ModFilterProxyModel(QObject *parent = nullptr, QAbstractItemModel *model = nullptr);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    virtual bool filterAccepts(const QModelIndex &sourceModelIndex) const;

    QString filter() const;

public slots:
    void setFilter(const QString &filter);

protected:
    Qt::CheckState tristateValue(const QModelIndex &sourceModelIndex, int role) const;

private:
    QString m_filter = "";
};

class EnabledModsProxyModel : public ModFilterProxyModel
{
public:
    EnabledModsProxyModel(QObject *parent = nullptr, QAbstractItemModel *model = nullptr);
    bool filterAccepts(const QModelIndex &sourceModelIndex) const override;
};

class DisabledModsProxyModel : public ModFilterProxyModel
{
public:
    DisabledModsProxyModel(QObject *parent = nullptr, QAbstractItemModel *model = nullptr);
    bool filterAccepts(const QModelIndex &sourceModelIndex) const override;
};
