#pragma once

#include <QAbstractItemDelegate>
#include <QPlainTextEdit>

class ModNameEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    ModNameEditor(const QModelIndex &index = QModelIndex(), QWidget *parent = nullptr);

    QModelIndex index() const;

    QString text() const;
    void setText(const QString &text);
    void setWordWrap(bool wrap);

    int bottomMargin() const;
    void setBottomMargin(int margin);

signals:
    void sizeHintChanged(const QModelIndex &index);
    void commitData(QWidget *editor);
    void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onTextChanged();

private:
    QModelIndex m_index;
    int m_bottomMargin = 0;
};

