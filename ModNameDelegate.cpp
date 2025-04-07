#include <QApplication>
#include <QDebug>
#include <QPainter>

#include "ModNameEditor.h"

#include "ModNameDelegate.h"

ModNameDelegate::ModNameDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    connect(this, &ModNameDelegate::closeEditor, this, [this](QWidget* editorWidget, EndEditHint hint) {
        if (hint == EndEditHint::RevertModelCache) {
            ModNameEditor *editor = qobject_cast<ModNameEditor*>(editorWidget);
            emit sizeHintChanged(editor->index());
        }
    });
}

QWidget *ModNameDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const
{
    ModNameEditor* editor = new ModNameEditor(index, parent);
    editor->setBottomMargin(m_bottomMargin);
    editor->setWordWrap(option.features & QStyleOptionViewItem::WrapText);
    connect(editor, &ModNameEditor::sizeHintChanged, this, &ModNameDelegate::sizeHintChanged);
    connect(editor, &ModNameEditor::commitData, this, &ModNameDelegate::commitData);
    connect(editor, &ModNameEditor::closeEditor, this, &ModNameDelegate::closeEditor);
    return editor;
}

void ModNameDelegate::setEditorData(QWidget *editorWidget, const QModelIndex &index) const
{
    QString value = index.model()->data(index, Qt::EditRole).toString();
    ModNameEditor *editor = qobject_cast<ModNameEditor*>(editorWidget);
    editor->setText(value);
}

void ModNameDelegate::setModelData(QWidget *editorWidget, QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    ModNameEditor *editor = qobject_cast<ModNameEditor*>(editorWidget);
    model->setData(index, editor->text(), Qt::EditRole);
}

QSize ModNameDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect sizeRect = {0, 0, option.widget->width(), 1};
    if (option.textElideMode == Qt::ElideNone)
    {
        sizeRect.setWidth(sizeRect.width() - 50);  // Reduce width to avoid text eliding
    }

    int flags = Qt::AlignLeft;
    if (option.features & QStyleOptionViewItem::WrapText) {
        flags |= Qt::TextWordWrap;
    }

    QFontMetrics metrics(option.font);
    QString text = index.model()->data(index, Qt::EditRole).toString();
    QRect textRect = metrics.boundingRect(sizeRect, flags, text);
    sizeRect.setHeight(textRect.height() + m_bottomMargin);
    return sizeRect.size();
}

void ModNameDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    painter->save();
    painter->setPen(QColor(229, 229, 229));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    painter->restore();
}

void ModNameDelegate::updateEditorGeometry(QWidget *editorWidget, const QStyleOptionViewItem &option,
                                           const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.showDecorationSelected = true;

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    QRect geometry = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
    geometry.setX(geometry.x() + 3);
    editorWidget->setGeometry(geometry);
}
