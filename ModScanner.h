#ifndef MODSCANNER_H
#define MODSCANNER_H

#include <QObject>

class DatabaseModel;

class ModScanner : public QObject
{
	Q_OBJECT

public:
	enum EnabledFlagValue
	{
		NotOverride,
		ForceTrue,
		ForceFalse
	};

	explicit ModScanner(QObject *parent = nullptr);
	void scanMods(const QString &modsFolderPath, DatabaseModel &databaseModel,
				  const EnabledFlagValue enabledFlagValue = NotOverride);

signals:
	void modScanned(int countOfScannedMods);

private:
	void scanMod(const QString &modFolderName, const QString &modFolderPath,
				 DatabaseModel &databaseModel, const int oldDatabaseSize,
				 const EnabledFlagValue enabledFlagValue = NotOverride);

	QRegExp m_initRegExp;
	QRegExp m_modInitRegExp;
	QRegExp m_filterInitRegExp;
	QRegExp m_modTagsRegExp;

	QRegExp m_dictionaryKeyInBracketsRegExp;
	QRegExp m_innerExpressionInBracesRegExp;

	QRegExp m_pythonCommentRegExp;
	QRegExp m_allFromBeginToQuoteRegExp;
	QRegExp m_allFromQuoteToEndRegExp;
};

#endif // MODSCANNER_H
