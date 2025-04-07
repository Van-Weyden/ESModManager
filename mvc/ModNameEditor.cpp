#include <QDebug>
#include <QKeyEvent>
#include <QScrollBar>

#include "ModNameEditor.h"

ModNameEditor::ModNameEditor(const QModelIndex &index, QWidget *parent)
    : QPlainTextEdit(parent)
    , m_index(index)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    document()->setDocumentMargin(0);
    setFrameShape(QFrame::NoFrame);

    connect(this, &QPlainTextEdit::textChanged, this, &ModNameEditor::onTextChanged);
}

QModelIndex ModNameEditor::index() const
{
    return m_index;
}

QString ModNameEditor::text() const
{
    return toPlainText();
}

void ModNameEditor::setText(const QString &text)
{
    if (text != this->text()) {
        setPlainText(text);
    }
}

void ModNameEditor::setWordWrap(bool wrap)
{
    setWordWrapMode(wrap ? QTextOption::WordWrap : QTextOption::NoWrap);
}

int ModNameEditor::bottomMargin() const
{
    return m_bottomMargin;
}

void ModNameEditor::setBottomMargin(int margin)
{
    m_bottomMargin = margin;
}

void ModNameEditor::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key::Key_Return:
            Q_FALLTHROUGH();
        case Qt::Key::Key_Enter:
            if (!(event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier))) {
                emit commitData(this);
                emit closeEditor(this, QAbstractItemDelegate::SubmitModelCache);
                return;
            }
        break;

        default:
        break;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void ModNameEditor::onTextChanged()
{
    QFontMetrics metrics(font());
    int flags = Qt::AlignLeft;
    if (wordWrapMode() == QTextOption::WordWrap) {
        flags |= Qt::TextWordWrap;
    }
    QRect textRect = metrics.boundingRect(rect(), flags, text());
    if (height() != textRect.height() + m_bottomMargin) {
        emit commitData(this);
        emit sizeHintChanged(m_index);
    }
}
