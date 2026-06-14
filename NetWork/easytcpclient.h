#ifndef EASYTCPCLIENT_H
#define EASYTCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QThread>
#include <QQueue>
#include <QTimer>
#include <QFile>
#include <QAtomicInt>

class EasyTcpClient : public QObject
{
    Q_OBJECT
public:
    explicit EasyTcpClient(QObject *parent = nullptr);
    ~EasyTcpClient();

    // 连接管理
    void connectToHost(const QString &host, quint16 port);
    void disconnect();
    bool isConnected() const;
    void setAutoReconnect(bool enable, int intervalMs = 3000);

    // 普通消息发送（线程安全）
    void send(const QByteArray &data);

    // 大文件发送（异步，支持进度和取消）
    void sendFile(const QString &filePath);
    void cancelFileSend();

signals:
    // 普通消息接收（主线程）
    void dataReceived(const QByteArray &msg);
    // 连接状态
    void connected();
    void disconnected();
    void errorOccurred(const QString &err);

    // 文件发送进度（主线程）
    void fileProgress(qint64 sentBytes, qint64 totalBytes);
    // 文件发送完成（成功/失败）
    void fileSendFinished(bool success, const QString &errorMsg);

    // 文件接收完成（主线程，文件保存在临时目录）
    void fileReceived(const QString &filePath);

private slots:
    void onThreadStarted();
    void onWorkerMessageReady(const QByteArray &msg);
    void onWorkerConnected();
    void onWorkerDisconnected();
    void onWorkerError(const QString &err);
    void onWorkerFileProgress(qint64 sent, qint64 total);
    void onWorkerFileFinished(bool success, const QString &err);

private:
    class Worker;  // 实际工作对象（移动到子线程）
    QThread *m_thread;
    Worker *m_worker;


};

#endif // EASYTCPCLIENT_H
