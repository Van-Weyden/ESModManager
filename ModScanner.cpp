#include <QApplication>
#include <QDebug>
#include <QDirIterator>
#include <QThread>

#include "mvc/ModDatabaseModel.h"
#include "utils/RegExpPatterns.h"

#include "ModScanner.h"

static QString renPyInitRegExp(const char *tag, const QString &dictionaryKeyInBracketsPattern)
{
    return RegExpPatterns::zeroOrOneOccurences(RegExpPatterns::escapedSymbol("$"))
        + RegExpPatterns::whitespace
        + tag + RegExpPatterns::whitespace
        + dictionaryKeyInBracketsPattern + RegExpPatterns::whitespace
        + "=[^\"']*"
    ;
}

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

    m_initRegExp        = QRegExp(renPyInitRegExp("((mods)|(filters))", dictionaryKeyInBracketsPattern));
    m_modInitRegExp     = QRegExp(renPyInitRegExp("(mods)",             dictionaryKeyInBracketsPattern));
    m_filterInitRegExp  = QRegExp(renPyInitRegExp("(filters)",          dictionaryKeyInBracketsPattern));
    m_modTagsRegExp     = QRegExp(renPyInitRegExp("(mod_tags)",         dictionaryKeyInBracketsPattern));

    m_dictionaryKeyInBracketsRegExp = QRegExp(dictionaryKeyInBracketsPattern);
    m_innerExpressionInBracesRegExp = QRegExp(
        RegExpPatterns::quotedExpession(
            RegExpPatterns::escapedSymbol("{"),
            RegExpPatterns::escapedSymbol("}")
        )
    );

    QString singleQuotationMark = RegExpPatterns::escapedSymbol("\'");
    QString whileExceptSingleQuotationMark = RegExpPatterns::symbolsUntilNotFromSet({
        singleQuotationMark,
        RegExpPatterns::escapedSymbol("n"),
        RegExpPatterns::escapedSymbol("r")
    });

    QString doubleQuotationMark = RegExpPatterns::escapedSymbol("\"");
    QString whileExceptDoubleQuotationMark = RegExpPatterns::symbolsUntilNotFromSet({
        doubleQuotationMark,
        RegExpPatterns::escapedSymbol("n"),
        RegExpPatterns::escapedSymbol("r")
    });

    QString whileExceptQuotationMarkAndHashSymbol = RegExpPatterns::symbolsUntilNotFromSet({
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
            RegExpPatterns::symbolsUntilNotFromSet({
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
            RegExpPatterns::symbolsUntilNotFromSet({
                singleQuotationMark,
                doubleQuotationMark
            }),
            RegExpPatterns::lineEnd
        })
    );
}

void ModScanner::setModsFolderPath(const QString &modsFolderPath)
{
    m_modsFolderPath = modsFolderPath;
}

void ModScanner::setModel(ModDatabaseModel *model)
{
    m_model = model;
}

void ModScanner::setEnabledFlag(EnabledFlagInitValue enabledFlag)
{
    m_enabledFlag = enabledFlag;
}

void ModScanner::scanMods()
{
    if (m_isRunning || !m_model || m_modsFolderPath.isEmpty()) {
        return;
    }
    m_isRunning = true;
    m_model->reset([&]() {
        m_model->setModsExistsState(false);
        m_countOfScannedMods = 0;
        int oldModelSize = m_model->size();
        QStringList modsFolders = QDir(m_modsFolderPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        QList<ModInfo *> modsWithUnknownName;

        while (!modsFolders.isEmpty()) {
            QString modFolderName = modsFolders.takeFirst();
            registerMod({modFolderName, m_modsFolderPath + modFolderName, oldModelSize}, modsWithUnknownName);
        }

        while (!modsWithUnknownName.isEmpty() && !QThread::currentThread()->isInterruptionRequested()) {
            tryResolveName(*modsWithUnknownName.takeFirst());
        }
    });
    m_isRunning = false;

    emit modsScanned();
}



//private:

void ModScanner::registerMod(ScanData data, QList<ModInfo *> &modsWithUnknownName)
{
    bool isModNameValid = false;
    int row = indexOfModWithUnknownNameInDatabase(data, &isModNameValid);

    if (isModNameValid) {
        if (row != -1 && m_enabledFlag != EnabledFlagInitValue::NotOverride) {
            m_model->modInfoRef(row).setEnabled(
                m_enabledFlag == EnabledFlagInitValue::ForceTrue ? true : false
            );
        }

        // emits for invalid names will be sent from the 'tryResolveName' method
        emit modScanned(++m_countOfScannedMods);
        return;
    }

    ModInfo modInfo;
    modInfo.sourcesName = ModInfo::generateUnknownNameStub();
    modInfo.folderName = data.modFolderName;

    if (m_enabledFlag != EnabledFlagInitValue::NotOverride) {
        modInfo.setEnabled(m_enabledFlag == EnabledFlagInitValue::ForceTrue ? true : false);
    }

    if (row == -1) {
        row = m_model->size();
        m_model->add(modInfo);
    }

    modsWithUnknownName.append(&m_model->modInfoRef(row));
}

void ModScanner::tryResolveName(ModInfo &modInfo)
{
    QString initKey;
    QString prioryModName;
    QString prioryModNameKey;
    QMap<QString, QString> initMap;

    bool isModTagsFounded = false;
    QDir modDir(m_modsFolderPath + modInfo.folderName);
    QDirIterator it(modDir.path(), {"*.rpy"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext() && !QThread::currentThread()->isInterruptionRequested()) {
        QFile file(it.next());
        file.open(QFile::ReadOnly);
        //'QFile::canReadLine' may return false in some cases, so check the end of the file with 'QFile::atEnd'
        while (!file.atEnd() && !QThread::currentThread()->isInterruptionRequested()) {
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

                QString initValue = line;
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
        file.close();
    }

    if (QThread::currentThread()->isInterruptionRequested())
    {
        return;
    }

    if (!prioryModName.isEmpty()) {
        initMap[prioryModNameKey] = prioryModName;
    }

    modInfo.sourcesName.clear();
    QString outOf = '/' + QString::number(initMap.count()) + "]: ";

    if (initMap.isEmpty()) {
        modInfo.sourcesName = ModInfo::generateFailedToGetNameStub();
    } else {
        if (initMap.count() == 1) {
            modInfo.sourcesName = initMap[initKey];
        } else {
            int modNumber = 1;

            for (const QString &initKey : initMap.keys()) {
                if (modInfo.sourcesName.isEmpty()) {
                    modInfo.sourcesName = "[1" + outOf + initMap[initKey];
                } else {
                    modInfo.sourcesName.append("\n[" + QString::number(modNumber) + outOf + initMap[initKey]);
                }
                ++modNumber;
            }
        }
    }

    emit modScanned(++m_countOfScannedMods);
}

int ModScanner::indexOfModWithUnknownNameInDatabase(ScanData &data, bool *isModNameValid)
{
    QString managerPath = QCoreApplication::applicationDirPath().replace('/', '\\');

    // Self check
    if (managerPath.contains(data.modFolderPath)) {
        if (isModNameValid) {
            *isModNameValid = true;
        }

        return -1;
    }

    for (int row = 0; row < data.oldModelSize; ++row) {
        auto& modInfo = m_model->modInfoRef(row);
        if (data.modFolderName == modInfo.folderName) {
            modInfo.setExists(true);

            if (isModNameValid) {
                *isModNameValid = ModInfo::isNameValid(modInfo.sourcesName);
            }

            return row;
        }
    }

    if (isModNameValid) {
        *isModNameValid = false;
    }

    return -1;
}
