#ifndef INIUTILS_H
#define INIUTILS_H

#include <QString>
#include <QMutex>
#include <QSettings>
#include <QStringList>
#include <QMap>
#include <QByteArray>

/**
 * @brief 增强版INI配置文件读写工具类
 * @details 支持多文件操作、全类型读写、批量操作、文件备份、线程安全、跨平台
 * @author Qt 5.14.2 专用
 * @note 每个对象独立管理一个INI文件，可创建多个对象操作多个INI
 */
class IniUtils
{
public:
    /**
     * @brief 构造函数 - 绑定INI文件路径
     * @param filePath INI文件路径（相对路径/绝对路径均可）
     */
    explicit IniUtils(const QString& filePath);

    /**
     * @brief 析构函数 - 自动释放资源
     */
    ~IniUtils();

    // ==============================================
    // 基础文件操作
    // ==============================================
    /**
     * @brief 获取当前绑定的INI文件路径
     * @return 文件完整路径
     */
    QString getFilePath() const;

    /**
     * @brief 判断INI文件是否存在
     * @return 存在返回true，不存在返回false
     */
    bool isFileExist() const;

    /**
     * @brief 重新从硬盘加载INI文件（放弃内存中未同步的修改）
     */
    void reloadFile();

    /**
     * @brief 立即同步内存数据到硬盘文件
     */
    void syncToFile();

    /**
     * @brief 备份当前INI文件到指定路径
     * @param backupPath 备份文件保存路径
     * @return 备份成功返回true，失败返回false
     */
    bool backupFile(const QString& backupPath);

    // ==============================================
    // 基础写入操作（支持所有常用类型）
    // ==============================================
    /**
     * @brief 写入字符串类型值
     * @param group 分组名（如：UserInfo）
     * @param key 键名
     * @param value 写入的值
     */
    void writeString(const QString& group, const QString& key, const QString& value);

    /**
     * @brief 写入整型值
     */
    void writeInt(const QString& group, const QString& key, int value);

    /**
     * @brief 写入浮点型值
     */
    void writeDouble(const QString& group, const QString& key, double value);

    /**
     * @brief 写入布尔值
     */
    void writeBool(const QString& group, const QString& key, bool value);

    /**
     * @brief 写入字节数组（用于二进制/加密数据）
     */
    void writeByteArray(const QString& group, const QString& key, const QByteArray& value);

    // ==============================================
    // 基础读取操作（支持默认值，读取失败自动兜底）
    // ==============================================
    /**
     * @brief 读取字符串
     * @param defaultValue 键不存在时返回的默认值
     */
    QString readString(const QString& group, const QString& key, const QString& defaultValue = "");

    /**
     * @brief 读取整型
     */
    int readInt(const QString& group, const QString& key, int defaultValue = 0);

    /**
     * @brief 读取浮点型
     */
    double readDouble(const QString& group, const QString& key, double defaultValue = 0.0);

    /**
     * @brief 读取布尔值
     */
    bool readBool(const QString& group, const QString& key, bool defaultValue = false);

    /**
     * @brief 读取字节数组
     */
    QByteArray readByteArray(const QString& group, const QString& key, const QByteArray& defaultValue = QByteArray());

    // ==============================================
    // 批量操作（超级实用，效率极高）
    // ==============================================
    /**
     * @brief 批量写入键值对（一次性写入多个键）
     * @param group 目标分组
     * @param keyValueMap 键值对集合
     */
    void writeBatch(const QString& group, const QMap<QString, QString>& keyValueMap);

    /**
     * @brief 批量读取分组下所有键值对
     * @return 该分组下所有 键=值 集合
     */
    QMap<QString, QString> readBatch(const QString& group);

    /**
     * @brief 批量删除分组下多个键
     * @param group 分组
     * @param keyList 要删除的键列表
     */
    void removeKeys(const QString& group, const QStringList& keyList);

    // ==============================================
    // 分组高级操作
    // ==============================================
    /**
     * @brief 获取INI文件中所有分组名称
     * @return 分组列表
     */
    QStringList getAllGroups();

    /**
     * @brief 获取指定分组下所有键名称
     */
    QStringList getGroupKeys(const QString& group);

    /**
     * @brief 判断分组是否存在
     */
    bool isGroupExist(const QString& group);

    /**
     * @brief 判断指定分组下的键是否存在
     */
    bool isKeyExist(const QString& group, const QString& key);

    // ==============================================
    // 删除/清空操作
    // ==============================================
    /**
     * @brief 删除单个键
     */
    void removeKey(const QString& group, const QString& key);

    /**
     * @brief 删除整个分组（包含所有键）
     */
    void removeGroup(const QString& group);

    /**
     * @brief 清空整个INI文件所有内容
     */
    void clearAll();

private:
    QSettings* m_pSettings;     // Qt原生INI操作对象
    QString m_filePath;         // 当前INI文件路径
    QMutex m_mutex;             // 线程互斥锁，保证多线程安全
};

#endif // INIUTILS_H
