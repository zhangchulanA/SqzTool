#ifndef TIMERUTILS_H
#define TIMERUTILS_H

#include <QObject>
#include <QTimer>
#include <functional>
#include <chrono>
#include <deque>


/**
 * @brief 定时器工具类
 *
 * TimerUtils 是一个基于 QTimer 封装的高级定时器工具类，提供以下特性：
 *
 * 核心功能：
 * - 链式调用配置，所有 setter 返回引用（&）以支持连续调用
 * - 支持 std::chrono 时长字面量（如 500ms、2s、1min）设置间隔
 * - 单次/周期性定时、暂停/恢复、重置、手动触发
 * - 重复次数限制（执行指定次数后自动停止）
 * - 可指定定时器精度：PreciseTimer / CoarseTimer / VeryCoarseTimer
 * - 启动时立即执行选项（immediate execution）
 * - 运行频率统计（基于滑动窗口的平均频率计算）
 * - 单次定时器自动删除机制（避免内存泄漏）
 * - 丰富的 Qt 信号（活跃状态、间隔、剩余时间、执行次数、频率变化）
 * - 操作符重载（+=/-= 调整间隔，bool 转换判断活跃状态）
 *
 * 创建方式（工厂方法）：
 * - TimerUtils::create(parent)       → 创建空定时器，需手动配置并 start()
 * - TimerUtils::singleShot(...)      → 创建单次定时器，立即启动，默认自动删除
 * - TimerUtils::periodic(...)        → 创建周期定时器，立即启动，持续执行
 *
 * 线程安全：
 * - 必须在拥有事件循环的线程（如主线程）中使用
 * - 不可跨线程操作同一个 TimerUtils 对象
 *
 * 生命周期：
 * - 继承 QObject，随父对象销毁自动清理
 * - 单次模式可通过 setAutoDelete(true) 在执行后自动 deleteLater()
 *
 * 依赖：
 * - Qt 5.14.2+ (Core 模块)
 * - C++17 或更高版本
 * 使用前引入 using namespace std::chrono_literals; 时间可以写成2s
 */
class TimerUtils : public QObject
{
    Q_OBJECT

    // ─────────────────── Qt 属性（可在 QML/设计器中使用） ───────────────────
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
    Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY intervalChanged)
    Q_PROPERTY(int remainingTime READ remainingTime NOTIFY remainingTimeChanged)
    Q_PROPERTY(bool singleShot READ isSingleShot NOTIFY singleShotChanged)
    Q_PROPERTY(int executionCount READ executionCount NOTIFY executionCountChanged)
    Q_PROPERTY(double averageFrequency READ averageFrequency NOTIFY averageFrequencyChanged)

public:
    // ─────────────────── 工厂方法 ───────────────────

    /**
     * @brief 创建一个待配置的定时器（未启动状态）
     *
     * 使用此方法创建后，可以通过链式调用进行完整配置，最后手动调用 start()。
     * 适合需要延迟启动或运行时动态调整参数的场景。
     *
     * @param parent 父对象，用于 Qt 对象树管理生命周期
     * @return 新创建的 TimerUtils 指针（调用者负责管理或交由父对象管理）
     *
     * @code
     * auto *timer = TimerUtils::create(this);
     * timer->setInterval(1000)->onTimeout(callback)->start();
     * @endcode
     */
    static TimerUtils* create(QObject *parent = nullptr);

    /**
     * @brief 创建并立即启动一个单次定时器
     *
     * 定时器将在指定延迟后执行回调一次，完成后默认自动删除。
     * 适合延迟执行、超时保护、防抖等一次性任务。
     *
     * @param msec 延迟时间（毫秒），必须 > 0
     * @param callback 超时回调函数
     * @param parent 父对象
     * @param autoDelete 执行后是否自动 deleteLater()，默认 true
     * @return 新创建的 TimerUtils 指针
     *
     * @code
     * // 2秒后执行
     * TimerUtils::singleShot(2000, [] { qDebug() << "done"; });
     *
     * // 手动管理生命周期
     * auto *t = TimerUtils::singleShot(5000, cb, this, false);
     * @endcode
     */
    static TimerUtils* singleShot(int msec, std::function<void()> callback,
                                  QObject *parent = nullptr, bool autoDelete = true);

    /**
     * @brief 创建并立即启动一个单次定时器（std::chrono 版本）
     *
     * @tparam Rep 数值类型（如 int, long long）
     * @tparam Period 时间单位（如 std::ratio<1>, std::milli）
     * @param interval 延迟时长（支持 500ms、2s、1min 等字面量）
     * @param callback 超时回调函数
     * @param parent 父对象
     * @param autoDelete 执行后是否自动 deleteLater()，默认 true
     * @return 新创建的 TimerUtils 指针
     *
     * @code
     * using namespace std::chrono_literals;
     * TimerUtils::singleShot(3s, [] { qDebug() << "3秒后"; });
     * @endcode
     */
    template<typename Rep, typename Period>
    static TimerUtils* singleShot(std::chrono::duration<Rep, Period> interval,
                                  std::function<void()> callback,
                                  QObject *parent = nullptr, bool autoDelete = true);

    /**
     * @brief 创建并立即启动一个周期性定时器
     *
     * 定时器将按照指定间隔重复执行回调，直到手动 stop() 或达到重复次数限制。
     * 适合心跳检测、数据轮询、界面刷新等持续性任务。
     *
     * @param msec 定时间隔（毫秒），必须 > 0
     * @param callback 每次超时的回调函数
     * @param parent 父对象
     * @return 新创建的 TimerUtils 指针
     *
     * @code
     * auto *timer = TimerUtils::periodic(1000, [] {
     *     qDebug() << "每秒执行";
     * });
     * @endcode
     */
    static TimerUtils* periodic(int msec, std::function<void()> callback,
                                QObject *parent = nullptr);

    /**
     * @brief 创建并立即启动一个周期性定时器（std::chrono 版本）
     *
     * @tparam Rep 数值类型
     * @tparam Period 时间单位
     * @param interval 定时间隔（支持 500ms、2s、1min 等字面量）
     * @param callback 每次超时的回调函数
     * @param parent 父对象
     * @return 新创建的 TimerUtils 指针
     *
     * @code
     * using namespace std::chrono_literals;
     * TimerUtils::periodic(5min, [] { qDebug() << "每5分钟"; });
     * @endcode
     */
    template<typename Rep, typename Period>
    static TimerUtils* periodic(std::chrono::duration<Rep, Period> interval,
                                std::function<void()> callback,
                                QObject *parent = nullptr);

    // ─────────────────── 构造与析构 ───────────────────

    /**
     * @brief 析构函数，自动停止定时器并清理资源
     */
    ~TimerUtils() override;

    /**
     * @brief 禁止拷贝构造
     */
    TimerUtils(const TimerUtils&) = delete;

    /**
     * @brief 禁止拷贝赋值
     */
    TimerUtils& operator=(const TimerUtils&) = delete;

    // ─────────────────── 链式配置接口（全部返回引用 & 以支持连续调用） ───────────────────

    /**
     * @brief 设置定时间隔
     *
     * @param msec 间隔毫秒数，必须 > 0（≤0 时自动设为 1ms）
     * @return 自身引用，支持链式调用
     *
     * @code
     * timer->setInterval(500)->onTimeout(cb)->start();
     * @endcode
     */
    TimerUtils& setInterval(int msec);

    /**
     * @brief 设置定时间隔（std::chrono 版本）
     *
     * @tparam Rep 数值类型
     * @tparam Period 时间单位
     * @param interval 间隔时长
     * @return 自身引用，支持链式调用
     *
     * @code
     * using namespace std::chrono_literals;
     * timer->setInterval(500ms)->onTimeout(cb)->start();
     * @endcode
     */
    template<typename Rep, typename Period>
    TimerUtils& setInterval(std::chrono::duration<Rep, Period> interval);

    /**
     * @brief 设置是否为单次触发模式
     *
     * @param single true 为单次触发，false 为周期性触发
     * @return 自身引用，支持链式调用
     *
     * @note 单次模式执行完成后，若 autoDelete 为 true 则自动 deleteLater()
     */
    TimerUtils& setSingleShot(bool single);

    /**
     * @brief 设置执行完成后是否自动删除（仅单次模式有效）
     *
     * @param autoDelete true 则执行完成后自动调用 deleteLater()
     * @return 自身引用，支持链式调用
     */
    TimerUtils& setAutoDelete(bool autoDelete);

    /**
     * @brief 设置超时回调函数
     *
     * @param callback 可调用对象（Lambda、函数指针、std::bind 等）
     * @return 自身引用，支持链式调用
     *
     * @code
     * timer->onTimeout([] { qDebug() << "触发"; });
     * timer->onTimeout(std::bind(&MyClass::method, &obj));
     * @endcode
     */
    TimerUtils& onTimeout(std::function<void()> callback);

    /**
     * @brief 设置定时器精度类型
     *
     * @param type 精度类型：
     *   - Qt::PreciseTimer    → 毫秒级精度（高 CPU 占用）
     *   - Qt::CoarseTimer     → 约 5% 误差（默认，平衡型）
     *   - Qt::VeryCoarseTimer → 秒级精度（省电，适合非关键任务）
     * @return 自身引用，支持链式调用
     */
    TimerUtils& setTimerType(Qt::TimerType type);

    /**
     * @brief 设置最大执行次数
     *
     * @param count 最大执行次数，-1 表示无限（默认），≤0 时自动设为 -1
     * @return 自身引用，支持链式调用
     *
     * @note 达到指定次数后，单次模式会自动删除，周期模式会自动 stop()
     */
    TimerUtils& setRepeatCount(int count);

    /**
     * @brief 设置 start() 时是否立即执行一次回调
     *
     * @param immediate true 则在 start() 时立即触发一次回调（不计入 repeatCount）
     * @return 自身引用，支持链式调用
     *
     * @note 该回调在定时器启动前同步执行
     */
    TimerUtils& setImmediateExecution(bool immediate);

    /**
     * @brief 设置频率统计的滑动窗口大小
     *
     * @param size 保留最近多少次的时间戳用于计算平均频率（默认 10，最小 2）
     * @return 自身引用，支持链式调用
     */
    TimerUtils& setFrequencyWindowSize(int size);

    // ─────────────────── 控制接口 ───────────────────

    /**
     * @brief 启动定时器
     *
     * 重置执行计数和频率统计数据，开始定时。
     * 若设置了 immediateExecution(true)，则先同步执行一次回调。
     */
    void start();

    /**
     * @brief 停止定时器
     *
     * 完全停止，重置所有运行时状态（执行计数、频率统计、暂停状态等）。
     */
    void stop();

    /**
     * @brief 重启定时器
     *
     * 等价于 stop() + start()，重置所有状态后重新开始。
     */
    void reset();

    /**
     * @brief 暂停定时器
     *
     * 记录当前剩余时间，停止定时器。
     * 暂停状态下 isActive() 仍返回 true。
     * 若已暂停则无操作。
     */
    void pause();

    /**
     * @brief 恢复定时器
     *
     * 从暂停时记录的剩余时间开始继续计时。
     * 若未处于暂停状态则无操作。
     */
    void resume();

    /**
     * @brief 手动触发一次回调
     *
     * 立即执行回调函数并发射 timeout() 信号。
     * 不影响定时器运行状态、执行计数和频率统计。
     */
    void triggerNow();

    // ─────────────────── 查询接口 ───────────────────

    /**
     * @brief 是否处于活跃状态
     * @return true 表示正在运行或暂停中
     */
    bool isActive() const;

    /**
     * @brief 是否为单次触发模式
     * @return true 表示单次，false 表示周期
     */
    bool isSingleShot() const;

    /**
     * @brief 获取当前间隔
     * @return 间隔毫秒数
     */
    int interval() const;

    /**
     * @brief 获取剩余时间
     * @return 距离下次触发的毫秒数，暂停时返回暂停时的剩余时间，未激活返回 -1
     */
    int remainingTime() const;

    /**
     * @brief 获取已执行次数
     * @return 从最近一次 start()/reset() 以来的执行次数
     */
    int executionCount() const;

    /**
     * @brief 获取最近一段时间内的平均触发频率
     * @return 频率（Hz），数据不足 2 个采样点时返回 0.0
     */
    double averageFrequency() const;

    /**
     * @brief 获取定时器精度类型
     * @return 当前设置的 Qt::TimerType
     */
    Qt::TimerType timerType() const;

    // ─────────────────── 操作符重载 ───────────────────

    /**
     * @brief 增加定时间隔（毫秒）
     * @param msec 增加的毫秒数
     * @return 自身引用
     */
    TimerUtils& operator+=(int msec);

    /**
     * @brief 减少定时间隔（毫秒），最小为 1ms
     * @param msec 减少的毫秒数
     * @return 自身引用
     */
    TimerUtils& operator-=(int msec);

    /**
     * @brief bool 转换操作符
     * @return true 表示定时器处于活跃状态（运行中或暂停中）
     *
     * @code
     * if (*timer) { qDebug() << "运行中"; }
     * @endcode
     */
    explicit operator bool() const { return isActive(); }

signals:
    /**
     * @brief 每次定时超时时发射
     */
    void timeout();

    /**
     * @brief 活跃状态变化时发射
     * @param active 当前活跃状态
     */
    void activeChanged(bool active);

    /**
     * @brief 间隔时间变化时发射
     * @param interval 新的间隔毫秒数
     */
    void intervalChanged(int interval);

    /**
     * @brief 剩余时间变化时发射（每 100ms 更新一次，避免高频触发）
     * @param remaining 当前剩余毫秒数
     */
    void remainingTimeChanged(int remaining);

    /**
     * @brief 单次触发模式变化时发射
     * @param single 当前是否为单次模式
     */
    void singleShotChanged(bool single);

    /**
     * @brief 执行次数变化时发射
     * @param count 当前已执行次数
     */
    void executionCountChanged(int count);

    /**
     * @brief 平均频率变化时发射
     * @param freq 当前平均频率（Hz）
     */
    void averageFrequencyChanged(double freq);

private:
    /**
     * @brief 私有构造函数，通过工厂方法创建实例
     * @param parent 父对象
     */
    explicit TimerUtils(QObject *parent = nullptr);

    /**
     * @brief 内部超时处理槽函数
     *
     * 更新时间戳、执行计数、频率统计，执行回调，检查重复次数限制，处理自动删除。
     */
    void onInternalTimeout();

    /**
     * @brief 恢复暂停的定时器
     *
     * 使用临时单次 QTimer 跑完暂停时记录的剩余时间，
     * 触发后如果是周期模式则重新启动主定时器。
     */
    void startResumeTimer();

    /**
     * @brief 更新频率统计
     *
     * 维护时间戳队列，计算并发射平均频率。
     */
    void updateFrequencyStats();

    // ─────────────────── 成员变量 ───────────────────

    QTimer *m_timer = nullptr;                              ///< 核心 QTimer
    std::function<void()> m_callback;                       ///< 超时回调函数
    int m_interval = 0;                                     ///< 定时间隔（毫秒）
    bool m_singleShot = false;                              ///< 是否单次触发
    bool m_autoDelete = false;                              ///< 单次完成后是否自动删除
    bool m_paused = false;                                  ///< 是否处于暂停状态
    bool m_immediateExecution = false;                      ///< start()时是否立即执行
    int m_repeatCount = -1;                                 ///< 重复次数限制（-1 无限）
    int m_remainingOnPause = 0;                             ///< 暂停时记录的剩余时间
    int m_executionCount = 0;                               ///< 已执行次数

    int m_freqWindowSize = 10;                              ///< 频率统计窗口大小
    std::deque<std::chrono::steady_clock::time_point> m_timestamps; ///< 时间戳队列

    QTimer *m_remainUpdateTimer = nullptr;                  ///< 剩余时间更新专用定时器
};

// ─────────────────── 模板实现（必须放在头文件中以支持模板实例化） ───────────────────

template<typename Rep, typename Period>
TimerUtils* TimerUtils::singleShot(std::chrono::duration<Rep, Period> interval,
                                    std::function<void()> callback,
                                    QObject *parent, bool autoDelete)
{
    auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(interval).count();
    return singleShot(static_cast<int>(msec), std::move(callback), parent, autoDelete);
}

template<typename Rep, typename Period>
TimerUtils* TimerUtils::periodic(std::chrono::duration<Rep, Period> interval,
                                  std::function<void()> callback,
                                  QObject *parent)
{
    auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(interval).count();
    return periodic(static_cast<int>(msec), std::move(callback), parent);
}

template<typename Rep, typename Period>
TimerUtils& TimerUtils::setInterval(std::chrono::duration<Rep, Period> interval)
{
    auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(interval).count();
    return setInterval(static_cast<int>(msec));
}

/*
 * ╔══════════════════════════════════════════════════════════════════════════════╗
 * ║                              使用示例                                        ║
 * ╚══════════════════════════════════════════════════════════════════════════════╝
 *
 * using namespace std::chrono_literals;
 *
 * // ─── 方式一：单次定时器（延迟执行、超时保护、防抖） ───
 *
 *   TimerUtils::singleShot(2s, [] {
 *       qDebug() << "2秒后执行一次，自动删除";
 *   });
 *
 *   // 不自动删除，手动管理
 *   auto *t1 = TimerUtils::singleShot(5s, [] {
 *       qDebug() << "5秒后执行，手动管理";
 *   }, this, false);
 *
 * // ─── 方式二：周期定时器（心跳、轮询、刷新） ───
 *
 *   auto *heartbeat = TimerUtils::periodic(10s, [this] {
 *       sendHeartbeat();
 *   });
 *
 *   // 动态控制
 *   heartbeat->pause();
 *   heartbeat->resume();
 *   heartbeat->stop();
 *
 * // ─── 方式三：完整配置（复杂场景） ───
 *
 *   auto *timer = TimerUtils::create(this);
 *   timer->setInterval(500ms)                  // chrono 字面量
 *         ->onTimeout([] {                      // Lambda 回调
 *             static int count = 0;
 *             qDebug() << "触发次数:" << ++count;
 *         })
 *         ->setRepeatCount(10)                  // 只执行 10 次
 *         ->setTimerType(Qt::PreciseTimer)      // 高精度
 *         ->setImmediateExecution(true)         // 启动时立即执行一次
 *         ->setFrequencyWindowSize(20)          // 统计最近 20 次的频率
 *         ->start();                            // 最后启动
 *
 *   // 监听信号
 *   connect(timer, &TimerUtils::executionCountChanged,
 *           [](int c) { qDebug() << "已执行:" << c << "次"; });
 *   connect(timer, &TimerUtils::averageFrequencyChanged,
 *           [](double f) { qDebug() << "频率:" << f << "Hz"; });
 *
 * // ─── 暂停与恢复 ───
 *
 *   auto *t2 = TimerUtils::periodic(1s, [] {
 *       qDebug() << "每秒执行";
 *   });
 *
 *   TimerUtils::singleShot(3s, [t2] {
 *       t2->pause();
 *       qDebug() << "暂停，剩余:" << t2->remainingTime() << "ms";
 *
 *       TimerUtils::singleShot(2s, [t2] {
 *           t2->resume();
 *           qDebug() << "恢复运行";
 *       });
 *   });
 *
 * // ─── 操作符重载 ───
 *
 *   *timer += 200;       // 间隔增加 200ms
 *   *timer -= 100;       // 间隔减少 100ms
 *   if (*timer) {        // 判断是否活跃
 *       qDebug() << "定时器运行中";
 *   }
 *
 * // ─── 手动触发 ───
 *
 *   timer->triggerNow();  // 立即执行一次回调，不影响定时器状态
 *
 * // ─── 成员函数回调 ───
 *
 *   class MyClass {
 *   public:
 *       void onTick() { qDebug() << "成员函数回调"; }
 *   };
 *   MyClass obj;
 *   TimerUtils::periodic(500ms, std::bind(&MyClass::onTick, &obj));
 *
 * ╔══════════════════════════════════════════════════════════════════════════════╗
 * ║                              选择建议                                        ║
 * ╠══════════════════════════════════════════════════════════════════════════════╣
 * ║                                                                            ║
 * ║   singleShot()  → "过一会儿做一次"（延迟、超时、防抖）                      ║
 * ║   periodic()    → "每隔一会儿做一次"（心跳、轮询、刷新）                    ║
 * ║   create()      → "我需要完全控制"（动态调整、延迟启动、复杂配置）          ║
 * ║                                                                            ║
 * ╚══════════════════════════════════════════════════════════════════════════════╝
 */

#endif // TIMERUTILS_H
