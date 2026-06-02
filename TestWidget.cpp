#include "TestWidget.h"
#include "ui_TestWidget.h"
#include "SqzFactory.h"
#include "SqzTranslator.h"
#include "SqzBus.h"
#include <CustomSearchBox.h>
#include "SqzBus.h"
Q_DECLARE_METATYPE(QHostAddress)


TestWidget::TestWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TestWidget)
{
    ui->setupUi(this);
    MySqzTranslator.registerWidget(this);
    M_InitTrUi();
    qRegisterMetaType<QHostAddress>("QHostAddress");

    // 创建搜索框，使用独立的 history key
    CustomSearchBox *searchBox = new CustomSearchBox(this, "main_search");
    searchBox->setPlaceholderText("Enter keywords...");
    searchBox->resize(300,50);
    searchBox->move(100,100);

    m_udp = new UdpServer;
    m_udp->SetSendIpAndPort("192.168.50.164",1111);

    m_udp->SetAnyHostPort();
    if(m_udp->SetIpAndPort("192.168.50.164",1111))
        logdebug <<"绑定成功";
    connect(m_udp,&UdpServer::DataReceived,[=](const QByteArray& data){
       logdebug<<data.size();
    });
    SqzBus::receive(this,"GET_UDP_DATA",this,&TestWidget::ReceiveUdpData);
//    SqzBus::receive(this,"111",[](const int& a){

//    });
}

TestWidget::~TestWidget()
{
    delete ui;
}

void TestWidget::M_InitTrUi()
{
    ui->pushButton->setText(TR("语言同步测试"));
    setWindowTitle(TR("测试弹窗"));
}

bool ret = false;
void TestWidget::on_pushButton_clicked()
{
    if(ret)
    SqzTranslator::instance().safeSwitchLanguage("简体中文");
    else
    SqzTranslator::instance().safeSwitchLanguage("English");

    ret = !ret;

//    SqzBus::send("111",1);
    logdebug;
    m_udp->SendMessage(QByteArray("333333"));
    logdebug;
}

void TestWidget::ReceiveUdpData(const QVariantList &list)
{
    logdebug;
    QHostAddress address = list[0].value<QHostAddress>();
    quint16 port = list[1].toUInt();
    QByteArray data = list[2].toByteArray();
    logdebug <<address.toString()<<port<<data;
}
REGISTER_CLASS_NO_ARG(TestWidget)
