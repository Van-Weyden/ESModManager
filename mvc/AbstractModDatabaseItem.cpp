#include <QJsonObject>

#include "AbstractModDatabaseItem.h"

AbstractModDatabaseItem::AbstractModDatabaseItem(const QString &name)
    : m_name(name)
{}

const QString &AbstractModDatabaseItem::name() const
{
    return m_name;
}

const QString &AbstractModDatabaseItem::displayedName() const
{
    return m_name;
}

bool AbstractModDatabaseItem::canEnable() const
{
    return exists() && !locked() && !enabled();
}

bool AbstractModDatabaseItem::canDisable() const
{
    return exists() && !locked() && enabled();
}

void AbstractModDatabaseItem::setName(const QString &name)
{
    m_name = name;
}


void AbstractModDatabaseItem::fromJsonObject(const QJsonObject &object)
{
    m_name = object["name"].toString();
}

void AbstractModDatabaseItem::toJsonObject(QJsonObject &object) const
{
    object["name"] = m_name;
}

QJsonObject AbstractModDatabaseItem::toNewJsonObject() const
{
    QJsonObject object;
    toJsonObject(object);
    return object;
}
