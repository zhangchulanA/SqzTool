#include "UdpServer.h"

#include <QDebug>
#include <QNetworkConfiguration>
#include <SqzLog.h>
#include <SqzBus.h>
#include <QMetaType>
#define log (qDebug()<<"["<<__LINE__<<__FUNCTION__<<"]")

Q_DECLARE_METATYPE(QHostAddress)

UdpServer::UdpServer()
{
    m_Socket = new QUdpSocket(this);
    groupSocket = new QUdpSocket(this);
    groupSocket->setSocketOption(QAbstractSocket::MulticastTtlOption,1);

    connect(m_Socket,&QUdpSocket::readyRead,this,&UdpServer::onReadyRead);
    connect(groupSocket, &QUdpSocket::readyRead, this, &UdpServer::groupRecvData);

}

UdpServer::~UdpServer()
{

}

void UdpServer::SetSendIpAndPort(QString ip, quint16 port)
{
    _ip.setAddress(ip);
    _port = port;
}

void UdpServer::SetSendIpAndPort(QHostAddress address, quint16 port)
{
    _ip = address;
    _port = port;
}


void UdpServer::SendSendInfo(QString ip, quint16 port)
{
    _ip.setAddress(ip);
    _port = port;
}

void UdpServer::SetIpAndPort(QString ip, quint16 port)
{
    _sendIp.setAddress(ip);
    _sendPort = port;
    if(m_Socket->bind(_sendIp,_sendPort))
    {
        logdebug <<"绑定成功";
        connect(m_Socket,&QUdpSocket::stateChanged,this,&UdpServer::stateChangedSlot);
    }
    else
    {
        logwarn<<"绑定失败";
    }
}

void UdpServer::SetAnyHostPort(quint16 port)
{
    _sendIp.setAddress(QHostAddress::Any);
    _sendPort = port;
    if(m_Socket->bind(_sendIp,_sendPort))
    {
        logdebug <<"绑定成功";
    }
    else
    {
        logwarn<<"绑定失败";
    }
}

void UdpServer::SetLocalHostPort(quint16 port)
{
    _sendIp.setAddress(QHostAddress::LocalHost);
    _sendPort = port;
    if(m_Socket->bind(_sendIp,_sendPort))
    {
        logdebug <<"绑定成功";
    }
    else
    {
        logwarn<<"绑定失败";
    }
}

void UdpServer::SetMulticastIpAndPort(QString groupIp, int groupPort)
{
    groupAddr = QHostAddress(groupIp);
    _groupPort = groupPort;
    if(groupSocket->bind(QHostAddress::AnyIPv4,_groupPort,QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
    {
        //加入组播
        if(groupSocket->joinMulticastGroup(groupAddr))
            logdebug <<"加入组播成功"<<"当前网卡：全部  组播IP: "<<groupIp <<"绑定端口: "+QString::number(groupPort);
        else
            logwarn << "加入组播失败";
    }
    else
        logwarn<<"绑定端口失败";
}

void UdpServer::SendMulticastMessage(const char *data, int len)
{
    groupSocket->writeDatagram(data,len,groupAddr,_groupPort);
}
void UdpServer::SendMessage(const QString &data)
{

    m_Socket->writeDatagram(data.toUtf8(),_ip,_port);
}

void UdpServer::SendMessage(const char *data, int len)
{

    m_Socket->writeDatagram(data,len,_ip,_port);
}

void UdpServer::SendMessage(const char *data, int len, QString ip, quint16 port)
{
    _ip.setAddress(ip);
    _port = port;
    m_Socket->writeDatagram(data,len,_ip,_port);
}

//void UdpServer::SendMessage(const char *data, int len, QHostAddress ip, quint16 port)
//{
//    m_Socket->writeDatagram(data,len,ip,port);
//}

void UdpServer::SendMessage(const QString &data, QString ip, quint16 port)
{
    _ip.setAddress(ip);
    _port = port;
    m_Socket->writeDatagram(data.toUtf8(),_ip,_port);
}

void UdpServer::SendMessage(const QByteArray &data)
{
    m_Socket->writeDatagram(data,_ip,_port);
}

void UdpServer::SendMessage(const QHostAddress &address, const quint16 &port, const QByteArray &data)
{
    m_Socket->writeDatagram(data,address,port);
}

void UdpServer::ReceiveData()
{
 //   log<<"Enter!";
    while (m_Socket->hasPendingDatagrams()) {
        QByteArray data;
        data.resize(m_Socket->pendingDatagramSize());
        QHostAddress senderAddress;
        quint16 senderPort;
        qint64 size = m_Socket->readDatagram(data.data(),data.size(),&senderAddress,&senderPort);
        if(size == -1)
        {
            logwarn<<"读取消息失败";
        }
        else
        {
            SqzBus::send("GET_UDP_DATA",{QVariant::fromValue(senderAddress),senderPort,data});
/*            emit DataReceived(data);
            emit DataReceivedAndIP(senderAddress,senderPort,data)*/;
            //log<<"收到消息内容:"<<senderAddress.toString()<<":"<<senderPort<<"---"<<data.data();
        }
    }

}

void UdpServer::ReceiveGroupData()
{
       while (groupSocket->hasPendingDatagrams()) {
           QByteArray data;
           data.resize(groupSocket->pendingDatagramSize());
           QHostAddress senderAddress;
           quint16 senderPort;
           qint64 aa = groupSocket->readDatagram(data.data(),data.size(),&senderAddress,&senderPort);
           if(aa == -1)
           {
               logwarn<<"读取组播消息失败";
           }
           else
           {
               emit GroupDataReceived(data);
               //log<<"收到组播消息内容:"<<senderAddress.toString()<<":"<<senderPort<<"---"<<data.data();
           }

   //        emit DataReceived(data);
       }
}

QUdpSocket *UdpServer::GetUdpSocket()
{
    return m_Socket;
}

void UdpServer::Close()
{
    m_Socket->close();
}

void UdpServer::GroupClose()
{
    groupSocket->close();
}

void UdpServer::onReadyRead()
{
    ReceiveData();
}

void UdpServer::groupRecvData()
{
    ReceiveGroupData();
}

void UdpServer::stateChangedSlot(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState:
    {
        logdebug<<"UnconnectedState";
        break;
    }
    case QAbstractSocket::HostLookupState:
    {
        logdebug<<"HostLookupState";
        break;
    }
    case QAbstractSocket::ConnectingState:
    {
        logdebug<<"ConnectingState";
        break;
    }
    case QAbstractSocket::ConnectedState:
    {
        logdebug<<"ConnectedState";
        break;
    }
    case QAbstractSocket::BoundState:
    {
        logdebug<<"BoundState";
        break;
    }
    case QAbstractSocket::ListeningState:
    {
        logdebug<<"ListeningState";
        break;
    }
    case QAbstractSocket::ClosingState:
    {
        logdebug<<"ClosingState";
        break;
    }
    default:
        break;
    }
}

QAtomicPointer<DanLiUdp> DanLiUdp::instance_{nullptr} ;
QMutex DanLiUdp::mutex_;
DanLiUdp &DanLiUdp::instance()
{
    if(!instance_)
    {
        QMutexLocker locker(&mutex_);
        if(!instance_)
        {
            instance_ = new DanLiUdp();
        }
    }
    return *instance_;
}

DanLiUdp::DanLiUdp()
{
    _udpServer = new UdpServer();
}

void DanLiUdp::destory()
{
    if(instance_){
        delete instance_;
        instance_ = nullptr;
    }
}

UdpServer *DanLiUdp::GetUdpServer()
{
    return _udpServer;
}

bool DanLiUdp::GetCardID(QString name, QString &BDCard)
{
    auto iterinfo = _sendInfo.find(name);

    if(iterinfo == _sendInfo.end())
    {
        return false;
    }
    else
    {
        SendInfo tempInfo = iterinfo->second;
        BDCard = tempInfo.cardID;
        return true;
    }

    return false;
}

bool DanLiUdp::GetCardName(QString BDCard, QString &name)
{
    for(auto it=_sendInfo.begin();it!=_sendInfo.end();it++)
    {
        if(it->second.cardID == BDCard)
        {
            name = it->first;
            return  true;
        }
    }
    return false;
}

bool DanLiUdp::GetCardIP(QString ip, QString &name)
{
    for(auto it=_sendInfo.begin();it!=_sendInfo.end();it++)
    {
        if(it->second.ip == ip)
        {
            name = it->first;
            return  true;
        }
    }
    return false;
}

void DanLiUdp::SendMessageToBD(const QString &data)
{
    _udpServer->SendMessage(data, _sendInfo.find("北斗指挥机")->second.ip
                            ,_sendInfo.find("北斗指挥机")->second.port);
}

void DanLiUdp::SendMessageToZZW(QString name, const char *data, int len)
{
    auto iterinfo = _sendInfo.find(name);

    if(iterinfo == _sendInfo.end())
    {
        log<<"=============================================**"<<name;
        return;
    }
    else
    {
        SendInfo tempInfo = iterinfo->second;

        _udpServer->SendMessage(data, len, tempInfo.ip, tempInfo.port);
        QString str = "";
        for(int i=0;i<len;i++)
        {
//            log<< QString::number((uchar)data[i],16);
        }
        log<<tempInfo.ip<<tempInfo.port<<"  "<<len<<"  "<<str;
    }

}

void DanLiUdp::UpdateSendInfo()
{
//    _sendInfo.clear();

//    Crane_XML tongxin;

//    Minglu_XML minglu;

//    std::vector<Crane_XMLIndex> tongxinData = tongxin.GetCrane_XMLAll();

//    QVector<Team> mingluTeam = minglu.GetMinglu();

//    for(size_t i = 0; i < tongxinData.size(); ++i)
//    {
//        Crane_XMLIndex currentInfo = tongxinData.at(i);

//        if(currentInfo.Name == "搜救业务信息终端")
//        {
//            SendInfo tempInfo;
//            tempInfo.ip = currentInfo._ip;
//            tempInfo.port = currentInfo._port.toUShort();
//            _sendInfo.insert(std::make_pair(tongxinData.at(i).Name, tempInfo));
//        }
//        else if(currentInfo.Name == "北斗指挥机")
//        {
//            SendInfo tempInfo;
//            tempInfo.ip = currentInfo._ip;
//            tempInfo.port = currentInfo._port.toUShort();
//            tempInfo.cardID = currentInfo._cardID;
//            _sendInfo.insert(std::make_pair(tongxinData.at(i).Name, tempInfo));
//        }
//        else if(currentInfo.Name == "无人机地面站")
//        {
//            SendInfo tempInfo;
//            tempInfo.ip = currentInfo._ip;
//            tempInfo.port = currentInfo._port.toUShort();
//            tempInfo.cardID = currentInfo._cardID;
//            _sendInfo.insert(std::make_pair(tongxinData.at(i).Name, tempInfo));
//        }
//        else if(currentInfo.Name == "手持搜寻终端1")
//        {
//            SendInfo tempInfo;
//            tempInfo.ip = currentInfo._ip;
//            tempInfo.port = currentInfo._port.toUShort();
//            tempInfo.cardID = currentInfo._cardID;
//            _sendInfo.insert(std::make_pair(tongxinData.at(i).Name, tempInfo));
//        }
//        else if(currentInfo.Name == "手持搜寻终端2")
//        {
//            SendInfo tempInfo;
//            tempInfo.ip = currentInfo._ip;
//            tempInfo.port = currentInfo._port.toUShort();
//            tempInfo.cardID = currentInfo._cardID;
//            _sendInfo.insert(std::make_pair(tongxinData.at(i).Name, tempInfo));
//        }
//        else if(currentInfo.Name == "手持卫勤信息终端1")
//        {
//            SendInfo tempInfo;
//            tempInfo.ip = currentInfo._ip;
//            tempInfo.port = currentInfo._port.toUShort();
//            tempInfo.cardID = currentInfo._cardID;
//            _sendInfo.insert(std::make_pair(tongxinData.at(i).Name, tempInfo));
//        }
//        else if(currentInfo.Name == "手持卫勤信息终端2")
//        {
//            SendInfo tempInfo;
//            tempInfo.ip = currentInfo._ip;
//            tempInfo.port = currentInfo._port.toUShort();
//            tempInfo.cardID = currentInfo._cardID;
//            _sendInfo.insert(std::make_pair(tongxinData.at(i).Name, tempInfo));
//        }
//        else
//        {
//            //do nothing
//        }
//    }

//    for(size_t i = 0; i < mingluTeam.size(); ++i)
//    {
//        Team currentTeam = mingluTeam.at(i);

//        if(currentTeam.name.contains("搜救小队"))
//        {
//            for(size_t j = 0; j < currentTeam.persons.size(); ++j)
//            {
//                Person currentPerson = currentTeam.persons.at(j);
//                QString equipName;
//                if((equipName = currentPerson.equipment1).contains("手持搜寻终端")
//                        ||(equipName = currentPerson.equipment2).contains("手持搜寻终端")
//                        ||(equipName = currentPerson.equipment3).contains("手持搜寻终端")
//                        ||(equipName = currentPerson.equipment4).contains("手持搜寻终端"))
//                {
//                    for(size_t k = 0; k < tongxinData.size(); ++k)
//                    {
//                        if(tongxinData.at(k).Name == equipName)
//                        {
//                            SendInfo tempInfo;
//                            tempInfo.ip = tongxinData.at(k)._ip;
//                            tempInfo.port = tongxinData.at(k)._port.toUShort();
//                            tempInfo.cardID = tongxinData.at(k)._cardID;
//                            _sendInfo.insert(std::make_pair(currentTeam.name, tempInfo));
//                        }
//                    }
//                }
//                else
//                {
//                    //do nothing
//                }
//            }
//        }
//        else if(currentTeam.name.contains("直升机救护组"))
//        {
//            for(size_t j = 0; j < currentTeam.persons.size(); ++j)
//            {
//                Person currentPerson = currentTeam.persons.at(j);
//                QString equipName;
//                if((equipName = currentPerson.equipment1).contains("手持卫勤信息终端")
//                        ||(equipName = currentPerson.equipment2).contains("手持卫勤信息终端")
//                        ||(equipName = currentPerson.equipment3).contains("手持卫勤信息终端")
//                        ||(equipName = currentPerson.equipment4).contains("手持卫勤信息终端"))
//                {
//                    for(size_t k = 0; k < tongxinData.size(); ++k)
//                    {
//                        if(tongxinData.at(k).Name == equipName)
//                        {
//                            SendInfo tempInfo;
//                            tempInfo.ip = tongxinData.at(k)._ip;
//                            tempInfo.port = tongxinData.at(k)._port.toUShort();
//                            tempInfo.cardID = tongxinData.at(k)._cardID;
//                            _sendInfo.insert(std::make_pair(currentTeam.name, tempInfo));
//                        }
//                    }
//                }
//            }
//        }
//        else if(currentTeam.name.contains("机动卫勤分队"))
//        {
//            for(size_t j = 0; j < currentTeam.persons.size(); ++j)
//            {
//                Person currentPerson = currentTeam.persons.at(j);
//                QString equipName;
//                if((equipName = currentPerson.equipment1).contains("手持卫勤信息终端")
//                        ||(equipName = currentPerson.equipment2).contains("手持卫勤信息终端")
//                        ||(equipName = currentPerson.equipment3).contains("手持卫勤信息终端")
//                        ||(equipName = currentPerson.equipment4).contains("手持卫勤信息终端"))
//                {
//                    for(size_t k = 0; k < tongxinData.size(); ++k)
//                    {
//                        if(tongxinData.at(k).Name == equipName)
//                        {
//                            SendInfo tempInfo;
//                            tempInfo.ip = tongxinData.at(k)._ip;
//                            tempInfo.port = tongxinData.at(k)._port.toUShort();
//                            tempInfo.cardID = tongxinData.at(k)._cardID;
//                            _sendInfo.insert(std::make_pair(currentTeam.name, tempInfo));
//                        }
//                    }
//                }
//            }
//        }
//    }

//    for(auto iter = _sendInfo.begin(); iter != _sendInfo.end(); ++iter)
//    {
//        log<<iter->first<<" "<<iter->second.ip<<" "<<iter->second.port
//          <<" "<<iter->second.cardID;
//    }
}

