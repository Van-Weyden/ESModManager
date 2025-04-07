#pragma once

#include <QWidget>

#include "mvc/ModDatabaseModel.h"

namespace Ui {
class ModInfoWidget;
}

class ModInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ModInfoWidget(QWidget *parent = nullptr);
    ~ModInfoWidget();

    void setIndex(const QModelIndex &index);
    void setModel(ModDatabaseModel *model);
    void clear();

public slots:
    void setEnabled(bool enabled);

signals:
    void nameChanged(const QString &name);
    void openFolder(const QModelIndex &modIndex);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    Ui::ModInfoWidget *ui = nullptr;
    ModDatabaseModel *m_model = nullptr;
    QModelIndex m_modIndex;
};

