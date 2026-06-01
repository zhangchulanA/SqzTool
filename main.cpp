
#include <QApplication>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QTextEdit>
#include "SqzTranslator.h"
#include "SqzFactory.h"
#include "SqzThreadPool.h"
#include "SqzLog.h"
#include "GlobalDefine-old.h"
#include "TimerUtils.h"
#include "TestWidget.h"
#include "CustomSearchBox.h"
#include "sqzudptest.h"
#include "FlexData.h"
#include "ProtocolSchema.h"
void initGlobalBusiness(){
//    Logger::instance().init(GlobalInstance->pathLogs(),GlobalInstance->g_logFileName,GlobalInstance->g_maxLogSize,GlobalInstance->g_enableConsole,true);
//    loginfo << "初始化打印日志功能完成";

    //初始化目录和配置文件
//    GlobalInstance->initDirs();
    Fac::SetThreadPrefix(MODULE_PREFIX);
    GlobalInstance->loadConfig();
    loginfo<<"加载配置文件完成";

    //初始化翻译
    for(const QString& str : GlobalInstance->g_languageList)
    {
        QString strPath = GlobalInstance->pathTranslators()+"/"+str+".json";
        SqzTranslator::instance().registerLanguage(str,strPath);
    }
    SqzTranslator::instance().preloadAllLanguages();
    SqzTranslator::instance().disableLanguage(GlobalInstance->g_disLanguage);
    SqzTranslator::instance().setDefaultLanguage(GlobalInstance->g_deflanguage);
    SqzTranslator::instance().safeSwitchLanguage(GlobalInstance->g_language);
    loginfo<<"初始化翻译功能成功"<<"语言列表:"<<GlobalInstance->g_languageList<<"默认语言"<<GlobalInstance->g_deflanguage
          <<"当前语言"<<GlobalInstance->g_language<<"禁用语言"<<GlobalInstance->g_disLanguage;
}
using namespace std::chrono_literals;
int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
        SqzLog::instance().init(GlobalInstance->pathLogs(),GlobalInstance->g_logFileName,GlobalInstance->g_maxLogSize,GlobalInstance->g_enableConsole,true);
    initGlobalBusiness();
    FacIn.CreateWidget("TestWidget");

    FacIn.PrintRegClass();
    FlexData bb;
    bb["k"] = 1;

    logdebug<<bb.value("k").toString();

    // 定义一个简单的协议：温度（16位有符号，系数0.01，偏移0），湿度（16位无符号，系数0.1）
     ProtocolSchema schema;
     schema.addField("temperature", 0, 4, 4, ProtocolSchema::Int,
                     ProtocolSchema::LittleEndian, ProtocolSchema::MsbFirst, false)
           .addField("humidity",    2, 0, 8, ProtocolSchema::UInt,
                     ProtocolSchema::LittleEndian, ProtocolSchema::MsbFirst, false,0.1);

     // 打包：实际温度25.6度，实际湿度80.0%
//     QJsonObject tx;
//     tx["temperature"] = 25.6;
//     tx["humidity"] = 80.0;

//     QString err;
//     QByteArray packed = schema.packToArray(tx, &err);
//     if (!err.isEmpty()) {
//         qDebug() << "Pack error:" << err;
//         return 1;
//     }
//     qDebug() << "Packed hex:" << packed.toHex();
     QString err;
QByteArray packed;
    packed.append(0xF1);
   packed.append(0x01);
   packed.append(0xFF);
   packed.append(0xFF);
   packed.append(0xFF);
     // 解析回 JSON
     QJsonObject rx = schema.parse(packed,  &err);
     if (!err.isEmpty()) {
         qDebug() << "Parse error:" << err;
         return 1;
     }
     qDebug().noquote() << "Parsed JSON:" << QJsonDocument(rx).toJson();

     // 检查是否有字段重叠（可选）
     QStringList overlaps = schema.checkOverlap();
     if (!overlaps.isEmpty())
         qDebug() << "Field overlaps detected:" << overlaps;
//    qDebug().noquote() << QJsonDocument(rx).toJson();
//    SqzUdpTest s;
//    s.show();
//    QMainWindow window;
//     QWidget *central = new QWidget(&window);
//     window.setCentralWidget(central);
//     QVBoxLayout *layout = new QVBoxLayout(central);

//     // 创建搜索框，使用独立的 history key
//     CustomSearchBox *searchBox = new CustomSearchBox(central, "main_search");
//     searchBox->setPlaceholderText("Enter keywords...");
//     // 可选：设置预置补全词
//     // searchBox->setCompletionWords({"Qt", "C++", "Linux", "Windows"});

//     QTextEdit *resultArea = new QTextEdit(central);
//     resultArea->setReadOnly(true);

//     layout->addWidget(searchBox);
//     layout->addWidget(resultArea);

//     // 连接搜索信号
//     QObject::connect(searchBox, &CustomSearchBox::searchTriggered,
//         [resultArea](const QString &keyword) {
//             resultArea->append("Searching for: " + keyword);
//             // 这里执行实际搜索逻辑...
//         });

//     window.resize(500, 400);
//     window.show();
    return  a.exec();
}
