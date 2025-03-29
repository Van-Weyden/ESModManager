#include <QDebug>
#include <QDesktopServices>
#include <QKeyEvent>

#include "ModInfoWidget.h"
#include "ui_ModInfoWidget.h"

ModInfoWidget::ModInfoWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ModInfoWidget)
{
    ui->setupUi(this);

    connect(ui->modNameLineEdit, &QLineEdit::editingFinished, this, [this]() {
        if (!m_model || !m_modIndex.isValid()) {
            return;
        }
        ModInfo &info = m_model->modInfoRef(m_modIndex);
        if (info.name != ui->modNameLineEdit->text()) {
            info.name = ui->modNameLineEdit->text();
            m_model->updateRow(m_modIndex);
        }
    });

    connect(ui->lockedCheckBox, &QCheckBox::clicked, this, [this]() {
        if (!m_model || !m_modIndex.isValid()) {
            return;
        }
        ModInfo &info = m_model->modInfoRef(m_modIndex);
        if (info.locked() != ui->lockedCheckBox->isChecked()) {
            info.setLocked(ui->lockedCheckBox->isChecked());
            m_model->updateRow(m_modIndex);
        }
    });

    connect(ui->markedCheckBox, &QCheckBox::clicked, this, [this]() {
        if (!m_model || !m_modIndex.isValid()) {
            return;
        }
        ModInfo &info = m_model->modInfoRef(m_modIndex);
        if (info.marked() != ui->markedCheckBox->isChecked()) {
            info.setMarked(ui->markedCheckBox->isChecked());
            m_model->updateRow(m_modIndex);
        }
    });

    connect(ui->openModFolderPushButton, &QPushButton::clicked, this, [this]() {
        if (!m_model || !m_modIndex.isValid()) {
            return;
        }
        emit openFolder(m_modIndex);
    });

    connect(ui->openWorkshopPushButton, &QPushButton::clicked, this, [this]() {
        if (!m_model || !m_modIndex.isValid()) {
            return;
        }
        QDesktopServices::openUrl(QUrl(
            (ui->openWorkshopWithSteamCheckBox->isChecked()
                 ? "steam://url/CommunityFilePage/"
                 : "https://steamcommunity.com/sharedfiles/filedetails/?id="
            ) + m_model->modInfo(m_modIndex).folderName
        ));
    });
}

ModInfoWidget::~ModInfoWidget()
{
    delete ui;
}

void ModInfoWidget::setIndex(const QModelIndex &index)
{
    if (!m_model || !index.isValid()) {
        return;
    }

    m_modIndex = QModelIndex();
    const ModInfo &info = m_model->modInfo(index);
    ui->openModFolderPushButton->setEnabled(info.exists());
    ui->modNameLineEdit->setText(info.name);
    ui->modFolderNameLineEdit->setText(info.folderName);
    ui->steamModNameLineEdit->setText(info.steamName);
    ui->lockedCheckBox->setChecked(info.locked());
    ui->markedCheckBox->setChecked(info.marked());
    ui->openModFolderPushButton->setEnabled(info.exists());
    ui->openWorkshopPushButton->setEnabled(ModInfo::isSteamId(info.folderName));
    m_modIndex = index;
}

void ModInfoWidget::setModel(ModDatabaseModel *model)
{
    m_model = model;
}

void ModInfoWidget::clear()
{
    m_modIndex = QModelIndex();
    ui->modNameLineEdit->clear();
    ui->modFolderNameLineEdit->clear();
    ui->steamModNameLineEdit->clear();
    ui->lockedCheckBox->setChecked(false);
    ui->markedCheckBox->setChecked(false);
}

void ModInfoWidget::setEnabled(bool enabled)
{
    QWidget::setEnabled(enabled);
    if (!m_model || !m_modIndex.isValid()) {
        return;
    }

    ui->openModFolderPushButton->setEnabled(m_model->modIsExists(m_modIndex));
    ui->openWorkshopPushButton->setEnabled(ModInfo::isSteamId(m_model->modFolderName(m_modIndex)));
}

void ModInfoWidget::keyPressEvent(QKeyEvent *event)
{
    QLineEdit *focus = qobject_cast<QLineEdit *>(focusWidget());
    if (!event || focus != ui->modNameLineEdit) {
        QWidget::keyPressEvent(event);
        return;
    }

    if (!m_model || !m_modIndex.isValid()) {
        QWidget::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
        case Qt::Key_Escape:
            ui->modNameLineEdit->setText(m_model->modName(m_modIndex));
            Q_FALLTHROUGH();
        case Qt::Key_Enter:
            Q_FALLTHROUGH();
        case Qt::Key_Return:
            ui->modNameLineEdit->clearFocus();
            break;
    }

    QWidget::keyPressEvent(event);
}
