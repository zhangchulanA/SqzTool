#ifndef SQZUDPTRANSFER_H
#define SQZUDPTRANSFER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QHash>
#include <QTimer>
#include <QFile>
#include <QDateTime>
#include <QSet>
#include <QMap>
#include <QSharedPointer>
#include <QEventLoop>
#include <QtConcurrent/QtConcurrent>
#include <QMutex>

#pragma pack(push, 1)
struct SqzPacketHeader {
    quint8 type;            // 0=数据分片，1=累积ACK，2=ClientHello，3=ClientIdAssign
    quint32 clientId;
    quint32 dataId;
    quint16 totalFrags;
    quint16 fragIndex;
    quint32 dataLen;
    quint16 window;         // 接收端可用窗口大小（暂未使用，预留）
    quint16 crc16;
};
#pragma pack(pop)

class SqzUdpTransfer : public QObject
{
    Q_OBJECT

public:
    explicit SqzUdpTransfer(QObject *parent = nullptr);
    ~SqzUdpTransfer();

    // 基础配置
    bool bind(quint16 port);
    void setRemoteAddress(const QHostAddress &addr, quint16 port);
    void setMaxRetries(int retries);
    void setTimeout(int ms);
    void setMaxPacketSize(int bytes);
    void setReceiveBufferSize(int bytes);
    void setEnableCumulativeAck(bool enable);
    void setAckDelayMs(int ms);
    void setServerMode(bool isServer);
    void setFileBlockSize(qint64 bytes);   // 建议 256KB~1MB

    bool sendData(const QByteArray &data, bool reliable = true);
    bool sendData(const QByteArray &data, const QHostAddress &destAddr,
                  quint16 destPort, bool reliable = true);
    bool sendDataTo(quint32 clientId, const QByteArray &data, bool reliable = true);

    bool sendLargeFile(const QString &filePath, bool reliable = true);
    bool sendLargeFile(const QString &filePath, const QHostAddress &destAddr,
                       quint16 destPort, bool reliable = true);
    bool sendLargeFileTo(quint32 clientId, const QString &filePath, bool reliable = true);

    bool registerClient(const QHostAddress &serverAddr, quint16 serverPort);

signals:
    void dataReceived(quint32 clientId, const QByteArray &data,
                      const QHostAddress &sender, quint16 port);
    void fileProgress(quint32 clientId, qint64 current, qint64 total);
    void fileReceived(quint32 clientId, const QString &savedPath,
                      const QString &originalName, qint64 size);
    void fileSent(quint32 clientId, const QString &filePath, bool success);
    void clientRegistered(quint32 clientId);
    void errorOccurred(const QString &errorMsg);

private slots:
    void onReadyRead();
    void onTimeoutCheck();
    void onFlushAck();

private:
    // ---------- 内部数据结构 ----------
    struct SendTask {
        quint32 clientId;
        quint32 dataId;
        QHostAddress destAddr;
        quint16 destPort;
        QByteArray fullData;
        QList<QByteArray> frags;
        QVector<bool> ackFlags;
        int retries = 0;
        qint64 lastSendTime = 0;
        int currentTimeout = 500;
        bool completed = false;
    };

    struct ReceiveTask {
        quint32 clientId;
        quint32 dataId;
        QHostAddress senderAddr;
        quint16 senderPort;
        int totalFrags = 0;
        quint32 totalLen = 0;
        QHash<int, QByteArray> frags;
        qint64 lastUpdateTime = 0;
        int lastContinuousAck = -1;
    };

    struct FileSession {
        quint32 clientId;
        quint32 sessionId;
        QString fileName;
        QString savePath;
        qint64 totalSize;
        int totalBlocks;
        qint64 blockSize;
        QFile outputFile;
        QSet<int> receivedBlocks;         // 已收到的块索引
        QMap<int, QByteArray> pendingBlocks; // 乱序块缓存（已按顺序写入后会移出）
        qint64 nextExpectedBlock = 0;
        bool completed = false;
        QMutex mutex;                     // 保护文件写入
    };

    struct BlockWaitContext {
        QEventLoop *loop = nullptr;
        bool acked = false;
    };

    struct PendingAck {
        quint32 clientId;
        quint32 dataId;
        int lastContinuousFrag;
        qint64 scheduledTime;
    };

    // 私有方法
    void initSocket();
    bool sendDataFragment(quint32 clientId, quint32 dataId, int fragIndex,
                          const QByteArray &fragData, const QHostAddress &addr,
                          quint16 port, int totalFrags, quint32 totalLen);
    void sendCumulativeAck(quint32 clientId, quint32 dataId, int lastContFrag,
                           const QHostAddress &addr, quint16 port, quint16 window);
    quint16 calculateCrc16(const QByteArray &data);
    quint32 generateClientId(const QHostAddress &addr, quint16 port);

    bool sendReliableDataAndWait(quint32 clientId, const QByteArray &data, int timeoutMs);

    void handleDataPacket(const SqzPacketHeader &header, const QByteArray &payload,
                          const QHostAddress &sender, quint16 port);
    void handleAckPacket(const SqzPacketHeader &header, const QByteArray &payload);
    void handleClientHello(const SqzPacketHeader &header, const QByteArray &payload,
                           const QHostAddress &sender, quint16 port);
    void handleClientIdAssign(const SqzPacketHeader &header, const QByteArray &payload);

    void checkSendTasksTimeout();
    void retransmitFragments(SendTask &task, const QVector<int> &missingIndices);
    void completeSendTask(quint32 clientId, quint32 dataId);
    void tryAssembleData(quint32 clientId, quint32 dataId);
    void handleFileBegin(quint32 clientId, const QByteArray &data);
    void handleFileData(quint32 clientId, const QByteArray &data);
    void flushFileSession(FileSession &session);  // 仅用于顺序写入的同步部分
    void asyncWriteBlock(quint64 sessionKey, int blockIndex, const QByteArray &blockData); // 异步写入

    // 成员变量
    QUdpSocket *m_socket = nullptr;
    QHostAddress m_defaultAddr;
    quint16 m_defaultPort = 0;

    int m_maxRetries = 5;
    int m_baseTimeoutMs = 500;
    int m_maxPacketSize = 1400;
    int m_recvBufferSize = 64 * 1024 * 1024;
    bool m_enableCumulativeAck = true;
    int m_ackDelayMs = 10;
    bool m_isServerMode = false;
    qint64 m_fileBlockSize = 512 * 1024;   // 默认 512KB（稳定且较快）

    QHash<quint32, QHostAddress> m_clientAddrMap;
    QHash<QString, quint32> m_addrToClientId;

    QHash<quint64, SendTask> m_sendTasks;         // key = (clientId<<32)|dataId
    QHash<quint64, ReceiveTask> m_recvTasks;
    QHash<quint64, QSharedPointer<FileSession>> m_fileSessions; // key = (clientId<<32)|sessionId
    QHash<quint64, QList<QByteArray>> m_orphanBlocks;          // 孤儿块缓存

    QHash<quint32, BlockWaitContext> m_blockWaitMap; // 同步等待确认
    QHash<quint64, PendingAck> m_pendingAcks;

    QTimer *m_ackFlushTimer = nullptr;
    QTimer *m_timeoutTimer = nullptr;
};

#endif // SQZUDPTRANSFER_H
