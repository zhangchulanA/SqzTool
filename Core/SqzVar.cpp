#include "SqzVar.h"
#include <QHash>
#include <QMutex>
#include <QMutexLocker>

namespace SqzVar {

// 内部结构：记录一个监听器
struct Listener {
    int id;             // 唯一ID
    Callback callback;  // 回调函数
};

// 全局实现结构（单例模式，C++11 线程安全初始化）
struct Impl {
    QHash<QString, QVariant> data;                    // 存储变量
    QHash<QString, QList<Listener>> listeners;       // key -> 监听器列表
    int nextId = 0;                                  // 下一个可用的监听器ID
    QMutex mutex;                                    // 保护所有成员
};

// 获取单例实例（函数局部静态变量，线程安全）
static Impl& impl() {
    static Impl i;
    return i;
}

// ---------- 基本操作实现 ----------

void Set(const QString& key, const QVariant& value) {
    Impl& i = impl();
    QMutexLocker locker(&i.mutex);

    // 如果值没有真正改变，不触发回调
    if (i.data.contains(key) && i.data[key] == value)
        return;

    // 更新值
    i.data[key] = value;

    // 触发该 key 的所有监听器
    auto it = i.listeners.find(key);
    if (it != i.listeners.end()) {
        for (const Listener& lst : it.value()) {
            if (lst.callback)
                lst.callback(value);   // 回调函数在调用线程中执行
        }
    }
}

QVariant Get(const QString& key, const QVariant& defaultValue) {
    QMutexLocker locker(&impl().mutex);
    return impl().data.value(key, defaultValue);
}

bool Has(const QString& key) {
    QMutexLocker locker(&impl().mutex);
    return impl().data.contains(key);
}

void Remove(const QString& key) {
    QMutexLocker locker(&impl().mutex);
    impl().data.remove(key);
    // 注意：不自动移除监听器，因为监听器是用户主动添加的，应显式调用 Off
}

void Clear() {
    QMutexLocker locker(&impl().mutex);
    impl().data.clear();
}

// ---------- 监听管理实现 ----------

int On(const QString& key, Callback callback) {
    Impl& i = impl();
    QMutexLocker locker(&i.mutex);

    Listener lst;
    lst.id = i.nextId++;
    lst.callback = callback;
    i.listeners[key].append(lst);
    return lst.id;
}

bool Off(const QString& key, int listenerId) {
    Impl& i = impl();
    QMutexLocker locker(&i.mutex);

    auto it = i.listeners.find(key);
    if (it == i.listeners.end())
        return false;

    QList<Listener>& list = it.value();
    for (int idx = 0; idx < list.size(); ++idx) {
        if (list[idx].id == listenerId) {
            list.removeAt(idx);
            if (list.isEmpty())
                i.listeners.erase(it);
            return true;
        }
    }
    return false;
}

void Off(const QString& key) {
    QMutexLocker locker(&impl().mutex);
    impl().listeners.remove(key);
}

} // namespace SqzVar
