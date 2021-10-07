#include <QDateTime>
#include <QFile>

#include "Logger.h"

Logger::Logger()
{
    m_debugLogFile      = new QFile();
    m_infoLogFile       = new QFile();
    m_warningLogFile    = new QFile();
    m_criticalLogFile   = new QFile();
    m_fatalLogFile      = new QFile();

    setLogFile_("log.txt");
}

Logger::~Logger()
{
    delete m_debugLogFile;
    delete m_infoLogFile;
    delete m_warningLogFile;
    delete m_criticalLogFile;
    delete m_fatalLogFile;
}

Logger &Logger::instance()
{
    static Logger theSingleInstance;
    return theSingleInstance;
}

void Logger::attach()
{
    qInstallMessageHandler(processMessage);
}

void Logger::detach()
{
    qInstallMessageHandler(0);
}

void Logger::setLogFile(const QString &fileName,
                        const bool overwriteNewLogFile, const bool removeOldLogFile)
{
    instance().setLogFile_(fileName, overwriteNewLogFile, removeOldLogFile);
}

void Logger::setLogFile(QtMsgType type, const QString &fileName,
                        const bool overwriteNewLogFile, const bool removeOldLogFile)
{
    instance().setLogFile_(type, fileName, overwriteNewLogFile, removeOldLogFile);
}

QString Logger::logFileName(QtMsgType type)
{
    QFile *logFile = instance().logFile_(type);
    return (logFile == nullptr ? "" : logFile->fileName());
}

void Logger::processMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    instance().processMessage_(type, context, msg);
}

void Logger::setLogFile_(const QString &fileName,
                         const bool overwriteNewLogFile, const bool removeOldLogFile)
{
    if (removeOldLogFile) {
        m_debugLogFile->remove();
        m_infoLogFile->remove();
        m_warningLogFile->remove();
        m_criticalLogFile->remove();
        m_fatalLogFile->remove();
    }

    m_debugLogFile->setFileName(fileName);
    m_infoLogFile->setFileName(fileName);
    m_warningLogFile->setFileName(fileName);
    m_criticalLogFile->setFileName(fileName);
    m_fatalLogFile->setFileName(fileName);

    if (overwriteNewLogFile) {
        m_debugLogFile->open(QFile::WriteOnly);
        m_debugLogFile->close();
    }
}

void Logger::setLogFile_(QtMsgType type, const QString &fileName,
                         const bool overwriteNewLogFile, const bool removeOldLogFile)
{
    QFile *logFile = logFile_(type);

    if (logFile == nullptr) {
        return;
    }

    if (removeOldLogFile) {
        logFile->remove();
    }

    logFile->setFileName(fileName);

    if (overwriteNewLogFile) {
        logFile->open(QFile::WriteOnly);
        logFile->close();
    }
}

QFile *Logger::logFile_(QtMsgType type)
{
    switch (type) {
        case QtDebugMsg:    return m_debugLogFile;
        case QtInfoMsg:     return m_infoLogFile;
        case QtWarningMsg:  return m_warningLogFile;
        case QtCriticalMsg: return m_criticalLogFile;
        case QtFatalMsg:    return m_fatalLogFile;
    }

    return nullptr;
}

void Logger::processMessage_(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QFile *logFile = logFile_(type);

    if (logFile == nullptr) {
        return;
    }

    QString messagePrefix = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss.zzz");
    switch (type) {
        case QtDebugMsg:
            messagePrefix += " [Debug]: ";
        break;

        case QtInfoMsg:
            messagePrefix += " [Info]: ";
        break;

        case QtWarningMsg:
            messagePrefix += " [Warning]: ";
        break;

        case QtCriticalMsg:
            messagePrefix += " [Critical]: ";
        break;

        case QtFatalMsg:
            messagePrefix += " [Fatal]: ";
        break;
    }

    logFile->open(QFile::Append);
    logFile->write(
        (messagePrefix + "%1 (%2, in %3: %4)\n").
            arg(msg).
            arg(context.function).
            arg(context.file).
            arg(context.line).
        toLocal8Bit()
    );
    logFile->close();

    if (type == QtFatalMsg) {
        abort();
    }
}
