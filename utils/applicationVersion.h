#pragma once

class QString;

constexpr int applicationVersion(const int major, const int minor = 0, const int micro = 0)
{
    return ((major << 20) | (minor << 10) | micro);
}

constexpr int currentApplicationVersion()
{
    return applicationVersion(2, 0, 0);
}

constexpr int majorApplicationVersion(int version)
{
    return (version >> 20);
}

constexpr int minorApplicationVersion(int version)
{
    return ((version  & 1047552) >> 10);    //1047552 == 11111 11111 00000 00000
}

constexpr int microApplicationVersion(int version)
{
    return (version & 1023);                //1023 == 11111 11111
}

QString applicationVersionToString(const int version = currentApplicationVersion(), const char separator = '.');
int applicationVersionFromString(const QString &version, const char separator = '.');
