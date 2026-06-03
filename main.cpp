
#include <QApplication>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QSpinBox>
#include <QTextEdit>
#include "SqzTranslator.h"
#include "SqzFactory.h"
#include "SqzThreadPool.h"
#include "SqzLog.h"
#include "GlobalDefine-old.h"
#include "TimerUtils.h"
#include "TestWidget.h"
#include "CustomSearchBox.h"
#include "FlexData.h"
#include "ProtocolSchema.h"
#include "TableBuilder.h"
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
    SqzLog::instance().init("./log","chatlog");
//    initGlobalBusiness();
//    FacIn.CreateWidget("TestWidget");


//     1. 注册列模板
       TableBuilder::registerColumnTemplate("ID", [](TableBuilder& builder, const QString& title) {
           builder.addColumn(title)
                  .setWidth(80)
                  .setAlignment(Qt::AlignCenter)
                  .setEditable(false)
                  .setSortable(true);
       });
       TableBuilder::registerColumnTemplate("Name", [](TableBuilder& builder, const QString& title) {
           builder.addColumn(title)
                  .setStretch(1)
                  .setEditable(true);
       });

       // 2. 创建 Builder 并配置
       TableBuilder builder;
       builder.addColumnFromTemplate("ID", "工号")
              .addColumnFromTemplate("Name", "姓名")
              .addColumn("年龄")
                   .setWidth(100)
                   .setAlignment(Qt::AlignCenter)
                   .setEditor([](QWidget* p) { return new QSpinBox(p); })
              .addColumn("工资")
                   .setWidth(120)
                   .setAlignment(Qt::AlignRight)
                   .setEditable(true);

       // 3. 样式与主题
       builder.applyTheme(TableStyle::LightTheme)
              .setRowHeight(30)
              .setAlternatingRowColors(true);

       // 4. 数据绑定
       QList<QList<QVariant>> data = {
           {1001, "张三", 28, 8500},
           {1002, "李四", 32, 9200},
           {1003, "王五", 26, 7800}
       };
       builder.setDataSource(data);

       // 5. 启用全局过滤
       builder.enableGlobalFilter(false);
       builder.setFilterPlaceholder("输入关键字过滤...");

       // 6. 启用列拖拽排序
//       builder.setDragDropEnabled(false, true);

       // 7. 构建表格
       QTableView* table = builder.build();

       // 8. 动态列管理演示（插入新列）
       builder.insertColumn(2, "部门");

       // 9. 导出/导入列配置
       QJsonObject cfg = builder.exportColumnConfig();
       qDebug() << "Exported config:" << cfg;

       // 10. 显示窗口
       QMainWindow win;
       QWidget* central = new QWidget;
       QVBoxLayout* layout = new QVBoxLayout(central);
       layout->addWidget(table);
       win.setCentralWidget(central);
       win.resize(800, 400);
       win.show();

#if 0
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
#endif
    return  a.exec();
}
