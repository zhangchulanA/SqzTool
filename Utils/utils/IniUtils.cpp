#include "IniUtils.h"
#include <QTextCodec>
#include <QMutexLocker>
#include <QFile>
#include <QFileInfo>
#include <QDir>

/**
 * @brief 构造函数：初始化INI文件
 */
IniUtils::IniUtils(const QString& filePath)
    : m_filePath(filePath)
{
    // 初始化Qt INI操作对象，设置为UTF-8编码解决中文乱码
    m_pSettings = new QSettings(filePath, QSettings::IniFormat);
    m_pSettings->setIniCodec(QTextCodec::codecForName("UTF-8"));
}

/**
 * @brief 析构函数：释放资源
 */
IniUtils::~IniUtils()
{
    delete m_pSettings;
}

// ==============================================
// 基础文件操作实现
// ==============================================
QString IniUtils::getFilePath() const
{
    return m_filePath;
}

bool IniUtils::isFileExist() const
{
    return QFile::exists(m_filePath);
}

void IniUtils::reloadFile()
{
    QMutexLocker locker(&m_mutex);
    m_pSettings->sync();
}

void IniUtils::syncToFile()
{
    QMutexLocker locker(&m_mutex);
    m_pSettings->sync();
}

bool IniUtils::backupFile(const QString& backupPath)
{
    QMutexLocker locker(&m_mutex);
    if (!isFileExist()) return false;

    QFile srcFile(m_filePath);
    return srcFile.copy(backupPath);
}

// ==============================================
// 基础写入实现
// ==============================================
void IniUtils::writeString(const QString& group, const QString& key, const QString& value)
{
    QMutexLocker locker(&m_mutex);
    m_pSettings->setValue(QString("%1/%2").arg(group, key), value);
    syncToFile();
}

void IniUtils::writeInt(const QString& group, const QString& key, int value)
{
    writeString(group, key, QString::number(value));
}

void IniUtils::writeDouble(const QString& group, const QString& key, double value)
{
    writeString(group, key, QString::number(value, 'f', 2));
}

void IniUtils::writeBool(const QString& group, const QString& key, bool value)
{
    writeString(group, key, value ? "true" : "false");
}

void IniUtils::writeByteArray(const QString& group, const QString& key, const QByteArray& value)
{
    QMutexLocker locker(&m_mutex);
    m_pSettings->setValue(QString("%1/%2").arg(group, key), value);
    syncToFile();
}

// ==============================================
// 基础读取实现
// ==============================================
QString IniUtils::readString(const QString& group, const QString& key, const QString& defaultValue)
{
    QMutexLocker locker(&m_mutex);
    return m_pSettings->value(QString("%1/%2").arg(group, key), defaultValue).toString();
}

int IniUtils::readInt(const QString& group, const QString& key, int defaultValue)
{
    bool ok = false;
    int val = readString(group, key).toInt(&ok);
    return ok ? val : defaultValue;
}

double IniUtils::readDouble(const QString& group, const QString& key, double defaultValue)
{
    bool ok = false;
    double val = readString(group, key).toDouble(&ok);
    return ok ? val : defaultValue;
}

bool IniUtils::readBool(const QString& group, const QString& key, bool defaultValue)
{
    QString val = readString(group, key).toLower();
    if (val == "true")  return true;
    if (val == "false") return false;
    return defaultValue;
}

QByteArray IniUtils::readByteArray(const QString& group, const QString& key, const QByteArray& defaultValue)
{
    QMutexLocker locker(&m_mutex);
    return m_pSettings->value(QString("%1/%2").arg(group, key), defaultValue).toByteArray();
}

// ==============================================
// 批量操作实现
// ==============================================
void IniUtils::writeBatch(const QString& group, const QMap<QString, QString>& keyValueMap)
{
    QMutexLocker locker(&m_mutex);
    for (auto it = keyValueMap.begin(); it != keyValueMap.end(); ++it) {
        m_pSettings->setValue(QString("%1/%2").arg(group, it.key()), it.value());
    }
    syncToFile();
}

QMap<QString, QString> IniUtils::readBatch(const QString& group)
{
    QMutexLocker locker(&m_mutex);
    QMap<QString, QString> map;

    m_pSettings->beginGroup(group);
    QStringList keys = m_pSettings->childKeys();
    for (const QString& key : keys) {
        map.insert(key, m_pSettings->value(key).toString());
    }
    m_pSettings->endGroup();

    return map;
}

void IniUtils::removeKeys(const QString& group, const QStringList& keyList)
{
    QMutexLocker locker(&m_mutex);
    for (const QString& key : keyList) {
        m_pSettings->remove(QString("%1/%2").arg(group, key));
    }
    syncToFile();
}

// ==============================================
// 分组高级操作实现
// ==============================================
QStringList IniUtils::getAllGroups()
{
    QMutexLocker locker(&m_mutex);
    return m_pSettings->childGroups();
}

QStringList IniUtils::getGroupKeys(const QString& group)
{
    QMutexLocker locker(&m_mutex);
    m_pSettings->beginGroup(group);
    QStringList keys = m_pSettings->childKeys();
    m_pSettings->endGroup();
    return keys;
}

bool IniUtils::isGroupExist(const QString& group)
{
    return getAllGroups().contains(group);
}

bool IniUtils::isKeyExist(const QString& group, const QString& key)
{
    QMutexLocker locker(&m_mutex);
    return m_pSettings->contains(QString("%1/%2").arg(group, key));
}

// ==============================================
// 删除/清空操作实现
// ==============================================
void IniUtils::removeKey(const QString& group, const QString& key)
{
    QMutexLocker locker(&m_mutex);
    m_pSettings->remove(QString("%1/%2").arg(group, key));
    syncToFile();
}

void IniUtils::removeGroup(const QString& group)
{
    QMutexLocker locker(&m_mutex);
    m_pSettings->beginGroup(group);
    m_pSettings->remove("");
    m_pSettings->endGroup();
    syncToFile();
}

void IniUtils::clearAll()
{
    QMutexLocker locker(&m_mutex);
    m_pSettings->clear();
    syncToFile();
}
