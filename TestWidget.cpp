#include "TestWidget.h"
#include "ui_TestWidget.h"
#include "SqzFactory.h"
#include "SqzTranslator.h"
#include "SqzBus.h"
#include <CustomSearchBox.h>
TestWidget::TestWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TestWidget)
{
    ui->setupUi(this);
    MySqzTranslator.registerWidget(this);
    M_InitTrUi();

    // 创建搜索框，使用独立的 history key
    CustomSearchBox *searchBox = new CustomSearchBox(this, "main_search");
    searchBox->setPlaceholderText("Enter keywords...");
    searchBox->resize(300,50);
    searchBox->move(100,100);

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

    SqzBus::send("111",1);
}
REGISTER_CLASS_NO_ARG(TestWidget)
