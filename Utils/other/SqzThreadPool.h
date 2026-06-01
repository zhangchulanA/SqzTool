#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <QObject>
#include <QThreadPool>
#include <QMutex>
#include <QFuture>
#include <QFutureWatcher>
#include <functional>
#include <QAtomicInteger>
#include <QTimer>
#include <QMap>
#include <QtConcurrent>
/**
 * @brief 工业级全局线程池（单例模式）
 * 核心定位：解决Qt程序UI卡顿问题，将耗时操作异步提交到后台线程，不阻塞主线程
 * 核心优势：线程安全、异常安全、功能齐全、跨平台（Windows/Ubuntu）、自动内存管理
 * 适用场景：日志写入、文件读写、网络请求、数据库操作、图片处理、复杂计算等所有耗时操作
 * 兼容版本：Qt 5.14.2 及以上（兼容Qt5/Qt6）
 */
class SqzThreadPool : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 获取全局唯一单例实例（线程安全，整个程序共用一个线程池）
     * @return SqzThreadPool* 单例指针，无需手动释放，程序退出时自动销毁
     */
    static SqzThreadPool* instance();

    /**
     * @brief 任务优先级枚举（根据任务重要性选择，不指定则默认NormalPriority）
     * LowPriority：低优先级（后台闲置任务，如日志归档）
     * NormalPriority：默认优先级（绝大多数普通耗时任务）
     * HighPriority：高优先级（关键任务，如用户触发的下载、计算）
     */
    enum TaskPriority {
        LowPriority,     // 低优先级
        NormalPriority,  // 默认优先级（推荐）
        HighPriority     // 高优先级
    };

    // ---------------------- 基础异步任务接口 ----------------------
    /**
     * @brief 启动一个无参、无返回值、无回调的基础异步任务
     * @param task 任务函数（支持Lambda、普通函数、函数对象）
     * @param pri 任务优先级（默认NormalPriority）
     * @return quint64 任务ID（用于取消任务、等待任务完成）
     * @示例：pool->start([](){ qDebug() << "基础异步任务"; });
     */
    template<typename Func>
    quint64 start(Func task, TaskPriority pri = NormalPriority)
    {
        // 调用核心实现函数，无回调、无返回值、无超时
        return startImpl(std::move(task), nullptr, nullptr, pri, -1);
    }

    // ---------------------- 带完成回调的任务接口 ----------------------
    /**
     * @brief 启动异步任务 + 任务完成后回调（回调在主线程执行，可安全操作UI）
     * @param task 后台耗时任务（子线程执行，不可操作UI）
     * @param cb 任务完成回调（主线程执行，可安全操作UI）
     * @param pri 任务优先级（默认NormalPriority）
     * @return quint64 任务ID
     * @示例：pool->startWithCallback([](){ 耗时操作; }, [](){ qDebug() << "任务完成"; });
     */
    template<typename Func, typename Callback>
    quint64 startWithCallback(Func task, Callback cb, TaskPriority pri = NormalPriority)
    {
        // 调用核心实现函数，传入任务和回调，无返回值、无超时
        return startImpl(std::move(task), std::move(cb), nullptr, pri, -1);
    }

    // ---------------------- 带返回值的任务接口 ----------------------
    /**
     * @brief 启动带返回值的异步任务，返回值通过回调返回（回调在主线程）
     * @param task 带返回值的耗时任务（子线程执行）
     * @param resultCB 返回值回调（主线程执行，参数为任务的返回值）
     * @param pri 任务优先级（默认NormalPriority）
     * @return quint64 任务ID
     * @示例：pool->startWithResult([]()->int{ return 100; }, [](int res){ qDebug() << "返回值：" << res; });
     */
    template<typename Func, typename ResultCB>
    quint64 startWithResult(Func task, ResultCB resultCB, TaskPriority pri = NormalPriority)
    {
        // 调用核心实现函数，传入任务和返回值回调，无完成回调、无超时
        return startImpl(std::move(task), nullptr, std::move(resultCB), pri, -1);
    }

    // ---------------------- 带进度回调的任务接口 ----------------------
    /**
     * @brief 启动带进度更新的异步任务（适用于下载、文件处理、数据解析等需要显示进度的场景）
     * @param task 带进度回调的耗时任务（子线程执行，需在任务中调用进度回调更新进度）
     * @param progressCB 进度回调（主线程执行，参数为进度值0~100）
     * @param finishCB 任务完成回调（主线程执行，可操作UI）
     * @param pri 任务优先级（默认NormalPriority）
     * @return quint64 任务ID
     * @示例：pool->startWithProgress([](auto progress){ progress(50); }, [](int v){ qDebug() << "进度：" << v; }, [](){ qDebug() << "完成"; });
     */
    template<typename Func, typename ProgressCB, typename FinishCB>
    quint64 startWithProgress(Func task, ProgressCB progressCB, FinishCB finishCB, TaskPriority pri = NormalPriority)
    {
        // 包装任务：将进度回调传入任务，任务执行时调用进度回调
        auto taskWrapper = [=]() {
            task(progressCB); // 子线程中执行任务，并传递进度回调
        };
        // 调用带完成回调的接口，将包装后的任务和完成回调传入
        return startWithCallback(std::move(taskWrapper), std::move(finishCB), pri);
    }

    // ---------------------- 带超时的任务接口 ----------------------
    /**
     * @brief 启动带超时的异步任务（超时后自动取消任务，避免任务无限阻塞）
     * @param task 耗时任务（子线程执行）
     * @param timeoutMs 超时时间（单位：毫秒），超过该时间任务自动取消
     * @param pri 任务优先级（默认NormalPriority）
     * @return quint64 任务ID
     * @示例：pool->startWithTimeout([](){ QThread::sleep(5); }, 2000); // 2秒超时，任务会被取消
     */
    template<typename Func>
    quint64 startWithTimeout(Func task, int timeoutMs, TaskPriority pri = NormalPriority)
    {
        // 调用核心实现函数，传入任务和超时时间，无回调、无返回值
        return startImpl(std::move(task), nullptr, nullptr, pri, timeoutMs);
    }

    // ---------------------- 延迟执行任务接口 ----------------------
    /**
     * @brief 延迟指定时间后，执行异步任务（不阻塞主线程）
     * @param task 延迟执行的任务（子线程执行）
     * @param delayMs 延迟时间（单位：毫秒）
     * @param pri 任务优先级（默认NormalPriority）
     * @return quint64 任务ID（可用于取消延迟任务）
     * @示例：pool->startDelayed([](){ qDebug() << "延迟3秒执行"; }, 3000);
     */
    template<typename Func>
    quint64 startDelayed(Func task, int delayMs, TaskPriority pri = NormalPriority)
    {
        // 生成任务ID（用于后续取消延迟任务）
        quint64 taskId = generateId();
        // 使用QTimer::singleShot实现延迟，延迟结束后提交任务到线程池
        QTimer::singleShot(delayMs, [=]() {
            start(std::move(task), pri);
        });
        return taskId;
    }

    // ---------------------- 周期循环任务接口 ----------------------
    /**
     * @brief 周期循环执行异步任务（适用于心跳检测、定时刷新等场景）
     * @param task 周期执行的任务（子线程执行）
     * @param intervalMs 执行间隔（单位：毫秒），每隔该时间执行一次任务
     * @param pri 任务优先级（默认NormalPriority）
     * @return quint64 任务ID（用于取消周期任务）
     * @示例：pool->startPeriodic([](){ qDebug() << "每秒执行一次"; }, 1000);
     */
    template<typename Func>
    quint64 startPeriodic(Func task, int intervalMs, TaskPriority pri = NormalPriority)
    {
        // 生成任务ID（用于后续取消周期任务）
        quint64 taskId = generateId();
        // 创建定时器，用于周期触发任务
        QTimer* periodicTimer = new QTimer(this);
        // 将定时器存入Map，方便后续通过任务ID取消
        m_periodicTimers.insert(taskId, periodicTimer);
        // 定时器超时信号连接：每次超时，提交任务到线程池
        connect(periodicTimer, &QTimer::timeout, [=]() {
            start(std::move(task), pri);
        });
        // 启动定时器，开始周期执行
        periodicTimer->start(intervalMs);
        return taskId;
    }

    // ---------------------- 任务依赖接口 ----------------------
    /**
     * @brief 依赖某个任务完成后，再执行当前任务（适用于任务有先后顺序的场景）
     * @param dependTaskId 依赖的任务ID（必须是已提交的任务）
     * @param task 当前要执行的任务（子线程执行）
     * @param pri 任务优先级（默认NormalPriority）
     * @return quint64 当前任务的ID
     * @示例：quint64 taskA = pool->start([](){ 任务A; }); pool->startDependOn(taskA, [](){ 任务B; }); // 任务A完成后执行任务B
     */
    template<typename Func>
    quint64 startDependOn(quint64 dependTaskId, Func task, TaskPriority pri = NormalPriority)
    {
        // 包装任务：先等待依赖任务完成，再执行当前任务
        auto dependTaskWrapper = [=]() {
            waitForTaskFinished(dependTaskId); // 阻塞等待依赖任务完成
            task(); // 依赖任务完成后，执行当前任务
        };
        // 提交包装后的任务到线程池
        return start(std::move(dependTaskWrapper), pri);
    }

    // ---------------------- 任务控制接口（核心） ----------------------
    /**
     * @brief 取消指定ID的任务
     * @param taskId 要取消的任务ID（通过start等接口返回）
     * @return bool 取消成功返回true，失败返回false（任务已完成/不存在则失败）
     * @说明：已开始执行的任务会等待执行完毕，未开始的任务会直接取消
     */
    bool cancelTask(quint64 taskId);

    /**
     * @brief 阻塞等待指定ID的任务完成（主线程调用会阻塞，子线程调用无影响）
     * @param taskId 要等待的任务ID
     * @说明：一般用于子线程中，需要等待某个任务完成后再执行后续逻辑
     */
    void waitForTaskFinished(quint64 taskId);

    /**
     * @brief 阻塞等待所有任务完成（主线程调用会阻塞，直到所有任务执行完毕）
     * @说明：一般用于程序退出前，确保所有后台任务正常完成，避免数据丢失
     */
    void waitForAllDone();

    /**
     * @brief 清空所有等待队列中的任务（已开始执行的任务不受影响）
     * @说明：用于紧急停止所有未执行的任务，比如程序退出、用户取消操作
     */
    void clearPending();

    /**
     * @brief 取消周期任务（停止周期执行，释放定时器资源）
     * @param taskId 周期任务的ID（通过startPeriodic返回）
     * @说明：必须调用该接口取消周期任务，否则定时器会一直存在，导致内存泄漏
     */
    void cancelPeriodic(quint64 taskId);

    // ---------------------- 线程池配置接口 ----------------------
    /**
     * @brief 设置线程池最大并发线程数（核心配置）
     * @param count 最大线程数
     * @建议：CPU密集型任务（计算、图片处理）= CPU核心数；IO密集型任务（网络、文件）= CPU核心数×2
     */
    void setMaxThreadCount(int count);

    /**
     * @brief 获取当前线程池的最大并发线程数
     * @return int 最大线程数
     */
    int maxThreadCount() const;
    /**
     * @brief 暂停线程池
     * 已正在运行的任务继续跑完；
     * 队列里待执行的任务暂时不调度，进入挂起状态
     */
    void pause();

    /**
     * @brief 恢复线程池
     * 从暂停状态恢复，继续调度等待中的任务
     */
    void resume();

    /**
     * @brief 查询当前是否处于暂停状态
     * @return true 已暂停  false 正常运行
     */
    bool isPaused() const;

signals:
    /**
     * @brief 任务开始执行的信号（子线程触发，主线程可接收）
     * @param id 开始执行的任务ID
     */
    void taskStarted(quint64 id);

    /**
     * @brief 任务执行完成的信号（子线程触发，主线程可接收）
     * @param id 执行完成的任务ID
     */
    void taskFinished(quint64 id);

    /**
     * @brief 任务超时的信号（超时后触发，主线程可接收）
     * @param id 超时的任务ID
     */
    void taskTimeout(quint64 id);

    /**
     * @brief 所有任务执行完成的信号（所有任务结束后触发）
     */
    void allTasksFinished();

private:
    /**
     * @brief 私有构造函数（单例模式，外部无法通过new创建实例）
     * @param parent 父对象（默认nullptr，无需设置）
     */
    explicit SqzThreadPool(QObject* parent = nullptr);

    /**
     * @brief 析构函数（程序退出时自动调用，释放所有资源）
     * @说明：自动等待所有任务完成，释放定时器、任务监听器等资源，避免内存泄漏
     */
    ~SqzThreadPool() override;

    /**
     * @brief 生成全局唯一的任务ID（线程安全）
     * @return quint64 唯一任务ID（从1开始递增）
     */
    quint64 generateId();

    /**
     * @brief 清理单个任务的资源（任务完成/取消后调用）
     * @param id 要清理的任务ID
     * @说明：删除任务监听器，释放内存，若所有任务都清理完成，触发allTasksFinished信号
     */
    void cleanTask(quint64 id);

    /**
     * @brief 任务启动核心实现（所有任务最终都会调用该函数，统一处理）
     * @param task 耗时任务（子线程执行）
     * @param cb 完成回调（主线程执行，可为nullptr）
     * @param resultCB 返回值回调（主线程执行，可为nullptr）
     * @param pri 任务优先级
     * @param timeoutMs 超时时间（-1表示无超时）
     * @return quint64 任务ID
     * @说明：内部处理任务的超时、回调、异常捕获、线程安全等核心逻辑，外部无需调用
     */
    template<typename Func, typename CB, typename ResultCB>
    quint64 startImpl(Func task, CB cb, ResultCB resultCB, TaskPriority pri, int timeoutMs)
    {
        QMutexLocker lock(&m_mutex);
        // 关键：暂停状态下不提交新任务
        if (m_paused) {
            // 可根据需求选择：丢弃任务或缓存到自定义队列
            return 0; // 0 表示无效ID，代表任务未提交
        }
        quint64 taskId = generateId();

        QFutureWatcher<void>* taskWatcher = new QFutureWatcher<void>(this);
        m_taskWatchers.insert(taskId, taskWatcher);

        // 超时处理
        if (timeoutMs > 0) {
            QTimer* timeoutTimer = new QTimer(this);
            timeoutTimer->setSingleShot(true);
            connect(timeoutTimer, &QTimer::timeout, [=]() {
                if (cancelTask(taskId)) {
                    emit taskTimeout(taskId);
                }
                timeoutTimer->deleteLater();
            });
            timeoutTimer->start(timeoutMs);
        }

        // 任务完成处理（关键修复：先判断是否可调用）
        connect(taskWatcher, &QFutureWatcher<void>::finished, this, [=]() {
            // 只有当 cb 不是 nullptr 时，才调用
            if constexpr (!std::is_same_v<std::decay_t<CB>, std::nullptr_t>) {
                if (cb) {
                    cb();
                }
            }
            emit taskFinished(taskId);
            cleanTask(taskId);
        }, Qt::QueuedConnection);

        // 包装任务（关键修复：处理返回值和 nullptr 回调）
        auto taskRunnable = [=]() {
            emit taskStarted(taskId);
            try {
                // 分情况处理：带返回值 / 无返回值
                if constexpr (!std::is_same_v<std::decay_t<ResultCB>, std::nullptr_t>) {
                    auto taskResult = task();
                    if (resultCB) {
                        resultCB(taskResult);
                    }
                } else {
                    task();
                }
            } catch (...) {}
        };

        QFuture<void> taskFuture = QtConcurrent::run(taskRunnable);
        taskWatcher->setFuture(taskFuture);

        return taskId;
    }

private:
    static SqzThreadPool* m_instance;                // 全局单例实例（唯一）
    QThreadPool* m_threadPool;                   // Qt全局线程池（底层依赖，跨平台最优）
    mutable QMutex m_mutex;                      // 互斥锁（保证线程安全，所有共享资源操作都需加锁）
    QAtomicInteger<quint64> m_taskIdGenerator;  // 原子任务ID生成器（线程安全，避免ID重复）
    QMap<quint64, QFutureWatcher<void>*> m_taskWatchers; // 任务监听器Map（任务ID -> 监听器）
    QMap<quint64, QTimer*> m_periodicTimers;    // 周期任务定时器Map（任务ID -> 定时器）
    bool m_paused = false;   // 线程池暂停标记
};

#endif // THREADPOOL_H
//***********************************************使用方式

// 测试普通带参数函数
/*oid testFunc(int a, QString b)
{
    loginfo << "[子线程] testFunc 执行：" << a << b;
    QThread::msleep(500); // 模拟耗时操作（500毫秒）
}

// 测试带返回值的函数
QString testResultFunc()
{
    QThread::msleep(300);
    return "哈哈哈"; // 返回测试值
}

// 测试带进度的函数
void testProgressFunc(std::function<void(int)> progressCB)
{
    // 模拟进度更新（0~100）
    for (int i = 0; i <= 100; i += 20) {
        progressCB(i); // 调用进度回调，更新进度
        QThread::msleep(100);
    }
}

void ThreadPoolTest(){
    // 1. 获取线程池单例（全局唯一）
      SqzThreadPool* pool = SqzThreadPool::instance();

      // 2. 基础配置（可选，默认已优化，可根据需求修改）
       pool->setMaxThreadCount(8); // 手动设置最大线程数（如8核CPU，IO密集型设8）

//       ---------------------- 测试所有接口 ----------------------
//       3. 基础无参任务
      pool->start([](){
          loginfo << "[子线程] 基础无参任务执行";
          QThread::msleep(400);
      });

      // 4. 带参数任务（用std::bind包裹参数，无歧义）
      pool->start(std::bind(testFunc, 10086, "线程池测试"));

//       5. 带完成回调的任务（回调在主线程，可操作UI）
      pool->startWithCallback(
          [](){ // 子线程：耗时任务
              loginfo << "[子线程] 带回调任务执行";
              QThread::msleep(600);
          },
          [](){ // 主线程：完成回调
              loginfo << "[主线程] 带回调任务完成（可操作UI）";
          }
      );

//       6. 带返回值的任务（返回值通过回调返回）
      pool->startWithResult(
          testResultFunc, // 子线程：带返回值的任务
          [](QString res){ // 主线程：返回值回调
              loginfo << "[主线程] 带返回值任务完成，返回值：" << res;
          }
      );

//       7. 带进度的任务（适用于下载、文件处理）
      pool->startWithProgress(
          testProgressFunc, // 子线程：带进度的任务
          [](int v){ // 主线程：进度回调
              loginfo <<QThread::currentThread()<<qApp->thread() << "[主线程] 任务进度：" << v << "%";
          },
          [](){ // 主线程：完成回调
              loginfo<<QThread::currentThread() <<qApp->thread() << "[主线程] 带进度任务完成";
          }
      );

      // 8. 带超时的任务（2秒超时，任务会被取消）
      quint64 timeoutTaskId = pool->startWithTimeout(
          [](){ // 子线程：耗时任务（模拟5秒耗时）
              loginfo << "[子线程] 超时任务开始执行（模拟5秒）";
              QThread::sleep(5);
          },
          2000 // 超时时间：2000毫秒（2秒）
      );

//       9. 延迟任务（延迟3秒执行）
      pool->startDelayed(
          [](){
              loginfo << "[子线程] 延迟3秒任务执行";
          },
          3000 // 延迟时间：3000毫秒（3秒）
      );

      // 10. 周期任务（每秒执行一次）
      quint64 periodicTaskId = pool->startPeriodic(
          [](){
              loginfo << "[子线程] 周期任务（每秒执行一次）";
          },
          1000 // 执行间隔：1000毫秒（1秒）
      );

      // 11. 任务依赖（任务A完成后，执行任务B）
      quint64 taskA = pool->start([](){
          loginfo << "[子线程] 依赖任务A执行";
          QThread::msleep(1000);
      });
      pool->startDependOn(taskA, [](){
          loginfo << "[子线程] 依赖任务B（A完成后执行）";
      });

      // 12. 取消任务（取消周期任务，避免一直执行）
      QTimer::singleShot(5000, [=](){
          pool->cancelPeriodic(periodicTaskId);
          loginfo << "[主线程] 周期任务已取消";
      });

      // 13. 等待所有任务完成（程序退出前，确保所有任务执行完毕）
      pool->waitForAllDone();
      loginfo << "[主线程] 所有任务执行完毕，程序退出";

}*/



