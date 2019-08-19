#include <QRegExpValidator>

#include "modinfo.h"

bool ModInfo::isSteamId(QString str)
{
	int pos = 0;
	return !(QRegExpValidator(QRegExp("[0-9]*")).validate(str, pos) == QValidator::State::Invalid);
}
