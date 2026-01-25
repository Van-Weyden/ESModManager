#pragma once

#include <QTreeView>

class AutoExpandTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit AutoExpandTreeView(QWidget *parent = nullptr);

    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>()) override;
    void rowsInserted(const QModelIndex &parent, int start, int end) override;
    void reset() override;

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void syncExpansion(int top, int bottom);
};

