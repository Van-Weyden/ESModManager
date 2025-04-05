#include <functional>

#include <QCollator>
#include <QColor>
#include <QFont>
#include <QIcon>
#include <QThread>

#include "RegExpPatterns.h"
#include "SteamRequester.h"

#include "ModDatabaseModel.h"

//public:

ModDatabaseModel::ModDatabaseModel()
{
    m_completeModNames = true;
    m_useSteamModNames = true;

    qRegisterMetaType<QVector<int>>("QVector<int>");
}

bool ModDatabaseModel::isNameValid(const QString &name) const
{
    return !(
        name.isEmpty() ||
        name.contains(ModInfo::generateUnknownNameStub()) ||
        name.contains(ModInfo::generateFailedToGetNameStub()) ||
        name.contains(ModInfo::generateWaitingForSteamResponseStub()) ||
        QRegExp(RegExpPatterns::whitespace).exactMatch(name)
    );
}

void ModDatabaseModel::add(const ModInfo &modInfo)
{
    beginInsertRows(QModelIndex(), m_database.size(), m_database.size());
    m_database.append(modInfo);
    if (m_database.last().steamName.isEmpty()) {
        m_database.last().steamName = ModInfo::generateFailedToGetNameStub();
        //m_database.last().steamName = ModInfo::generateWaitingForSteamResponseStub();
    }

    endInsertRows();
}

QString ModDatabaseModel::displayedModName(const ModInfo &modInfo) const
{
    if (m_completeModNames) {
        bool isModNameValid = isNameValid(modInfo.name);
        bool isSteamModNameValid = isNameValid(modInfo.steamName);
        if (m_useSteamModNames) {
            if (isSteamModNameValid || !isModNameValid) {
                return modInfo.steamName;
            } else {
                return modInfo.name;
            }
        } else {
            if (isModNameValid || !isSteamModNameValid) {
                return modInfo.name;
            } else {
                return modInfo.steamName;
            }
        }
    } else {
        if (m_useSteamModNames) {
            return modInfo.steamName;
        } else {
            return modInfo.name;
        }
    }
}

QVariant ModDatabaseModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    switch (role) {
        case Qt::EditRole:
            Q_FALLTHROUGH();
        case Qt::DisplayRole:
            return displayedModName(m_database[index.row()]);
        break;

        case Qt::ForegroundRole:
            if (!m_database[index.row()].exists()) {
                return QColor(Qt::gray);
            }
        break;

        case Qt::CheckStateRole:
            return (m_database[index.row()].marked() ? Qt::Checked : Qt::Unchecked);
        break;

        case Qt::FontRole:
            if (m_database[index.row()].locked()) {
                QFont font;
                font.setBold(true);
                return font;
            }
        break;

        case Qt::DecorationRole:
            if (m_database[index.row()].locked()) {
                return QIcon(":/images/ui_outline/lock.svg");
            }
        break;

        case ModRole::Enabled:
            return m_database[index.row()].enabled();
        break;

        case ModRole::Exists:
            return m_database[index.row()].exists();
        break;

        case ModRole::Marked:
            return m_database[index.row()].marked();
        break;

        case ModRole::Locked:
            return m_database[index.row()].locked();
        break;

        default:
        break;
    }

    return QVariant();
}

Qt::ItemFlags ModDatabaseModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
}

QVariant ModDatabaseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            return tr("Mod name");
        } else {
            return QString::number(section + 1);
        }
    }

    return QVariant();
}

void ModDatabaseModel::removeFromDatabase(const QModelIndex &index)
{
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    m_database.removeAt(index.row());
    endRemoveRows();
}

void ModDatabaseModel::setCompleteModNames(const bool enabled)
{
    if (m_completeModNames == enabled) {
        return;
    }

    m_completeModNames = enabled;
    emit dataChanged(index(0), index(m_database.size() - 1));
}

bool ModDatabaseModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    bool isDataChanged = false;

    switch (role) {
        case Qt::EditRole:
            if (m_useSteamModNames) {
                m_database[index.row()].steamName = value.toString();
            } else {
                m_database[index.row()].name = value.toString();
            }
            isDataChanged = true;
        break;

        case Qt::CheckStateRole:
            m_database[index.row()].setMarked(Qt::CheckState(value.toInt()) == Qt::Checked);
            isDataChanged = true;
        break;

        case ModRole::Enabled:
            isDataChanged = (m_database[index.row()].enabled() != value.toBool());
            m_database[index.row()].setEnabled(value.toBool());
            isDataChanged &= (m_database[index.row()].enabled() == value.toBool());
        break;

        case ModRole::Exists:
            isDataChanged = (m_database[index.row()].exists() != value.toBool());
            m_database[index.row()].setExists(value.toBool());
        break;

        case ModRole::Marked:
            isDataChanged = (m_database[index.row()].marked() != value.toBool());
            m_database[index.row()].setMarked(value.toBool());
        break;

        case ModRole::Locked:
            isDataChanged = (m_database[index.row()].locked() != value.toBool());
            m_database[index.row()].setLocked(value.toBool());
        break;
    }

    if (isDataChanged) {
        emit dataChanged(index, index, QVector<int>(role));
    }

    return isDataChanged;
}

void ModDatabaseModel::sort(int column, Qt::SortOrder order)
{
    QCollator collator;
    collator.setNumericMode(true);
    std::function<bool(const ModInfo &i, const ModInfo &j)> comparator;

    switch (column) {
        case 0: //By display name
            comparator = [this, &collator](const ModInfo &i, const ModInfo &j) {
                return collator(displayedModName(i), displayedModName(j));
            };
        break;

        case 1: //By folder name
            comparator = [&collator](const ModInfo &i, const ModInfo &j) {
                return collator(i.folderName, j.folderName);
            };
        break;

        default:
        return;
    }

    beginResetModel();
    std::sort(m_database.begin(), m_database.end(),
        [&comparator, order](const ModInfo &i, const ModInfo &j) {
            return ((i.exists() ^ j.exists()) ? i.exists() : comparator(i, j) ^ (order == Qt::DescendingOrder));
        }
    );
    endResetModel();
}

void ModDatabaseModel::setModsExistsState(const bool isExists)
{
    for (int i = 0; i < m_database.size(); ++i) {
        m_database[i].setExists(isExists);
    }
}

void ModDatabaseModel::reset(std::function<void()> actions)
{
    beginResetModel();
    actions();
    endResetModel();
}

void ModDatabaseModel::setUsingSteamModNames(const bool use)
{
    if (m_useSteamModNames == use) {
        return;
    }

    m_useSteamModNames = use;
    emit dataChanged(index(0), index(m_database.size() - 1));
}

//public slots:

void ModDatabaseModel::enableMod(const QModelIndex &index)
{
    if (m_database[index.row()].canEnable()) {
        m_database[index.row()].setEnabled(true);
        emit dataChanged(index, index);
    }
}

void ModDatabaseModel::disableMod(const QModelIndex &index)
{
    if (m_database[index.row()].canDisable()) {
        m_database[index.row()].setEnabled(false);
        emit dataChanged(index, index);
    }
}
