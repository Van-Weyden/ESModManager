#include <QCoreApplication>
#include <QJsonObject>
#include <QRegExpValidator>
#include <QTranslator>

#include "utils/RegExpPatterns.h"

#include "ModInfo.h"

QTranslator *ModInfo::s_translator = nullptr;
QString ModInfo::s_unknownNameStub = QCoreApplication::tr(UnknownNameStub);
QString ModInfo::s_failedToGetNameStub = QCoreApplication::tr(FailedToGetNameStub);
QString ModInfo::s_waitingForSteamResponseStub = QCoreApplication::tr(WaitingForSteamResponseStub);

bool ModInfo::isSteamId(QString str)
{
    int pos = 0;
    return !(QRegExpValidator(QRegExp("[0-9]*")).validate(str, pos) == QValidator::State::Invalid);
}

bool ModInfo::isNameValid(const QString &name)
{
    return isNameValid(name, s_translator);
}

QString ModInfo::unknownNameStub()
{
    return s_unknownNameStub;
}

QString ModInfo::failedToGetNameStub()
{
    return s_failedToGetNameStub;
}

QString ModInfo::waitingForSteamResponseStub()
{
    return s_waitingForSteamResponseStub;
}

void ModInfo::setTranslator(QTranslator *translator)
{
    s_translator = translator;
    s_unknownNameStub = generateUnknownNameStub(s_translator);
    s_failedToGetNameStub = generateFailedToGetNameStub(s_translator);
    s_waitingForSteamResponseStub = generateWaitingForSteamResponseStub(s_translator);
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
    sourcesName = object["sources_name"].toString();
    folderName  = object["folder_name"].toString();
    steamName   = object["steam_name"].toString();
    m_enabled   = object["enabled"].toBool(true);
    m_locked    = object["locked"].toBool(false);
}

void ModInfo::toJsonObject(QJsonObject &object) const
{
    AbstractModDatabaseItem::toJsonObject(object);
    object["sources_name"] = sourcesName;
    object["folder_name"]  = folderName;
    object["steam_name"]   = steamName;
    object["enabled"]      = m_enabled;
    object["locked"]       = m_locked;
}

void ModInfo::updateStubs(QTranslator *newTranslator)
{
    if (!isNameValid(name()) || !isNameValid(name(), newTranslator)) {
        setName("");
    }

    if (sourcesName == unknownNameStub()) {
        sourcesName = newTranslator->translate(StubsContext, UnknownNameStub);
    } else if (sourcesName == failedToGetNameStub()) {
        sourcesName = newTranslator->translate(StubsContext, FailedToGetNameStub);
    } else if (sourcesName == waitingForSteamResponseStub()) {
        sourcesName = newTranslator->translate(StubsContext, WaitingForSteamResponseStub);
    }

    if (steamName == unknownNameStub()) {
        steamName = newTranslator->translate(StubsContext, UnknownNameStub);
    } else if (steamName == failedToGetNameStub()) {
        steamName = newTranslator->translate(StubsContext, FailedToGetNameStub);
    } else if (steamName == waitingForSteamResponseStub()) {
        steamName = newTranslator->translate(StubsContext, WaitingForSteamResponseStub);
    }
}

// private:

bool ModInfo::isNameValid(const QString &name, QTranslator *translator)
{
    return !(
        name.isEmpty() ||
        name == generateUnknownNameStub(translator) ||
        name == generateFailedToGetNameStub(translator) ||
        name == generateWaitingForSteamResponseStub(translator) ||
        QRegExp(RegExpPatterns::whitespace).exactMatch(name)
    );
}

QString ModInfo::generateUnknownNameStub(QTranslator *translator)
{
    return translator ? translator->translate(StubsContext, UnknownNameStub)
                      : QCoreApplication::tr(UnknownNameStub);
}

QString ModInfo::generateFailedToGetNameStub(QTranslator *translator)
{
    return translator ? translator->translate(StubsContext, FailedToGetNameStub)
                      : QCoreApplication::tr(FailedToGetNameStub);
}

QString ModInfo::generateWaitingForSteamResponseStub(QTranslator *translator)
{
    return translator ? translator->translate(StubsContext, WaitingForSteamResponseStub)
                      : QCoreApplication::tr(WaitingForSteamResponseStub);
}
