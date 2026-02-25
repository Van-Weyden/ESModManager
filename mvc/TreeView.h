#pragma once

#include "AutoExpandTreeView.h"

class TreeView : public AutoExpandTreeView
{
    Q_OBJECT

public:
    TreeView(QWidget *parent = nullptr);

protected slots:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

private:
    QItemSelection childrenWithWrongSelection(bool isParentSelected) const;
};
