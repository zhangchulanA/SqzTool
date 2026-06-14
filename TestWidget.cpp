#include "TestWidget.h"
#include "ui_TestWidget.h"

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


    // 发送 int 消息
    SqzBus::Send("age", 25);

        QString a = StateToString(State::Done);
        logdebug <<a;

        ui->pushButton->setStyleSheet(QSS_STYLE("QPushButton", "background: red; color: white;"));

      logdebug << age();
      logdebug << property("age").toInt();

      logdebug <<toJson();

}

TestWidget::~TestWidget()
{
     AUTO_CONNECT_DESTRUCTOR;
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

    m_udp->SendMessage(QByteArray("333333"));

    SqzBus::Send("123",4326);
}

void TestWidget::ReceiveUdpData(const QVariantList &list)
{
    QHostAddress address = list[0].value<QHostAddress>();
    quint16 port = list[1].toUInt();
    QByteArray data = list[2].toByteArray();
    logdebug <<address.toString()<<port<<data;
}
SQZ_HUB(TestWidget)
