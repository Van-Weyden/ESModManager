#include <QApplication>
#include <QDebug>
#include <QDirIterator>

#include "databasemodel.h"
#include "regexppatterns.h"

#include "modscanner.h"

QString renPyInitRegExp(const char *tag, const QString &dictionaryKeyInBracketsPattern);

ModScanner::ModScanner(QObject *parent) :
	QObject(parent)
{
	QString expressionInQuotesPattern = RegExpPatterns::oneOf({
										  RegExpPatterns::quotedExpression("\'"),
										  RegExpPatterns::quotedExpression("\"")
										});
	QString dictionaryKeyInBracketsPattern = RegExpPatterns::escapedSymbol("[")
										   + RegExpPatterns::whitespace
										   + expressionInQuotesPattern
										   + RegExpPatterns::whitespace
										   + RegExpPatterns::escapedSymbol("]");

	m_initRegExp		= QRegExp(renPyInitRegExp("((mods)|(filters))", dictionaryKeyInBracketsPattern));
	m_modInitRegExp		= QRegExp(renPyInitRegExp("(mods)",				dictionaryKeyInBracketsPattern));
	m_filterInitRegExp	= QRegExp(renPyInitRegExp("(filters)",			dictionaryKeyInBracketsPattern));
	m_modTagsRegExp		= QRegExp(renPyInitRegExp("(mod_tags)",			dictionaryKeyInBracketsPattern));

	m_dictionaryKeyInBracketsRegExp = QRegExp(dictionaryKeyInBracketsPattern);
	m_innerExpressionInBracesRegExp = QRegExp(
		RegExpPatterns::quotedExpession(
			RegExpPatterns::escapedSymbol("{"),
			RegExpPatterns::escapedSymbol("}")
		)
	);

	QString singleQuotationMark = RegExpPatterns::escapedSymbol("\'");
	QString whileExceptSingleQuotationMark = RegExpPatterns::whileExcept({
		singleQuotationMark,
		RegExpPatterns::escapedSymbol("n"),
		RegExpPatterns::escapedSymbol("r")
	});

	QString doubleQuotationMark = RegExpPatterns::escapedSymbol("\"");
	QString whileExceptDoubleQuotationMark = RegExpPatterns::whileExcept({
		doubleQuotationMark,
		RegExpPatterns::escapedSymbol("n"),
		RegExpPatterns::escapedSymbol("r")
	});

	QString whileExceptQuotationMarkAndHashSymbol = RegExpPatterns::whileExcept({
		singleQuotationMark,
		doubleQuotationMark,
		RegExpPatterns::escapedSymbol("#"),
		RegExpPatterns::escapedSymbol("n"),
		RegExpPatterns::escapedSymbol("r")
	});

	m_pythonCommentRegExp = QRegExp(
		RegExpPatterns::allOf({
			RegExpPatterns::lineBegin,
			whileExceptQuotationMarkAndHashSymbol,

			RegExpPatterns::zeroOrMoreOccurences(
				RegExpPatterns::oneOf({
					RegExpPatterns::allOf({
						doubleQuotationMark,
						whileExceptDoubleQuotationMark,
						doubleQuotationMark,
						whileExceptQuotationMarkAndHashSymbol
					}),
					RegExpPatterns::allOf({
						singleQuotationMark,
						whileExceptSingleQuotationMark,
						singleQuotationMark,
						whileExceptQuotationMarkAndHashSymbol
					})
				})
			),

			RegExpPatterns::oneOf({
				RegExpPatterns::putInParentheses(RegExpPatterns::lineEnd),
				RegExpPatterns::allOf({
					RegExpPatterns::escapedSymbol("#"),
					RegExpPatterns::anySymbolSequence,
					RegExpPatterns::lineEnd
				})
			})
		})
	);

	m_allFromBeginToQuoteRegExp = QRegExp(
		RegExpPatterns::allOf({
			RegExpPatterns::lineBegin,
			RegExpPatterns::whileExcept({
				singleQuotationMark,
				doubleQuotationMark
			}),
			RegExpPatterns::oneOf({
				singleQuotationMark,
				doubleQuotationMark
			})
		})
	);

	m_allFromQuoteToEndRegExp = QRegExp(
		RegExpPatterns::allOf({
			RegExpPatterns::oneOf({
				singleQuotationMark,
				doubleQuotationMark
			}),
			RegExpPatterns::whileExcept({
				singleQuotationMark,
				doubleQuotationMark
			}),
			RegExpPatterns::lineEnd
		})
	);

	RegExpPatterns::debugOutput("comment pattern:", m_pythonCommentRegExp);
}

void ModScanner::scanMods(const QString &modsFolderPath, DatabaseModel &database)
{
	int countOfScannedMods = 0;
	int oldDatabaseSize = database.databaseSize();
	QStringList modsFolders = QDir(modsFolderPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot);

	while (!modsFolders.isEmpty()) {
		QString modFolderName = modsFolders.takeFirst();
		scanMod(modFolderName, modsFolderPath + modFolderName, database, oldDatabaseSize);
		emit modScanned(++countOfScannedMods);
		QApplication::processEvents();
	}
}



//private:

int indexOfModWithUnknownNameInDatabase(const QString &modFolderName, const QString &modFolderPath,
										DatabaseModel &database, const int oldDatabaseSize, bool *isModNameValid);

void ModScanner::scanMod(const QString &modFolderName, const QString &modFolderPath,
						 DatabaseModel &database, const int oldDatabaseSize)
{
	ModInfo modInfo;
	modInfo.folderName = modFolderName;

	bool isModNameValid;
	int indexInDatabase = indexOfModWithUnknownNameInDatabase(
								modFolderName, modFolderPath,
								database, oldDatabaseSize, &isModNameValid);

	if (isModNameValid) {
		return;
	}

	QMap<QString, QString> initMap;
	QFile file;
	QString outOf;
	QString initKey;
	QString initValue;
	QString prioryModName;
	QString prioryModNameKey;
	QDir modDir(modFolderPath);
	bool isModTagsFounded = false;

	QDirIterator it(modDir.path(), {"*.rpy"}, QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext()) {
		file.setFileName(it.next());
		file.open(QFile::ReadOnly);
		//'QFile::canReadLine()' may return false in some cases, so check the end of the file with 'QFile::atEnd()'
		while (!file.atEnd()) {
			QString line = file.readLine();

			if (m_pythonCommentRegExp.indexIn(line) > -1 && !m_pythonCommentRegExp.cap(17).isEmpty()) {
				line.remove(m_pythonCommentRegExp.cap(17));
			}

			isModTagsFounded = line.contains(m_modTagsRegExp);

			if (line.contains(m_initRegExp)) {
				m_dictionaryKeyInBracketsRegExp.indexIn(line);
				initKey = m_dictionaryKeyInBracketsRegExp.cap();
				initKey.remove(m_allFromBeginToQuoteRegExp);
				initKey.remove(m_allFromQuoteToEndRegExp);

				initValue = line;
				initValue.remove(m_initRegExp);
				initValue.remove(m_allFromBeginToQuoteRegExp);
				initValue.remove(m_allFromQuoteToEndRegExp);

				while (initValue.contains(m_innerExpressionInBracesRegExp)) {
					initValue.remove(m_innerExpressionInBracesRegExp);
				}

				if (!initValue.isEmpty()) {
					if (line.contains(m_filterInitRegExp)) {
						initValue.append(tr(" [filter]"));
					}

					initMap[initKey] = initValue;

					if (isModTagsFounded && line.contains(m_modInitRegExp)) {
						prioryModName = initValue;
						prioryModNameKey = initKey;
					}
				}
			}
		}
		isModTagsFounded = false;
		file.close();
	}

	if (!prioryModName.isEmpty()) {
		initMap[prioryModNameKey] = prioryModName;
	}

	modInfo.name.clear();
	outOf = '/' + QString::number(initMap.count()) + "]: ";

	if (initMap.isEmpty()) {
		modInfo.name = ModInfo::generateFailedToGetNameStub();
	} else {
		if (initMap.count() == 1) {
			modInfo.name = initMap[initKey];
		} else {
			int modNumber = 1;

			for (const QString &initKey : initMap.keys()) {
				if (modInfo.name.isEmpty()) {
					modInfo.name = "[1" + outOf + initMap[initKey];
				} else {
					modInfo.name.append("\n[" + QString::number(modNumber) + outOf + initMap[initKey]);
				}
				++modNumber;
			}
		}
	}

	if (indexInDatabase == -1) {
		database.appendDatabase(modInfo);
	} else {
		database.modInfoRef(indexInDatabase).name = modInfo.name;
	}
}



//non-members of class ModScanner:

QString renPyInitRegExp(const char *tag, const QString &dictionaryKeyInBracketsPattern)
{
	return RegExpPatterns::zeroOrOneOccurences(RegExpPatterns::escapedSymbol("$"))
			+ RegExpPatterns::whitespace
			+ tag + RegExpPatterns::whitespace
			+ dictionaryKeyInBracketsPattern + RegExpPatterns::whitespace
			+ "=[^\"']*";
}

int indexOfModWithUnknownNameInDatabase(const QString &modFolderName, const QString &modFolderPath,
										DatabaseModel &database, const int oldDatabaseSize, bool *isModNameValid)
{
	QString managerPath = QDir::currentPath().replace('/', '\\');

	// Self check
	if (managerPath.contains(modFolderPath)) {
		if (isModNameValid) {
			*isModNameValid = true;
		}

		return -1;
	}

	for (int indexInDatabase = 0; indexInDatabase < oldDatabaseSize; ++indexInDatabase) {
		if (modFolderName == database.modFolderName(indexInDatabase)) {
			database.modInfoRef(indexInDatabase).exists = true;

			if (isModNameValid) {
				*isModNameValid = database.isNameValid(database.modName(indexInDatabase));
			}

			return indexInDatabase;
		}
	}

	if (isModNameValid) {
		*isModNameValid = false;
	}

	return -1;
}
