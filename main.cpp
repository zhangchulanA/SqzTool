
#include <QApplication>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QSpinBox>
#include <QTextEdit>
#include "Translator.h"
#include "SqzHub.h"
#include "ThreadPool.h"
#include "Logger.h"
#include "TimerUtils.h"
#include "TestWidget.h"
#include "CustomSearchBox.h"
#include "FlexData.h"
#include "ProtocolSchema.h"
#include "TableBuilder.h"
#include "ChainBranch.h"
#include <FluentCard.h>
#include "MsgBox.h"
#include "SuperTableAll.h"
#include "UiUtils.h"
#include "RadioLink.h"

using namespace std::chrono_literals;
int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    Logger::instance().init("./log","chatlog",10,true);
    SqzHub::SetThreadPrefix(MODULE_PREFIX);
    Sqz.PrintRegClass();
    //    Sqz.CreateWidget("TestWidget");
//    Sqz.CreateWidget("SqzViewTest");

//    Sqz.CreateQmlWidget("LoginWindow");
    Sqz.CreateWidget("MainWindowTest");
    RadioLink manager;

    // 配置参数
    const QString RADIO_IP = "192.168.50.164"; // 改成你的电台IP，本机测试用 "127.0.0.1"
    const quint16 RADIO_PORT = 8888;           // 电台接收数据的端口
    const QString LOCAL_IP = "192.168.50.164";
    const quint16 LOCAL_PORT = 9999;           // 本地绑定的端口（电台向此端口回复ACK）
    const qint64 MAX_RATE = 2048;              // 2KB/s 限流

    // 初始化发送器
    manager.init(QHostAddress(LOCAL_IP), LOCAL_PORT,QHostAddress(RADIO_IP), RADIO_PORT, MAX_RATE);

    QObject::connect(&manager,&RadioLink::getMessage,[=](const QByteArray& data){
       logdebug << data.size();
    });


    // ====== 5. 业务层发送数据（每5ms发一个20B包） ======
    QTimer producer;
    int pktId = 0;
    QObject::connect(&producer, &QTimer::timeout, [&]() {
        QByteArray data(20, 'X');
        manager.sendData(data);
        pktId++;
        if (pktId % 100 == 0) qDebug() << "[App] Produced" << pktId << "packets";
    });
    producer.start(500);

    // ====== 6. 运行15秒后退出 ======
//    QTimer::singleShot(15000, [&]() {
//        producer.stop();
////        mockRadio->close();
//        qDebug() << "[App] Test finished.";
//        a.quit();
//    });






    // 运行事件循环
    int ret = a.exec();

    // 程序退出前主动清理（可选）
    Sqz.CloseAll();
    return  ret;
}
