#ifndef LOGGER_H
#define LOGGER_H

#include <QtGlobal>

class QFile;

class Logger
{
public:
    static void attach();
    static void detach();
    static void setLogFile(const QString &fileName,
                           const bool overwriteNewLogFile = true, const bool removeOldLogFile = false);
    static void setLogFile(QtMsgType type, const QString &fileName,
                           const bool overwriteNewLogFile = true, const bool removeOldLogFile = false);
    static QString logFileName(QtMsgType type);
    static void processMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);

private:
    Logger();
    ~Logger();
    Logger(const Logger &other) = delete;
    Logger &operator=(const Logger &other) = delete;

    static Logger &instance();
    void setLogFile_(const QString &fileName,
                     const bool overwriteNewLogFile = true, const bool removeOldLogFile = false);
    void setLogFile_(QtMsgType type, const QString &fileName,
                     const bool overwriteNewLogFile = true, const bool removeOldLogFile = false);
    QFile *logFile_(QtMsgType type);
    void processMessage_(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    QFile *m_debugLogFile;
    QFile *m_infoLogFile;
    QFile *m_warningLogFile;
    QFile *m_criticalLogFile;
    QFile *m_fatalLogFile;
};

#endif // LOGGER_H
