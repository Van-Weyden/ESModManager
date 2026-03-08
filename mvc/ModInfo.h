#pragma once

#include <QSet>
#include <QtGlobal>

#include "AbstractModDatabaseItem.h"

class QTranslator;

class ModCollection;

class ModInfo : public AbstractModDatabaseItem
{
    constexpr static const char* StubsContext = "ModInfoStubs";
    constexpr static const char* UnknownNameStub = QT_TRANSLATE_NOOP(
        "ModInfoStubs",
        "WARNING: unknown mod name. Set the name manually."
    );
    constexpr static const char* FailedToGetNameStub = QT_TRANSLATE_NOOP(
        "ModInfoStubs",
        "WARNING: couldn't get the name of the mod. Set the name manually."
    );
    constexpr static const char* WaitingForSteamResponseStub = QT_TRANSLATE_NOOP(
        "ModInfoStubs",
        "Waiting for Steam mod name response..."
    );

public:
    static bool isSteamId(QString str);
    static bool isNameValid(const QString &name);
    static QString unknownNameStub();
    static QString failedToGetNameStub();
    static QString waitingForSteamResponseStub();

    static void setTranslator(QTranslator *translator);

public:
    ModInfo(const QString &name = QString());
    ModInfo(const QJsonObject &object);
    ~ModInfo() override = default;

    const QString &displayedName() const override;
    Type type() const override;
    Qt::CheckState exists() const override;
    Qt::CheckState enabled() const override;
    Qt::CheckState locked() const override;

    bool setEnabled(const bool enabled, const bool force = false) override;
    bool setExists(const bool exists) override;
    bool setLocked(const bool locked) override;

    const QSet<ModCollection *> &collections() const;
    QSet<ModCollection *> &collectionsRef();

    void fromJsonObject(const QJsonObject &object) override;
    void toJsonObject(QJsonObject &object) const override;

    void updateStubs(QTranslator *newTranslator);

private:
    static bool isNameValid(const QString &name, QTranslator *translator);
    static QString generateUnknownNameStub(QTranslator *translator);
    static QString generateFailedToGetNameStub(QTranslator *translator);
    static QString generateWaitingForSteamResponseStub(QTranslator *translator);

public:
    QString sourcesName;
    QString steamName;
    QString folderName;

private:
    bool m_enabled = true;
    bool m_locked = false;
    bool m_exists = true;
    QSet<ModCollection *> m_collections;

    static QTranslator *s_translator;
    static QString s_unknownNameStub;
    static QString s_failedToGetNameStub;
    static QString s_waitingForSteamResponseStub;
};
Q_DECLARE_METATYPE(ModInfo);
