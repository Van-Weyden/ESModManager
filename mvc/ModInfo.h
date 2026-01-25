#pragma once

#include "AbstractModDatabaseItem.h"

#include <QSet>

class ModCollection;

class ModInfo : public AbstractModDatabaseItem
{
public:
    static bool isSteamId(QString str);
    static bool isNameValid(const QString &name);

    static QString generateUnknownNameStub();
    static QString generateFailedToGetNameStub();
    static QString generateWaitingForSteamResponseStub();

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

public:
    QString sourcesName;
    QString steamName;
    QString folderName;

private:
    bool m_enabled = true;
    bool m_locked = false;
    bool m_exists = true;
    QSet<ModCollection *> m_collections;
};
Q_DECLARE_METATYPE(ModInfo);
