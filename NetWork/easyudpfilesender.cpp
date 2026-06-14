#include "easyudpfilesender.h"
#include <QFile>
#include <QDataStream>
#include <QDir>
#include <QUdpSocket>
#include <QFileInfo>
#include <QTimer>
#include <QNetworkDatagram>
#include <QtMath>
EasyUdpSocket::EasyUdpSocket(QObject *parent) : QObject(parent)
{
    connect(&m_socket, &QUdpSocket::readyRead, this, &EasyUdpSocket::onReadyRead);
}

EasyUdpSocket::~EasyUdpSocket()
{
    if (m_sendFile) {
        delete m_sendFile;
        m_sendFile = nullptr;
    }
}

bool EasyUdpSocket::bind(quint16 port, QUdpSocket::BindMode mode)
{
    return m_socket.bind(port, mode);
}

void EasyUdpSocket::sendDatagram(const QByteArray &data, const QHostAddress &addr, quint16 port)
{
    // 普通数据报：第一个字节为 0x00 表示普通消息
    QByteArray packet;
    packet.append(char(0x00));
    packet.append(data);
    m_socket.writeDatagram(packet, addr, port);
}

void EasyUdpSocket::sendFile(const QString &filePath, const QHostAddress &addr, quint16 port)
{
    if (m_sendFile) {
        emit fileSendFinished(false, tr("Already sending a file"));
        return;
    }
    m_cancelSend = false;
    m_sendFile = new QFile(filePath);
    if (!m_sendFile->open(QIODevice::ReadOnly)) {
        emit fileSendFinished(false, m_sendFile->errorString());
        delete m_sendFile;
        m_sendFile = nullptr;
        return;
    }
    m_sendFileTotal = m_sendFile->size();
    m_sendFileSent = 0;
    m_sendTargetAddr = addr;
    m_sendTargetPort = port;

    // 发送元数据包（类型 0x01，文件名+总大小+分片大小）
    QByteArray meta;
    QDataStream ds(&meta, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << quint8(0x01) << QFileInfo(filePath).fileName() << m_sendFileTotal << quint16(MAX_PACKET_SIZE);
    m_socket.writeDatagram(meta, addr, port);

    // 开始发送第一个分片
    sendNextChunk();
}

void EasyUdpSocket::cancelFileSend()
{
    m_cancelSend = true;
}

void EasyUdpSocket::onReadyRead()
{
    processPendingDatagrams();
}

void EasyUdpSocket::processPendingDatagrams()
{
    while (m_socket.hasPendingDatagrams()) {
        QNetworkDatagram dg = m_socket.receiveDatagram();
        QByteArray data = dg.data();
        if (data.isEmpty()) continue;

        quint8 type = static_cast<quint8>(data[0]);
        QByteArray payload = data.mid(1);

        if (type == 0x00) {
            // 普通数据报
            emit datagramReceived(payload, dg.senderAddress(), dg.senderPort());
        } else if (type == 0x01) {
            // 文件元数据
            QDataStream ds(payload);
            ds.setByteOrder(QDataStream::BigEndian);
            QString fileName;
            qint64 totalBytes;
            quint16 chunkSize;
            ds >> fileName >> totalBytes >> chunkSize;
            QString sessionKey = QString("%1:%2").arg(dg.senderAddress().toString()).arg(dg.senderPort());
            ReceiveContext &ctx = m_receiveMap[sessionKey];
            ctx.totalPackets = (totalBytes + chunkSize - 1) / chunkSize;
            ctx.receivedCount = 0;
            ctx.chunks.clear();
            ctx.fileName = fileName;
        } else if (type == 0x02) {
            // 文件数据分片
            QDataStream ds(payload);
            ds.setByteOrder(QDataStream::BigEndian);
            quint16 index;
            QByteArray chunkData;
            ds >> index >> chunkData;
            QString sessionKey = QString("%1:%2").arg(dg.senderAddress().toString()).arg(dg.senderPort());
            auto it = m_receiveMap.find(sessionKey);
            if (it != m_receiveMap.end()) {
                ReceiveContext &ctx = *it;
                if (!ctx.chunks.contains(index)) {
                    ctx.chunks[index] = chunkData;
                    ctx.receivedCount++;
                }
                if (ctx.receivedCount == ctx.totalPackets) {
                    // 重组文件
                    QByteArray fileData;
                    for (int i = 0; i < ctx.totalPackets; ++i) {
                        if (!ctx.chunks.contains(i)) {
                            // 丢包，放弃本次接收（简化处理）
                            m_receiveMap.erase(it);
                            return;
                        }
                        fileData.append(ctx.chunks[i]);
                    }
                    QString savePath = QDir::temp().absoluteFilePath(ctx.fileName);
                    QFile outFile(savePath);
                    if (outFile.open(QIODevice::WriteOnly)) {
                        outFile.write(fileData);
                        outFile.close();
                        emit fileReceived(savePath);
                    }
                    m_receiveMap.erase(it);
                }
            }
        }
    }
}

void EasyUdpSocket::sendNextChunk()
{
    if (m_cancelSend) {
        if (m_sendFile) {
            m_sendFile->close();
            delete m_sendFile;
            m_sendFile = nullptr;
        }
        emit fileSendFinished(false, tr("Cancelled"));
        return;
    }

    if (m_sendFileSent >= m_sendFileTotal) {
        m_sendFile->close();
        delete m_sendFile;
        m_sendFile = nullptr;
        emit fileSendFinished(true, QString());
        return;
    }

    qint64 toRead = qMin(MAX_PACKET_SIZE, m_sendFileTotal - m_sendFileSent);
    QByteArray chunk = m_sendFile->read(toRead);
    if (chunk.isEmpty()) {
        m_sendFile->close();
        delete m_sendFile;
        m_sendFile = nullptr;
        emit fileSendFinished(false, tr("File read error"));
        return;
    }

    // 构造分片包（类型 0x02 + 序号 + 数据）
    QByteArray packet;
    QDataStream ds(&packet, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << quint8(0x02) << quint16(m_sendFileSent / MAX_PACKET_SIZE) << chunk;
    m_socket.writeDatagram(packet, m_sendTargetAddr, m_sendTargetPort);

    m_sendFileSent += chunk.size();
    emit fileSendProgress(m_sendFileSent, m_sendFileTotal);

    // 继续发送下一个分片（使用定时器避免递归深度过大）
    QTimer::singleShot(0, this, &EasyUdpSocket::sendNextChunk);
}
