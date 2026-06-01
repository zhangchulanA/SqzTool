#include "TimeUtils.h"

// ==============================
// 当前时间
// ==============================
QString TimeUtils::currentDateTime() {
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

QString TimeUtils::currentDate() {
    return QDate::currentDate().toString("yyyy-MM-dd");
}

QString TimeUtils::currentTime() {
    return QTime::currentTime().toString("HH:mm:ss");
}

QString TimeUtils::currentDateTimeForFile() {
    return QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
}

QString TimeUtils::currentDateTimeChinese() {
    return QDateTime::currentDateTime().toString("yyyy年MM月dd日 HH:mm:ss");
}

// ==============================
// 时间戳
// ==============================
qint64 TimeUtils::currentTimestamp() {
    return QDateTime::currentDateTime().toSecsSinceEpoch();
}

qint64 TimeUtils::currentTimestampMs() {
    return QDateTime::currentDateTime().toMSecsSinceEpoch();
}

QString TimeUtils::timestampToDateTime(qint64 ts) {
    return ts <= 0 ? "" : QDateTime::fromSecsSinceEpoch(ts).toString("yyyy-MM-dd HH:mm:ss");
}

QString TimeUtils::timestampMsToDateTime(qint64 tsMs) {
    return tsMs <= 0 ? "" : QDateTime::fromMSecsSinceEpoch(tsMs).toString("yyyy-MM-dd HH:mm:ss");
}

qint64 TimeUtils::dateTimeToTimestamp(const QString &dtStr) {
    QDateTime dt = QDateTime::fromString(dtStr, "yyyy-MM-dd HH:mm:ss");
    return dt.isValid() ? dt.toSecsSinceEpoch() : 0;
}

qint64 TimeUtils::dateTimeToTimestampMs(const QString &dtStr) {
    QDateTime dt = QDateTime::fromString(dtStr, "yyyy-MM-dd HH:mm:ss");
    return dt.isValid() ? dt.toMSecsSinceEpoch() : 0;
}

// ==============================
// 格式化
// ==============================
QString TimeUtils::format(const QDateTime &dt, const QString &fmt) {
    return dt.isValid() ? dt.toString(fmt) : "";
}

QString TimeUtils::formatDate(const QDate &d) {
    return d.toString("yyyy-MM-dd");
}

QString TimeUtils::formatTime(const QTime &t) {
    return t.toString("HH:mm:ss");
}

// ==============================
// 时间差
// ==============================
qint64 TimeUtils::secsBetween(const QDateTime &s, const QDateTime &e) {
    return s.secsTo(e);
}

qint64 TimeUtils::minsBetween(const QDateTime &s, const QDateTime &e) {
    return s.secsTo(e) / 60;
}

qint64 TimeUtils::hoursBetween(const QDateTime &s, const QDateTime &e) {
    return s.secsTo(e) / 3600;
}

qint64 TimeUtils::daysBetween(const QDateTime &s, const QDateTime &e) {
    return s.secsTo(e) / (3600 * 24);
}

// ==============================
// 秒/毫秒 → 00:00:00
// ==============================
QString TimeUtils::secToHMS(qint64 sec) {
    if (sec < 0) return "00:00:00";
    int h = sec / 3600;
    int m = (sec % 3600) / 60;
    int s = sec % 60;
    return QString("%1:%2:%3").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0'));
}

QString TimeUtils::secToHM(qint64 sec) {
    if (sec < 0) return "00:00";
    int h = sec / 3600;
    int m = (sec % 3600) / 60;
    return QString("%1:%2").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0'));
}

QString TimeUtils::msecToHMSzzz(qint64 msec) {
    if (msec < 0) return "00:00:00.000";
    qint64 sec = msec / 1000;
    int z = msec % 1000;
    int h = sec / 3600;
    int m = (sec % 3600) / 60;
    int s = sec % 60;
    return QString("%1:%2:%3.%4").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0')).arg(z,3,10,QChar('0'));
}

// ==============================
// 友好时间
// ==============================
QString TimeUtils::friendlyTime(qint64 timestamp) {
    return friendlyTime(QDateTime::fromSecsSinceEpoch(timestamp));
}

QString TimeUtils::friendlyTime(const QDateTime &dt) {
    if (!dt.isValid()) return "未知时间";
    QDateTime now = QDateTime::currentDateTime();
    qint64 sec = dt.secsTo(now);

    if (sec < 0) return "未来时间";
    if (sec < 60) return "刚刚";
    if (sec < 3600) return QString("%1分钟前").arg(sec/60);
    if (sec < 3600*24) return QString("%1小时前").arg(sec/3600);
    if (sec < 3600*24*3) return QString("%1天前").arg(sec/(3600*24));
    return dt.toString("yyyy-MM-dd");
}

// ==============================
// UTC ↔ 本地
// ==============================
QDateTime TimeUtils::utcToLocal(const QDateTime &utcDt) {
    return utcDt.toLocalTime();
}

QDateTime TimeUtils::localToUtc(const QDateTime &localDt) {
    return localDt.toUTC();
}

// ==============================
// 时间加减
// ==============================
QDateTime TimeUtils::addDays(const QDateTime &dt, int days) {
    return dt.addDays(days);
}

QDateTime TimeUtils::addHours(const QDateTime &dt, int hours) {
    // 1小时 = 3600 秒，通过 addSecs 实现
        return dt.addSecs(hours * 3600);
}

QDateTime TimeUtils::addMonths(const QDateTime &dt, int months) {
    return dt.addMonths(months);
}

QDateTime TimeUtils::addYears(const QDateTime &dt, int years) {
    return dt.addYears(years);
}

// ==============================
// 日期判断
// ==============================
bool TimeUtils::isLeapYear(int year) {
    return QDate::isLeapYear(year);
}

bool TimeUtils::isToday(const QDate &d) {
    return d == QDate::currentDate();
}

bool TimeUtils::isYesterday(const QDate &d) {
    return d == QDate::currentDate().addDays(-1);
}

bool TimeUtils::isSameDay(const QDateTime &a, const QDateTime &b) {
    return a.date() == b.date();
}

bool TimeUtils::isSameMonth(const QDateTime &a, const QDateTime &b) {
    return a.date().year() == b.date().year() && a.date().month() == b.date().month();
}

bool TimeUtils::isSameYear(const QDateTime &a, const QDateTime &b) {
    return a.date().year() == b.date().year();
}

// ==============================
// 星期
// ==============================
QString TimeUtils::weekDayChinese(const QDate &d) {
    static QStringList ws = {"周日","周一","周二","周三","周四","周五","周六"};
    return ws.value(d.dayOfWeek() % 7);
}

QString TimeUtils::weekDayEnglish(const QDate &d) {
    return d.toString("dddd");
}

// ==============================
// 校验
// ==============================
bool TimeUtils::isValidDateTime(const QString &str, const QString &fmt) {
    return QDateTime::fromString(str, fmt).isValid();
}

bool TimeUtils::isValidDate(const QString &str, const QString &fmt) {
    return QDate::fromString(str, fmt).isValid();
}

// ==============================
// 唯一时间串
// ==============================
QString TimeUtils::uniqueTimeStr() {
    return QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz");
}
