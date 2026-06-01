#include "sqzudptest.h"
#include "ui_sqzudptest.h"

#include "SqzLog.h"
SqzUdpTest::SqzUdpTest(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SqzUdpTest)
{
    ui->setupUi(this);
    if(receiver.bind(8888))
        logdebug <<"bind succeccful";
    receiver.setReceiveBufferSize(256 * 1024 * 1024);
    connect(&receiver, &SqzUdpTransfer::dataReceived,
            [](quint32 clientId, const QByteArray &data,
            const QHostAddress &sender, quint16 port){
//        qDebug()<<clientId;
        logdebug<<clientId<<sender.toString()<<port<<QString::fromUtf8(data);
    });

    connect(&receiver, &SqzUdpTransfer::fileReceived,
               [](quint32 clientId, const QString &path, const QString &orig, qint64 size){
                    qDebug() << "文件已保存:" << path;
                });

    connect(&receiver, &SqzUdpTransfer::fileProgress,[=](quint32 clientId, qint64 current, qint64 total){
       logdebug<< (current * 100 / total) << "%";
    });
    connect(&receiver,&SqzUdpTransfer::errorOccurred,[=](const QString &errorMsg){
        logdebug<<errorMsg;
    });
}

SqzUdpTest::~SqzUdpTest()
{
    delete ui;
}

void SqzUdpTest::on_pushButton_clicked()
{
    connect(&sender,&SqzUdpTransfer::errorOccurred,[=](const QString &errorMsg){
        logdebug<<errorMsg;
    });

    if(sender.bind(7777))
        logdebug<<"bind succeccful";

    sender.setRemoteAddress(QHostAddress("192.168.50.164"), 8888);
//sender.setFileBlockSize(16 * 1024 * 1024);
//sender.sendLargeFile("/home/sqz/图片/621.mp4");
    QString A = "你好";
    ///home/sqz/视频/Desktop 2025.08.17 - 19.23.32.07.mp4
///home/sqz/桌面/gst.zip   /home/sqz/图片/2026-05-23 05-48-56 的屏幕截图.png
/// // /home/sqz/图片/621.mp4
    bool ret = sender.sendData(QByteArray("abcafkagjqigjqknfkjqf十三阿发挥父亲"));
    logdebug<<ret;
}
