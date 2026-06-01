#include "SqzThreadPool.h"
#include <QThread>
#include <QtConcurrent>

// 全局单例实例初始化（初始为nullptr，第一次调用instance()时创建）
SqzThreadPool* SqzThreadPool::m_instance = nullptr;

/**
 * @brief 构造函数（私有，仅内部调用）
 * 初始化线程池默认配置，确保线程池启动时处于最优状态
 */
SqzThreadPool::SqzThreadPool(QObject *parent)
    : QObject(parent)
    , m_taskIdGenerator(1) // 任务ID从1开始递增（0留作无效ID）
{
    // 获取Qt全局线程池（底层由Qt管理，跨平台、性能稳定，无需手动创建线程）
    m_threadPool = QThreadPool::globalInstance();

    // 默认配置：IO密集型任务最优（绝大多数项目都是IO密集型，如网络、文件、数据库）
    // 获取CPU核心数，IO密集型任务设置为核心数×2，充分利用系统资源，不浪费CPU
    int cpuCoreCount = QThread::idealThreadCount();
    m_threadPool->setMaxThreadCount(cpuCoreCount * 2);

    // 默认空闲线程存活时间：30秒（空闲线程超过30秒未被使用，自动释放，节省内存）
    m_threadPool->setExpiryTimeout(30000);
}

/**
 * @brief 析构函数（程序退出时自动调用）
 * 安全释放所有资源，避免内存泄漏，确保所有后台任务正常完成
 */
SqzThreadPool::~SqzThreadPool()
{
    // 等待所有任务完成，再释放资源（避免任务未执行完导致数据丢失）
    waitForAllDone();

    // 清理所有周期任务定时器（防止定时器泄漏）
    QMutexLocker lock(&m_mutex);
    for (QTimer* timer : m_periodicTimers.values()) {
        timer->stop();
        timer->deleteLater();
    }
    m_periodicTimers.clear();

    // 清理所有任务监听器（防止监听器泄漏）
    for (QFutureWatcher<void>* watcher : m_taskWatchers.values()) {
        watcher->deleteLater();
    }
    m_taskWatchers.clear();
}

/**
 * @brief 获取全局单例实例（线程安全）
 * 双重检查锁定模式，确保多线程环境下，只创建一个单例实例
 */
SqzThreadPool* SqzThreadPool::instance()
{
    // 第一次检查：未加锁，快速判断实例是否存在（避免每次调用都加锁，提升性能）
    if (!m_instance) {
        // 加互斥锁，确保多线程环境下，只有一个线程能创建实例
        static QMutex createMutex;
        QMutexLocker lock(&createMutex);
        // 第二次检查：加锁后再次判断，防止多个线程同时通过第一次检查
        if (!m_instance) {
            m_instance = new SqzThreadPool;
        }
    }
    return m_instance;
}

/**
 * @brief 生成全局唯一的任务ID（线程安全）
 * 使用原子操作，避免多线程同时生成ID导致重复
 * @return quint64 唯一任务ID
 */
quint64 SqzThreadPool::generateId()
{
    // fetchAndAddRelaxed：原子操作，先获取当前值，再自增1，线程安全
    return m_taskIdGenerator.fetchAndAddRelaxed(1);
}

/**
 * @brief 清理单个任务的资源（任务完成/取消后调用）
 * @param id 要清理的任务ID
 */
void SqzThreadPool::cleanTask(quint64 id)
{
    // 加互斥锁，避免多线程同时清理资源，导致内存访问异常
    QMutexLocker lock(&m_mutex);

    // 若任务监听器存在，删除监听器并从Map中移除
    if (m_taskWatchers.contains(id)) {
        QFutureWatcher<void>* watcher = m_taskWatchers.take(id);
        watcher->deleteLater(); // 释放监听器内存
    }

    // 若所有任务监听器都已清理（所有任务完成），触发allTasksFinished信号
    if (m_taskWatchers.isEmpty()) {
        emit allTasksFinished();
    }
}

/**
 * @brief 取消指定ID的任务
 * @param taskId 要取消的任务ID
 * @return bool 取消成功返回true，失败返回false
 */
bool SqzThreadPool::cancelTask(quint64 taskId)
{
    // 加互斥锁，确保线程安全（避免同时取消同一个任务）
    QMutexLocker lock(&m_mutex);

    // 若任务不存在（已完成/已取消），返回false
    if (!m_taskWatchers.contains(taskId)) {
        return false;
    }

    // 获取任务监听器，取消任务并等待任务完成（已开始的任务会执行完毕）
    QFutureWatcher<void>* watcher = m_taskWatchers[taskId];
    watcher->cancel(); // 取消任务（未开始的任务直接取消，已开始的任务继续执行）
    watcher->waitForFinished(); // 等待任务执行完毕（确保任务资源释放）
    cleanTask(taskId); // 清理任务资源

    return true;
}

/**
 * @brief 阻塞等待指定ID的任务完成
 * @param taskId 要等待的任务ID
 */
void SqzThreadPool::waitForTaskFinished(quint64 taskId)
{
    // 加互斥锁，确保线程安全（避免同时等待同一个任务）
    QMutexLocker lock(&m_mutex);

    // 若任务存在，等待任务完成
    if (m_taskWatchers.contains(taskId)) {
        m_taskWatchers[taskId]->waitForFinished();
    }
}

/**
 * @brief 阻塞等待所有任务完成
 */
void SqzThreadPool::waitForAllDone()
{
    // 调用Qt线程池的waitForDone()，阻塞等待所有任务完成
    m_threadPool->waitForDone();
}

/**
 * @brief 清空所有等待队列中的任务（已开始的任务不受影响）
 */
void SqzThreadPool::clearPending()
{
    // 调用Qt线程池的clear()，清空等待队列
    m_threadPool->clear();
}

/**
 * @brief 取消周期任务
 * @param taskId 周期任务的ID
 */
void SqzThreadPool::cancelPeriodic(quint64 taskId)
{
    // 加互斥锁，确保线程安全
    QMutexLocker lock(&m_mutex);

    // 若周期任务存在，停止定时器并释放资源
    if (m_periodicTimers.contains(taskId)) {
        QTimer* timer = m_periodicTimers.take(taskId);
        timer->stop(); // 停止定时器，不再触发任务
        timer->deleteLater(); // 释放定时器内存
    }
}

/**
 * @brief 设置线程池最大并发线程数
 * @param count 最大线程数
 */
void SqzThreadPool::setMaxThreadCount(int count)
{
    // 加互斥锁，确保线程安全（避免同时修改线程数）
    QMutexLocker lock(&m_mutex);
    m_threadPool->setMaxThreadCount(count);
}

/**
 * @brief 获取当前线程池的最大并发线程数
 * @return int 最大线程数
 */
int SqzThreadPool::maxThreadCount() const
{
    // 加互斥锁，确保线程安全（避免读取时，线程数被修改）
    QMutexLocker lock(&m_mutex);
    return m_threadPool->maxThreadCount();
}


void SqzThreadPool::pause()
{
    QMutexLocker lock(&m_mutex);
    if (m_paused) return;

    m_paused = true;
    // 用Qt的暂停方式：清空等待队列，不再接受新任务
    m_threadPool->clear();
}

void SqzThreadPool::resume()
{
    QMutexLocker lock(&m_mutex);
    if (!m_paused) return;

    m_paused = false;
    // 恢复：允许提交新任务（无需额外操作，提交逻辑已包含判断）
}

bool SqzThreadPool::isPaused() const
{
    QMutexLocker lock(&m_mutex);
    return m_paused;
}
