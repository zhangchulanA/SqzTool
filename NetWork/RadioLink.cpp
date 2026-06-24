#include "RadioLink.h"
#include <QDebug>
#include <QDateTime>

RadioLink::RadioLink(QObject *parent)
    : QObject(parent)
    , m_udpSocket(nullptr)
    , m_targetPort(0)
    , m_maxBytesPerSecond(5120)
    , m_nextSendTimeMs(0)
    , m_totalPendingBytes(0)
    , m_currentSeqNum(0)
    , m_rtt(100)        // 初始RTT假设
    , m_rto(300)        // 初始RTO
    , m_highWaterMark(50 * 1024)
    , m_lowWaterMark(20 * 1024)
    , m_isBufferWarning(false)
    , m_bytesSentInCurrentSecond(0)
    , m_lastSpeedEmitTime(0)
    , m_isSending(false)
{
    m_udpSocket = new QUdpSocket(this);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &RadioLink::onReadyRead);
    connect(m_udpSocket, static_cast<void (QUdpSocket::*)(QAbstractSocket::SocketError)>(&QUdpSocket::error),
            this, &RadioLink::onSocketError);
    m_sendTimer = new QTimer(this);
    m_sendTimer->setSingleShot(true);
    connect(m_sendTimer, &QTimer::timeout, this, &RadioLink::onSendTimer);

    m_monitorTimer = new QTimer(this);
    m_monitorTimer->setInterval(1000);
    connect(m_monitorTimer, &QTimer::timeout, this, &RadioLink::onMonitorTimer);
    m_monitorTimer->start();

    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setInterval(200);
    connect(m_timeoutTimer, &QTimer::timeout, this, &RadioLink::onTimeoutTimer);
    m_timeoutTimer->start();

    m_clock.start();
}

RadioLink::~RadioLink()
{
    m_sendTimer->stop();
    m_monitorTimer->stop();
    m_timeoutTimer->stop();
}

void RadioLink::init(const QHostAddress &localAddr, quint16 localPort,
                           const QHostAddress &targetAddr, quint16 targetPort,
                           qint64 maxBytesPerSecond)
{
    QMutexLocker locker(&m_mutex);
    m_targetAddr = targetAddr;
    m_targetPort = targetPort;
    m_maxBytesPerSecond = maxBytesPerSecond;

    if (m_udpSocket->state() == QAbstractSocket::BoundState)
        m_udpSocket->close();

    if (!m_udpSocket->bind(localAddr, localPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qWarning() << "[UDP] Bind failed:" << m_udpSocket->errorString();
        emit socketError(m_udpSocket->errorString());
    } else {
        qDebug() << "[UDP] Init OK, local:" << localAddr << localPort
                 << "target:" << targetAddr << targetPort
                 << "rate:" << maxBytesPerSecond << "B/s";
    }
}

void RadioLink::sendData(const QByteArray &data)
{
    if (data.isEmpty()) return;

    QMutexLocker locker(&m_mutex);
    m_normalQueue.enqueue(data);
    m_totalPendingBytes += data.size();

    // 水位告警
    if (m_totalPendingBytes > m_highWaterMark && !m_isBufferWarning) {
        m_isBufferWarning = true;
        emit bufferWarning(true, m_totalPendingBytes);
        qDebug() << "[Buffer] High water exceeded, pending:" << m_totalPendingBytes;
    }

    // 如果发送定时器未激活，立即启动（触发一次发送）
    if (!m_sendTimer->isActive()) {
        m_sendTimer->start(0);
    }
}

void RadioLink::processAck(quint16 seqLow)
{
    QMutexLocker locker(&m_mutex);
    auto it = m_pendingMap.find(seqLow);
    if (it != m_pendingMap.end()) {
        qint64 now = m_clock.elapsed();
        qint64 rtt = now - it.value().sendTimeMs;
        if (rtt > 0) updateRtt(rtt);

        m_totalPendingBytes -= it.value().data.size();
        m_pendingMap.erase(it);
        // emit packetAcked(seqLow);
    }
}

void RadioLink::setRateLimit(qint64 bytesPerSecond)
{
    QMutexLocker locker(&m_mutex);
    m_maxBytesPerSecond = bytesPerSecond;
    // 重新激活定时器，应用新速率
    if (!m_sendTimer->isActive() && (!m_normalQueue.isEmpty() || !m_retryQueue.isEmpty())) {
        m_sendTimer->start(0);
    }
    qDebug() << "[Rate] Updated to" << bytesPerSecond << "B/s";
}

void RadioLink::setWaterMarks(qint64 highWater, qint64 lowWater)
{
    QMutexLocker locker(&m_mutex);
    m_highWaterMark = highWater;
    m_lowWaterMark = lowWater;
}

// ---------- 私有槽函数 ----------

void RadioLink::onSendTimer()
{
    if (m_isSending) return;
    m_isSending = true;

    QMutexLocker locker(&m_mutex);

    // 如果队列为空，停止定时器
    if (m_normalQueue.isEmpty() && m_retryQueue.isEmpty()) {
        m_sendTimer->stop();
        m_isSending = false;
        return;
    }

    qint64 now = m_clock.elapsed();

    // 检查是否达到下次发送时间
    if (now < m_nextSendTimeMs) {
        // 未到时间，重新设置定时器
        m_sendTimer->start(m_nextSendTimeMs - now);
        m_isSending = false;
        return;
    }

    // 发送一个包
    if (!sendOnePacket()) {
        // 发送失败（例如socket错误），稍后重试
        m_sendTimer->start(50);
        m_isSending = false;
        return;
    }

    // 发送成功，计算下次发送时间（基于当前发送的包大小，但我们无法获取已发送包的大小？）
    // 我们可以在sendOnePacket中返回发送的包大小，但当前设计未返回。为了简化，我们取队列头部的大小（如果有包的话）
    // 但已发送的包已出队，我们无法获取其大小，但我们可以从之前保存的变量？我们可以让sendOnePacket返回size。
    // 重构：让sendOnePacket返回发送的字节数，并更新m_nextSendTime。
    // 但为了简化，我们在sendOnePacket内部更新m_nextSendTime，但需要传入当前时间。
    // 改进：sendOnePacket内部计算间隔并设置m_nextSendTime。
    // 因为我们发送的是当前包，所以我们可以知道大小。我们修改sendOnePacket，使其返回bool，并在内部设置m_nextSendTime。
    // 由于我们已经实现了sendOnePacket，需要重写。下面我们将重新实现sendOnePacket。

    // 由于上述设计，我们将在sendOnePacket中处理m_nextSendTime。
    // 但这里我们调用sendOnePacket后，它已经更新了m_nextSendTime，所以我们不再重复计算。

    // 如果队列还有包，启动定时器在合适时间触发
    if (!m_normalQueue.isEmpty() || !m_retryQueue.isEmpty()) {
        qint64 interval = m_nextSendTimeMs - m_clock.elapsed();
        if (interval < 0) interval = 0;
        m_sendTimer->start(interval);
    } else {
        m_sendTimer->stop();
    }

    m_isSending = false;
}

// 重新实现发送单个包的函数，返回是否成功，并更新下次发送时间
bool RadioLink::sendOnePacket()
{
    // 此函数必须在锁内调用
    if (m_udpSocket->state() != QAbstractSocket::BoundState) {
        qWarning() << "[UDP] Socket not bound";
        return false;
    }

    QByteArray packet;
    bool isRetry = false;
    quint32 fullSeq = 0;

    // 优先重传队列
    if (!m_retryQueue.isEmpty()) {
        packet = m_retryQueue.dequeue();
        isRetry = true;
        // 从数据中提取序列号（低16位），但我们需要找到对应的pending包来更新重传次数？
        // 我们可以在pendingMap中根据低16位找到，但为了简便，我们从pendingMap中获取重传次数，但低16位可能冲突？但窗口限制保证唯一。
        if (packet.size() < 2) return false;
        quint16 seqLow = (static_cast<quint16>(static_cast<unsigned char>(packet[1])) << 8)
                         | static_cast<quint16>(static_cast<unsigned char>(packet[0]));
        auto it = m_pendingMap.find(seqLow);
        if (it != m_pendingMap.end()) {
            // 更新重传时间和计数
            it.value().sendTimeMs = m_clock.elapsed();
            // 重传次数已在超时定时器中增加，这里不再增加
        } else {
            // 如果找不到（可能已被确认或丢弃），则丢弃这个重传包
            return true; // 当作成功，但实际未发送
        }
    } else if (!m_normalQueue.isEmpty()) {
        packet = m_normalQueue.dequeue();
        isRetry = false;
        fullSeq = getNextSeqNum();
        if (packet.size() < 2) {
            packet.prepend(QByteArray(2, char(0)));
        }
        packet[0] = (fullSeq >> 8) & 0xFF;
        packet[1] = fullSeq & 0xFF;
    } else {
        return false; // 无包
    }

    // 发送数据报
    qint64 sent = m_udpSocket->writeDatagram(packet, m_targetAddr, m_targetPort);
    if (sent == -1) {
        // 发送失败，放回队列
        if (isRetry) m_retryQueue.prepend(packet);
        else m_normalQueue.prepend(packet);
        return false;
    }

    // 更新总积压
    m_totalPendingBytes -= packet.size();

    // 如果是新包，加入pendingMap
    if (!isRetry) {
        PacketMeta meta;
        meta.data = packet; // 拷贝，因为要重传时使用
        meta.seqNum = fullSeq;
        meta.sendTimeMs = m_clock.elapsed();
        meta.retryCount = 0;
        // 检查飞行窗口
        if (m_pendingMap.size() < MAX_FLIGHT_WINDOW) {
            m_pendingMap.insert(static_cast<quint16>(fullSeq & 0xFFFF), meta);
        } else {
            // 窗口满，将包放回普通队列（紧急情况，不应发生）
            m_normalQueue.prepend(packet);
            m_totalPendingBytes += packet.size();
            qWarning() << "[UDP] Flight window full, packet re-queued";
            return false;
        }
    } else {
        // 重发包：更新pending中的发送时间已在之前完成，我们已更新
    }

    // 计算下次发送时间：基于当前包大小
    qint64 intervalMs = static_cast<qint64>(packet.size() * 1000.0 / m_maxBytesPerSecond);
    if (intervalMs < 1) intervalMs = 1;  // 至少1ms
    m_nextSendTimeMs = m_clock.elapsed() + intervalMs;

    // 统计
    m_bytesSentInCurrentSecond += packet.size();

    return true;
}

void RadioLink::onMonitorTimer()
{
    emit speedUpdated(m_bytesSentInCurrentSecond);
    m_bytesSentInCurrentSecond = 0;

    // 也可输出当前积压
    // qDebug() << "[Monitor] Pending:" << m_totalPendingBytes;
}

void RadioLink::onTimeoutTimer()
{
    QMutexLocker locker(&m_mutex);
    qint64 now = m_clock.elapsed();
    QList<quint16> toRemove;

    for (auto it = m_pendingMap.begin(); it != m_pendingMap.end(); ++it) {
        PacketMeta &meta = it.value();
        qint64 elapsed = now - meta.sendTimeMs;
        if (elapsed > m_rto) {
            if (meta.retryCount < MAX_RETRY_COUNT) {
                meta.retryCount++;
                // 将数据放入重传队列（高优先级）
                m_retryQueue.enqueue(meta.data);
                m_totalPendingBytes += meta.data.size(); // 增加积压
                // 更新时间戳，防止立即再次超时
                meta.sendTimeMs = now;
                qDebug() << "[Retry] Seq" << (meta.seqNum & 0xFFFF)
                         << "retry" << meta.retryCount << ", RTO=" << m_rto;
                emit packetRetrying(static_cast<quint16>(meta.seqNum & 0xFFFF), meta.retryCount);

                // 如果发送定时器未激活，立即启动
                if (!m_sendTimer->isActive()) {
                    m_sendTimer->start(0);
                }
            } else {
                // 超重传次数，丢弃
                qDebug() << "[Drop] Seq" << (meta.seqNum & 0xFFFF) << "dropped";
                m_totalPendingBytes -= meta.data.size();
                toRemove.append(it.key());
            }
        }
    }

    for (quint16 key : toRemove) {
        m_pendingMap.remove(key);
    }

    // 检查低水位清除告警
    if (m_isBufferWarning && m_totalPendingBytes < m_lowWaterMark) {
        m_isBufferWarning = false;
        emit bufferWarning(false, m_totalPendingBytes);
        qDebug() << "[Buffer] Recovered, pending:" << m_totalPendingBytes;
    }
}

void RadioLink::onReadyRead()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 port;
        qint64 readSize = m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &port);
        if (readSize == -1) {
            qWarning() << "[UDP] Read error:" << m_udpSocket->errorString();
            continue;
        }

        if (datagram.size() < 2) continue;

        // 如果只有2字节，视为ACK
        if (datagram.size() == 2) {
            quint16 seqLow = (static_cast<quint16>(static_cast<unsigned char>(datagram[1])) << 8)
                             | static_cast<quint16>(static_cast<unsigned char>(datagram[0]));
            processAck(seqLow);
        } else {
            // 业务数据：回复ACK（前2字节）
            QByteArray ack = datagram.left(2);
            m_udpSocket->writeDatagram(ack, sender, port); // 回复给发送方
            // 向上层抛数据（去掉前2字节）
            emit getMessage(datagram.mid(2));
        }
    }
}

void RadioLink::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    QString err = m_udpSocket->errorString();
    qWarning() << "[UDP] Socket error:" << err;
    emit socketError(err);
    // 尝试重新绑定（简单处理）
    // 实际应通知上层重新init
}

// ---------- 辅助函数 ----------

quint32 RadioLink::getNextSeqNum()
{
    return m_currentSeqNum++;
}

void RadioLink::updateRtt(qint64 rttMs)
{
    // 简单平滑：SRTT = 0.875*SRTT + 0.125*RTT
    // RTO = SRTT + 4*RTTVAR，这里简化为 SRTT + 4*|RTT-SRTT|
    static const double ALPHA = 0.125;
    static const double BETA = 0.25;
    qint64 diff = rttMs - m_rtt;
    m_rtt = static_cast<qint64>(m_rtt * (1 - ALPHA) + rttMs * ALPHA);
    qint64 rttVar = static_cast<qint64>(m_rtt * (1 - BETA) + qAbs(diff) * BETA);
    m_rto = m_rtt + 4 * rttVar;
    m_rto = qBound(static_cast<qint64>(MIN_RTO), m_rto, static_cast<qint64>(MAX_RTO));
}
