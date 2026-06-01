#ifndef JSONUTILS_H
#define JSONUTILS_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QString>
#include <QFile>
#include <QDebug>
#include <QVariant>
#include <QStringList>

/**
 * @brief 最终完整版 JSON 工具类（Qt 5.14.2 跨平台 Windows/Ubuntu）
 * @func  文件读写 | 字符串互转 | 安全取值 | 深度操作 | 数组高级 | 合并比较 | 扁平化 | 空值清理
 *        键转换 | XML/URL互转 | BOM文件 | 错误管理 | 深拷贝 | 类型校验
 * @note  纯静态类，禁止实例化，直接调用静态方法
 */
class JsonUtils
{
public:
    JsonUtils() = delete;

    // ========================== 1. JSON 文件操作 ==========================
    /**
     * @brief  从文件读取 JSON 转为 QJsonDocument
     * @param  filePath 文件路径
     * @return 成功返回文档，失败返回空文档
     * @用法   QJsonDocument doc = JsonUtils::loadFileToDoc("test.json");
     */
    static QJsonDocument loadFileToDoc(const QString& filePath);

    /**
     * @brief  从文件读取 JSON 转为 QJsonObject
     * @用法   QJsonObject obj = JsonUtils::loadFileToObject("test.json");
     */
    static QJsonObject loadFileToObject(const QString& filePath);

    /**
     * @brief  从文件读取 JSON 转为 QJsonArray
     * @用法   QJsonArray arr = JsonUtils::loadFileToArray("array.json");
     */
    static QJsonArray loadFileToArray(const QString& filePath);

    /**
     * @brief  将 QJsonDocument 写入文件
     * @param  format true=格式化缩进 false=紧凑模式
     * @用法   JsonUtils::writeDocToFile(doc, "out.json", true);
     */
    static bool writeDocToFile(const QJsonDocument& doc, const QString& filePath, bool format = true);

    /**
     * @brief  将 QJsonObject 写入文件
     * @用法   JsonUtils::writeObjectToFile(obj, "out.json");
     */
    static bool writeObjectToFile(const QJsonObject& obj, const QString& filePath, bool format = true);

    /**
     * @brief  将 QJsonArray 写入文件
     * @用法   JsonUtils::writeArrayToFile(arr, "out.json");
     */
    static bool writeArrayToFile(const QJsonArray& arr, const QString& filePath, bool format = true);

    // ========================== 2. JSON 字符串 <-> 对象/数组 互转 ==========================
    /**
     * @brief  QJsonObject 转 JSON 字符串
     * @用法   QString str = JsonUtils::objectToString(obj, true);
     */
    static QString objectToString(const QJsonObject& obj, bool format = true);

    /**
     * @brief  QJsonArray 转 JSON 字符串
     * @用法   QString str = JsonUtils::arrayToString(arr, false);
     */
    static QString arrayToString(const QJsonArray& arr, bool format = true);

    /**
     * @brief  JSON 字符串转 QJsonObject
     * @用法   QJsonObject obj = JsonUtils::stringToObject(str);
     */
    static QJsonObject stringToObject(const QString& jsonStr);

    /**
     * @brief  JSON 字符串转 QJsonArray
     * @用法   QJsonArray arr = JsonUtils::stringToArray(str);
     */
    static QJsonArray stringToArray(const QString& jsonStr);

    // ========================== 3. 安全取值（防崩溃，带默认值） ==========================
    /**
     * @brief  安全获取 int 类型值
     * @用法   int v = JsonUtils::getInt(obj, "id", 0);
     */
    static int getInt(const QJsonObject& obj, const QString& key, int def = 0);

    /**
     * @brief  安全获取 double 类型值
     * @用法   double d = JsonUtils::getDouble(obj, "score");
     */
    static double getDouble(const QJsonObject& obj, const QString& key, double def = 0.0);

    /**
     * @brief  安全获取 QString 类型值
     * @用法   QString s = JsonUtils::getString(obj, "name", "");
     */
    static QString getString(const QJsonObject& obj, const QString& key, const QString& def = "");

    /**
     * @brief  安全获取 bool 类型值
     * @用法   bool b = JsonUtils::getBool(obj, "vip");
     */
    static bool getBool(const QJsonObject& obj, const QString& key, bool def = false);

    /**
     * @brief  安全获取子对象
     * @用法   QJsonObject o = JsonUtils::getObject(obj, "info");
     */
    static QJsonObject getObject(const QJsonObject& obj, const QString& key);

    /**
     * @brief  安全获取子数组
     * @用法   QJsonArray a = JsonUtils::getArray(obj, "list");
     */
    static QJsonArray getArray(const QJsonObject& obj, const QString& key);

    /**
     * @brief  安全获取原始 QJsonValue
     * @用法   QJsonValue v = JsonUtils::getValue(obj, "key");
     */
    static QJsonValue getValue(const QJsonObject& obj, const QString& key);

    // ========================== 4. 深度取值（支持 a.b.c 嵌套） ==========================
    /**
     * @brief  深度获取任意值
     * @用法   QJsonValue v = JsonUtils::getDeepValue(obj, "a.b.c");
     */
    static QJsonValue getDeepValue(const QJsonObject& rootObj, const QString& keyPath, const QJsonValue& def = QJsonValue());

    /**
     * @brief  深度获取字符串
     * @用法   QString city = JsonUtils::getDeepString(obj, "info.city");
     */
    static QString getDeepString(const QJsonObject& rootObj, const QString& keyPath, const QString& def = "");

    /**
     * @brief  深度获取 int
     * @用法   int age = JsonUtils::getDeepInt(obj, "user.age");
     */
    static int getDeepInt(const QJsonObject& rootObj, const QString& keyPath, int def = 0);

    // ========================== 5. 安全赋值 & 深度赋值 ==========================
    /**
     * @brief  普通赋值
     * @用法   JsonUtils::setValue(obj, "name", "张三");
     */
    static void setValue(QJsonObject& obj, const QString& key, const QJsonValue& value);

    /**
     * @brief  深度赋值（自动创建嵌套对象）
     * @用法   JsonUtils::setDeepValue(obj, "info.address", "北京");
     */
    static void setDeepValue(QJsonObject& rootObj, const QString& keyPath, const QJsonValue& value);

    // ========================== 6. 键操作（判断/删除/重命名） ==========================
    /**
     * @brief  判断是否包含键
     * @用法   bool has = JsonUtils::contains(obj, "id");
     */
    static bool contains(const QJsonObject& obj, const QString& key);

    /**
     * @brief  深度判断是否包含键
     * @用法   bool has = JsonUtils::containsDeep(obj, "info.city");
     */
    static bool containsDeep(const QJsonObject& rootObj, const QString& keyPath);

    /**
     * @brief  删除键
     * @用法   JsonUtils::removeKey(obj, "tmp");
     */
    static void removeKey(QJsonObject& obj, const QString& key);

    /**
     * @brief  深度删除键
     * @用法   JsonUtils::removeDeepKey(obj, "info.tmp");
     */
    static void removeDeepKey(QJsonObject& rootObj, const QString& keyPath);

    /**
     * @brief  获取所有键
     * @用法   QStringList keys = JsonUtils::getAllKeys(obj);
     */
    static QStringList getAllKeys(const QJsonObject& obj);

    /**
     * @brief  重命名键
     * @用法   JsonUtils::renameKey(obj, "old", "new");
     */
    static void renameKey(QJsonObject& obj, const QString& oldKey, const QString& newKey);

    // ========================== 7. 数组基础操作 ==========================
    /**
     * @brief  数组追加元素
     * @用法   JsonUtils::arrayAppend(arr, "test");
     */
    static void arrayAppend(QJsonArray& arr, const QJsonValue& value);

    /**
     * @brief  数组插入元素
     * @用法   JsonUtils::arrayInsert(arr, 1, "val");
     */
    static void arrayInsert(QJsonArray& arr, int index, const QJsonValue& value);

    /**
     * @brief  删除指定索引元素
     * @用法   JsonUtils::arrayRemoveAt(arr, 2);
     */
    static void arrayRemoveAt(QJsonArray& arr, int index);

    /**
     * @brief  替换指定索引元素
     * @用法   JsonUtils::arrayReplace(arr, 0, "newVal");
     */
    static void arrayReplace(QJsonArray& arr, int index, const QJsonValue& newValue);

    /**
     * @brief  获取指定索引元素
     * @用法   QJsonValue v = JsonUtils::arrayGet(arr, 0);
     */
    static QJsonValue arrayGet(const QJsonArray& arr, int index, const QJsonValue& def = QJsonValue());

    /**
     * @brief  查找元素索引
     * @用法   int idx = JsonUtils::arrayFind(arr, "val");
     */
    static int arrayFind(const QJsonArray& arr, const QJsonValue& value);

    // ========================== 8. 合并 & 键过滤 ==========================
    /**
     * @brief  简单合并对象
     * @用法   QJsonObject res = JsonUtils::mergeObject(obj1, obj2);
     */
    static QJsonObject mergeObject(const QJsonObject& target, const QJsonObject& source, bool overwrite = true);

    /**
     * @brief  只保留指定键
     * @用法   QJsonObject res = JsonUtils::filterKeys(obj, {"id","name"});
     */
    static QJsonObject filterKeys(const QJsonObject& obj, const QStringList& keepKeys);

    // ========================== 9. 类型校验 ==========================
    /**
     * @brief  是否为整数
     * @用法   bool ok = JsonUtils::isInt(val);
     */
    static bool isInt(const QJsonValue& val);

    /**
     * @brief  是否为字符串
     */
    static bool isString(const QJsonValue& val);

    /**
     * @brief  是否为对象
     */
    static bool isObject(const QJsonValue& val);

    /**
     * @brief  是否为数组
     */
    static bool isArray(const QJsonValue& val);

    /**
     * @brief  是否为空/未定义
     */
    static bool isNull(const QJsonValue& val);

    /**
     * @brief  对象是否为空
     */
    static bool isObjectEmpty(const QJsonObject& obj);

    /**
     * @brief  数组是否为空
     */
    static bool isArrayEmpty(const QJsonArray& arr);

    // ========================== 10. QVariant 互转 ==========================
    /**
     * @brief  QVariant 转 QJsonValue
     */
    static QJsonValue variantToJson(const QVariant& var);

    /**
     * @brief  QJsonValue 转 QVariant
     */
    static QVariant jsonToVariant(const QJsonValue& val);

    /**
     * @brief  QVariantMap 转 QJsonObject
     */
    static QJsonObject variantMapToObject(const QVariantMap& map);

    /**
     * @brief  QJsonObject 转 QVariantMap
     */
    static QVariantMap objectToVariantMap(const QJsonObject& obj);

    // ========================== 11. 格式化 ==========================
    /**
     * @brief  JSON 字符串格式化美化
     */
    static QString formatJsonString(const QString& jsonStr);

    /**
     * @brief  JSON 字符串压缩
     */
    static QString compactJsonString(const QString& jsonStr);

    // ========================== 12. 深拷贝 ==========================
    /**
     * @brief  深拷贝对象
     */
    static QJsonObject copyObject(const QJsonObject& obj);

    /**
     * @brief  深拷贝数组
     */
    static QJsonArray copyArray(const QJsonArray& arr);

    // ========================== 13. 错误信息 ==========================
    /**
     * @brief  获取最后一次错误信息
     */
    static QString getLastError();

    // ========================== 14. 高级：深度递归合并 ==========================
    /**
     * @brief  递归深度合并（嵌套对象也合并）
     * @用法   QJsonObject res = JsonUtils::mergeDeep(a, b);
     */
    static QJsonObject mergeDeep(const QJsonObject &target, const QJsonObject &source, bool overwrite = true);

    // ========================== 15. 高级：深度比较 ==========================
    /**
     * @brief  任意两个 JSON 值深度比较
     */
    static bool isEqual(const QJsonValue &a, const QJsonValue &b);

    /**
     * @brief  对象深度比较
     */
    static bool isObjectEqual(const QJsonObject &a, const QJsonObject &b);

    /**
     * @brief  数组深度比较
     */
    static bool isArrayEqual(const QJsonArray &a, const QJsonArray &b);

    // ========================== 16. 高级：扁平化/反扁平化 ==========================
    /**
     * @brief  扁平化：{a:{b:1}} → {"a.b":1}
     */
    static QJsonObject flatten(const QJsonObject &obj);

    /**
     * @brief  反扁平化：{"a.b":1} → {a:{b:1}}
     */
    static QJsonObject unflatten(const QJsonObject &flatObj);

    // ========================== 17. 高级：清理空值 ==========================
    /**
     * @brief  清理 null/空串/空对象/空数组
     */
    static QJsonObject cleanEmptyValues(const QJsonObject &obj);

    // ========================== 18. 高级：键大小写转换 ==========================
    /**
     * @brief  键名全小写
     */
    static QJsonObject keysToLower(const QJsonObject &obj);

    /**
     * @brief  键名全大写
     */
    static QJsonObject keysToUpper(const QJsonObject &obj);

    // ========================== 19. 高级：数组排序/去重/过滤 ==========================
    /**
     * @brief  数组排序
     */
    static void arraySort(QJsonArray &arr, bool ascending = true);

    /**
     * @brief  数组按对象字段排序
     */
    static void arraySortByField(QJsonArray &arr, const QString &field, bool ascending = true);

    /**
     * @brief  数组去重
     */
    static void arrayRemoveDuplicates(QJsonArray &arr);

    /**
     * @brief  数组按字段过滤
     */
    static QJsonArray arrayFilter(const QJsonArray &arr, const QString &field, const QJsonValue& matchValue);

    // ========================== 20. 高级：JSON 转 XML ==========================
    /**
     * @brief  JSON 对象转 XML 字符串
     */
    static QString jsonToXml(const QJsonObject &obj, const QString &rootName = "root");

    // ========================== 21. 高级：JSON <-> URL 参数 ==========================
    /**
     * @brief  转 URL 参数 a=1&b=2
     */
    static QString toUrlParams(const QJsonObject &obj);

    /**
     * @brief  从 URL 参数转 JSON
     */
    static QJsonObject fromUrlParams(const QString &urlParams);

    // ========================== 22. 高级：UTF-8 BOM 文件读写 ==========================
    /**
     * @brief  读取带 BOM 的 JSON（Windows 记事本兼容）
     */
    static QJsonObject loadFileWithoutBom(const QString &filePath);

    /**
     * @brief  写入带 BOM 的 JSON
     */
    static bool writeFileWithBom(const QJsonObject &obj, const QString &filePath, bool format = true);

    // ========================== 23. 高级：提取所有叶子节点值 ==========================
    /**
     * @brief  提取所有最底层叶子值
     */
    static QVariantList getAllLeafValues(const QJsonObject &obj);

private:
    static QString m_lastError;
    static void setError(const QString& err);
};

#endif // JSONUTILS_H
