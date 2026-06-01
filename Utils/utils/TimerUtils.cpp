#include "TimerUtils.h"
#include <algorithm>

// ═══════════════════════════════════════════════════════════════
// 构造与析构
// ═══════════════════════════════════════════════════════════════

TimerUtils::TimerUtils(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_remainUpdateTimer(new QTimer(this))
{
    // 主定时器超时连接
    connect(m_timer, &QTimer::timeout, this, &TimerUtils::onInternalTimeout);

    // 剩余时间更新定时器：每 100ms 发射一次 remainingTimeChanged 信号
    // 避免高频更新导致信号泛滥
    m_remainUpdateTimer->setInterval(100);
    connect(m_remainUpdateTimer, &QTimer::timeout, this, [this]() {
        if (isActive()) {
            emit remainingTimeChanged(remainingTime());
        }
    });
}

TimerUtils::~TimerUtils()
{
    stop(); // 确保定时器停止，清理资源
}

// ═══════════════════════════════════════════════════════════════
// 工厂方法
// ═══════════════════════════════════════════════════════════════

TimerUtils* TimerUtils::create(QObject *parent)
{
    return new TimerUtils(parent);
}

TimerUtils* TimerUtils::singleShot(int msec, std::function<void()> callback,
                                    QObject *parent, bool autoDelete)
{
    auto *timer = new TimerUtils(parent);
    timer->setInterval(msec)
          .setSingleShot(true)
          .setAutoDelete(autoDelete)
          .onTimeout(std::move(callback));
    timer->start();
    return timer;
}

TimerUtils* TimerUtils::periodic(int msec, std::function<void()> callback,
                                  QObject *parent)
{
    auto *timer = new TimerUtils(parent);
    timer->setInterval(msec)
          .setSingleShot(false)
          .onTimeout(std::move(callback));
    timer->start();
    return timer;
}

// ═══════════════════════════════════════════════════════════════
// 链式配置（全部返回 & 引用）
// ═══════════════════════════════════════════════════════════════

TimerUtils& TimerUtils::setInterval(int msec)
{
    // 保护：间隔不能小于等于 0
    if (msec <= 0) {
        msec = 1;
    }

    if (m_interval != msec) {
        m_interval = msec;
        // 如果定时器正在运行且未暂停，同步更新底层 QTimer 间隔
        if (m_timer->isActive() && !m_paused) {
            m_timer->setInterval(msec);
        }
        emit intervalChanged(m_interval);
    }
    return *this;
}

TimerUtils& TimerUtils::setSingleShot(bool single)
{
    if (m_singleShot != single) {
        m_singleShot = single;
        m_timer->setSingleShot(single);
        emit singleShotChanged(m_singleShot);
    }
    return *this;
}

TimerUtils& TimerUtils::setAutoDelete(bool autoDelete)
{
    m_autoDelete = autoDelete;
    return *this;
}

TimerUtils& TimerUtils::onTimeout(std::function<void()> callback)
{
    m_callback = std::move(callback);
    return *this;
}

TimerUtils& TimerUtils::setTimerType(Qt::TimerType type)
{
    m_timer->setTimerType(type);
    return *this;
}

TimerUtils& TimerUtils::setRepeatCount(int count)
{
    // 非正整数视为无限
    m_repeatCount = (count <= 0) ? -1 : count;
    return *this;
}

TimerUtils& TimerUtils::setImmediateExecution(bool immediate)
{
    m_immediateExecution = immediate;
    return *this;
}

TimerUtils& TimerUtils::setFrequencyWindowSize(int size)
{
    // 至少保留 2 个采样点才能计算频率
    m_freqWindowSize = std::max(2, size);
    // 裁剪已有的时间戳数据
    while (static_cast<int>(m_timestamps.size()) > m_freqWindowSize) {
        m_timestamps.pop_front();
    }
    return *this;
}

// ═══════════════════════════════════════════════════════════════
// 控制接口
// ═══════════════════════════════════════════════════════════════

void TimerUtils::start()
{
    // 重置运行时状态
    m_paused = false;
    m_executionCount = 0;
    m_timestamps.clear();

    // 启动剩余时间更新定时器
    m_remainUpdateTimer->start();

    // 如果设置了立即执行，先同步触发一次回调
    if (m_immediateExecution && m_callback) {
        m_callback();
        emit timeout();
    }

    // 启动底层 QTimer
    m_timer->start(m_interval);
    emit activeChanged(true);
}

void TimerUtils::stop()
{
    m_paused = false;
    m_timer->stop();
    m_remainUpdateTimer->stop();
    m_timestamps.clear();
    emit activeChanged(false);
}

void TimerUtils::reset()
{
    stop();
    start();
}

void TimerUtils::pause()
{
    // 只有在运行中且未暂停时才执行暂停
    if (!m_timer->isActive() || m_paused) {
        return;
    }

    // 记录当前的剩余时间
    m_remainingOnPause = m_timer->remainingTime();
    m_timer->stop();
    m_remainUpdateTimer->stop();
    m_paused = true;
    emit remainingTimeChanged(m_remainingOnPause);
}

void TimerUtils::resume()
{
    // 只有在暂停状态才执行恢复
    if (!m_paused) {
        return;
    }

    m_paused = false;
    m_remainUpdateTimer->start();
    startResumeTimer();
}

void TimerUtils::triggerNow()
{
    if (m_callback) {
        m_callback();
        emit timeout();
    }
}

// ═══════════════════════════════════════════════════════════════
// 查询接口
// ═══════════════════════════════════════════════════════════════

bool TimerUtils::isActive() const
{
    // 运行中或暂停中都视为活跃
    return m_timer->isActive() || m_paused;
}

bool TimerUtils::isSingleShot() const
{
    return m_singleShot;
}

int TimerUtils::interval() const
{
    return m_interval;
}

int TimerUtils::remainingTime() const
{
    if (m_paused) {
        return m_remainingOnPause;
    }
    return m_timer->isActive() ? m_timer->remainingTime() : -1;
}

int TimerUtils::executionCount() const
{
    return m_executionCount;
}

double TimerUtils::averageFrequency() const
{
    // 至少需要 2 个采样点才能计算频率
    if (m_timestamps.size() < 2) {
        return 0.0;
    }

    // 计算第一个和最后一个时间戳之间的总时长
    auto totalDuration = m_timestamps.back() - m_timestamps.front();
    double totalSeconds = std::chrono::duration<double>(totalDuration).count();

    // 频率 = (采样点数 - 1) / 总时间（秒）
    return (totalSeconds > 0.0)
           ? (m_timestamps.size() - 1) / totalSeconds
           : 0.0;
}

Qt::TimerType TimerUtils::timerType() const
{
    return m_timer->timerType();
}

// ═══════════════════════════════════════════════════════════════
// 操作符重载
// ═══════════════════════════════════════════════════════════════

TimerUtils& TimerUtils::operator+=(int msec)
{
    return setInterval(m_interval + msec);
}

TimerUtils& TimerUtils::operator-=(int msec)
{
    // 间隔最小为 1ms
    return setInterval(std::max(1, m_interval - msec));
}

// ═══════════════════════════════════════════════════════════════
// 内部实现
// ═══════════════════════════════════════════════════════════════

void TimerUtils::onInternalTimeout()
{
    // ── 更新频率统计 ──
    m_timestamps.push_back(std::chrono::steady_clock::now());
    // 保持时间戳队列不超过窗口大小
    while (static_cast<int>(m_timestamps.size()) > m_freqWindowSize) {
        m_timestamps.pop_front();
    }

    // ── 更新执行计数 ──
    m_executionCount++;
    emit executionCountChanged(m_executionCount);

    // ── 发射频率变化信号 ──
    if (m_timestamps.size() >= 2) {
        emit averageFrequencyChanged(averageFrequency());
    }

    // ── 执行用户回调 ──
    if (m_callback) {
        m_callback();
    }

    // ── 发射超时信号 ──
    emit timeout();

    // ── 检查重复次数限制 ──
    if (m_repeatCount > 0 && m_executionCount >= m_repeatCount) {
        if (m_autoDelete) {
            // 自动删除：在事件循环中安全删除
            deleteLater();
        } else {
            stop();
        }
        return;
    }

    // ── 单次模式自动删除 ──
    if (m_singleShot && m_autoDelete) {
        deleteLater();
    }
}

void TimerUtils::startResumeTimer()
{
    // 使用临时单次 QTimer 跑完暂停时记录的剩余时间
    QTimer::singleShot(m_remainingOnPause, this, [this]() {
        // 可能在等待期间又被暂停了
        if (m_paused) {
            return;
        }

        // 触发一次回调（模拟正常超时）
        onInternalTimeout();

        // 如果仍处于活跃状态且是周期模式，重新启动主定时器
        if (!m_paused && !m_singleShot) {
            m_timer->start(m_interval);
        }
    });
}

void TimerUtils::updateFrequencyStats()
{
    // 此方法保留，供未来扩展使用
    // 当前频率计算在 onInternalTimeout() 和 averageFrequency() 中完成
}
