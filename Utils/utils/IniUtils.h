#ifndef INIUTILS_H
#define INIUTILS_H

#include <QString>
#include <QMutex>
#include <QSettings>
#include <QStringList>
#include <QMap>
#include <QByteArray>

/**
 * @brief 增强版 INI 配置文件读写工具类
 * @details
 * - 支持多文件操作（每个对象独立管理一个 INI 文件）
 * - 全类型读写（字符串、整数、浮点数、布尔、字节数组）
 * - 批量操作、分组管理、文件备份
 * - 线程安全（使用非递归互斥锁，通过拆分无锁私有函数避免递归）
 * - 跨平台（Windows / Linux / macOS）
 * - 可配置自动同步（性能优化）
 * @note 适用于 Qt 5.12.9+
 * @warning 禁止拷贝对象，请使用指针或引用传递
 */
class IniUtils
{
public:
    /**
     * @brief 构造函数 - 绑定 INI 文件路径
     * @param filePath INI 文件路径（相对/绝对路径均可）
     * @param autoSync 是否每次写入操作后自动同步到磁盘（默认 true，保持数据安全）
     *                 若设为 false，需手动调用 flush() 或 syncToFile() 保存修改
     */
    explicit IniUtils(const QString& filePath, bool autoSync = true);

    /**
     * @brief 析构函数 - 自动同步并释放资源
     */
    ~IniUtils();

    // 禁用拷贝（防止悬空指针和双重删除）
    IniUtils(const IniUtils&) = delete;
    IniUtils& operator=(const IniUtils&) = delete;

    // ==============================================
    // 基础文件操作
    // ==============================================

    QString getFilePath() const;
    bool isFileExist() const;
    void reloadFile();
    void syncToFile();
    void flush();
    bool backupFile(const QString& backupPath);
    void setAutoSync(bool enabled);
    bool autoSync() const;

    // ==============================================
    // 基础写入操作
    // ==============================================

    void writeString(const QString& group, const QString& key, const QString& value);
    void writeInt(const QString& group, const QString& key, int value);
    void writeDouble(const QString& group, const QString& key, double value, int precision = 6);
    void writeBool(const QString& group, const QString& key, bool value);
    void writeByteArray(const QString& group, const QString& key, const QByteArray& value);

    // ==============================================
    // 基础读取操作
    // ==============================================

    QString readString(const QString& group, const QString& key, const QString& defaultValue = "") const;
    int readInt(const QString& group, const QString& key, int defaultValue = 0) const;
    double readDouble(const QString& group, const QString& key, double defaultValue = 0.0) const;
    bool readBool(const QString& group, const QString& key, bool defaultValue = false) const;
    QByteArray readByteArray(const QString& group, const QString& key, const QByteArray& defaultValue = QByteArray()) const;

    // ==============================================
    // 批量操作
    // ==============================================

    void writeBatch(const QString& group, const QMap<QString, QString>& keyValueMap);
    QMap<QString, QString> readBatch(const QString& group) const;
    void removeKeys(const QString& group, const QStringList& keyList);

    // ==============================================
    // 分组高级操作
    // ==============================================

    QStringList getAllGroups() const;
    QStringList getGroupKeys(const QString& group) const;
    bool isGroupExist(const QString& group) const;
    bool isKeyExist(const QString& group, const QString& key) const;

    // ==============================================
    // 删除/清空操作
    // ==============================================

    void removeKey(const QString& group, const QString& key);
    void removeGroup(const QString& group);
    void clearAll();

private:
    /**
     * @brief 内部同步实现（不加锁，由调用者保证互斥）
     */
    void doSyncToFile();

    /**
     * @brief 内部写入实现（不加锁）
     */
    void doWriteString(const QString& group, const QString& key, const QString& value);

    QSettings*      m_pSettings;    // Qt 原生 INI 操作对象
    QString         m_filePath;     // 当前 INI 文件路径
    mutable QMutex  m_mutex;        // 线程互斥锁（非递归，性能更优）
    bool            m_autoSync;     // 是否自动同步
};

#endif // INIUTILS_H
