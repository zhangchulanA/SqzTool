#ifndef RADIOUDPMANAGER_H
#define RADIOUDPMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QQueue>
#include <QMap>
#include <QTimer>
#include <QMutex>
#include <QHostAddress>
#include <QDateTime>

/**
 * @brief 电台UDP可靠发送管理器（支持限流、水位监控、重传）
 *
 * 【核心功能】
 * 1. 流速管控：基于“每秒最大字节数”和“时间片配额”，精准控制UDP发出速率，防止电台缓冲溢出。
 * 2. 缓冲区监控：实时统计待发送队列（新数据+重传数据）总字节数，触发高/低水位报警。
 * 3. 可靠重传：监测已发报文的ACK超时，优先重传丢失报文，避免高时延下重复重传。
 *
 * 【使用流程】
 * 1. 实例化并调用 init(ip, port, maxBytesPerSecond) 初始化。
 * 2. 连接信号：bufferWarning(bool) 接收水位预警，speedUpdated(int) 查看实时速率。
 * 3. 业务层调用 sendData(data) 将数据入队。
 * 4. 当收到电台回传的ACK时，调用 processAck(seqNum) 确认报文。
 *
 * 【注意】所有操作需在同一个线程（如主线程）中调用，内部已做互斥保护。
 */
class RadioUdpManager : public QObject
{
    Q_OBJECT
public:
    explicit RadioUdpManager(QObject *parent = nullptr);
    ~RadioUdpManager();

    /**
     * @brief init
     * @param localAddr 绑定本地地址
     * @param localPort 绑定本地端口
     * @param targetAddr 目标地址
     * @param targetPort 目标端口
     * @param maxBytesPerSecond 每秒最大发送字节数（如 5120 代表 5KB/s）
     */
    void init(const QHostAddress &localAddr, quint16 localPort,
              const QHostAddress &targetAddr, quint16 targetPort, qint64 maxBytesPerSecond);

    // 业务层调用：发送数据（线程安全）
    void sendData(const QByteArray &data);

    // 接收ACK：由外部UDP接收线程/槽调用，传入序列号
    void processAck(quint16 seqNum);

    // 动态调整限流速率（KB/s）
    void setRateLimit(qint64 bytesPerSecond);

    // 设置水位报警阈值（字节）
    void setWaterMarks(qint64 highWater, qint64 lowWater);

signals:
    // 缓冲区预警：true=超过高水位，false=恢复至低水位以下
    void bufferWarning(bool isOverHigh);
    // 实时发送速率（字节/秒），用于UI展示
    void speedUpdated(qint64 bytesPerSecond);
    // 报文确认信号（用于外部日志）
    void packetAcked(quint16 seqNum);
    // 报文丢失/重传信号（用于外部日志）
    void packetRetrying(quint16 seqNum, int retryCount);
    //获取真正想要的数据，不包括ack
    void getMessage(const QByteArray& data);

private slots:
    // 核心发送定时器：负责根据配额取包发送（限流 + 优先级重传）
    void onSendTimer();
    // 监控定时器：计算实时速率，检查水位
    void onMonitorTimer();
    // 超时重传定时器：扫描飞行队列，判定超时
    void onTimeoutTimer();
    //接收数据
     void onReadyRead();

private:
    // 报文元数据结构
    struct PacketMeta {
        QByteArray data;          // 原始数据
        quint16 seqNum;           // 序列号
        qint64 sendTimeMs;        // 最后一次发送的时间戳(ms)
        int retryCount;           // 已重传次数
    };

    // 内部发送实现（取出队头数据，放入飞行队列，执行write）
    bool sendPacketFromQueue(QByteArray &packet, bool isRetry, quint16 seqNum = 0);

    // 生成下一个序列号
    quint16 getNextSeqNum();

private:
    // --- 网络组件 ---
    QUdpSocket *m_udpSocket;
    QHostAddress m_targetAddr;
    quint16 m_targetPort;

    // --- 限流参数 ---
    qint64 m_maxBytesPerSecond;   // 每秒最大字节数
    qint64 m_timeSliceMs;         // 时间片大小(毫秒)，固定20ms
    bool m_isSending;             // 防重入标志

    // --- 核心缓冲区（双队列）---
    QQueue<QByteArray> m_normalQueue;   // 普通新数据队列
    QQueue<QByteArray> m_retryQueue;    // 高优先级重传队列
    QMutex m_mutex;                     // 保护队列和Map

    // --- 飞行中报文（待确认）---
    QMap<quint16, PacketMeta> m_pendingMap;

    // --- 序列号生成 ---
    quint16 m_currentSeqNum;

    // --- 缓冲区监控 ---
    qint64 m_highWaterMark;       // 高水位（字节）
    qint64 m_lowWaterMark;        // 低水位（字节）
    bool m_isBufferWarning;       // 当前是否处于告警状态

    // --- 速率统计 ---
    qint64 m_bytesSentInCurrentSecond;
    qint64 m_lastSpeedEmitTime;   // 上次结算时间（用于计算实际速率）

    // --- 定时器 ---
    QTimer *m_sendTimer;          // 发送驱动定时器 (20ms)
    QTimer *m_monitorTimer;       // 监控定时器 (1s)
    QTimer *m_timeoutTimer;       // 超时重传检测定时器 (200ms)

    // --- 重传配置 ---
    static const int MAX_RETRY_COUNT = 3;       // 最大重传次数
    static const int RTO_TIMEOUT_MS = 800;      // 重传超时时间(毫秒)，可依据RTT动态调整
};

#endif // RADIOUDPMANAGER_H



#if A
/**
 * @brief UDP ACK 接收解析器
 * 监听指定端口，从收到的UDP包中提取序列号(SeqNum)，并通知RadioUdpManager
 */
class AckReceiver : public QObject
{
    Q_OBJECT
public:
    /**
     * @param manager 关联的发送管理器（用于调用 processAck）
     * @param listenPort 监听的本地端口（必须与 manager->bindLocalPort 绑定的端口一致）
     * @param parent 父对象
     */
    AckReceiver(RadioUdpManager *manager, quint16 listenPort, QObject *parent = nullptr)
        : QObject(parent), m_manager(manager)
    {
        m_socket = new QUdpSocket(this);

        // 绑定端口，允许广播和地址复用（实际电台通常不需要，但加上无妨）
        if (!m_socket->bind(QHostAddress::Any, listenPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
            qWarning() << "[AckReceiver] Failed to bind port" << listenPort << m_socket->errorString();
            return;
        }

        // 连接 readyRead 信号，有数据到达时触发
        connect(m_socket, &QUdpSocket::readyRead, this, &AckReceiver::onReadyRead);
        qDebug() << "[AckReceiver] Listening on port" << listenPort;
    }

private slots:
    void onReadyRead()
    {
        // 循环读取所有待处理的数据报（防止UDP粘包，每个包独立读）
        while (m_socket->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(m_socket->pendingDatagramSize());
            QHostAddress senderAddr;
            quint16 senderPort;

            // 读取数据报（senderAddr/senderPort 可用来校验是否来自合法的电台IP，可选）
            qint64 readSize = m_socket->readDatagram(datagram.data(), datagram.size(), &senderAddr, &senderPort);
            if (readSize == -1) {
                qWarning() << "[AckReceiver] Read error:" << m_socket->errorString();
                continue;
            }

            // ---- 【核心】解析序列号 ----
            // 假设1：前2个字节是大端序（网络序）的 quint16
            // 如果你的协议不同，请在此处修改解析逻辑
            if (datagram.size() < 2) {
                qWarning() << "[AckReceiver] Packet too small (<2 bytes), ignore.";
                continue;
            }

            // 小端序转换：将 char[1] 作为高位，char[0] 作为低位
            quint16 seqNum = (static_cast<quint16>(static_cast<unsigned char>(datagram[1])) << 8)
                           | static_cast<quint16>(static_cast<unsigned char>(datagram[0]));

            // 可选：校验发送方IP，防止恶意包或串台（建议开启）
//             if (senderAddr != expectedRadioIp) { continue; }


            // 回调给管理器，触发ACK确认
            m_manager->processAck(seqNum);

            // 可在此增加日志（注意频率）
            // qDebug() << "[AckReceiver] ACK received for Seq" << seqNum << "from" << senderAddr.toString() << senderPort;
        }
    }

private:
    QUdpSocket *m_socket;
    RadioUdpManager *m_manager;
};



// ====== 1. 创建发送管理器 ======
 RadioUdpManager manager;

 // 配置参数
 const QString RADIO_IP = "192.168.1.100"; // 改成你的电台IP，本机测试用 "127.0.0.1"
 const quint16 RADIO_PORT = 8888;           // 电台接收数据的端口
 const quint16 LOCAL_PORT = 9999;           // 本地绑定的端口（电台向此端口回复ACK）
 const qint64 MAX_RATE = 2048;              // 2KB/s 限流

 // 初始化发送器
 manager.init(QHostAddress(RADIO_IP), RADIO_PORT, MAX_RATE);

 // 绑定本地端口（必须和AckReceiver监听的端口一致）
 if (!manager.bindLocalPort(LOCAL_PORT)) {
     qCritical() << "Bind local port failed, exiting.";
     return -1;
 }

 // ====== 2. 创建ACK接收解析器（监听同一个端口） ======
 AckReceiver ackReceiver(&manager, LOCAL_PORT);

 // ====== 3. （可选）模拟电台回显ACK - 仅用于本机自测 ======
 // 真实电台会自己回ACK，以下代码仅在你的电脑上模拟电台行为
 // 注意：如果真实电台存在，请注释掉下面的模拟代码！
 QUdpSocket *mockRadio = new QUdpSocket(&a);
 // 绑定电台的发送端口（即RADIO_PORT），模拟电台收到数据后发ACK
 mockRadio->bind(RADIO_PORT);
 QObject::connect(mockRadio, &QUdpSocket::readyRead, [mockRadio, &manager]() {
     while (mockRadio->hasPendingDatagrams()) {
         QByteArray data;
         data.resize(mockRadio->pendingDatagramSize());
         QHostAddress sender;
         quint16 port;
         mockRadio->readDatagram(data.data(), data.size(), &sender, &port);

         if (data.size() >= 2) {
             // 模拟电台：直接提取收到的序列号，原封不动塞入ACK包前2字节，回传给发送方
             // 注意：我们假设发送方就是数据包的来源（即LOCAL_PORT）
             quint16 seq = (static_cast<quint16>(static_cast<unsigned char>(data[0])) << 8)
                         | static_cast<quint16>(static_cast<unsigned char>(data[1]));
             QByteArray ackData;
             ackData.resize(2);
             ackData[0] = (seq >> 8) & 0xFF;
             ackData[1] = seq & 0xFF;
             // 回传给发送者（即manager绑定的LOCAL_PORT）
             mockRadio->writeDatagram(ackData, sender, LOCAL_PORT);
             qDebug() << "[MockRadio] Echo ACK for Seq" << seq;
         }
     }
 });
 qDebug() << "[MockRadio] Started simulation (will echo ACK back).";

 // ====== 4. 连接业务信号 ======
 QObject::connect(&manager, &RadioUdpManager::bufferWarning,
                  [](bool high) { qDebug() << "[App] Buffer Warning:" << (high ? "HIGH" : "LOW"); });
 QObject::connect(&manager, &RadioUdpManager::speedUpdated,
                  [](qint64 speed) { qDebug() << "[App] Speed:" << speed << "B/s"; });
 QObject::connect(&manager, &RadioUdpManager::packetRetrying,
                  [](quint16 seq, int cnt) { qDebug() << "[App] Retry Seq" << seq << "count" << cnt; });

 // ====== 5. 业务层发送数据（每5ms发一个500B包） ======
 QTimer producer;
 int pktId = 0;
 QObject::connect(&producer, &QTimer::timeout, [&]() {
     QByteArray data(500, 'X');
     manager.sendData(data);
     pktId++;
     if (pktId % 100 == 0) qDebug() << "[App] Produced" << pktId << "packets";
 });
 producer.start(5);

 // ====== 6. 运行15秒后退出 ======
 QTimer::singleShot(15000, [&]() {
     producer.stop();
     mockRadio->close();
     qDebug() << "[App] Test finished.";
     a.quit();
 });

#endif
