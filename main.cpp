
#include <QApplication>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QSpinBox>
#include <QTextEdit>
#include "SqzTranslator.h"
#include "SqzHub.h"
#include "SqzThreadPool.h"
#include "SqzLog.h"
#include "TimerUtils.h"
#include "TestWidget.h"
#include "CustomSearchBox.h"
#include "FlexData.h"
#include "ProtocolSchema.h"
#include "TableBuilder.h"
#include "ChainBranch.h"
#include <FluentCard.h>

using namespace std::chrono_literals;
int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    SqzLog::instance().init("./log","chatlog",10,true);
     SqzHub::SetThreadPrefix(MODULE_PREFIX);

    Sqz.CreateWidget("TestWidget");

    Sqz.dump();

    return  a.exec();
}
