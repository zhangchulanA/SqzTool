#ifndef RADIOUDPMANAGER_H
#define RADIOUDPMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QQueue>
#include <QMap>
#include <QTimer>
#include <QMutex>
#include <QHostAddress>
#include <QElapsedTimer>
#include <QSharedPointer>

/**
 * @brief 电台UDP可靠发送管理器（优化版）
 *
 * 核心特性：
 * - 动态发送间隔：根据包大小和速率自动计算，实现平滑限流
 * - 可靠传输：32位序列号 + 动态RTO + 有限重传
 * - 高性能：零拷贝入队，增量积压计数器
 * - 自适应：RTT估算，窗口流量控制
 */
class RadioLink : public QObject
{
    Q_OBJECT
public:
    explicit RadioLink(QObject *parent = nullptr);
    ~RadioLink();

    /**
     * @brief 初始化
     * @param localAddr 本地绑定地址
     * @param localPort 本地绑定端口
     * @param targetAddr 目标地址
     * @param targetPort 目标端口
     * @param maxBytesPerSecond 每秒最大发送字节数（如 5120 = 5KB/s）
     */
    void init(const QHostAddress &localAddr, quint16 localPort,
              const QHostAddress &targetAddr, quint16 targetPort,
              qint64 maxBytesPerSecond);

    // 业务层发送数据（线程安全）
    void sendData(const QByteArray &data);

    // 外部收到ACK时调用（线程安全）
    void processAck(quint16 seqNum);

    // 动态调整速率（字节/秒）
    void setRateLimit(qint64 bytesPerSecond);

    // 设置水位阈值（字节）
    void setWaterMarks(qint64 highWater, qint64 lowWater);

signals:
    void bufferWarning(bool isOverHigh, qint64 pendingBytes);  //缓冲区告警 附带当前积压
    void speedUpdated(qint64 bytesPerSecond);// 实时速率
    void packetAcked(quint16 seqNum); //当某个包成功收到对方的ACK确认 触发
    void packetRetrying(quint16 seqNum, int retryCount);//当某个包超时重传时触发 用于日志
    void getMessage(const QByteArray &data);//接收电台发来的数据
    void socketError(const QString &error);   // 底层socket错误

private slots:
    void onSendTimer();          // 单次发送定时器
    void onMonitorTimer();       // 每秒监控
    void onTimeoutTimer();       // 超时重传扫描（200ms）
    void onReadyRead();          // 接收数据
    void onSocketError(QAbstractSocket::SocketError error);

private:
    // 报文元数据
    struct PacketMeta {
        QByteArray data;
        quint32 seqNum;          // 内部完整32位序列号
        qint64 sendTimeMs;
        int retryCount;
    };

    // 发送一个包（从队列取出并发送），返回是否成功发送
    bool sendOnePacket();

    // 生成下一个32位序列号
    quint32 getNextSeqNum();

    // 更新RTO估算
    void updateRtt(qint64 rttMs);

private:
    // --- 网络 ---
    QUdpSocket *m_udpSocket;
    QHostAddress m_targetAddr;
    quint16 m_targetPort;

    // --- 限流参数 ---
    qint64 m_maxBytesPerSecond;    // 字节/秒
    qint64 m_nextSendTimeMs;       // 下次允许发送的时间戳(ms)
    QElapsedTimer m_clock;         // 高精度计时器

    // --- 队列与缓冲区 ---
    QQueue<QByteArray> m_normalQueue;   // 新数据
    QQueue<QByteArray> m_retryQueue;    // 重传（高优先级）
    QMap<quint16, PacketMeta> m_pendingMap; // 键为低16位序列号
    qint64 m_totalPendingBytes;      // 总积压（增量计数）
    QMutex m_mutex;

    // --- 序列号 ---
    quint32 m_currentSeqNum;         // 32位循环递增

    // --- 飞行窗口 ---
    static const int MAX_FLIGHT_WINDOW = 1000;  // 最大飞行包数

    // --- 重传参数 ---
    qint64 m_rtt;       // 平滑RTT (ms)
    qint64 m_rto;       // 当前RTO (ms)
    static const int MAX_RETRY_COUNT = 3;
    static const int MIN_RTO = 200;     // 最小RTO
    static const int MAX_RTO = 3000;

    // --- 水位监控 ---
    qint64 m_highWaterMark;
    qint64 m_lowWaterMark;
    bool m_isBufferWarning;

    // --- 速率统计 ---
    qint64 m_bytesSentInCurrentSecond;
    qint64 m_lastSpeedEmitTime;

    // --- 定时器 ---
    QTimer *m_sendTimer;      // 单次触发
    QTimer *m_monitorTimer;   // 1s
    QTimer *m_timeoutTimer;   // 200ms

    // --- 标志 ---
    bool m_isSending;         // 防重入
};

#endif // RADIOUDPMANAGER_H
