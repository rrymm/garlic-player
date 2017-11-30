#include "logger.h"

Logger* Logger::instance= nullptr;
std::once_flag Logger::initInstanceFlag;

Logger::Logger(QObject *parent) : QObject(parent)
{
    qtdebug_log.reset(new LogFile(TConfiguration::getLogDir() + "qtdebug.log"));
    debug_log.reset(new LogFile(TConfiguration::getLogDir() + "debug.log"));
    event_log.reset(new LogFile(TConfiguration::getLogDir() + "event_log.xml"));
    play_log.reset(new LogFile(TConfiguration::getLogDir() + "play_log.xml"));
}

void Logger::initSingleton()
{
    instance = new Logger;
}

Logger& Logger::getInstance()
{
    std::call_once(initInstanceFlag, &Logger::initSingleton);
    return *instance;
}

void Logger::dispatchMessages(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (QString(context.category) == "EventLog" ||
            QString(context.category) == "SmilParser" ||
            QString(context.category) == "ContentManager" ||
            QString(context.category) == "MediaPlayer" ||
            QString(context.category) == "MediaControl" ||
            QString(context.category) == "System" ||
            QString(context.category) == "TimeService")
    {
        event_log.data()->write(collectEventLog(type, context, msg));
    }
    else if (QString(context.category).mid(0,3) == "qt.")
    {
       qtdebug_log.data()->write(collectDebugLog(type, context, msg)); // get all qt log rubbish here not poisoning debug_log
    }
    else if (QString(context.category) == "PlayLog")
    {
        play_log.data()->write(msg);
    }
    else
    {
        debug_log.data()->write(collectDebugLog(type, context, msg));
    }
}

QString Logger::createPlayLogEntry(QString start_time, QString content_id)
{
    return "<contentPlayed><contentId>"+content_id+"</contentId><startTime>"+start_time+"</startTime><endTime>" +getCurrentIsoDateTime()+"</endTime></contentPlayed>";
}

QString Logger::createEventLogMetaData(QString event_name, QStringList meta_data)
{
    QString xml = "<eventName>"+event_name+"</eventName><metadata>";
    for (int i = 0; i < meta_data.size(); i = i+2)
    {
        xml += "<meta name=\""+meta_data.at(i)+"\" content=\""+meta_data.at(i+1)+"\"/>";
    }
    return xml+"</metadata>";
}

void Logger::rotateLog(QString log_name)
{
    if (log_name == "event")
        event_log.data()->rotateFile();
    else if (log_name == "play")
        event_log.data()->rotateFile();
}

QString Logger::getCurrentIsoDateTime()
{
    // ugly workaround from https://stackoverflow.com/questions/21976264/qt-isodate-formatted-date-time-including-timezone
    // cause otherwise we get no time zone in date string
    return QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc()).toString(Qt::ISODate);
}

QString Logger::collectDebugLog(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " "
        + determineSeverity(type) + " "
        + context.category + " "
        + context.file + "(" + QString::number(context.line) + ")"
        + msg;
}

QString Logger::collectEventLog(QtMsgType type, const QMessageLogContext &context, const QString &meta_data)
{
    QString meta = meta_data;
    meta.replace("\\", "");
    return "<event><eventType>"+determineSeverity(type) + "</eventType><eventTime>"
                            + getCurrentIsoDateTime()+"</eventTime><eventSource>"
                            + context.category + "</eventSource>" + meta.mid(1, meta.length()-2)+ "</event>";
}

QString Logger::determineSeverity(QtMsgType type)
{
    switch (type)
    {
        case QtDebugMsg:
            return "debug";
        case QtInfoMsg:
            return "informational";
        case QtWarningMsg:
            return "warning";
        case QtCriticalMsg:
            return "error";
        case QtFatalMsg:
            return "critical";
    }
    return "UNKNOWN";
}
