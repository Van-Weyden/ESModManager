#ifndef MODINFO_H
#define MODINFO_H

#include <QString>

struct ModInfo
{
	QString name;
	QString steamName;
	QString folderName;
	bool enabled = true;
	bool exists = true;

	inline bool existsAndEnabledCheck(const bool &existsValue, const bool &enabledValue) const
		 {return (existsValue == exists && enabledValue == enabled);}
};

#endif // MODINFO_H
