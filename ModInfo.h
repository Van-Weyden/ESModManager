#ifndef MODINFO_H
#define MODINFO_H

#include <QMetaType>
#include <QString>

class QJsonObject;

class ModInfo
{
public:
    static bool isSteamId(QString str);

    static QString generateUnknownNameStub();
    static QString generateFailedToGetNameStub();
    static QString generateWaitingForSteamResponseStub();

public:
    ModInfo() = default;
    ModInfo(const QJsonObject &object);

    inline bool marked() const;
    inline bool exists() const;
    inline bool enabled() const;
    inline bool canEnable() const;
    inline bool canDisable() const;
    inline bool locked() const;

    void setMarked(const bool marked);
    void setExists(const bool exists);
    void setEnabled(const bool enabled, const bool force = false);
    void setLocked(const bool locked);

    void fromJsonObject(const QJsonObject &object);
    void toJsonObject(QJsonObject &object) const;
    QJsonObject toJsonObject() const;

public:
    QString name;
    QString steamName;
    QString folderName;

private:
    bool m_enabled = true;
    bool m_locked = false;
    bool m_marked = false;
    bool m_exists = true;
};
Q_DECLARE_METATYPE(ModInfo);


inline bool ModInfo::marked() const
{
    return m_marked;
}

inline bool ModInfo::exists() const
{
    return m_exists;
}

inline bool ModInfo::enabled() const
{
    return m_enabled;
}

inline bool ModInfo::canEnable() const
{
    return exists() && !locked() && !m_enabled;
}

inline bool ModInfo::canDisable() const
{
    return exists() && !locked() && m_enabled;
}

inline bool ModInfo::locked() const
{
    return m_locked;
}

#endif // MODINFO_H
