#ifndef TIMEUTILS_H
#define TIMEUTILS_H

#include <QString>
#include <QDateTime>
#include <QDate>
#include <QTime>

/**
 * @brief 时间工具类
 * @class  TimeUtils
 * @note   全部静态函数，无需创建对象，直接调用
 * @note   Qt5.14.2 完美兼容，线程安全、空安全
 *
 * 功能覆盖：
 * 1. 获取当前时间/日期/时间戳（秒/毫秒）
 * 2. 时间戳 ↔ 日期时间互转
 * 3. 标准/中文/文件名 格式化
 * 4. 时间差计算（秒/分/时/天）
 * 5. 秒/毫秒 → 00:00:00 / 00:00:00.000
 * 6. 友好时间显示：刚刚、几分钟前、几小时前
 * 7. UTC ↔ 本地时间互转
 * 8. 时间加减（天/小时/月/年）
 * 9. 判断：今天、昨天、闰年、同天/同月/同年
 * 10. 获取星期（中文/英文）
 * 11. 时间合法性校验
 * 12. 生成唯一时间串（用于ID/文件名）
 */
class TimeUtils
{
public:
    // ==============================================
    // 1. 获取当前时间（各种格式）
    // ==============================================

    /**
     * @brief  获取当前标准日期时间
     * @return yyyy-MM-dd HH:mm:ss
     * @example "2025-05-01 12:00:00"
     */
    static QString currentDateTime();

    /**
     * @brief  获取当前日期
     * @return yyyy-MM-dd
     */
    static QString currentDate();

    /**
     * @brief  获取当前时间
     * @return HH:mm:ss
     */
    static QString currentTime();

    /**
     * @brief  获取适合文件名的时间串
     * @return yyyyMMdd_HHmmss
     * @example "20250501_120000"
     */
    static QString currentDateTimeForFile();

    /**
     * @brief  获取中文格式日期时间
     * @return yyyy年MM月dd日 HH:mm:ss
     */
    static QString currentDateTimeChinese();

    // ==============================================
    // 2. 时间戳（秒 / 毫秒）
    // ==============================================

    /**
     * @brief  获取当前秒级时间戳
     * @return qint64
     */
    static qint64 currentTimestamp();

    /**
     * @brief  获取当前毫秒级时间戳
     * @return qint64
     */
    static qint64 currentTimestampMs();

    /**
     * @brief  秒级时间戳 → 标准日期时间字符串
     * @param  ts 秒时间戳
     * @return yyyy-MM-dd HH:mm:ss
     */
    static QString timestampToDateTime(qint64 ts);

    /**
     * @brief  毫秒级时间戳 → 标准日期时间字符串
     * @param  tsMs 毫秒时间戳
     * @return yyyy-MM-dd HH:mm:ss
     */
    static QString timestampMsToDateTime(qint64 tsMs);

    /**
     * @brief  标准时间字符串 → 秒级时间戳
     * @param  dtStr yyyy-MM-dd HH:mm:ss
     * @return 时间戳（失败返回0）
     */
    static qint64 dateTimeToTimestamp(const QString &dtStr);

    /**
     * @brief  标准时间字符串 → 毫秒级时间戳
     * @param  dtStr yyyy-MM-dd HH:mm:ss
     * @return 毫秒时间戳
     */
    static qint64 dateTimeToTimestampMs(const QString &dtStr);

    // ==============================================
    // 3. 自定义格式化
    // ==============================================

    /**
     * @brief  格式化 QDateTime
     * @param  dt 时间对象
     * @param  fmt 格式串，默认 yyyy-MM-dd HH:mm:ss
     * @return 格式化字符串
     */
    static QString format(const QDateTime &dt, const QString &fmt = "yyyy-MM-dd HH:mm:ss");

    /**
     * @brief  格式化 QDate
     * @return yyyy-MM-dd
     */
    static QString formatDate(const QDate &d);

    /**
     * @brief  格式化 QTime
     * @return HH:mm:ss
     */
    static QString formatTime(const QTime &t);

    // ==============================================
    // 4. 时间差计算
    // ==============================================

    /**
     * @brief  计算两个时间相差多少秒
     * @param  s 开始时间
     * @param  e 结束时间
     * @return 秒数
     */
    static qint64 secsBetween(const QDateTime &s, const QDateTime &e);

    /**
     * @brief  计算相差分钟数
     */
    static qint64 minsBetween(const QDateTime &s, const QDateTime &e);

    /**
     * @brief  计算相差小时数
     */
    static qint64 hoursBetween(const QDateTime &s, const QDateTime &e);

    /**
     * @brief  计算相差天数
     */
    static qint64 daysBetween(const QDateTime &s, const QDateTime &e);

    // ==============================================
    // 5. 秒/毫秒 → 00:00:00 格式
    // ==============================================

    /**
     * @brief  秒数 → 00:00:00
     * @param  sec 秒
     * @return HH:mm:ss
     */
    static QString secToHMS(qint64 sec);

    /**
     * @brief  秒数 → 00:00
     * @return HH:mm
     */
    static QString secToHM(qint64 sec);

    /**
     * @brief  毫秒 → 00:00:00.000
     * @param  msec 毫秒
     * @return HH:mm:ss.zzz
     */
    static QString msecToHMSzzz(qint64 msec);

    // ==============================================
    // 6. 友好时间显示
    // ==============================================

    /**
     * @brief  秒时间戳 → 友好时间
     * @example 刚刚、5分钟前、2小时前
     */
    static QString friendlyTime(qint64 timestamp);

    /**
     * @brief  QDateTime → 友好时间
     */
    static QString friendlyTime(const QDateTime &dt);

    // ==============================================
    // 7. UTC ↔ 本地时间
    // ==============================================

    /**
     * @brief  UTC时间 → 本地时间
     */
    static QDateTime utcToLocal(const QDateTime &utcDt);

    /**
     * @brief  本地时间 → UTC时间
     */
    static QDateTime localToUtc(const QDateTime &localDt);

    // ==============================================
    // 8. 时间加减（Qt5.14.2 兼容）
    // ==============================================

    /**
     * @brief  增加/减少天数
     */
    static QDateTime addDays(const QDateTime &dt, int days);

    /**
     * @brief  增加/减少小时数
     */
    static QDateTime addHours(const QDateTime &dt, int hours);

    /**
     * @brief  增加/减少月数
     */
    static QDateTime addMonths(const QDateTime &dt, int months);

    /**
     * @brief  增加/减少年数
     */
    static QDateTime addYears(const QDateTime &dt, int years);

    // ==============================================
    // 9. 日期判断
    // ==============================================

    /**
     * @brief  判断是否闰年
     */
    static bool isLeapYear(int year);

    /**
     * @brief  判断是否今天
     */
    static bool isToday(const QDate &d);

    /**
     * @brief  判断是否昨天
     */
    static bool isYesterday(const QDate &d);

    /**
     * @brief  判断是否同一天
     */
    static bool isSameDay(const QDateTime &a, const QDateTime &b);

    /**
     * @brief  判断是否同一月
     */
    static bool isSameMonth(const QDateTime &a, const QDateTime &b);

    /**
     * @brief  判断是否同一年
     */
    static bool isSameYear(const QDateTime &a, const QDateTime &b);

    // ==============================================
    // 10. 星期
    // ==============================================

    /**
     * @brief  获取中文星期
     * @return 周一、周二...周日
     */
    static QString weekDayChinese(const QDate &d);

    /**
     * @brief  获取英文星期
     * @return Monday、Tuesday...
     */
    static QString weekDayEnglish(const QDate &d);

    // ==============================================
    // 11. 合法性校验
    // ==============================================

    /**
     * @brief  校验日期时间字符串是否合法
     * @param  str 时间串
     * @param  fmt 格式
     * @return bool
     */
    static bool isValidDateTime(const QString &str, const QString &fmt = "yyyy-MM-dd HH:mm:ss");

    /**
     * @brief  校验日期是否合法
     */
    static bool isValidDate(const QString &str, const QString &fmt = "yyyy-MM-dd");

    // ==============================================
    // 12. 唯一时间字符串
    // ==============================================

    /**
     * @brief  生成全局唯一时间串
     * @return yyyyMMddHHmmsszzz
     * @example 20250501120000123
     */
    static QString uniqueTimeStr();
};

#endif // TIMEUTILS_H
