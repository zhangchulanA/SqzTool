#ifndef SQZVAR_H
#define SQZVAR_H

#include <QString>
#include <QVariant>
#include <functional>

/*!
 * \namespace SqzVar
 * \brief 全局变量管理工具，支持字符串索引、类型安全的读写和值变化监听。
 *
 * 特性：
 * - 存储任意 Qt 支持的类型（通过 QVariant）。
 * - 提供模板版本 Set<T> / Get<T>，使用更方便、类型安全。
 * - 支持监听变量变化（回调函数），当值真正改变时触发。
 * - 线程安全（内部使用 QMutex，可在多线程环境中调用）。
 *
 * 使用示例：
 * \code
 * // 写入
 * SqzVar::Set("volume", 80);
 * SqzVar::Set<QString>("name", "Alice");
 *
 * // 读取
 * int vol = SqzVar::Get("volume", 50).toInt();
 * QString name = SqzVar::Get<QString>("name", "unknown");
 *
 * // 监听变化
 * int id = SqzVar::On("volume", [](const QVariant& val) {
 *     qDebug() << "Volume changed to" << val.toInt();
 * });
 * SqzVar::Set("volume", 90);  // 回调被触发
 *
 * // 移除监听
 * SqzVar::Off("volume", id);  // 移除特定监听器
 * SqzVar::Off("volume");      // 移除所有监听器
 *
 * // 其他操作
 * if (SqzVar::Has("name")) { ... }
 * SqzVar::Remove("name");
 * SqzVar::Clear();
 * \endcode
 */
namespace SqzVar {

//! 监听回调函数类型：参数为新值（QVariant 形式）
using Callback = std::function<void(const QVariant&)>;

/*! @name 基本读写操作 */
///@{

/*! 设置变量的值（非模板版本，接受 QVariant） */
void Set(const QString& key, const QVariant& value);

/*! 设置变量的值（模板版本，自动转换为 QVariant） */
template<typename T>
void Set(const QString& key, const T& value) {
    Set(key, QVariant::fromValue(value));
}

/*! 读取变量的值（非模板版本，返回 QVariant） */
QVariant Get(const QString& key, const QVariant& defaultValue = QVariant());

/*! 读取变量的值（模板版本，直接返回指定类型） */
template<typename T>
T Get(const QString& key, const T& defaultValue = T()) {
    QVariant val = Get(key);
    if (val.isNull() && defaultValue != T())
        return defaultValue;
    return val.value<T>();
}

/*! 检查变量是否存在 */
bool Has(const QString& key);

/*! 删除指定变量 */
void Remove(const QString& key);

/*! 清空所有变量 */
void Clear();

///@}

/*! @name 值变化监听 */
///@{

/*! 添加监听器，当 key 的值发生变化时调用 callback。
 *  @return 监听器 ID，用于后续移除。
 */
int On(const QString& key, Callback callback);

/*! 移除指定监听器。
 *  @return 是否成功移除
 */
bool Off(const QString& key, int listenerId);

/*! 移除 key 下的所有监听器。 */
void Off(const QString& key);

///@}

} // namespace SqzVar

#endif // SQZVAR_H
