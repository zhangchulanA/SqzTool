#include "SqzViewTest2.h"
#include "ui_SqzViewTest2.h"

SqzViewTest2::SqzViewTest2(SqzView *parent) :
    SqzView(parent),
    ui(new Ui::SqzViewTest2)
{
    ui->setupUi(this);

    logdebug <<"SqzViewTest2 start";
    Open("LoginWindow");
}

SqzViewTest2::~SqzViewTest2()
{
    delete ui;
}
SQZ_HUB(SqzViewTest2)
