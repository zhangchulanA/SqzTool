#include "Logger.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QDebug>

// ---------- 控制台 ANSI 颜色定义 ----------
#define COLOR_DEBUG  "\033[34m"   // 蓝色
#define COLOR_INFO   "\033[32m"   // 绿色
#define COLOR_WARN   "\033[33m"   // 黄色
#define COLOR_ERROR  "\033[31m"   // 红色
#define COLOR_CLEAR  "\033[0m"    // 清空颜色

// ==================== Logger 实现 ====================

Logger::Logger()
    : m_runIndex(1)           // 默认从 1 开始
    , m_partIndex(1)          // 分片也从 1 开始
    , m_logLevel(E_LOG_DEBUG) // 默认显示所有日志
    , m_maxFileSize(10 * 1024 * 1024) // 默认 10 MB
    , m_enableConsole(false)   // 默认开启控制台
    , m_enableFile(false)      // 默认开启文件
    , m_flushCounter(0)       // flush 计数器初始为 0
{
    // 构造函数不做复杂操作，由 init() 完成实际初始化
}

Logger::~Logger()
{
    QMutexLocker locker(&m_mutex); // 加锁保护，避免析构时其他线程仍在写日志
    if (m_logFile.isOpen()) {
        m_fileStream.flush();      // 刷新缓冲区，确保所有数据落盘
        m_logFile.close();         // 关闭文件
    }
}

Logger& Logger::instance()
{
    static Logger obj;   // C++11 保证线程安全的静态局部变量初始化
    return obj;
}

void Logger::init(const QString& logDir,
                  const QString& filePrefix,
                  qint64 maxSizeMB,
                  bool enableConsole,
                  bool enableFile,
                  int keepDays)
{
    QMutexLocker locker(&m_mutex); // 整个初始化过程加锁，防止并发

    // 赋值基本参数
    m_logDir        = logDir;
    m_filePrefix    = filePrefix;
    m_enableConsole = enableConsole;
    m_enableFile    = enableFile;
    m_maxFileSize   = maxSizeMB * 1024 * 1024;  // MB 转字节
    m_curDate       = QDateTime::currentDateTime().toString("yyyyMMdd");
    m_partIndex     = 1;                        // 每次初始化重置分片序号

    // 确保日志目录存在
    QDir dir(m_logDir);
    if (!dir.exists()) {
        dir.mkpath(".");         // 创建目录（包括父目录）
        m_runIndex = 1;          // 新目录下首次启动
    } else {
        // 目录已存在：扫描当天最大 run 序号
        m_runIndex = getMaxRunForDate(m_curDate) + 1;
    }

    // 预编译正则表达式，用于后续文件匹配（提高性能）
    // 格式：前缀_日期_run数字[_part数字].log
    QString pattern = QString("^%1_%2_run(\\d+)(_part\\d+)?\\.log$")
                      .arg(m_filePrefix)
                      .arg(m_curDate);
    m_logFileRegex = QRegularExpression(pattern);
    m_logFileRegex.optimize();   // Qt 5.12 支持优化正则表达式

    // 如果启用文件日志，立即创建第一个日志文件
    if (m_enableFile) {
        createNewRollFile();
    }

    // 自动清理旧日志（如果 keepDays > 0）
    if (keepDays > 0) {
        cleanOldLogs(keepDays);
    }
}

void Logger::setLogLevel(LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_logLevel = level;
}

QString Logger::levelToStr(LogLevel level) const
{
    switch (level) {
        case E_LOG_DEBUG: return "DEBUG";
        case E_LOG_INFO:  return "INFO";
        case E_LOG_WARN:  return "WARN";
        case E_LOG_ERROR: return "ERROR";
        default:          return "UNKNOWN";
    }
}

QString Logger::colorPrefix(LogLevel level) const
{
    switch (level) {
        case E_LOG_DEBUG: return COLOR_DEBUG;
        case E_LOG_INFO:  return COLOR_INFO;
        case E_LOG_WARN:  return COLOR_WARN;
        case E_LOG_ERROR: return COLOR_ERROR;
        default:          return "";
    }
}

QString Logger::colorSuffix() const
{
    return COLOR_CLEAR;
}

QString Logger::escapeNewlines(const QString& msg) const
{
    QString escaped = msg;
    // 顺序很重要：先转义反斜杠，再转义换行符，避免产生额外的转义
    escaped.replace("\\", "\\\\");   // \ -> \\
    escaped.replace("\n", "\\n");    // 换行符 -> \n 字面量
    escaped.replace("\r", "\\r");    // 回车符 -> \r 字面量
    return escaped;
}

bool Logger::needRollFile()
{
    // 未开启文件保存，不需要切分
    if (!m_enableFile)
        return false;

    QString nowDate = QDateTime::currentDateTime().toString("yyyyMMdd");

    // 情况1：跨天
    if (nowDate != m_curDate) {
        m_curDate = nowDate;                          // 更新当前日期
        m_runIndex = getMaxRunForDate(m_curDate) + 1; // 重新获取新日期下的最大 run 序号
        m_partIndex = 1;                              // 重置分片序号
        // 更新正则表达式中的日期部分
        QString pattern = QString("^%1_%2_run(\\d+)(_part\\d+)?\\.log$")
                          .arg(m_filePrefix)
                          .arg(m_curDate);
        m_logFileRegex.setPattern(pattern);
        m_logFileRegex.optimize();
        return true;  // 需要切分
    }

    // 情况2：文件未打开（可能是首次运行或上次打开失败）
    if (!m_logFile.isOpen())
        return true;

    // 情况3：当前文件大小超过限制
    if (m_logFile.size() >= m_maxFileSize) {
        m_partIndex++;   // 分片序号加1
        return true;
    }

    return false;
}

int Logger::getMaxRunForDate(const QString& date)
{
    int maxRun = 0;
    QDir dir(m_logDir);
    // 构建匹配指定日期的正则表达式
    QRegularExpression rx(QString("^%1_%2_run(\\d+)(_part\\d+)?\\.log$")
                          .arg(m_filePrefix)
                          .arg(date));
    rx.optimize();

    // 遍历目录下所有文件
    for (const QFileInfo& info : dir.entryInfoList(QDir::Files)) {
        QRegularExpressionMatch match = rx.match(info.fileName());
        if (match.hasMatch()) {
            int run = match.captured(1).toInt();  // 捕获 run 数字
            if (run > maxRun)
                maxRun = run;
        }
    }
    return maxRun;
}

void Logger::createNewRollFile()
{
    // 先关闭当前打开的文件（如果有）
    if (m_logFile.isOpen()) {
        m_fileStream.flush();
        m_logFile.close();
    }

    // 根据当前 run 和 part 构建文件名
    QString fileName;
    if (m_partIndex <= 1) {
        // 第一个分片：不需要 _part 后缀
        fileName = QString("%1_%2_run%3.log")
                   .arg(m_filePrefix)
                   .arg(m_curDate)
                   .arg(m_runIndex);
    } else {
        // 后续分片：添加 _partN
        fileName = QString("%1_%2_run%3_part%4.log")
                   .arg(m_filePrefix)
                   .arg(m_curDate)
                   .arg(m_runIndex)
                   .arg(m_partIndex);
    }

    QString fullPath = QDir(m_logDir).filePath(fileName);
    m_logFile.setFileName(fullPath);

    // 以追加模式打开（文本模式）
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        // 打开失败：自动禁用文件保存，避免后续反复尝试
        m_enableFile = false;
        m_partIndex = 1;   // 重置分片序号
        // 输出警告到控制台（此时文件已不可用，只能用 qWarning）
        qWarning().noquote() << QString("[Logger] Failed to open log file: %1, file logging disabled.")
                                     .arg(fullPath);
        return;
    }

    // 绑定 QTextStream，设置 UTF-8 编码
    m_fileStream.setDevice(&m_logFile);
    m_fileStream.setCodec("UTF-8");
    // 重置 flush 计数器
    m_flushCounter = 0;
}

void Logger::cleanOldLogs(int keepDays)
{
    if (keepDays <= 0) return;

    QDateTime now = QDateTime::currentDateTime();
    qint64 keepMsecs = static_cast<qint64>(keepDays) * 24 * 3600 * 1000;  // 转换为毫秒

    QDir dir(m_logDir);
    // 匹配所有符合前缀_日期_runN[_partM].log 格式的文件
    // 使用 QRegularExpression::escape 防止前缀中的正则特殊字符被解释
    QRegularExpression rx(QString("^%1_\\d{8}_run\\d+(_part\\d+)?\\.log$")
                          .arg(QRegularExpression::escape(m_filePrefix)));
    rx.optimize();

    for (const QFileInfo& info : dir.entryInfoList(QDir::Files)) {
        if (!rx.match(info.fileName()).hasMatch())
            continue;  // 不是日志文件，跳过

        // 计算文件最后修改时间距今是否超过保留天数
        if (info.lastModified().msecsTo(now) > keepMsecs) {
            if (QFile::remove(info.absoluteFilePath())) {
                // 删除成功，输出到控制台（避免递归使用日志系统）
                qDebug().noquote() << QString("[Logger] Removed old log: %1").arg(info.fileName());
            } else {
                qWarning().noquote() << QString("[Logger] Failed to remove old log: %1").arg(info.fileName());
            }
        }
    }
}

void Logger::log(LogLevel level, const char* file, int line, const char* function, const QString& msg)
{
    QMutexLocker locker(&m_mutex);  // 确保线程安全

    // 等级过滤
    if (level < m_logLevel || m_logLevel == E_LOG_OFF)
        return;

    // 检查是否需要滚动文件（跨天/超大小），若需要则自动创建新文件
    if (needRollFile())
        createNewRollFile();

    // 组装日志文本
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString shortFile = QFileInfo(file).fileName();  // 只保留文件名，不含路径
    QString safeMsg = escapeNewlines(msg);           // 转义换行符，保证单行
    QString logText = QString("[%1] [%2] [%3 : %4 : %5] | %6")
                      .arg(timeStr)
                      .arg(levelToStr(level))
                      .arg(shortFile)
                      .arg(function)
                      .arg(line)
                      .arg(safeMsg);

    // 1. 写入文件（纯净文本，无颜色）
    if (m_enableFile && m_logFile.isOpen()) {
        m_fileStream << logText << "\n";
        // 性能优化：每 FLUSH_INTERVAL 条日志才真正 flush 一次，避免频繁磁盘 IO
        if (++m_flushCounter >= FLUSH_INTERVAL) {
            m_fileStream.flush();
            m_flushCounter = 0;
        }
    }

    // 2. 控制台彩色输出
    if (m_enableConsole) {
        QString colorText = colorPrefix(level) + logText + colorSuffix();
        // noquote() 防止 qDebug 自动添加引号
        qDebug().noquote() << colorText;
    }
}
