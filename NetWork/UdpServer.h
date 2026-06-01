#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <QMutex>
#include <QNetworkConfigurationManager>
#include <QNetworkSession>
#include <QTimer>
#include <QUdpSocket>

class UdpServer : public QObject
{
    Q_OBJECT


public:
    enum Send{
        WuRenJi,
        WuRenChe,
        SouJiuLian,
    };

    UdpServer();
    ~UdpServer();
//******************************************************单播
    //设置接收方ip和port
    void SetSendIpAndPort(QString ip,quint16 port);
    void SetSendIpAndPort(QHostAddress address,quint16 port);
    //设置接收方ip和port
    void SendSendInfo(QString ip,quint16 port);
//设置本地ip和端口
    void SetIpAndPort(QString ip,quint16 port);
//ip设为任意地址，设置端口
    void SetAnyHostPort(quint16 port);
//ip设为本地循环 ，设置端口
    void SetLocalHostPort(quint16 port);
    //单播发送消息
    void SendMessage(const QString& data);
    void SendMessage(const char* data,int len);
    void SendMessage(const char* data,int len,QString ip,quint16 port);
    void SendMessage(const QString& data,QString ip,quint16 port);
    void SendMessage(const QByteArray& data);
    void SendMessage(const QHostAddress& address,const quint16& port,const QByteArray& data);

//******************************************************组播
    //组播设置224.0.2.0
    void SetMulticastIpAndPort(QString groupIp,int groupPort = 5678);
    //组播发送消息
    void SendMulticastMessage(const char* data, int len);

    //

    //接收消息
    void ReceiveData();
    void ReceiveGroupData();
    //获得udp
    QUdpSocket* GetUdpSocket();
    //断开当前链接
    void Close();
    void GroupClose();
public slots:
    void onReadyRead();
    void groupRecvData();

    //监听网络是否连接成功
    void stateChangedSlot(QAbstractSocket::SocketState state);
signals:
    void DataReceived(const QByteArray& data);
    void DataReceivedAndIP(const QHostAddress address ,const int port, const QByteArray& data);
    void GroupDataReceived(const QByteArray& data);


private:

    QUdpSocket* m_Socket = nullptr;
    //接收方ip端口
    QHostAddress _ip;
    quint16 _port;
    //绑定发送方的端口
    QHostAddress _sendIp;
    quint16 _sendPort;
//--------------------------------组播
    QUdpSocket *groupSocket = nullptr;
    QHostAddress groupAddr;//组播地址
    int _groupPort;//组播端口


    //监听网络是否连接成功
    QTimer* reconnectTimer;
    QTimer* networkCheckTimer;
    bool isConnected;


};
#endif // UDPSERVER_H

class DanLiUdp : public QObject
{
    Q_OBJECT

public:

    struct SendInfo{
        QString ip;
        quint16 port;
        QString cardID;
    };

    static DanLiUdp& instance();

    DanLiUdp(const DanLiUdp&) = delete;

    DanLiUdp& operator=(const DanLiUdp&) = delete;

    static void destory();

    UdpServer* GetUdpServer();

    bool GetCardID(QString name, QString &BDCard);

    bool GetCardName(QString BDCard,QString& name);

    bool GetCardIP(QString ip, QString& name);
//    bool GetCardIDFromSheBei()

    void SendMessageToBD(const QString& data);

    void SendMessageToZZW(QString name, const char* data, int len);

public slots:

    void UpdateSendInfo();

protected:

    DanLiUdp();

private:

    static QAtomicPointer<DanLiUdp> instance_;

    static QMutex mutex_;

    UdpServer* _udpServer = nullptr;

    std::map<QString, SendInfo> _sendInfo;
};
