#include "MainWindowTest.h"
#include "ui_MainWindowTest.h"

MainWindowTest::MainWindowTest(QWidget *parent) :
    SqzMainWindow(parent),
    ui(new Ui::MainWindowTest)
{
    ui->setupUi(this);
}

MainWindowTest::~MainWindowTest()
{
    delete ui;
}

SQZWIDGET_NOARG(MainWindowTest)
