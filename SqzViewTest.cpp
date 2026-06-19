#include "SqzViewTest.h"
#include "ui_SqzViewTest.h"

SqzViewTest::SqzViewTest(SqzView *parent) :
    SqzView(parent),
    ui(new Ui::SqzViewTest)
{
    ui->setupUi(this);
    Open("SqzViewTest2");
    logdebug <<"SqzViewTest start";
}

SqzViewTest::~SqzViewTest()
{
    delete ui;
}
SQZ_HUB(SqzViewTest)
