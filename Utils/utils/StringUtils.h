#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QMap>

/**
 * @brief 字符串工具类
 * @class  StringUtils
 * @note   全部静态函数，无需实例化，直接调用
 * @note   空安全、线程安全、Qt5.14.2 完美运行
 */
class StringUtils
{
public:
    // ============================================================
    // 1. 空值与空白处理
    // ============================================================

    /**
     * @brief  判断字符串是否为【严格空】
     * @param  str  输入字符串
     * @return true = "" 空字符串
     * @example  StringUtils::isEmpty("") → true
     */
    static bool isEmpty(const QString &str);

    /**
     * @brief  判断字符串是否为【空白】（空/空格/换行/Tab）
     * @param  str
     * @return true = 空白内容
     * @example  StringUtils::isBlank("   \n\t") → true
     */
    static bool isBlank(const QString &str);

    /**
     * @brief  空安全取值：为空时返回默认值
     * @param  str        原字符串
     * @param  defaultVal 为空时返回的值
     * @return 安全字符串
     * @example  StringUtils::safeStr("", "未知") → "未知"
     */
    static QString safeStr(const QString &str, const QString &defaultVal = "");

    /**
     * @brief  去除字符串【两端】空格、Tab、换行
     * @param  str
     * @return 处理后字符串
     * @example  StringUtils::trim("  Hello World  ") → "Hello World"
     */
    static QString trim(const QString &str);

    /**
     * @brief  去除【所有】空格、换行、Tab
     * @param  str
     * @return 紧凑字符串
     * @example  StringUtils::trimAll("a b c \n d") → "abcd"
     */
    static QString trimAll(const QString &str);

    // ============================================================
    // 2. 补齐格式化
    // ============================================================

    /**
     * @brief  数字【前面补零】到指定长度
     * @param  num  数字
     * @param  len  目标长度
     * @return 补零字符串
     * @example  StringUtils::zeroFill(5, 3) → "005"
     */
    static QString zeroFill(int num, int len);

    /**
     * @brief  字符串【前面补零】
     * @param  numStr 数字字符串
     * @param  len    目标长度
     * @return 补零字符串
     * @example  StringUtils::zeroFillStr("7", 3) → "007"
     */
    static QString zeroFillStr(const QString &numStr, int len);

    /**
     * @brief  左补齐（右侧对齐，左侧填充）
     * @param  str  原串
     * @param  len  目标长度
     * @param  ch   填充字符
     * @return 补齐字符串
     * @example  StringUtils::padLeft("5", 3, '0') → "005"
     */
    static QString padLeft(const QString &str, int len, QChar ch = ' ');

    /**
     * @brief  右补齐（左侧对齐，右侧填充）
     * @example  padRight("AB", 4, '-') → "AB--"
     */
    static QString padRight(const QString &str, int len, QChar ch = ' ');

    /**
     * @brief  居中补齐
     * @example  padCenter("123", 7, '-') → "--123--"
     */
    static QString padCenter(const QString &str, int len, QChar ch = ' ');

    // ============================================================
    // 3. 切割、合并、去重
    // ============================================================

    /**
     * @brief  按分隔符分割字符串
     * @param  str        原串
     * @param  sep        分隔符
     * @param  skipEmpty  是否跳过空项
     * @return 分割后的列表
     * @example  split("a,b,c", ",") → ["a","b","c"]
     */
    static QStringList split(const QString &str, const QString &sep, bool skipEmpty = true);

    /**
     * @brief  按换行符分割（\n \r\n）
     */
    static QStringList splitLines(const QString &str, bool skipEmpty = true);

    /**
     * @brief  字符串列表合并
     * @example  join(["a","b","c"], "|") → "a|b|c"
     */
    static QString join(const QStringList &list, const QString &sep);

    /**
     * @brief  移除列表中的空/空白项
     */
    static QStringList removeEmpty(const QStringList &list);

    /**
     * @brief  字符串列表去重（保留顺序）
     */
    static QStringList removeDuplicate(const QStringList &list);

    // ============================================================
    // 4. 替换（普通 / 批量 / 正则）
    // ============================================================

    /**
     * @brief  替换所有指定字符串
     * @example  replace("Hello World", "World", "Qt") → "Hello Qt"
     */
    static QString replace(const QString &str, const QString &from, const QString &to);

    /**
     * @brief  批量替换（传入 QMap 键值对）
     */
    static QString replaceMap(const QString &str, const QMap<QString, QString> &map);

    /**
     * @brief  正则表达式替换
     */
    static QString regexReplace(const QString &str, const QString &pattern, const QString &to);

    // ============================================================
    // 5. 大小写、命名格式转换
    // ============================================================

    /**
     * @brief  转大写
     */
    static QString upper(const QString &str);

    /**
     * @brief  转小写
     */
    static QString lower(const QString &str);

    /**
     * @brief  首字母大写，其余不变
     * @example  firstUpper("hello") → "Hello"
     */
    static QString firstUpper(const QString &str);

    /**
     * @brief  驼峰命名 → 下划线命名
     * @example  camelToUnderline("userName") → "user_name"
     */
    static QString camelToUnderline(const QString &camel);

    /**
     * @brief  下划线命名 → 驼峰命名
     * @example  underlineToCamel("user_name") → "userName"
     */
    static QString underlineToCamel(const QString &line);

    // ============================================================
    // 6. 编码转换
    // ============================================================

    /**
     * @brief  UTF8 → GBK
     */
    static QString utf8ToGbk(const QString &utf8);

    /**
     * @brief  GBK → UTF8
     */
    static QString gbkToUtf8(const QString &gbk);

    /**
     * @brief  QString → UTF8 字节数组
     */
    static QByteArray toUtf8Bytes(const QString &str);

    /**
     * @brief  UTF8 字节数组 → QString
     */
    static QString fromUtf8Bytes(const QByteArray &bytes);

    // ============================================================
    // 7. 隐私脱敏
    // ============================================================

    /**
     * @brief  手机号脱敏
     * @example  maskPhone("13812345678") → "138****5678"
     */
    static QString maskPhone(const QString &phone);

    /**
     * @brief  身份证脱敏
     * @example  maskIdCard("110101199001011234") → "110101********1234"
     */
    static QString maskIdCard(const QString &idCard);

    /**
     * @brief  银行卡脱敏
     */
    static QString maskBankCard(const QString &card);

    /**
     * @brief  邮箱脱敏
     * @example  maskEmail("test@xxx.com") → "t****@xxx.com"
     */
    static QString maskEmail(const QString &email);

    /**
     * @brief  通用中间脱敏
     * @param  keepLeft  左边保留位数
     * @param  keepRight 右边保留位数
     * @param  mask      掩码
     * @example  maskMiddle("123456789", 2, 2) → "12*******89"
     */
    static QString maskMiddle(const QString &str, int keepLeft, int keepRight, const QString &mask = "****");

    // ============================================================
    // 8. 版本号比较
    // ============================================================

    /**
     * @brief  版本号比较
     * @return  1 = v1 > v2  |  0 = 相等  |  -1 = v2 > v1
     * @example  compareVersion("1.2.3", "1.1.9") → 1
     */
    static int compareVersion(const QString &v1, const QString &v2);

    /**
     * @brief  判断 v1 > v2
     */
    static bool versionGreater(const QString &v1, const QString &v2);

    /**
     * @brief  判断版本号相等
     */
    static bool versionEqual(const QString &v1, const QString &v2);

    // ============================================================
    // 9. 格式校验（正则）
    // ============================================================

    /**
     * @brief  是否纯数字
     */
    static bool isNumber(const QString &str);

    /**
     * @brief  是否纯字母
     */
    static bool isLetter(const QString &str);

    /**
     * @brief  是否字母+数字
     */
    static bool isLetterNum(const QString &str);

    /**
     * @brief  是否合法手机号
     */
    static bool isPhone(const QString &str);

    /**
     * @brief  是否合法邮箱
     */
    static bool isEmail(const QString &str);

    /**
     * @brief  是否强密码（字母+数字+特殊字符）
     */
    static bool isStrongPassword(const QString &str, int minLen = 8);

    // ============================================================
    // 10. 内容提取
    // ============================================================

    /**
     * @brief  提取字符串中所有数字
     * @example  extractNumber("abc123中文456") → "123456"
     */
    static QString extractNumber(const QString &str);

    /**
     * @brief  提取所有中文
     */
    static QString extractChinese(const QString &str);

    /**
     * @brief  提取所有英文
     */
    static QString extractEnglish(const QString &str);

    /**
     * @brief  提取文本中所有手机号
     */
    static QStringList extractPhones(const QString &str);

    /**
     * @brief  提取文本中所有邮箱
     */
    static QStringList extractEmails(const QString &str);

    // ============================================================
    // 11. 格式转换
    // ============================================================

    /**
     * @brief  数字转千分位金额格式
     * @example  toThousandSep(12345.67) → "12,345.67"
     */
    static QString toThousandSep(double num);

    /**
     * @brief  IP整数 → IP字符串
     */
    static QString ipIntToStr(quint32 ip);

    /**
     * @brief  IP字符串 → 整数
     */
    static quint32 ipStrToInt(const QString &ip);

    /**
     * @brief  字符串 → 十六进制
     */
    static QString strToHex(const QString &str);

    /**
     * @brief  十六进制 → 字符串
     */
    static QString hexToStr(const QString &hex);

    // ============================================================
    // 12. URL 转义 / 反转义
    // ============================================================

    /**
     * @brief  URL 编码
     */
    static QString urlEncode(const QString &str);

    /**
     * @brief  URL 解码
     */
    static QString urlDecode(const QString &str);

    // ============================================================
    // 13. 随机字符串
    // ============================================================

    /**
     * @brief  生成随机字符串
     * @param  len     长度
     * @param  num     包含数字
     * @param  lower   包含小写字母
     * @param  upper   包含大写字母
     * @return 随机串
     */
    static QString randomStr(int len, bool num = true, bool lower = true, bool upper = true);

    /**
     * @brief  生成纯数字验证码
     * @example  randomCode(6) → "582109"
     */
    static QString randomCode(int len);

    // ============================================================
    // 14. 首尾 / 包含判断
    // ============================================================

    /**
     * @brief  判断是否以指定字符串开头
     */
    static bool startsWith(const QString &str, const QString &prefix, bool ignoreCase = false);

    /**
     * @brief  判断是否以指定字符串结尾
     */
    static bool endsWith(const QString &str, const QString &suffix, bool ignoreCase = false);

    /**
     * @brief  判断是否包含子串
     */
    static bool contains(const QString &str, const QString &sub, bool ignoreCase = false);

    // ============================================================
    // 15. 字符串相似度（0~100）
    // ============================================================

    /**
     * @brief  计算两个字符串相似度
     * @return 0~100 分
     * @example  similarity("hello", "hell0") → 80
     */
    static int similarity(const QString &a, const QString &b);
};

#endif
