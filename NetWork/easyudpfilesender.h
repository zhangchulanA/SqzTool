#ifndef EASYUDPSOCKET_H
#define EASYUDPSOCKET_H

#include <QObject>
#include <QUdpSocket>
#include <QMap>
#include <QFile>

class EasyUdpSocket : public QObject
{
    Q_OBJECT
public:
    explicit EasyUdpSocket(QObject *parent = nullptr);
    ~EasyUdpSocket();

    // 绑定本地端口
    bool bind(quint16 port, QUdpSocket::BindMode mode = QUdpSocket::DefaultForPlatform);
    // 发送普通数据报（线程安全）
    void sendDatagram(const QByteArray &data, const QHostAddress &addr, quint16 port);
    // 发送小文件（自动分片）
    void sendFile(const QString &filePath, const QHostAddress &addr, quint16 port);
    // 取消当前文件发送（如果有正在发送的）
    void cancelFileSend();

signals:
    // 收到普通数据报（主线程）
    void datagramReceived(const QByteArray &data, const QHostAddress &senderAddr, quint16 senderPort);
    // 收到完整文件（保存在临时目录，发射文件路径）
    void fileReceived(const QString &filePath);
    // 文件发送进度
    void fileSendProgress(qint64 sentBytes, qint64 totalBytes);
    // 文件发送完成
    void fileSendFinished(bool success, const QString &errorMsg);

private slots:
    void onReadyRead();

private:
    void processPendingDatagrams();
    void sendNextChunk();  // 发送文件的下一个分片

    QUdpSocket m_socket;

    // 文件发送状态
    QFile *m_sendFile = nullptr;
    qint64 m_sendFileTotal = 0;
    qint64 m_sendFileSent = 0;
    QHostAddress m_sendTargetAddr;
    quint16 m_sendTargetPort = 0;
    bool m_cancelSend = false;
    static constexpr qint64  MAX_PACKET_SIZE = 1400; // 安全UDP负载大小

    // 文件接收状态
    struct ReceiveContext {
        int totalPackets = 0;
        QMap<int, QByteArray> chunks;
        int receivedCount = 0;
        QString fileName;
    };
    QMap<QString, ReceiveContext> m_receiveMap; // 用发送方地址+端口标识一个文件传输会话
};

#endif // EASYUDPSOCKET_H
