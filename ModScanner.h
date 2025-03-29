#ifndef MODSCANNER_H
#define MODSCANNER_H

#include <QObject>

class ModDatabaseModel;

class ModScanner : public QObject
{
    Q_OBJECT

public:
    enum EnabledFlagInitValue
    {
        NotOverride,
        ForceTrue,
        ForceFalse
    };

    explicit ModScanner(QObject *parent = nullptr);
    void scanMods(const QString &modsFolderPath, ModDatabaseModel &model,
                  const EnabledFlagInitValue enabledFlag = NotOverride);

signals:
    void modScanned(int countOfScannedMods);

private:
    struct ScanData
    {
        const QString &modFolderName;
        const QString &modFolderPath;
        ModDatabaseModel &model;
        const int oldModelSize;
    };

private:
    void scanMod(ScanData data, const EnabledFlagInitValue enabledFlagValue);
    int indexOfModWithUnknownNameInDatabase(ScanData& data, bool *isModNameValid);

private:
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
