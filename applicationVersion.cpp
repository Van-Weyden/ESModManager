#include <QStringList>

#include "applicationVersion.h"

QString applicationVersionToString(const int version, const char separator)
{
    return QString::number(majorApplicationVersion(version)) + separator +
           QString::number(minorApplicationVersion(version)) + separator +
           QString::number(microApplicationVersion(version));
}

int applicationVersionFromString(const QString &version, const char separator)
{
    QStringList subVersions = version.split(separator);
    int subVersionsCount = subVersions.count();
    return applicationVersion(subVersionsCount > 0 ? subVersions[0].toInt() : 1,
                              subVersionsCount > 1 ? subVersions[1].toInt() : 0,
                              subVersionsCount > 2 ? subVersions[2].toInt() : 0);
}
