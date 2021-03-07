#include <QCoreApplication>
#include <QRegExpValidator>

#include "ModInfo.h"

QString ModInfo::generateUnknownNameStub()
{
	return QCoreApplication::tr("WARNING: unknown mod name. Set the name manually.");
}

QString ModInfo::generateFailedToGetNameStub()
{
	return QCoreApplication::tr("WARNING: couldn't get the name of the mod. Set the name manually.");
}

QString ModInfo::generateWaitingForSteamResponseStub()
{
	return QCoreApplication::tr("Waiting for Steam mod name response...");
}

bool ModInfo::isSteamId(QString str)
{
	int pos = 0;
	return !(QRegExpValidator(QRegExp("[0-9]*")).validate(str, pos) == QValidator::State::Invalid);
}
