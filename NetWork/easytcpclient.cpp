#include "easytcpclient.h"
#include <QTcpSocket>
#include <QDataStream>
#include <QFile>
#include <QTimer>
#include <QDir>
#include <QDateTime>
#include <QMetaMethod>
#include <QThread>

// ---------- Worker 类（所有网络操作在这里）----------
class EasyTcpClient::Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr)
        : QObject(parent)
        , m_socket(nullptr)
        , m_autoReconnect(false)
        , m_reconnectInterval(3000)
        , m_reconnectTimer(nullptr)
        , m_file(nullptr)
        , m_fileTotalBytes(0)
        , m_fileSentBytes(0)
        , m_fileSendActive(false)
        , m_cancelSend(false)
        , m_recvTotalBytes(0)
        , m_recvBytesWritten(0)
        , m_recvChunkSize(0)
    {}

    ~Worker() { delete m_socket; }

    void doConnect(const QString &host, quint16 port) {
        if (m_socket) {
            m_socket->deleteLater();
            m_socket = nullptr;
        }
        m_socket = new QTcpSocket(nullptr);
        // 使用 QOverload 解决 error 信号重载问题
        connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
                this, &Worker::onSocketError);
        connect(m_socket, &QTcpSocket::connected, this, &Worker::onSocketConnected);
        connect(m_socket, &QTcpSocket::disconnected, this, &Worker::onSocketDisconnected);
        connect(m_socket, &QTcpSocket::readyRead, this, &Worker::onReadyRead);
        m_socket->connectToHost(host, port);
    }

    void doDisconnect() {
        if (m_socket) m_socket->disconnectFromHost();
    }

    bool isConnected() const { return m_socket && m_socket->state() == QTcpSocket::ConnectedState; }

    void setAutoReconnect(bool enable, int intervalMs) {
        m_autoReconnect = enable;
        m_reconnectInterval = intervalMs;
        if (enable && !m_reconnectTimer) {
            m_reconnectTimer = new QTimer(this);
            m_reconnectTimer->setSingleShot(true);
            connect(m_reconnectTimer, &QTimer::timeout, this, &Worker::doReconnect);
        } else if (!enable && m_reconnectTimer) {
            delete m_reconnectTimer;
            m_reconnectTimer = nullptr;
        }
    }

    void doSend(const QByteArray &data) {
        if (!m_socket || m_socket->state() != QTcpSocket::ConnectedState) {
            m_pendingSends.enqueue(data);
            if (m_pendingSends.size() > 1000) m_pendingSends.dequeue(); // 限制队列长度
            return;
        }
        // 构造长度头（网络字节序）
        QByteArray packet;
        QDataStream ds(&packet, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::BigEndian);
        ds << quint32(data.size());
        packet.append(data);
        m_socket->write(packet);
    }

    // 文件发送
    void doSendFile(const QString &filePath) {
        if (m_fileSendActive) {
            emit fileSendFinished(false, tr("Already sending a file"));
            return;
        }
        m_cancelSend = false;
        m_file = new QFile(filePath);
        if (!m_file->open(QIODevice::ReadOnly)) {
            emit fileSendFinished(false, m_file->errorString());
            delete m_file;
            m_file = nullptr;
            return;
        }
        m_fileTotalBytes = m_file->size();
        m_fileSentBytes = 0;
        m_fileSendActive = true;

        // 发送元数据消息（类型 0x01，文件名+总大小+块大小）
        QByteArray meta;
        QDataStream ds(&meta, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::BigEndian);
        ds << quint8(0x01) << QFileInfo(filePath).fileName() << m_fileTotalBytes << quint64(FILE_CHUNK_SIZE);
        doSend(meta);

        // 开始分块发送
        startNextChunk();
    }

    void cancelSendFile() {
        m_cancelSend = true;
    }

signals:
    void messageReady(const QByteArray &msg);
    void connected();
    void disconnected();
    void error(const QString &err);
    void fileProgress(qint64 sentBytes, qint64 totalBytes);
    void fileSendFinished(bool success, const QString &err);
    void fileReceived(const QString &filePath);

private slots:
    void onSocketConnected() {
        while (!m_pendingSends.isEmpty()) {
            doSend(m_pendingSends.dequeue());
        }
        emit connected();
        if (m_reconnectTimer) m_reconnectTimer->stop();
    }

    void onSocketDisconnected() {
        emit disconnected();
        if (m_autoReconnect && m_reconnectTimer && !m_reconnectTimer->isActive()) {
            m_reconnectTimer->start(m_reconnectInterval);
        }
    }

    void onSocketError(QAbstractSocket::SocketError) {
        emit error(m_socket->errorString());
    }

    void onReadyRead() {
        m_recvBuffer.append(m_socket->readAll());
        tryExtractMessages();
    }

    void doReconnect() {
        if (m_socket && m_socket->state() != QTcpSocket::ConnectedState) {
            m_socket->connectToHost(m_socket->peerName(), m_socket->peerPort());
        }
    }

    void startNextChunk() {
        if (m_cancelSend) {
            finishFileSend(false, tr("Cancelled by user"));
            return;
        }
        if (m_fileSentBytes >= m_fileTotalBytes) {
            finishFileSend(true, QString());
            return;
        }
        qint64 toRead = qMin(FILE_CHUNK_SIZE, m_fileTotalBytes - m_fileSentBytes);
        QByteArray chunk = m_file->read(toRead);
        if (chunk.isEmpty()) {
            finishFileSend(false, tr("File read error"));
            return;
        }
        // 构造数据块消息（类型 0x02 + 块序号 + 数据）
        QByteArray blockMsg;
        QDataStream ds(&blockMsg, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::BigEndian);
        ds << quint8(0x02) << quint32(m_fileSentBytes / FILE_CHUNK_SIZE) << chunk;
        doSend(blockMsg);
        m_fileSentBytes += chunk.size();
        emit fileProgress(m_fileSentBytes, m_fileTotalBytes);

        // 继续下一块（使用 QTimer 延迟，避免深度递归）
        QTimer::singleShot(0, this, &Worker::startNextChunk);
    }

    void finishFileSend(bool success, const QString &err) {
        m_fileSendActive = false;
        if (m_file) {
            m_file->close();
            delete m_file;
            m_file = nullptr;
        }
        emit fileSendFinished(success, err);
    }

    void tryExtractMessages() {
        while (true) {
            if (m_recvBuffer.size() < 4) break;
            QDataStream ds(m_recvBuffer);
            ds.setByteOrder(QDataStream::BigEndian);
            quint32 len;
            ds >> len;
            if (len > 1024 * 1024 * 10) { // 10MB 上限
                emit error(tr("Packet too large"));
                m_socket->disconnectFromHost();
                return;
            }
            if (m_recvBuffer.size() < static_cast<int>(4 + len)) break;
            QByteArray msg = m_recvBuffer.mid(4, len);
            m_recvBuffer.remove(0, 4 + len);
            if (!msg.isEmpty()) {
                quint8 type = static_cast<quint8>(msg[0]);
                if (type == 0x01) {
                    handleFileMeta(msg);
                } else if (type == 0x02) {
                    handleFileChunk(msg);
                } else {
                    emit messageReady(msg);
                }
            }
        }
    }

    void handleFileMeta(const QByteArray &data) {
        QDataStream ds(data);
        ds.setByteOrder(QDataStream::BigEndian);
        quint8 type;
        QString fileName;
        qint64 totalBytes;
        quint64 chunkSize;
        ds >> type >> fileName >> totalBytes >> chunkSize;
        if (type != 0x01) return;

        m_recvFile.reset(new QFile(QDir::temp().absoluteFilePath(fileName)));
        if (!m_recvFile->open(QIODevice::WriteOnly)) {
            emit error(tr("Cannot create file: %1").arg(m_recvFile->errorString()));
            return;
        }
        m_recvTotalBytes = totalBytes;
        m_recvBytesWritten = 0;
        m_recvChunkSize = chunkSize;
    }

    void handleFileChunk(const QByteArray &data) {
        if (!m_recvFile) return;
        QDataStream ds(data);
        ds.setByteOrder(QDataStream::BigEndian);
        quint8 type;
        quint32 index;
        QByteArray chunk;
        ds >> type >> index >> chunk;
        if (type != 0x02) return;

        m_recvFile->write(chunk);
        m_recvBytesWritten += chunk.size();
        if (m_recvBytesWritten >= m_recvTotalBytes) {
            m_recvFile->close();
            emit fileReceived(m_recvFile->fileName());
            m_recvFile.reset();
        }
    }

private:
    QTcpSocket *m_socket;
    QByteArray m_recvBuffer;
    QQueue<QByteArray> m_pendingSends;
    bool m_autoReconnect;
    int m_reconnectInterval;
    QTimer *m_reconnectTimer;

    static constexpr qint64 FILE_CHUNK_SIZE = 64 * 1024; // 64KB
    QFile *m_file;
    qint64 m_fileTotalBytes;
    qint64 m_fileSentBytes;
    bool m_fileSendActive;
    bool m_cancelSend;

    std::unique_ptr<QFile> m_recvFile;
    qint64 m_recvTotalBytes;
    qint64 m_recvBytesWritten;
    qint64 m_recvChunkSize;
};

// ---------- EasyTcpClient 实现 ----------
EasyTcpClient::EasyTcpClient(QObject *parent) : QObject(parent)
{
    m_thread = new QThread(this);
    m_worker = new Worker();
    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started, this, &EasyTcpClient::onThreadStarted);
    connect(m_worker, &Worker::messageReady, this, &EasyTcpClient::onWorkerMessageReady, Qt::QueuedConnection);
    connect(m_worker, &Worker::connected, this, &EasyTcpClient::onWorkerConnected, Qt::QueuedConnection);
    connect(m_worker, &Worker::disconnected, this, &EasyTcpClient::onWorkerDisconnected, Qt::QueuedConnection);
    connect(m_worker, &Worker::error, this, &EasyTcpClient::onWorkerError, Qt::QueuedConnection);
    connect(m_worker, &Worker::fileProgress, this, &EasyTcpClient::onWorkerFileProgress, Qt::QueuedConnection);
    connect(m_worker, &Worker::fileSendFinished, this, &EasyTcpClient::onWorkerFileFinished, Qt::QueuedConnection);
    connect(m_worker, &Worker::fileReceived, this, &EasyTcpClient::fileReceived, Qt::QueuedConnection);

    m_thread->start();
}

EasyTcpClient::~EasyTcpClient()
{
    m_thread->quit();
    m_thread->wait();
}

void EasyTcpClient::connectToHost(const QString &host, quint16 port)
{
    QMetaObject::invokeMethod(m_worker, "doConnect", Qt::QueuedConnection,
                              Q_ARG(QString, host), Q_ARG(quint16, port));
}

void EasyTcpClient::disconnect()
{
    QMetaObject::invokeMethod(m_worker, "doDisconnect", Qt::QueuedConnection);
}

bool EasyTcpClient::isConnected() const
{
    bool result = false;
    QMetaObject::invokeMethod(m_worker, "isConnected", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result));
    return result;
}

void EasyTcpClient::setAutoReconnect(bool enable, int intervalMs)
{
    QMetaObject::invokeMethod(m_worker, "setAutoReconnect", Qt::QueuedConnection,
                              Q_ARG(bool, enable), Q_ARG(int, intervalMs));
}

void EasyTcpClient::send(const QByteArray &data)
{
    QMetaObject::invokeMethod(m_worker, "doSend", Qt::QueuedConnection, Q_ARG(QByteArray, data));
}

void EasyTcpClient::sendFile(const QString &filePath)
{
    QMetaObject::invokeMethod(m_worker, "doSendFile", Qt::QueuedConnection, Q_ARG(QString, filePath));
}

void EasyTcpClient::cancelFileSend()
{
    QMetaObject::invokeMethod(m_worker, "cancelSendFile", Qt::QueuedConnection);
}

void EasyTcpClient::onThreadStarted() { }
void EasyTcpClient::onWorkerMessageReady(const QByteArray &msg) { emit dataReceived(msg); }
void EasyTcpClient::onWorkerConnected() { emit connected(); }
void EasyTcpClient::onWorkerDisconnected() { emit disconnected(); }
void EasyTcpClient::onWorkerError(const QString &err) { emit errorOccurred(err); }
void EasyTcpClient::onWorkerFileProgress(qint64 sent, qint64 total) { emit fileProgress(sent, total); }
void EasyTcpClient::onWorkerFileFinished(bool success, const QString &err) { emit fileSendFinished(success, err); }

