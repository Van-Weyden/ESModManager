#pragma once

#include <QModelIndex>

class AbstractModDatabaseItem
{
public:
    enum Type
    {
        Collection,
        Mod
    };

public:
    AbstractModDatabaseItem(const QString &name = QString());
    virtual ~AbstractModDatabaseItem() = default;

    const QString &name() const;
    virtual const QString &displayedName() const;
    virtual Type type() const = 0;
    virtual Qt::CheckState enabled() const = 0;
    virtual Qt::CheckState exists() const = 0;
    virtual Qt::CheckState locked() const = 0;
    bool canEnable() const;
    bool canDisable() const;

    virtual bool setEnabled(const bool enabled, const bool force = false) = 0;
    virtual bool setExists(const bool exists) = 0;
    virtual bool setLocked(const bool locked) = 0;
    void setName(const QString &name);

    virtual void fromJsonObject(const QJsonObject &object);
    virtual void toJsonObject(QJsonObject &object) const;
    QJsonObject toNewJsonObject() const;

private:
    QString m_name;
};
Q_DECLARE_METATYPE(AbstractModDatabaseItem::Type);
