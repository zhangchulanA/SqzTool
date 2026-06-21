
#include <QApplication>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QSpinBox>
#include <QTextEdit>
#include "SqzTranslator.h"
#include "SqzHub.h"
#include "ThreadPool.h"
#include "SqzLog.h"
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
#include "UiHelper.h"
using namespace std::chrono_literals;
int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    SqzLog::instance().init("./log","chatlog",10,true);
    SqzHub::SetThreadPrefix(MODULE_PREFIX);
    Sqz.PrintRegClass();
    //    Sqz.CreateWidget("TestWidget");
//    Sqz.CreateWidget("SqzViewTest");

//    Sqz.CreateQmlWidget("LoginWindow");


    // 右下角成功提示（默认）
    UiHelper::showToast("保存成功", TipType::Success,ToastPos::TopLeft);
    // 运行事件循环
    int ret = a.exec();

    // 程序退出前主动清理（可选）
    Sqz.CloseAll();
    return  ret;
}
