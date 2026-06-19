// SqzProp.cpp
#include "SqzProp.h"
#include <QDebug>

// ==================== 构造/析构 ====================

SqzProp::SqzProp(QObject* parent)
    : QObject(parent)
{
    // 基类初始化，无需额外操作
}

SqzProp::~SqzProp()
{
    // 默认析构，QPointer 会自动清理
}

// ==================== 对象注册/注销 ====================

/**
 * @brief 注册对象到全局对象池
 *
 * 注册逻辑：
 * 1. 空指针检查
 * 2. 获取写锁
 * 3. 检查是否已存在（避免重复插入）
 * 4. 插入对象池
 *
 * 线程安全：使用 QWriteLocker 保护
 */
void SqzProp::Reg(QObject* obj)
{
    if (!obj) return;  // 空指针直接返回

    QWriteLocker locker(&m_lock);  // 获取写锁
    if (!m_objects.contains(obj))  // 避免重复注册
        m_objects.insert(obj, obj);
}

/**
 * @brief 从全局对象池中注销对象
 *
 * 注销逻辑：
 * 1. 空指针检查
 * 2. 获取写锁
 * 3. 从对象池移除
 *
 * 线程安全：使用 QWriteLocker 保护
 */
void SqzProp::UnReg(QObject* obj)
{
    if (!obj) return;

    QWriteLocker locker(&m_lock);
    m_objects.remove(obj);  // remove 是安全的，即使不存在
}

// ==================== 对象查询 ====================

/**
 * @brief 获取所有已注册对象列表
 *
 * 实现逻辑：
 * 1. 获取读锁（允许并发读）
 * 2. 遍历对象池
 * 3. 过滤空指针（QPointer 可能已失效）
 * 4. 返回有效对象列表
 *
 * 线程安全：使用 QReadLocker 保护
 */
QList<QObject*> SqzProp::All() const
{
    QReadLocker locker(&m_lock);  // 获取读锁

    QList<QObject*> list;
    // 遍历对象池，过滤空指针
    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        if (!it.value().isNull())
            list.append(it.value());
    }
    return list;
}

/**
 * @brief 按类名查找对象
 *
 * 实现逻辑：
 * 1. 获取读锁
 * 2. 遍历对象池
 * 3. 使用 QLatin1String 比较类名（避免临时 QString 分配）
 * 4. 返回匹配的对象列表
 *
 * 性能优化：使用 QLatin1String 而不是 QString 比较
 * 线程安全：使用 QReadLocker 保护
 */
QList<QObject*> SqzProp::FindByClass(const QString& className) const
{
    QReadLocker locker(&m_lock);

    QList<QObject*> res;
    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        QObject* obj = it.value();
        if (obj && QLatin1String(obj->metaObject()->className()) == className)
            res.append(obj);
    }
    return res;
}

// ==================== 属性操作 ====================

/**
 * @brief 获取对象的所有 Qt 属性
 *
 * 实现逻辑：
 * 1. 空指针检查
 * 2. 获取对象的 QMetaObject
 * 3. 遍历所有属性（propertyCount）
 * 4. 检查属性是否可读（isReadable）
 * 5. 读取属性值并存入哈希表
 *
 * 注意：只返回可读属性，不可读属性会被忽略
 * 线程安全：纯函数，无共享数据访问
 */
QHash<QString, QVariant> SqzProp::Props(QObject* obj) const
{
    QHash<QString, QVariant> result;
    if (!obj) return result;  // 空指针返回空哈希

    const QMetaObject* mo = obj->metaObject();
    for (int i = 0; i < mo->propertyCount(); ++i) {
        QMetaProperty prop = mo->property(i);
        if (prop.isReadable()) {  // 只读取可读属性
            result[QLatin1String(prop.name())] = prop.read(obj);
        }
    }
    return result;
}

/**
 * @brief 按属性值查找对象
 *
 * 实现逻辑：
 * 1. 获取读锁
 * 2. 遍历对象池
 * 3. 读取对象的指定属性
 * 4. 与目标值比较
 * 5. 返回匹配的对象列表
 *
 * 使用场景：
 * - 按用户ID查找服务对象
 * - 按状态查找窗口
 * - 按角色查找用户
 *
 * 线程安全：使用 QReadLocker 保护
 */
QList<QObject*> SqzProp::FindByProp(const QString& name, const QVariant& value) const
{
    QReadLocker locker(&m_lock);

    QList<QObject*> res;
    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        QObject* obj = it.value();
        if (!obj) continue;

        // 读取属性并与目标值比较
        if (obj->property(name.toUtf8()) == value)
            res.append(obj);
    }
    return res;
}

/**
 * @brief 设置对象的 Qt 属性
 *
 * 实现逻辑：
 * 1. 获取读锁（读锁足够，因为只修改对象内部状态）
 * 2. 检查对象是否在对象池中
 * 3. 调用 setProperty 设置属性
 *
 * 注意：
 * - 对象必须已注册到对象池（安全性检查）
 * - setProperty 本身是线程不安全的，但 Qt 属性通常在主线程使用
 *
 * 线程安全：使用 QReadLocker 保护对象池访问
 */
bool SqzProp::SetProp(QObject* obj, const QString& name, const QVariant& value)
{
    QReadLocker locker(&m_lock);

    // 检查对象是否在池中
    if (!m_objects.contains(obj))
        return false;

    // 设置属性
    return obj->setProperty(name.toUtf8(), value);
}

// ==================== 调试工具 ====================

/**
 * @brief 输出所有已注册对象的调试信息
 *
 * 输出格式：
 * === SqzProp dump ===
 *  QObject(0x12345678) class: MyDialog
 *     windowTitle : "Login"
 *     width : 800
 *     height : 600
 *  QObject(0x87654321) class: UserService
 *     userId : 123
 *     userName : "Alice"
 *
 * 用途：
 * - 开发调试：查看运行时对象状态
 * - 内存泄漏检测：查看对象池中剩余对象
 * - 状态监控：查看所有对象的属性值
 *
 * 线程安全：使用 QReadLocker 保护
 */
void SqzProp::Dump() const
{
    QReadLocker locker(&m_lock);

    logdebug << "=== SqzProp dump ===";
    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        QObject* obj = it.value();
        if (!obj) {
            qDebug() << "  (null)";
            continue;
        }

        // 输出对象地址和类名
        logdebug << obj << "class:" << obj->metaObject()->className();

        // 输出所有属性
        auto propMap = Props(obj);
        for (auto pit = propMap.begin(); pit != propMap.end(); ++pit) {
            logdebug << "    " << pit.key() << ":" << pit.value();
        }
    }
}
