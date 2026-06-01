#include "StringUtils.h"
#include <QTextCodec>
#include <QRegExp>
#include <QUrl>
#include <cstdlib>
#include <ctime>
#include <QSet>
#include <QLocale>
#include <QTextCodec>
//=============================
// 空值处理
//=============================
bool StringUtils::isEmpty(const QString &str) { return str.isEmpty(); }
bool StringUtils::isBlank(const QString &str) { return str.trimmed().isEmpty(); }
QString StringUtils::safeStr(const QString &str, const QString &defaultVal) { return isBlank(str) ? defaultVal : str; }
QString StringUtils::trim(const QString &str) { return str.trimmed(); }
QString StringUtils::trimAll(const QString &str) { return str.simplified().remove(' '); }

//=============================
// 补齐
//=============================
QString StringUtils::zeroFill(int num, int len) { return QString("%1").arg(num, len, 10, QChar('0')); }
QString StringUtils::zeroFillStr(const QString &numStr, int len) { return padLeft(numStr, len, '0'); }
QString StringUtils::padLeft(const QString &str, int len, QChar ch) { return str.rightJustified(len, ch); }
QString StringUtils::padRight(const QString &str, int len, QChar ch) { return str.leftJustified(len, ch); }
QString StringUtils::padCenter(const QString &str, int len, QChar ch) {
    int left = (len - str.length()) / 2;
    int right = len - str.length() - left;
    return QString(left, ch) + str + QString(right, ch);
}

//=============================
// 切割合并
//=============================
QStringList StringUtils::split(const QString &str, const QString &sep, bool skipEmpty) {
    return str.split(sep, skipEmpty ? QString::SkipEmptyParts : QString::KeepEmptyParts);
}
QStringList StringUtils::splitLines(const QString &str, bool skipEmpty) {
    return str.split(QRegExp("[\r\n]"), skipEmpty ? QString::SkipEmptyParts : QString::KeepEmptyParts);
}
QString StringUtils::join(const QStringList &list, const QString &sep) { return list.join(sep); }
QStringList StringUtils::removeEmpty(const QStringList &list) {
    QStringList res;
    foreach (auto s, list) if (!isBlank(s)) res << s;
    return res;
}
QStringList StringUtils::removeDuplicate(const QStringList &list) {
    QStringList res;
    QSet<QString> set;
    foreach (auto s, list) if (!set.contains(s)) { set << s; res << s; }
    return res;
}

//=============================
// 替换
//=============================
QString StringUtils::replace(const QString &str, const QString &from, const QString &to) {
    QString s = str; return s.replace(from, to);
}
QString StringUtils::replaceMap(const QString &str, const QMap<QString, QString> &map) {
    QString s = str;
    for (auto it = map.begin(); it != map.end(); ++it) s.replace(it.key(), it.value());
    return s;
}
QString StringUtils::regexReplace(const QString &str, const QString &pattern, const QString &to) {
    QString s = str; return s.replace(QRegExp(pattern), to);
}

//=============================
// 大小写
//=============================
QString StringUtils::upper(const QString &str) { return str.toUpper(); }
QString StringUtils::lower(const QString &str) { return str.toLower(); }
QString StringUtils::firstUpper(const QString &str) {
    if (str.isEmpty()) return str;
    return str.left(1).toUpper() + str.mid(1);
}
QString StringUtils::camelToUnderline(const QString &camel) {
    QString r;
    for (int i = 0; i < camel.size(); ++i) {
        if (camel[i].isUpper() && i > 0) r += "_";
        r += camel[i].toLower();
    }
    return r;
}
QString StringUtils::underlineToCamel(const QString &line) {
    QString r;
    bool cap = false;
    foreach (auto c, line) {
        if (c == '_') { cap = true; continue; }
        r += cap ? c.toUpper() : c;
        cap = false;
    }
    return r;
}

//=============================
// 编码转换
//=============================
QString StringUtils::utf8ToGbk(const QString &utf8) {
    QTextCodec *codec = QTextCodec::codecForName("GBK");
    return codec ? codec->toUnicode(utf8.toUtf8()) : utf8;
}
QString StringUtils::gbkToUtf8(const QString &gbk) {
    QTextCodec *codec = QTextCodec::codecForName("GBK");
    return codec ? codec->fromUnicode(gbk) : gbk;
}
QByteArray StringUtils::toUtf8Bytes(const QString &str) { return str.toUtf8(); }
QString StringUtils::fromUtf8Bytes(const QByteArray &bytes) { return QString::fromUtf8(bytes); }

//=============================
// 脱敏
//=============================
QString StringUtils::maskPhone(const QString &phone) {
    return phone.length() == 11 ? phone.left(3) + "****" + phone.right(4) : phone;
}
QString StringUtils::maskIdCard(const QString &idCard) {
    return idCard.length() >= 10 ? idCard.left(6) + "********" + idCard.right(4) : idCard;
}
QString StringUtils::maskBankCard(const QString &card) {
    return card.length() > 8 ? card.left(4) + " **** **** " + card.right(4) : card;
}
QString StringUtils::maskEmail(const QString &email) {
    int at = email.indexOf('@');
    return at > 1 ? email.left(1) + "****" + email.mid(at) : email;
}
QString StringUtils::maskMiddle(const QString &str, int l, int r, const QString &m) {
    return str.length() > l + r ? str.left(l) + m + str.right(r) : str;
}

//=============================
// 版本比较
//=============================
int StringUtils::compareVersion(const QString &v1, const QString &v2) {
    QStringList l1 = split(v1, ".");
    QStringList l2 = split(v2, ".");
    int n = qMax(l1.size(), l2.size());
    for (int i = 0; i < n; i++) {
        int a = i < l1.size() ? l1[i].toInt() : 0;
        int b = i < l2.size() ? l2[i].toInt() : 0;
        if (a > b) return 1;
        if (a < b) return -1;
    }
    return 0;
}
bool StringUtils::versionGreater(const QString &v1, const QString &v2) { return compareVersion(v1, v2) > 0; }
bool StringUtils::versionEqual(const QString &v1, const QString &v2) { return compareVersion(v1, v2) == 0; }

//=============================
// 校验
//=============================
bool StringUtils::isNumber(const QString &str) { return QRegExp("^\\d+$").exactMatch(str); }
bool StringUtils::isLetter(const QString &str) { return QRegExp("^[a-zA-Z]+$").exactMatch(str); }
bool StringUtils::isLetterNum(const QString &str) { return QRegExp("^[a-zA-Z0-9]+$").exactMatch(str); }
bool StringUtils::isPhone(const QString &str) { return QRegExp("^1[3-9]\\d{9}$").exactMatch(str); }
bool StringUtils::isEmail(const QString &str) { return QRegExp("^\\w+([-+.]\\w+)*@\\w+([-.]\\w+)*\\.\\w+").exactMatch(str); }
bool StringUtils::isStrongPassword(const QString &str, int minLen) {
    if (str.length() < minLen) {
        return false;
    }
    // Qt5.14.2 用 indexIn 替代 hasMatch
    QRegExp numRx(".*[0-9].*");
    QRegExp letterRx(".*[a-zA-Z].*");
    QRegExp specialRx(".*[^a-zA-Z0-9].*");

    return numRx.indexIn(str) != -1 &&
           letterRx.indexIn(str) != -1 &&
           specialRx.indexIn(str) != -1;
}

//=============================
// 提取
//=============================
QString StringUtils::extractNumber(const QString &str) {
    QString result = str;
    QRegExp rx("[^0-9]");
    return result.replace(rx, "");
}
QString StringUtils::extractChinese(const QString &str) {
    QString result = str;
    QRegExp rx("[^\\u4e00-\\u9fa5]");
    return result.replace(rx, "");
}
QString StringUtils::extractEnglish(const QString &str) {
    QString result = str;
    QRegExp rx("[^a-zA-Z]");
    return result.replace(rx, "");
}
QStringList StringUtils::extractPhones(const QString &str) {
    QStringList res;
    QRegExp rx("1[3-9]\\d{9}");
    int pos = 0;
    while ((pos = rx.indexIn(str, pos)) != -1) { res << rx.cap(0); pos += rx.matchedLength(); }
    return res;
}
QStringList StringUtils::extractEmails(const QString &str) {
    QStringList res;
    QRegExp rx("\\w+([-+.]\\w+)*@\\w+([-.]\\w+)*\\.\\w+");
    int pos = 0;
    while ((pos = rx.indexIn(str, pos)) != -1) { res << rx.cap(0); pos += rx.matchedLength(); }
    return res;
}

//=============================
// 格式转换
//=============================
QString StringUtils::toThousandSep(double num) { return QLocale::system().toString(num, 'f', 2); }
QString StringUtils::ipIntToStr(quint32 ip) { return QString("%1.%2.%3.%4").arg(ip>>24&0xff).arg(ip>>16&0xff).arg(ip>>8&0xff).arg(ip&0xff); }
quint32 StringUtils::ipStrToInt(const QString &ip) {
    quint32 a,b,c,d; sscanf(ip.toUtf8().constData(), "%u.%u.%u.%u", &a, &b, &c, &d);
    return a<<24 | b<<16 | c<<8 | d;
}
QString StringUtils::strToHex(const QString &str) { return str.toUtf8().toHex(); }
QString StringUtils::hexToStr(const QString &hex) { return QByteArray::fromHex(hex.toUtf8()); }

//=============================
// 转义
//=============================
QString StringUtils::urlEncode(const QString &str) { return QUrl::toPercentEncoding(str); }
QString StringUtils::urlDecode(const QString &str) { return QUrl::fromPercentEncoding(str.toUtf8()); }

//=============================
// 随机字符串
//=============================
QString StringUtils::randomStr(int len, bool num, bool lower, bool upper) {
    const QString n = "0123456789";
    const QString l = "abcdefghijklmnopqrstuvwxyz";
    const QString u = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QString p;
    if (num) p += n;
    if (lower) p += l;
    if (upper) p += u;
    if (p.isEmpty()) return "";
    qsrand(time(0));
    QString r;
    for (int i=0; i<len; i++) r += p.at(qrand() % p.size());
    return r;
}
QString StringUtils::randomCode(int len) {
    qsrand(time(0));
    QString r;
    for (int i=0; i<len; i++) r += QChar('0' + qrand()%10);
    return r;
}

//=============================
// 首尾包含
//=============================
bool StringUtils::startsWith(const QString &str, const QString &prefix, bool ignoreCase) {
    return ignoreCase ? str.startsWith(prefix, Qt::CaseInsensitive) : str.startsWith(prefix);
}
bool StringUtils::endsWith(const QString &str, const QString &suffix, bool ignoreCase) {
    return ignoreCase ? str.endsWith(suffix, Qt::CaseInsensitive) : str.endsWith(suffix);
}
bool StringUtils::contains(const QString &str, const QString &sub, bool ignoreCase) {
    return ignoreCase ? str.contains(sub, Qt::CaseInsensitive) : str.contains(sub);
}

//=============================
// 相似度
//=============================
int StringUtils::similarity(const QString &a, const QString &b) {
    int n = a.size(), m = b.size();
    if (n == 0 && m == 0) return 100;
    int same = 0;
    int min = qMin(n, m);
    for (int i=0; i<min; i++) if (a[i]==b[i]) same++;
    return qMin(100, (int)(same * 100.0 / qMax(n, m)));
}
