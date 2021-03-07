#ifndef MODINFO_H
#define MODINFO_H

#include <QString>

struct ModInfo
{
	static bool isSteamId(QString str);

	static QString generateUnknownNameStub();
	static QString generateFailedToGetNameStub();
	static QString generateWaitingForSteamResponseStub();

	inline bool existsAndEnabledCheck(const bool existsValue, const bool enabledValue) const;

	QString name;
	QString steamName;
	QString folderName;
	bool enabled = true;
	bool exists = true;
};



inline bool ModInfo::existsAndEnabledCheck(const bool existsValue, const bool enabledValue) const
{
	return (existsValue == exists && enabledValue == enabled);
}

#endif // MODINFO_H
