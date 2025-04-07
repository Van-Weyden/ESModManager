#pragma once

#include <QObject>

class ModDatabaseModel;
class ModInfo;

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

    void setModsFolderPath(const QString &modsFolderPath);
    void setModel(ModDatabaseModel *model);
    void setEnabledFlag(EnabledFlagInitValue enabledFlag);

public slots:
    void scanMods();

signals:
    void modScanned(int countOfScannedMods);
    void modsScanned();

private:
    struct ScanData
    {
        const QString &modFolderName;
        const QString &modFolderPath;
        const int oldModelSize;
    };

private:
    void registerMod(ScanData data, QList<QModelIndex> &mods);
    void tryResolveName(ModInfo &modInfo);
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

    bool m_isRunning = false;
    int m_countOfScannedMods;
    QString m_modsFolderPath;
    ModDatabaseModel *m_model = nullptr;
    EnabledFlagInitValue m_enabledFlag = NotOverride;
};
