#include "ModDatabaseModel.h"

#include "AutoExpandTreeView.h"

AutoExpandTreeView::AutoExpandTreeView(QWidget *parent)
    : QTreeView(parent)
{}

void AutoExpandTreeView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    QTreeView::dataChanged(topLeft, bottomRight, roles);

    if (roles.contains(ModDatabaseModel::Role::Expanded)) {
        syncExpansion(topLeft.row(), bottomRight.row());
    }
}

void AutoExpandTreeView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    QTreeView::rowsInserted(parent, start, end);

    if (!parent.isValid()) {
        syncExpansion(start, end);
    }
}

void AutoExpandTreeView::reset()
{
    QTreeView::reset();
    syncExpansion(0, model()->rowCount() - 1);
}

void AutoExpandTreeView::mouseReleaseEvent(QMouseEvent* event)
{
    // fix missing animation on items expanded by double click
    const bool restoreAnimation = (state() == QAbstractItemView::AnimatingState);
    QTreeView::mouseReleaseEvent(event);
    if (restoreAnimation) {
        setState(QAbstractItemView::AnimatingState);
    }
}

void AutoExpandTreeView::syncExpansion(int top, int bottom)
{
    for (int row = top; row <= bottom; ++row) {
        QModelIndex index = model()->index(row, 0);
        setExpanded(index, model()->data(index, ModDatabaseModel::Role::Expanded).toBool());
    }
}
