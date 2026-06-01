#include "SqzUdpTransfer.h"
#include <QRandomGenerator>
#include <QDataStream>
#include <QFileInfo>
#include <QDir>
#include <QThread>
#include <QElapsedTimer>
#include <QDebug>
#include <utility>

// ======================= CRC16-CCITT 表 =======================
static const quint16 crc16_table[256] = {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50A5,0x60C6,0x70E7,
    0x8108,0x9129,0xA14A,0xB16B,0xC18C,0xD1AD,0xE1CE,0xF1EF,
    0x1231,0x0210,0x3273,0x2252,0x52B5,0x4294,0x72F7,0x62D6,
    0x9339,0x8318,0xB37B,0xA35A,0xD3BD,0xC39C,0xF3FF,0xE3DE,
    0x2462,0x3443,0x0420,0x1401,0x64E6,0x74C7,0x44A4,0x5485,
    0xA56A,0xB54B,0x8528,0x9509,0xE5EE,0xF5CF,0xC5AC,0xD58D,
    0x3653,0x2672,0x1611,0x0630,0x76D7,0x66F6,0x5695,0x46B4,
    0xB75B,0xA77A,0x9719,0x8738,0xF7DF,0xE7FE,0xD79D,0xC7BC,
    0x48C4,0x58E5,0x6886,0x78A7,0x0840,0x1861,0x2802,0x3823,
    0xC9CC,0xD9ED,0xE98E,0xF9AF,0x8948,0x9969,0xA90A,0xB92B,
    0x5AF5,0x4AD4,0x7AB7,0x6A96,0x1A71,0x0A50,0x3A33,0x2A12,
    0xDBFD,0xCBDC,0xFBBF,0xEBFE,0x9B79,0x8B58,0xBB3B,0xAB1A,
    0x6CA6,0x7C87,0x4CE4,0x5CC5,0x2C22,0x3C03,0x0C60,0x1C41,
    0xEDAE,0xFD8F,0xCDEC,0xDDCD,0xAD2A,0xBD0B,0x8D68,0x9D49,
    0x7E97,0x6EB6,0x5ED5,0x4EF4,0x3E13,0x2E32,0x1E51,0x0E70,
    0xFF9F,0xEFBE,0xDFDD,0xCFFC,0xBF1B,0xAF3A,0x9F59,0x8F78,
    0x9188,0x81A9,0xB1CA,0xA1EB,0xD10C,0xC12D,0xF14E,0xE16F,
    0x1080,0x00A1,0x30C2,0x20E3,0x5004,0x4025,0x7046,0x6067,
    0x83B9,0x9398,0xA3FB,0xB3DA,0xC33D,0xD31C,0xE37F,0xF35E,
    0x02B1,0x1290,0x22F3,0x32D2,0x4235,0x5214,0x6277,0x7256,
    0xB5EA,0xA5CB,0x95A8,0x8585,0xF56E,0xE54F,0xD52C,0xC50D,
    0x34E2,0x24C3,0x14A0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xA7DB,0xB7FA,0x8799,0x97B8,0xE75F,0xF77E,0xC71D,0xD73C,
    0x26D3,0x36F2,0x0691,0x16B0,0x6657,0x7676,0x4615,0x5634,
    0xD94C,0xC96D,0xF90E,0xE92F,0x99C8,0x89E9,0xB98A,0xA9AB,
    0x5844,0x4865,0x7806,0x6827,0x18C0,0x08E1,0x38B2,0x2893,
    0xCB7D,0xDB5C,0xEB3F,0xFB1E,0x8BF9,0x9BD8,0xABBB,0xBB9A,
    0x4A75,0x5A54,0x6A37,0x7A16,0x0AF1,0x1AD0,0x2AB3,0x3A92,
    0xFD2E,0xED0F,0xDD6C,0xCD4D,0xBDAA,0xAD8B,0x9DE8,0x8DC9,
    0x7C26,0x6C07,0x5C64,0x4C45,0x3CA2,0x2C83,0x1CE0,0x0CC1,
    0xEF1F,0xFF3E,0xCF5D,0xDF7C,0xAF9B,0xBFBA,0x8FD9,0x9FF8,
    0x6E17,0x7E36,0x4E55,0x5E74,0x2E93,0x3EB2,0x0ED1,0x1EF0
};

static quint16 crc16_update(quint16 crc, quint8 data) {
    return (crc << 8) ^ crc16_table[((crc >> 8) ^ data) & 0xFF];
}

// ======================= 构造/析构 =======================
SqzUdpTransfer::SqzUdpTransfer(QObject *parent) : QObject(parent) {
    initSocket();
    m_timeoutTimer = new QTimer(this);
    connect(m_timeoutTimer, &QTimer::timeout, this, &SqzUdpTransfer::onTimeoutCheck);
    m_timeoutTimer->start(100);
    m_ackFlushTimer = new QTimer(this);
    connect(m_ackFlushTimer, &QTimer::timeout, this, &SqzUdpTransfer::onFlushAck);
    m_ackFlushTimer->start(m_ackDelayMs);
}

SqzUdpTransfer::~SqzUdpTransfer() {
    if (m_socket) m_socket->close();
    delete m_socket;
}

void SqzUdpTransfer::initSocket() {
    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &SqzUdpTransfer::onReadyRead);
}

bool SqzUdpTransfer::bind(quint16 port) {
    if (!m_socket->bind(QHostAddress::Any, port)) {
        emit errorOccurred(tr("Bind failed: %1").arg(m_socket->errorString()));
        return false;
    }
    m_socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, m_recvBufferSize);
    return true;
}

void SqzUdpTransfer::setRemoteAddress(const QHostAddress &addr, quint16 port) {
    m_defaultAddr = addr;
    m_defaultPort = port;
}
void SqzUdpTransfer::setMaxRetries(int retries) { m_maxRetries = retries; }
void SqzUdpTransfer::setTimeout(int ms) { m_baseTimeoutMs = ms; }
void SqzUdpTransfer::setMaxPacketSize(int bytes) { if (bytes > 0) m_maxPacketSize = bytes; }
void SqzUdpTransfer::setReceiveBufferSize(int bytes) { m_recvBufferSize = bytes; if (m_socket && m_socket->isOpen()) m_socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, m_recvBufferSize); }
void SqzUdpTransfer::setEnableCumulativeAck(bool enable) { m_enableCumulativeAck = enable; }
void SqzUdpTransfer::setAckDelayMs(int ms) { m_ackDelayMs = ms; if (m_ackFlushTimer) m_ackFlushTimer->start(ms); }
void SqzUdpTransfer::setServerMode(bool isServer) { m_isServerMode = isServer; }

// ======================= 辅助函数 =======================
quint16 SqzUdpTransfer::calculateCrc16(const QByteArray &data) {
    quint16 crc = 0xFFFF;
    for (char c : data) crc = crc16_update(crc, static_cast<quint8>(c));
    return crc ^ 0xFFFF;
}

quint32 SqzUdpTransfer::generateClientId(const QHostAddress &addr, quint16 port) {
    return qHash(addr.toString() + ":" + QString::number(port)) & 0xFFFFFFFF;
}

bool SqzUdpTransfer::sendDataFragment(quint32 clientId, quint32 dataId, int fragIndex,
                                      const QByteArray &fragData, const QHostAddress &addr,
                                      quint16 port, int totalFrags, quint32 totalLen) {
    SqzPacketHeader header;
    header.type = 0;
    header.clientId = clientId;
    header.dataId = dataId;
    header.totalFrags = totalFrags;
    header.fragIndex = fragIndex;
    header.dataLen = totalLen;
    header.window = 0;
    header.crc16 = 0;

    QByteArray packet(reinterpret_cast<const char*>(&header), sizeof(SqzPacketHeader) - sizeof(quint16));
    packet.append(reinterpret_cast<const char*>(&header.crc16), sizeof(quint16));
    packet.append(fragData);
    quint16 crc = calculateCrc16(packet);
    header.crc16 = crc;
    memcpy(packet.data() + sizeof(SqzPacketHeader) - sizeof(quint16), &crc, sizeof(quint16));
    qint64 sent = m_socket->writeDatagram(packet, addr, port);
    return sent == packet.size();
}

void SqzUdpTransfer::sendCumulativeAck(quint32 clientId, quint32 dataId, int lastContFrag,
                                       const QHostAddress &addr, quint16 port, quint16 window) {
    SqzPacketHeader header;
    header.type = 1;
    header.clientId = clientId;
    header.dataId = dataId;
    header.totalFrags = 0;
    header.fragIndex = lastContFrag;
    header.dataLen = 0;
    header.window = window;
    header.crc16 = 0;
    QByteArray packet(reinterpret_cast<const char*>(&header), sizeof(SqzPacketHeader));
    quint16 crc = calculateCrc16(packet);
    header.crc16 = crc;
    memcpy(packet.data() + sizeof(SqzPacketHeader) - sizeof(quint16), &crc, sizeof(quint16));
    m_socket->writeDatagram(packet, addr, port);
}

// ======================= 普通数据发送 =======================
bool SqzUdpTransfer::sendData(const QByteArray &data, bool reliable) {
    if (m_defaultPort == 0) { emit errorOccurred(tr("No default remote address")); return false; }
    return sendData(data, m_defaultAddr, m_defaultPort, reliable);
}

bool SqzUdpTransfer::sendData(const QByteArray &data, const QHostAddress &destAddr, quint16 destPort, bool reliable) {
    quint32 clientId = generateClientId(destAddr, destPort);
    return sendDataTo(clientId, data, reliable);
}

bool SqzUdpTransfer::sendDataTo(quint32 clientId, const QByteArray &data, bool reliable) {
    if (data.isEmpty()) { emit errorOccurred(tr("Empty data")); return false; }
    QHostAddress destAddr = m_defaultAddr;
    quint16 destPort = m_defaultPort;
    if (m_clientAddrMap.contains(clientId)) {
        destAddr = m_clientAddrMap[clientId];
    }

    quint32 dataId = QRandomGenerator::global()->generate();
    int totalFrags = (data.size() + m_maxPacketSize - 1) / m_maxPacketSize;

    if (!reliable) {
        for (int i = 0; i < totalFrags; ++i) {
            QByteArray fragData = data.mid(i * m_maxPacketSize, m_maxPacketSize);
            sendDataFragment(clientId, dataId, i, fragData, destAddr, destPort, totalFrags, data.size());
        }
        return true;
    }

    SendTask task;
    task.clientId = clientId;
    task.dataId = dataId;
    task.destAddr = destAddr;
    task.destPort = destPort;
    task.fullData = data;
    task.ackFlags = QVector<bool>(totalFrags, false);
    task.retries = 0;
    task.completed = false;
    task.lastSendTime = QDateTime::currentMSecsSinceEpoch();
    task.currentTimeout = m_baseTimeoutMs;
    for (int i = 0; i < totalFrags; ++i)
        task.frags.append(data.mid(i * m_maxPacketSize, m_maxPacketSize));

    quint64 key = ((quint64)clientId << 32) | dataId;
    m_sendTasks.insert(key, task);
    for (int i = 0; i < totalFrags; ++i)
        sendDataFragment(clientId, dataId, i, task.frags[i], destAddr, destPort, totalFrags, data.size());
    return true;
}

// ======================= 同步可靠数据发送（带保守节流）=======================
bool SqzUdpTransfer::sendReliableDataAndWait(quint32 clientId, const QByteArray &data, int timeoutMs) {
    QHostAddress destAddr = m_defaultAddr;
    quint16 destPort = m_defaultPort;
    if (m_clientAddrMap.contains(clientId)) {
        destAddr = m_clientAddrMap[clientId];
    }
    if (destPort == 0) {
        emit errorOccurred(tr("No remote address set for clientId %1").arg(clientId));
        return false;
    }

    quint32 dataId = QRandomGenerator::global()->generate();
    int totalFrags = (data.size() + m_maxPacketSize - 1) / m_maxPacketSize;

//    qDebug() << "  >>> Sending dataId:" << dataId
//             << "size:" << data.size() << "bytes"
//             << "frags:" << totalFrags
//             << "timeout:" << timeoutMs << "ms";

    SendTask task;
    task.clientId = clientId;
    task.dataId = dataId;
    task.destAddr = destAddr;
    task.destPort = destPort;
    task.fullData = data;
    task.ackFlags = QVector<bool>(totalFrags, false);
    task.retries = 0;
    task.completed = false;
    task.lastSendTime = QDateTime::currentMSecsSinceEpoch();
    task.currentTimeout = m_baseTimeoutMs;
    for (int i = 0; i < totalFrags; ++i)
        task.frags.append(data.mid(i * m_maxPacketSize, m_maxPacketSize));

    quint64 key = ((quint64)clientId << 32) | dataId;
    m_sendTasks.insert(key, task);

    // 保守发送：每发 100 个分片，休眠 1 毫秒，防止瞬时拥塞
    const int BURST_LIMIT = 200;
    for (int i = 0; i < totalFrags; ++i) {
        sendDataFragment(clientId, dataId, i, task.frags[i], destAddr, destPort, totalFrags, data.size());
//        if (i % BURST_LIMIT == 0 && i > 0) {
//            QThread::msleep(1);
//        }
    }
//    qDebug() << "  <<< All fragments sent, waiting for ACK...";

    QEventLoop loop;
    BlockWaitContext ctx;
    ctx.loop = &loop;
    ctx.acked = false;
    m_blockWaitMap[dataId] = ctx;

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);

    loop.exec();

    bool success = (m_blockWaitMap.contains(dataId) && m_blockWaitMap[dataId].acked);
    m_blockWaitMap.remove(dataId);
    if (!success) {
        qDebug() << "  ❌ TIMEOUT for dataId:" << dataId;
        emit errorOccurred(tr("Reliable data send failed for dataId %1 (timeout)").arg(dataId));
        return false;
    }
//    qDebug() << "  ✅ ACK received for dataId:" << dataId;
    return true;
}

// ======================= 大文件发送（强制 256KB 块，保守策略）=======================
bool SqzUdpTransfer::sendLargeFile(const QString &filePath, bool reliable) {
    if (m_defaultPort == 0) { emit errorOccurred(tr("No default remote address")); return false; }
    return sendLargeFile(filePath, m_defaultAddr, m_defaultPort, reliable);
}

bool SqzUdpTransfer::sendLargeFile(const QString &filePath, const QHostAddress &destAddr, quint16 destPort, bool reliable) {
    quint32 clientId = generateClientId(destAddr, destPort);
    if (!m_clientAddrMap.contains(clientId)) {
        m_clientAddrMap[clientId] = destAddr;
    }
    return sendLargeFileTo(clientId, filePath, reliable);
}

bool SqzUdpTransfer::sendLargeFileTo(quint32 clientId, const QString &filePath, bool reliable) {
    QFile file(filePath);
       if (!file.open(QIODevice::ReadOnly)) {
           emit errorOccurred(tr("Cannot open file: %1").arg(file.errorString()));
           return false;
       }

       const qint64 BLOCK_SIZE = m_fileBlockSize;   // 默认 512KB
       qint64 totalSize = file.size();
       int totalBlocks = (totalSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
       quint32 sessionId = QRandomGenerator::global()->generate();

       qDebug() << "File transfer start, block size:" << BLOCK_SIZE << "total blocks:" << totalBlocks;

       QByteArray beginData;
       QDataStream ds(&beginData, QIODevice::WriteOnly);
       ds << QString("FILE_BEGIN") << sessionId << QFileInfo(filePath).fileName()
          << totalSize << totalBlocks << BLOCK_SIZE;

       if (!sendReliableDataAndWait(clientId, beginData, 5000)) {
           emit errorOccurred(tr("FILE_BEGIN not acknowledged"));
           return false;
       }

       qint64 totalSent = 0;
       QElapsedTimer globalTimer;
       globalTimer.start();

       for (int blockIdx = 0; blockIdx < totalBlocks; ++blockIdx) {
           qint64 offset = blockIdx * BLOCK_SIZE;
           qint64 blockLen = qMin(BLOCK_SIZE, totalSize - offset);
           file.seek(offset);
           QByteArray blockData = file.read(blockLen);
           if (blockData.size() != blockLen) {
               emit errorOccurred(tr("File read error at block %1").arg(blockIdx));
               return false;
           }
           QByteArray dataPacket;
           QDataStream pktStream(&dataPacket, QIODevice::WriteOnly);
           pktStream << QString("FILE_DATA") << sessionId << blockIdx << blockData;

           QElapsedTimer blockTimer;
           blockTimer.start();
           if (!sendReliableDataAndWait(clientId, dataPacket, 30000)) {
               emit errorOccurred(tr("Failed to send file block %1 (no ack)").arg(blockIdx));
               return false;
           }
           totalSent += blockLen;
           emit fileProgress(clientId, totalSent, totalSize);
           qDebug() << "Block" << blockIdx << "sent in" << blockTimer.elapsed() << "ms";
       }
       file.close();
       emit fileSent(clientId, filePath, true);
       qDebug() << "File transfer completed in" << globalTimer.elapsed() << "ms";
       return true;
}

// ======================= 接收处理 =======================
void SqzUdpTransfer::onReadyRead() {
    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        qint64 len = m_socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        if (len <= 0) continue;
        datagram.resize(len);
        if (datagram.size() < (int)sizeof(SqzPacketHeader)) continue;

        SqzPacketHeader header;
        memcpy(&header, datagram.constData(), sizeof(SqzPacketHeader));
        QByteArray checkData = datagram;
        quint16 recvCrc = header.crc16;
        header.crc16 = 0;
        memcpy(checkData.data() + sizeof(SqzPacketHeader) - sizeof(quint16), &header.crc16, sizeof(quint16));
        if (calculateCrc16(checkData) != recvCrc) {
            qDebug() << "Checksum error, dropping packet";
            continue;
        }

        QByteArray payload = datagram.mid(sizeof(SqzPacketHeader));
        switch (header.type) {
        case 0:
//            qDebug() << "RECV DATA: clientId=" << header.clientId << "dataId=" << header.dataId
//                     << "frag=" << header.fragIndex << "/" << header.totalFrags;
            handleDataPacket(header, payload, sender, senderPort);
            break;
        case 1:
//            qDebug() << "RECV ACK: clientId=" << header.clientId << "dataId=" << header.dataId
//                     << "lastFrag=" << header.fragIndex;
            handleAckPacket(header, payload);
            break;
        case 2: handleClientHello(header, payload, sender, senderPort); break;
        case 3: handleClientIdAssign(header, payload); break;
        }
    }
}

void SqzUdpTransfer::handleDataPacket(const SqzPacketHeader &header, const QByteArray &payload,
                                      const QHostAddress &sender, quint16 port) {
    quint64 key = ((quint64)header.clientId << 32) | header.dataId;
    ReceiveTask &task = m_recvTasks[key];
    task.clientId = header.clientId;
    task.dataId = header.dataId;
    task.senderAddr = sender;
    task.senderPort = port;
    task.totalFrags = header.totalFrags;
    task.totalLen = header.dataLen;
    task.lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
    if (!task.frags.contains(header.fragIndex))
        task.frags[header.fragIndex] = payload;

    int continuous = -1;
    for (int i = 0; i <= task.totalFrags; ++i) {
        if (task.frags.contains(i)) continuous = i;
        else break;
    }
    if (continuous != task.lastContinuousAck && m_enableCumulativeAck) {
        task.lastContinuousAck = continuous;
        PendingAck &pending = m_pendingAcks[key];
        pending.clientId = header.clientId;
        pending.dataId = header.dataId;
        pending.lastContinuousFrag = continuous;
        pending.scheduledTime = QDateTime::currentMSecsSinceEpoch() + m_ackDelayMs;
    } else if (!m_enableCumulativeAck) {
        sendCumulativeAck(header.clientId, header.dataId, header.fragIndex, sender, port, 65535);
    }

    if (task.frags.size() == task.totalFrags)
        tryAssembleData(header.clientId, header.dataId);
}

void SqzUdpTransfer::handleAckPacket(const SqzPacketHeader &header, const QByteArray &) {
    quint64 key = ((quint64)header.clientId << 32) | header.dataId;
    auto it = m_sendTasks.find(key);
    if (it != m_sendTasks.end()) {
        SendTask &task = *it;
        for (int i = 0; i <= header.fragIndex && i < task.ackFlags.size(); ++i)
            task.ackFlags[i] = true;
        bool allAcked = true;
        for (bool a : task.ackFlags) if (!a) { allAcked = false; break; }
        if (allAcked && !task.completed) {
            completeSendTask(header.clientId, header.dataId);
            auto waitIt = m_blockWaitMap.find(header.dataId);
            if (waitIt != m_blockWaitMap.end()) {
                waitIt->acked = true;
                if (waitIt->loop)
                    waitIt->loop->quit();
            }
        }
    }
}

void SqzUdpTransfer::handleClientHello(const SqzPacketHeader &header, const QByteArray &,
                                       const QHostAddress &sender, quint16 port) {
    if (!m_isServerMode) return;
    QString key = sender.toString() + ":" + QString::number(port);
    quint32 clientId;
    if (m_addrToClientId.contains(key)) {
        clientId = m_addrToClientId[key];
    } else {
        clientId = generateClientId(sender, port);
        m_addrToClientId[key] = clientId;
        m_clientAddrMap[clientId] = sender;
    }
    SqzPacketHeader resp;
    resp.type = 3;
    resp.clientId = clientId;
    resp.dataId = header.dataId;
    resp.totalFrags = 0;
    resp.fragIndex = 0;
    resp.dataLen = 0;
    resp.window = 0;
    resp.crc16 = 0;
    QByteArray packet(reinterpret_cast<const char*>(&resp), sizeof(SqzPacketHeader));
    quint16 crc = calculateCrc16(packet);
    resp.crc16 = crc;
    memcpy(packet.data() + sizeof(SqzPacketHeader) - sizeof(quint16), &crc, sizeof(quint16));
    m_socket->writeDatagram(packet, sender, port);
}

void SqzUdpTransfer::handleClientIdAssign(const SqzPacketHeader &header, const QByteArray &) {
    emit clientRegistered(header.clientId);
}

void SqzUdpTransfer::tryAssembleData(quint32 clientId, quint32 dataId) {
    quint64 key = ((quint64)clientId << 32) | dataId;
       auto it = m_recvTasks.find(key);
       if (it == m_recvTasks.end()) return;
       ReceiveTask &task = *it;
       if (task.frags.size() != task.totalFrags) return;

       QByteArray fullData;
       fullData.reserve(task.totalLen);
       for (int i = 0; i < task.totalFrags; ++i) {
           if (!task.frags.contains(i)) return;
           fullData.append(task.frags[i]);
       }
       if (fullData.size() > task.totalLen) fullData.resize(task.totalLen);

       // ★★★ 立即发送最终 ACK（不等待后续处理）★★★
       sendCumulativeAck(task.clientId, task.dataId, task.totalFrags - 1,
                         task.senderAddr, task.senderPort, 65535);

       // 解析数据并分发（文件写入会异步进行）
       QDataStream ds(fullData);
       QString magic;
       ds >> magic;
       if (magic == "FILE_BEGIN") {
           handleFileBegin(clientId, fullData);
       } else if (magic == "FILE_DATA") {
           handleFileData(clientId, fullData);
       } else {
           emit dataReceived(clientId, fullData, task.senderAddr, task.senderPort);
       }

       m_recvTasks.erase(it);
}

void SqzUdpTransfer::handleFileBegin(quint32 clientId, const QByteArray &data) {
    QDataStream ds(data);
    QString magic;
    quint32 sessionId;
    QString fileName;
    qint64 totalSize;
    int totalBlocks;
    qint64 blockSize;
    ds >> magic >> sessionId >> fileName >> totalSize >> totalBlocks >> blockSize;
    if (magic != "FILE_BEGIN") return;

    quint64 sessionKey = ((quint64)clientId << 32) | sessionId;
    if (m_fileSessions.contains(sessionKey) && !m_fileSessions[sessionKey]->completed) {
        return;
    }

    auto session = QSharedPointer<FileSession>::create();
    session->clientId = clientId;
    session->sessionId = sessionId;
    session->fileName = fileName;
    session->totalSize = totalSize;
    session->totalBlocks = totalBlocks;
    session->blockSize = blockSize;
    session->nextExpectedBlock = 0;
    session->completed = false;
    session->savePath = QDir::current().absoluteFilePath(fileName);
    session->outputFile.setFileName(session->savePath);
    if (!session->outputFile.open(QIODevice::WriteOnly)) {
        emit errorOccurred(tr("Cannot create file: %1").arg(session->savePath));
        return;
    }
    session->outputFile.resize(totalSize);
    m_fileSessions[sessionKey] = session;

    auto orphanIt = m_orphanBlocks.find(sessionKey);
    if (orphanIt != m_orphanBlocks.end()) {
        for (const QByteArray &orphanData : orphanIt.value()) {
            handleFileData(clientId, orphanData);
        }
        m_orphanBlocks.erase(orphanIt);
    }

    emit fileProgress(clientId, 0, totalSize);
    qDebug() << "File session started: " << fileName << "size=" << totalSize;
}

void SqzUdpTransfer::handleFileData(quint32 clientId, const QByteArray &data) {
    QDataStream ds(data);
        QString magic;
        quint32 sessionId;
        int blockIndex;
        QByteArray blockData;
        ds >> magic >> sessionId >> blockIndex >> blockData;
        if (magic != "FILE_DATA") return;

        quint64 sessionKey = ((quint64)clientId << 32) | sessionId;
        auto it = m_fileSessions.find(sessionKey);
        if (it == m_fileSessions.end()) {
            // 会话未建立，缓存为孤儿块（后续 BEGIN 到达时会处理）
            m_orphanBlocks[sessionKey].append(data);
            qDebug() << "Orphan block" << blockIndex << "cached for session" << sessionId;
            return;
        }

        QSharedPointer<FileSession> session = it.value();
        if (session->receivedBlocks.contains(blockIndex)) return;

        session->receivedBlocks.insert(blockIndex);
        // 注意：不立即写入，而是放入 pendingBlocks 并按顺序处理
        // 但为了异步，我们将顺序写入任务也放到线程池中
        // 简化：直接调用异步写入，由异步写入保证顺序（需要加锁）
        asyncWriteBlock(sessionKey, blockIndex, blockData);
}

void SqzUdpTransfer::flushFileSession(FileSession &session) {
//    while (session.pendingBlocks.contains(session.nextExpectedBlock)) {
//        QByteArray blockData = session.pendingBlocks.take(session.nextExpectedBlock);
//        qint64 offset = session.nextExpectedBlock * session.blockSize;
//        if (!session.outputFile.seek(offset)) {
//            emit errorOccurred(tr("File seek error at offset %1").arg(offset));
//            return;
//        }
//        qint64 written = session.outputFile.write(blockData);
//        if (written != blockData.size()) {
//            emit errorOccurred(tr("File write error: only %1 of %2 bytes written").arg(written).arg(blockData.size()));
//            return;
//        }
//        session.nextExpectedBlock++;
//        qint64 receivedSize = session.nextExpectedBlock * session.blockSize;
//        if (receivedSize > session.totalSize) receivedSize = session.totalSize;
//        emit fileProgress(session.clientId, receivedSize, session.totalSize);
//    }
//    if (session.receivedBlocks.size() == session.totalBlocks) {
//        session.outputFile.close();
//        emit fileReceived(session.clientId, session.savePath, session.fileName, session.totalSize);
//        session.completed = true;
//        quint64 sessionKey = ((quint64)session.clientId << 32) | session.sessionId;
//        m_fileSessions.remove(sessionKey);
//        qDebug() << "File session completed:" << session.fileName;
//    }
}

void SqzUdpTransfer::asyncWriteBlock(quint64 sessionKey, int blockIndex, const QByteArray &blockData)
{
    QtConcurrent::run([this, sessionKey, blockIndex, blockData]() {
           auto it = m_fileSessions.find(sessionKey);
           if (it == m_fileSessions.end()) return;
           QSharedPointer<FileSession> session = it.value();
           QMutexLocker locker(&session->mutex);

           // 将块暂存到 pendingBlocks
           session->pendingBlocks[blockIndex] = blockData;

           // 尝试顺序写入
           while (session->pendingBlocks.contains(session->nextExpectedBlock)) {
               QByteArray data = session->pendingBlocks.take(session->nextExpectedBlock);
               qint64 offset = session->nextExpectedBlock * session->blockSize;
               if (!session->outputFile.seek(offset)) {
                   qWarning() << "Seek error in async write";
                   return;
               }
               qint64 written = session->outputFile.write(data);
               if (written != data.size()) {
                   qWarning() << "Write error in async write";
                   return;
               }
               session->nextExpectedBlock++;
               qint64 receivedSize = session->nextExpectedBlock * session->blockSize;
               if (receivedSize > session->totalSize) receivedSize = session->totalSize;
               // 发送进度信号（注意跨线程，使用 Qt::QueuedConnection）
               QMetaObject::invokeMethod(this, "fileProgress", Qt::QueuedConnection,
                                         Q_ARG(quint32, session->clientId),
                                         Q_ARG(qint64, receivedSize),
                                         Q_ARG(qint64, session->totalSize));
           }

           if (session->receivedBlocks.size() == session->totalBlocks) {
               session->outputFile.close();
               QMetaObject::invokeMethod(this, "fileReceived", Qt::QueuedConnection,
                                         Q_ARG(quint32, session->clientId),
                                         Q_ARG(QString, session->savePath),
                                         Q_ARG(QString, session->fileName),
                                         Q_ARG(qint64, session->totalSize));
               session->completed = true;
               // 从会话表中移除（注意：在异步线程中操作主线程的哈希表可能不安全，改用信号通知主线程）
               QMetaObject::invokeMethod(this, [this, sessionKey]() {
                   m_fileSessions.remove(sessionKey);
               }, Qt::QueuedConnection);
           }
       });
}

// ======================= 超时与重传 =======================
void SqzUdpTransfer::onTimeoutCheck() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QList<quint64> timeoutTasks;
    for (auto it = m_sendTasks.begin(); it != m_sendTasks.end(); ++it) {
        SendTask &task = *it;
        if (task.completed) continue;
        if (now - task.lastSendTime >= task.currentTimeout) {
            QVector<int> missing;
            for (int i = 0; i < task.ackFlags.size(); ++i)
                if (!task.ackFlags[i]) missing.append(i);
            if (missing.isEmpty()) {
                completeSendTask(task.clientId, task.dataId);
                auto waitIt = m_blockWaitMap.find(task.dataId);
                if (waitIt != m_blockWaitMap.end()) {
                    waitIt->acked = true;
                    if (waitIt->loop) waitIt->loop->quit();
                }
                continue;
            }
            if (task.retries >= m_maxRetries) {
                emit errorOccurred(tr("Send task %1:%2 failed after max retries").arg(task.clientId).arg(task.dataId));
                m_sendTasks.erase(it);
                auto waitIt = m_blockWaitMap.find(task.dataId);
                if (waitIt != m_blockWaitMap.end()) {
                    waitIt->acked = false;
                    if (waitIt->loop) waitIt->loop->quit();
                }
                continue;
            }
            retransmitFragments(task, missing);
            task.retries++;
            task.lastSendTime = now;
            task.currentTimeout = qMin(task.currentTimeout * 2, 30000);
        }
    }

    QList<quint64> staleRecv;
    for (auto it = m_recvTasks.begin(); it != m_recvTasks.end(); ++it) {
        if (now - it->lastUpdateTime > 30000) staleRecv.append(it.key());
    }
    for (quint64 key : staleRecv) m_recvTasks.remove(key);
}

void SqzUdpTransfer::retransmitFragments(SendTask &task, const QVector<int> &missingIndices) {
    for (int idx : missingIndices) {
        sendDataFragment(task.clientId, task.dataId, idx, task.frags[idx],
                         task.destAddr, task.destPort,
                         task.ackFlags.size(), task.fullData.size());
    }
}

void SqzUdpTransfer::completeSendTask(quint32 clientId, quint32 dataId) {
    quint64 key = ((quint64)clientId << 32) | dataId;
    m_sendTasks.remove(key);
}

void SqzUdpTransfer::onFlushAck() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QList<quint64> toSend;
    for (auto it = m_pendingAcks.begin(); it != m_pendingAcks.end(); ++it) {
        if (now >= it->scheduledTime) toSend.append(it.key());
    }
    for (quint64 key : toSend) {
        PendingAck &ack = m_pendingAcks[key];
        auto recvIt = m_recvTasks.find(key);
        if (recvIt != m_recvTasks.end()) {
            const ReceiveTask &task = recvIt.value();
            sendCumulativeAck(ack.clientId, ack.dataId, ack.lastContinuousFrag,
                              task.senderAddr, task.senderPort, 65535);
        }
        m_pendingAcks.remove(key);
    }
}
