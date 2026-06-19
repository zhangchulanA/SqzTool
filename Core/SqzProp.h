// SqzProp.h
#pragma once

/**
 * @class SqzProp
 * @brief Qt 对象属性跟踪与查询基类
 *
 * @details
 * SqzProp 是一个轻量级的对象管理基类，提供全局对象注册、属性查询、
 * 反射调试等能力。它维护一个全局的对象池，所有继承自 SqzProp 的子类
 * 或通过 SqzHub 创建的对象都会自动被注册到该池中。
 *
 * ===== 核心功能 =====
 * 1. 对象注册/注销：自动跟踪所有 QObject 派生类实例
 * 2. 对象查询：按类名、按属性值查找对象
 * 3. 属性反射：运行时获取/设置对象的所有属性
 * 4. 线程安全：使用读写锁保护对象池
 * 5. 调试支持：一键 dump 所有对象及其属性
 *
 * ===== 使用场景 =====
 * - 全局对象注册表：统一管理所有业务对象
 * - 按类型查找服务：FindByClass("UserService")
 * - 按属性查找对象：FindByProp("userId", 123)
 * - 运行时调试：Dump() 输出所有对象状态
 * - 属性批量操作：遍历所有对象设置属性
 *
 * ===== 使用示例 =====
 * // 注册对象（通常由 SqzHub 自动调用）
 * SqzProp::Reg(myObject);
 *
 * // 查找所有 QWidget
 * QList<QObject*> widgets = SqzProp::FindByClass("QWidget");
 *
 * // 按属性查找
 * auto users = SqzProp::FindByProp("role", "admin");
 *
 * // 获取对象所有属性
 * auto props = SqzProp::Props(myObject);
 *
 * // 调试输出
 * SqzProp::Dump();
 *
 * ===== 注意事项 =====
 * - SqzProp 本身是 QObject 子类，可被继承
 * - 对象池使用 QPointer 防止野指针
 * - 所有公开方法都是线程安全的
 * - Reg/UnReg 通常由 SqzHub 自动管理，不建议手动调用
 */

#include <QObject>
#include <QHash>
#include <QReadWriteLock>
#include <QPointer>
#include <QMetaProperty>
#include "SqzLog.h"
#include "RF.h"

// 前置声明
class SqzHub;

class SqzProp : public QObject
{
    Q_OBJECT

    // SqzHub 作为友元，可以访问内部方法
    friend class SqzHub;

public:
    // ==================== 构造函数/析构函数 ====================

    /**
     * @brief 构造函数
     * @param parent 父对象，用于 Qt 内存管理
     */
    explicit SqzProp(QObject* parent = nullptr);

    /**
     * @brief 虚析构函数，确保子类正确析构
     */
    ~SqzProp();

    // ==================== 对象注册/注销 ====================

    /**
     * @brief 注册对象到全局对象池
     * @param obj 要注册的对象指针
     * @note 若对象已存在则忽略，线程安全
     * @warning 通常由 SqzHub 自动调用，不建议手动使用
     */
    void Reg(QObject* obj);

    /**
     * @brief 从全局对象池中注销对象
     * @param obj 要注销的对象指针
     * @note 若对象不存在则忽略，线程安全
     * @warning 通常由 SqzHub 自动调用，不建议手动使用
     */
    void UnReg(QObject* obj);

    // ==================== 对象查询 ====================

    /**
     * @brief 获取所有已注册对象列表
     * @return 所有对象的 QList，自动过滤空指针
     * @note 线程安全（读锁）
     * @usage auto all = SqzProp::All();
     */
    QList<QObject*> All() const;

    /**
     * @brief 按类名查找对象
     * @param className 类名字符串（如 "UserService"）
     * @return 匹配类名的对象列表
     * @note 线程安全（读锁）
     * @note 使用 QLatin1String 比较，避免临时 QString 分配
     * @usage auto services = SqzProp::FindByClass("UserService");
     */
    QList<QObject*> FindByClass(const QString& className) const;

    // ==================== 属性操作 ====================

    /**
     * @brief 获取对象的所有 Qt 属性
     * @param obj 目标对象
     * @return 属性名→属性值的哈希表
     * @note 只包含可读属性（Readable），线程安全
     * @usage auto props = SqzProp::Props(myWidget);
     *         QString title = props["windowTitle"].toString();
     */
    QHash<QString, QVariant> Props(QObject* obj) const;

    /**
     * @brief 按属性值查找对象
     * @param name 属性名
     * @param value 要匹配的属性值
     * @return 属性值匹配的对象列表
     * @note 线程安全（读锁）
     * @usage auto admins = SqzProp::FindByProp("role", "admin");
     */
    QList<QObject*> FindByProp(const QString& name, const QVariant& value) const;

    /**
     * @brief 设置对象的 Qt 属性
     * @param obj 目标对象
     * @param name 属性名
     * @param value 新属性值
     * @return true=设置成功，false=对象不存在或设置失败
     * @note 线程安全（读锁）
     * @note 对象必须已注册到对象池中
     * @usage SqzProp::SetProp(myObject, "state", "active");
     */
    bool SetProp(QObject* obj, const QString& name, const QVariant& value);

    // ==================== 调试工具 ====================

    /**
     * @brief 输出所有已注册对象的调试信息
     * @details 输出内容包括：
     *          - 对象地址
     *          - 类名
     *          - 所有可读属性及其值
     * @note 线程安全（读锁）
     * @usage SqzProp::Dump();  // 输出到 qDebug()
     */
    void Dump() const;

private:
    // 读写锁：保护对象池的线程安全
    mutable QReadWriteLock m_lock;

    // 对象池：存储所有注册的 QObject 指针（使用 QPointer 防止野指针）
    QHash<QObject*, QPointer<QObject>> m_objects;
};
