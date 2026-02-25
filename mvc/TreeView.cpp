#include "TreeView.h"

TreeView::TreeView(QWidget *parent)
    : AutoExpandTreeView(parent)
{}

void TreeView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    if (!selectionModel()) {
        QTreeView::selectionChanged(selected, deselected);
        return;
    }

    QItemSelection childrenToSelect = childrenWithWrongSelection(true);
    if (!childrenToSelect.isEmpty()) {
        selectionModel()->select(childrenToSelect, QItemSelectionModel::Select);
    } else {
        QTreeView::selectionChanged(selected, deselected);
    }
}

QItemSelection TreeView::childrenWithWrongSelection(bool isParentSelected) const
{
    QItemSelection children;
    for (int parentRow = 0; parentRow < model()->rowCount(); ++parentRow) {
        QModelIndex parent = model()->index(parentRow, 0);
        if (selectionModel()->isSelected(parent) != isParentSelected) {
            continue;
        }

        QModelIndex top = QModelIndex();
        QModelIndex bottom = QModelIndex();
        int rowCount = model()->rowCount(parent);
        for (int row = 0; row < rowCount; ++row) {
            QModelIndex child = model()->index(row, 0, parent);
            if (selectionModel()->isSelected(child) != isParentSelected) {
                if (!top.isValid()) {
                    top = bottom = child;
                } else if (bottom.row() == row - 1) {
                    bottom = child;
                } else {
                    children.append({top, bottom});
                }
            }
        }
        if (top.isValid()) {
            children.append({top, bottom});
        }
    }

    return children;
}
