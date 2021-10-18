#include <QStringList>

#include "applicationVersion.h"

QString applicationVersionToString(const int version)
{
    return QString::number(majorApplicationVersion(version)) + '.' +
           QString::number(minorApplicationVersion(version)) + '.' +
           QString::number(microApplicationVersion(version));
}

int applicationVersionFromString(const QString &version)
{
    QStringList subVersions = version.split('.');
    int subVersionsCount = subVersions.count();
    return applicationVersion(subVersionsCount > 0 ? subVersions[0].toInt() : 1,
                              subVersionsCount > 1 ? subVersions[1].toInt() : 0,
                              subVersionsCount > 2 ? subVersions[2].toInt() : 0);
}
