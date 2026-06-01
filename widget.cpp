#include "widget.h"
#include "ui_widget.h"
#include "SqzTranslator.h"
#include "SqzFactory.h"
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    SqzTranslator::instance().registerWidget(this);
    M_InitTrUi();
}

Widget::~Widget()
{
    delete ui;
}

bool ret = true;
void Widget::on_pushButton_clicked()
{
    if(ret)
    SqzTranslator::instance().safeSwitchLanguage("简体中文");
    else
    SqzTranslator::instance().safeSwitchLanguage("English");

    ret = !ret;
}

void Widget::M_InitTrUi()
{
    ui->label->setText(TR("主界面"));
    ui->label_2->setText(TR("打开弹窗"));
    setWindowTitle(TR("语言同步测试"));
}
REGISTER_CLASS_NO_ARG(Widget)
