#include <QCoreApplication>
#include <QJsonObject>
#include <QRegExpValidator>

#include "ModInfo.h"

bool ModInfo::isSteamId(QString str)
{
    int pos = 0;
    return !(QRegExpValidator(QRegExp("[0-9]*")).validate(str, pos) == QValidator::State::Invalid);
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

ModInfo::ModInfo(const QJsonObject &object)
{
    fromJsonObject(object);
}

void ModInfo::setMarked(const bool marked)
{
    m_marked = marked;
}

void ModInfo::setExists(const bool exists)
{
    m_exists = exists;
}

void ModInfo::setEnabled(const bool enabled, const bool force)
{
    if (!m_locked || force) {
        m_enabled = enabled;
    }
}

void ModInfo::setLocked(const bool locked)
{
    m_locked = locked;
}

void ModInfo::fromJsonObject(const QJsonObject &object)
{
    name       = object["Name"].toString();
    folderName = object["Folder name"].toString();
    steamName  = object["Steam name"].toString();
    m_enabled  = object["Enabled"].toBool(true);
    m_locked   = object["Locked"].toBool(false);
    m_marked   = object["Marked"].toBool(false);
}

void ModInfo::toJsonObject(QJsonObject &object) const
{
    object["Name"]        = name;
    object["Folder name"] = folderName;
    object["Steam name"]  = steamName;
    object["Enabled"]     = m_enabled;
    object["Locked"]      = m_locked;
    object["Marked"]      = m_marked;
}

QJsonObject ModInfo::toJsonObject() const
{
    QJsonObject object;
    toJsonObject(object);
    return object;
}
