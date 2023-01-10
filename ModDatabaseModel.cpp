#include <QCollator>
#include <QColor>
#include <QThread>

#include "RegExpPatterns.h"
#include "SteamRequester.h"

#include "ModDatabaseModel.h"

//public:

ModDatabaseModel::ModDatabaseModel()
{
    m_completeModNames = true;
    m_useSteamModNames = true;

    qRegisterMetaType<QVector<int> >("QVector<int>");
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

QVariant ModDatabaseModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role) {
        case Qt::DisplayRole:
            if (m_completeModNames) {
                bool isModNameValid = isNameValid(m_database.at(index.row()).name);
                bool isSteamModNameValid = isNameValid(m_database.at(index.row()).steamName);
                if (m_useSteamModNames) {
                    if (isSteamModNameValid || !isModNameValid) {
                        return m_database.at(index.row()).steamName;
                    } else {
                        return m_database.at(index.row()).name;
                    }
                } else {
                    if (isModNameValid || !isSteamModNameValid) {
                        return m_database.at(index.row()).name;
                    } else {
                        return m_database.at(index.row()).steamName;
                    }
                }
            } else {
                if (m_useSteamModNames) {
                    return m_database.at(index.row()).steamName;
                } else {
                    return m_database.at(index.row()).name;
                }
            }
        break;

        case Qt::ForegroundRole:
            if (!m_database.at(index.row()).exists) {
                return QColor(Qt::gray);
            }
        break;

        case Qt::CheckStateRole:
            if (index.column() == 0) {
                return (m_database.at(index.row()).enabled ? Qt::Checked : Qt::Unchecked);
            }
        break;

        default:
        break;
    }

    return QVariant();
}

Qt::ItemFlags ModDatabaseModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (index.column() == 0) {
        flags |= Qt::ItemIsUserCheckable;
    }
    return flags;
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

void ModDatabaseModel::removeFromDatabase(const int index)
{
    beginRemoveRows(QModelIndex(), index, index);
    m_database.removeAt(index);
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
        case Qt::DisplayRole:
            if (m_useSteamModNames) {
                m_database[index.row()].steamName = value.toString();
            } else {
                m_database[index.row()].name = value.toString();
            }
            isDataChanged = true;
        break;

        case Qt::CheckStateRole:
            if (index.column() == 0) {
                m_database[index.row()].enabled = (Qt::CheckState(value.toInt()) == Qt::Checked);
                emit modCheckStateChanged(index.row(), m_database.at(index.row()).enabled);
                isDataChanged = true;
            }
        break;
    }

    if (isDataChanged) {
        emit dataChanged(index, index, QVector<int>(role));
    }

    return isDataChanged;
}

void ModDatabaseModel::setModsExistsState(const bool isExists)
{
    for (int i = 0; i < m_database.size(); ++i) {
        m_database[i].exists = isExists;
    }
}

void ModDatabaseModel::setUsingSteamModNames(const bool use)
{
    if (m_useSteamModNames == use) {
        return;
    }

    m_useSteamModNames = use;
    emit dataChanged(index(0), index(m_database.size() - 1));
}

void ModDatabaseModel::sortDatabase()
{
    //Names sorting (order like in Windows explorer)
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(m_database.begin(), m_database.end(),
        [&collator](const ModInfo &i, const ModInfo &j) {
            return ((i.exists ^ j.exists) ? i.exists : collator(i.folderName, j.folderName));
        }
    );
}

//public slots:

void ModDatabaseModel::enableMod(const QModelIndex &index)
{
    if (!m_database.at(index.row()).enabled) {
        m_database[index.row()].enabled = true;
        emit dataChanged(index, index);
        emit modCheckStateChanged(index.row(), true);
    }
}

void ModDatabaseModel::disableMod(const QModelIndex &index)
{
    if (m_database.at(index.row()).enabled) {
        m_database[index.row()].enabled = false;
        emit dataChanged(index, index);
        emit modCheckStateChanged(index.row(), false);
    }
}

//protected slots:

void ModDatabaseModel::setCompleteModNames(const int mode)
{
    if (mode == Qt::CheckState::Checked) {
        setCompleteModNames(true);
    } else if (mode == Qt::CheckState::Unchecked) {
        setCompleteModNames(false);
    }
}

void ModDatabaseModel::setUsingSteamModNames(const int mode)
{
    if (mode == Qt::CheckState::Checked) {
        setUsingSteamModNames(true);
    } else if (mode == Qt::CheckState::Unchecked) {
        setUsingSteamModNames(false);
    }
}
