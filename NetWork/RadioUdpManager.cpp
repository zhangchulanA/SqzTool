#include "RadioUdpManager.h"
#include <QDebug>
#include "Logger.h"
RadioUdpManager::RadioUdpManager(QObject *parent)
    : QObject(parent)
    , m_udpSocket(nullptr)
    , m_targetPort(0)
    , m_maxBytesPerSecond(5120) // 默认 5KB/s
    , m_timeSliceMs(500)         // 固定20ms时间片
    , m_isSending(false)
    , m_currentSeqNum(0)
    , m_highWaterMark(50 * 1024)  // 默认高水位 50KB
    , m_lowWaterMark(20 * 1024)   // 默认低水位 20KB
    , m_isBufferWarning(false)
    , m_bytesSentInCurrentSecond(0)
    , m_lastSpeedEmitTime(0)
{
    // 初始化Socket
    m_udpSocket = new QUdpSocket(this);

    // 创建定时器
    m_sendTimer = new QTimer(this);
    m_monitorTimer = new QTimer(this);
    m_timeoutTimer = new QTimer(this);

    // 配置定时器间隔
    m_sendTimer->setInterval(m_timeSliceMs);
    m_monitorTimer->setInterval(1000); // 1秒
    m_timeoutTimer->setInterval(200);  // 200ms

    // 连接信号槽
    connect(m_sendTimer, &QTimer::timeout, this, &RadioUdpManager::onSendTimer);
    connect(m_monitorTimer, &QTimer::timeout, this, &RadioUdpManager::onMonitorTimer);
    connect(m_timeoutTimer, &QTimer::timeout, this, &RadioUdpManager::onTimeoutTimer);
    connect(m_udpSocket,&QUdpSocket::readyRead,this,&RadioUdpManager::onReadyRead);

    // 默认启动定时器（等待init后实际生效）
    m_sendTimer->start();
    m_monitorTimer->start();
    m_timeoutTimer->start();
}

RadioUdpManager::~RadioUdpManager()
{
    // 清理资源
    m_sendTimer->stop();
    m_monitorTimer->stop();
    m_timeoutTimer->stop();
}

// --- 初始化 ---
void RadioUdpManager::init(const QHostAddress &localAddr, quint16 localPort,
                           const QHostAddress &targetAddr, quint16 targetPort, qint64 maxBytesPerSecond)
{
    m_targetAddr = targetAddr;
    m_targetPort = targetPort;
    m_maxBytesPerSecond = maxBytesPerSecond;

    // 绑定端口，允许广播和地址复用（实际电台不需要，加上也没事）
    if (!m_udpSocket->bind(localAddr, localPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qWarning() << "[AckReceiver] Failed to bind port"<<localAddr.toString() << localPort << m_udpSocket->errorString();
        return;
    }

    qDebug() << "[RadioUdpManager] Init OK, "
                "Local:"  << localAddr.toString() << localPort
             << ", Target:" << targetAddr.toString() << targetPort
             << ", MaxRate:" << maxBytesPerSecond << "B/s";
}

// --- 业务层发送接口 ---
void RadioUdpManager::sendData(const QByteArray &data)
{
    if (data.isEmpty()) return;

    QMutexLocker locker(&m_mutex);
    // 入队到普通队列尾部
    m_normalQueue.enqueue(data);

    // 【缓冲区监控】实时计算总积压（普通队列 + 重传队列 + 飞行队列）
    qint64 totalPending = 0;
    for (const auto &d : m_normalQueue) totalPending += d.size();
    for (const auto &d : m_retryQueue) totalPending += d.size();
    for (auto it = m_pendingMap.begin(); it != m_pendingMap.end(); ++it) {
        totalPending += it.value().data.size();
    }

    // 触发高水位报警（带防抖逻辑）
    if (totalPending > m_highWaterMark && !m_isBufferWarning) {
        m_isBufferWarning = true;
        emit bufferWarning(true);
        qDebug() << "[Buffer] WARNING: High water mark exceeded! Pending:" << totalPending;
    }
}

// --- 接收ACK处理 ---
void RadioUdpManager::processAck(quint16 seqNum)
{
    QMutexLocker locker(&m_mutex);
    if (m_pendingMap.contains(seqNum)) {
        m_pendingMap.remove(seqNum);
        //        emit packetAcked(seqNum);
        // qDebug() << "[ACK] Seq" << seqNum << "confirmed.";
    } else {
        // 可能是重复ACK或已超时移除，忽略
        // qDebug() << "[ACK] Seq" << seqNum << "not found or already removed.";
    }
}

// --- 动态调整速率 ---
void RadioUdpManager::setRateLimit(qint64 bytesPerSecond)
{
    QMutexLocker locker(&m_mutex);
    m_maxBytesPerSecond = bytesPerSecond;
    qDebug() << "[Rate] Updated to" << bytesPerSecond << "B/s";
}

// --- 设置水位阈值 ---
void RadioUdpManager::setWaterMarks(qint64 highWater, qint64 lowWater)
{
    QMutexLocker locker(&m_mutex);
    m_highWaterMark = highWater;
    m_lowWaterMark = lowWater;
}

// --- 核心限流发送定时器（每20ms触发一次）---
void RadioUdpManager::onSendTimer()
{
    // 【防重入】如果上一次还没执行完，直接返回，避免定时器叠加
    if (m_isSending) return;
    m_isSending = true;
    // 1. 计算本时间片配额（例如 20ms 内允许发送的字节数）
    //    公式： (每秒最大字节数 * 时间片毫秒数) / 1000
    qint64 sliceQuota = (m_maxBytesPerSecond * m_timeSliceMs) / 1000;
    if (sliceQuota <= 0) {
        // 如果每秒限流太小（如<50B/s），强制至少发1个字节，防止饿死
        sliceQuota = 1;
    }
    QMutexLocker locker(&m_mutex);

    // 2. 优先填充发送队列：如果飞行队列太大（网络拥塞），暂缓从待发队列取包
    //    这里设定飞行队列最大允许 50 个包，防止内存爆炸
    if (m_pendingMap.size() > 50) {
        m_isSending = false;
        return;
    }
    // 3. 【核心优先级逻辑】在配额内循环取包发送
    //    优先级：m_retryQueue (重传) > m_normalQueue (新数据)
    qint64 remainingQuota = sliceQuota;

    while (remainingQuota > 0) {
        QByteArray packet;
        bool isRetry = false;
        quint16 seqNum = 0;

        // ---- 3.1 优先从重传队列取 ----
        if (!m_retryQueue.isEmpty()) {
            packet = m_retryQueue.dequeue();
            isRetry = true;
            if (packet.size() < 2) continue;
            // 小端序存储SeqNum
            seqNum = (quint16)((unsigned char)packet[1] << 8) | (unsigned char)packet[0];
        }
        // ---- 3.2 若重传队列空，则从普通队列取 ----
        else if (!m_normalQueue.isEmpty()) {
            packet = m_normalQueue.dequeue();
            isRetry = false;
            // 为新包分配序列号（将SeqNum填入包前2字节）
            seqNum = getNextSeqNum();
            if (packet.size() < 2) {
                // 数据太小，填充0，实际应用应保证协议头至少2字节
                packet.prepend(QByteArray(2, char(0)));
            }
            packet[0] = (seqNum >> 8) & 0xFF;
            packet[1] = seqNum & 0xFF;
        } else {
            // 队列全空，退出循环
            break;
        }

        // ---- 3.3 判断包大小是否超过当前剩余配额 ----
        if (packet.size() > remainingQuota) {
            // 包太大，本周期发不完
            // 如果是重传包，塞回重传队头；否则塞回普通队头，等待下一周期
            if (isRetry) {
                m_retryQueue.prepend(packet);
            } else {
                m_normalQueue.prepend(packet);
            }
            // 配额耗尽，跳出循环
            break;
        }

        // ---- 3.4 发送数据（交给系统协议栈） ----
        qint64 sent = m_udpSocket->writeDatagram(packet, m_targetAddr, m_targetPort);
        logdebug <<packet.toHex()<<m_targetAddr.toString()<<m_targetPort;
        if (sent == -1) {
            // 发送失败（如系统缓冲区满），将包放回队列，停止本次发送
            if (isRetry) m_retryQueue.prepend(packet);
            else m_normalQueue.prepend(packet);
            break;
        }

        // 扣除配额
        remainingQuota -= packet.size();
        // 统计速率
        m_bytesSentInCurrentSecond += packet.size();
        // ---- 3.5 更新飞行队列（只有非重传的新包才加入Pending等待ACK） ----
        if (!isRetry) {
            PacketMeta meta;
            meta.data = packet;
            meta.seqNum = seqNum;
            meta.sendTimeMs = QDateTime::currentMSecsSinceEpoch();
            meta.retryCount = 0;
            m_pendingMap.insert(seqNum, meta);
        } else {
            // 重传包：更新pendingMap中对应报文的时间戳，重置超时计时
            if (m_pendingMap.contains(seqNum)) {
                m_pendingMap[seqNum].sendTimeMs = QDateTime::currentMSecsSinceEpoch();
                // 重传次数+1（但要在超时检测时加，这里只更新时间）
            }
        }
    } // while (remainingQuota > 0)

    // 计算当前总缓冲区积压（用于低水位报警清除）
    qint64 totalPending = 0;
    for (const auto &d : m_normalQueue) totalPending += d.size();
    for (const auto &d : m_retryQueue) totalPending += d.size();
    for (auto it = m_pendingMap.begin(); it != m_pendingMap.end(); ++it) {
        totalPending += it.value().data.size();
    }
    // 如果当前处于告警状态，且积压低于低水位，清除告警
    if (m_isBufferWarning && totalPending < m_lowWaterMark) {
        m_isBufferWarning = false;
        emit bufferWarning(false);
        qDebug() << "[Buffer] Recovery: Low water mark reached. Pending:" << totalPending;
    }

    m_isSending = false;
}

// --- 监控定时器（每秒统计速率和水位）---
void RadioUdpManager::onMonitorTimer()
{
    // 发出当前速率（上一秒的发送字节数）
    emit speedUpdated(m_bytesSentInCurrentSecond);
    // 重置计数器
    m_bytesSentInCurrentSecond = 0;
}

// --- 超时重传检测定时器（每200ms扫描一次）---
void RadioUdpManager::onTimeoutTimer()
{
    QMutexLocker locker(&m_mutex);
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QList<quint16> timeoutKeys;

    // 遍历飞行队列，检查超时
    for (auto it = m_pendingMap.begin(); it != m_pendingMap.end(); ++it) {
        PacketMeta &meta = it.value();
        qint64 elapsed = now - meta.sendTimeMs;
        if (elapsed > RTO_TIMEOUT_MS) {
            // 判定为丢失
            if (meta.retryCount < MAX_RETRY_COUNT) {
                // 未达最大重传次数：移入重传队列（高优先级）
                meta.retryCount++;
                // 注意：这里将数据拷贝一份放入重传队列
                m_retryQueue.enqueue(meta.data);
                //                emit packetRetrying(meta.seqNum, meta.retryCount);
                qDebug() << "[Retry] Seq" << meta.seqNum << "timeout, retry" << meta.retryCount;

                // 更新时间戳，防止刚放入重传队列又被本定时器立即移出
                // （注意：下一次发送定时器发出时会更新时间戳，但为了防止在发送前再次触发超时，我们可以先更新）
                meta.sendTimeMs = now;
                // 但这里有个风险：如果重传队列很长，重传包没来得及发，会再次超时。
                // 因此我们最好在发送定时器发出重传包时更新，而不是这里。我们仅在放入队列时更新一次。
            } else {
                // 超过最大重传次数，彻底丢弃，并从pendingMap移除
                qDebug() << "[Drop] Seq" << meta.seqNum << "exceeded max retries, dropped.";
                timeoutKeys.append(it.key());
            }
        }
    }
    // 移除需要丢弃的报文
    for (quint16 key : timeoutKeys) {
        m_pendingMap.remove(key);
    }
}

void RadioUdpManager::onReadyRead()
{
    // 循环读取所有待处理的数据报（防止UDP粘包，每个包独立读）
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress senderAddr;
        quint16 senderPort;

        // 读取数据报（senderAddr/senderPort 可用来校验是否来自合法的电台IP，可选）
        qint64 readSize = m_udpSocket->readDatagram(datagram.data(), datagram.size(), &senderAddr, &senderPort);
        if (readSize == -1) {
            qWarning() << "[AckReceiver] Read error:" << m_udpSocket->errorString();
            continue;
        }

        // 假设1：前2个字节是小端序（网络序）的 quint16
        if (datagram.size() < 2) {
            qWarning() << "[AckReceiver] Packet too small (<2 bytes), ignore.";
            continue;
        }
        //收到的是ack反馈
        else if(datagram.size() == 2) {
            // 小端序转换：将 char[1] 作为高位，char[0] 作为低位
            quint16 seqNum = (static_cast<quint16>(static_cast<unsigned char>(datagram[1])) << 8)
                    | static_cast<quint16>(static_cast<unsigned char>(datagram[0]));
            // 回调给管理器，触发ACK确认
            processAck(seqNum);
        }
        else if(datagram.size()>2){
            //发送ack反馈数据
            qint64 sent = m_udpSocket->writeDatagram(datagram.mid(0,2), m_targetAddr, m_targetPort);
            if (sent == -1) {
                // 发送ack失败
                qWarning() << "发送ack失败";
                continue;
            }
            //抛出期望数据
            emit getMessage(datagram.mid(2,datagram.size()-2));
            // 可选：校验发送方IP，防止恶意包或串台
            // if (senderAddr != expectedRadioIp) { continue; }
        }
    }
}

// --- 辅助：生成序列号（0-65535循环）---
quint16 RadioUdpManager::getNextSeqNum()
{
    quint16 seq = m_currentSeqNum;
    m_currentSeqNum++;
    return seq;
}

// --- 此函数在类中未使用，移除了冗余实现，保留给外部调用 ---
// bool RadioUdpManager::sendPacketFromQueue(QByteArray &packet, bool isRetry, quint16 seqNum)
// {
//     // 实现已整合进 onSendTimer，此处留空
//     return true;
// }
