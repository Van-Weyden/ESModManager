#include <QCoreApplication>
#include <QJsonObject>
#include <QRegExpValidator>

#include "utils/RegExpPatterns.h"

#include "ModInfo.h"

bool ModInfo::isSteamId(QString str)
{
    int pos = 0;
    return !(QRegExpValidator(QRegExp("[0-9]*")).validate(str, pos) == QValidator::State::Invalid);
}

bool ModInfo::isNameValid(const QString &name)
{
    return !(
        name.isEmpty() ||
        name.contains(ModInfo::generateUnknownNameStub()) ||
        name.contains(ModInfo::generateFailedToGetNameStub()) ||
        name.contains(ModInfo::generateWaitingForSteamResponseStub()) ||
        QRegExp(RegExpPatterns::whitespace).exactMatch(name)
    );
}

QString ModInfo::generateUnknownNameStub()
{
    return QCoreApplication::tr("WARNING: unknown mod name. Set the name manually.");
}

QString ModInfo::generateFailedToGetNameStub()
{
    return QCoreApplication::tr("WARNING: couldn't get the name of the mod. Set the name manually.");
}

QString ModInfo::generateWaitingForSteamResponseStub()
{
    return QCoreApplication::tr("Waiting for Steam mod name response...");
}

ModInfo::ModInfo(const QString &name)
    : AbstractModDatabaseItem(name)
{}

ModInfo::ModInfo(const QJsonObject &object)
{
    ModInfo::fromJsonObject(object);
}

const QString &ModInfo::displayedName() const
{
    const QString &customName = AbstractModDatabaseItem::displayedName();
    if (!customName.isEmpty()) {
        return customName;
    }

    if (isNameValid(steamName) || !isNameValid(sourcesName)) {
        return steamName;
    } else {
        return sourcesName;
    }
}

AbstractModDatabaseItem::Type ModInfo::type() const
{
    return Type::Mod;
}

Qt::CheckState ModInfo::exists() const
{
    return m_exists ? Qt::Checked : Qt::Unchecked;
}

Qt::CheckState ModInfo::enabled() const
{
    return m_enabled ? Qt::Checked : Qt::Unchecked;
}

Qt::CheckState ModInfo::locked() const
{
    return m_locked ? Qt::Checked : Qt::Unchecked;
}

bool ModInfo::setEnabled(const bool enabled, const bool force)
{
    bool changed = false;
    if (!m_locked || force) {
        changed = (m_enabled != enabled);
        m_enabled = enabled;
    }
    return changed;
}

bool ModInfo::setExists(const bool exists)
{
    bool changed = (m_exists != exists);
    m_exists = exists;
    return changed;
}

bool ModInfo::setLocked(const bool locked)
{
    bool changed = (m_locked != locked);
    m_locked = locked;
    return changed;
}

const QSet<ModCollection *> &ModInfo::collections() const
{
    return m_collections;
}

QSet<ModCollection *> &ModInfo::collectionsRef()
{
    return m_collections;
}

void ModInfo::fromJsonObject(const QJsonObject &object)
{
    AbstractModDatabaseItem::fromJsonObject(object);
    sourcesName = object["Sources name"].toString();
    folderName  = object["Folder name"].toString();
    steamName   = object["Steam name"].toString();
    m_enabled   = object["Enabled"].toBool(true);
    m_locked    = object["Locked"].toBool(false);
}

void ModInfo::toJsonObject(QJsonObject &object) const
{
    AbstractModDatabaseItem::toJsonObject(object);
    object["Sources name"] = sourcesName;
    object["Folder name"]  = folderName;
    object["Steam name"]   = steamName;
    object["Enabled"]      = m_enabled;
    object["Locked"]       = m_locked;
}
