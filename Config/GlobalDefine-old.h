#ifndef GLOBALDEFINE_H
#define GLOBALDEFINE_H

#include <QObject>
#include <QString>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QSettings>
#include <QTextStream>
#include <QDateTime>
#include <QMap>
#include <QVersionNumber>
#include <Singleton.h>
#include "SqzLog.h"

/**
 * @brief 全局配置命名空间（最终精简稳定版）
 * 功能：软件信息 + GUI配置 + 账号 + 网络 + 数据库 + 日志 + 配置读写
 * 所有配置统一加载一次，全局直接调用，无额外类
 */

/** 配置文件增加用户账号密码 语言和ui分开  日志控制台、文件打印开关 日志文件最大存储 翻译文件位置 软件名称  软件版本 公司名称等等
 *  解析xml文件也放在此处
 * 所有文件均由此类操控 文件均放在此文件夹下
 */

class GlobalDefine :public Singleton<GlobalDefine>
{
public:
    //软件信息
    const QString AppName         = QStringLiteral("Sqz测试软件");  // 软件名称
    const QString AppVersion      = QStringLiteral("V2.0.0");      // 软件版本
    const QString AppCompany      = QStringLiteral("MyLog");        // 公司名称
    const QVersionNumber AppVerNum= QVersionNumber(2, 0, 0);        // 版本号（用于比较）

    //路径体系（绿色便携版，程序所在目录）
     //获取程序运行根目录（绝对路径）
    inline QString getAppRootPath()
    {
        return QCoreApplication::applicationDirPath();
    }

     inline QString pathConfig()  { return getAppRootPath() + "/SqzData/config"; }  // 配置目录
     inline QString pathData()    { return getAppRootPath() + "/SqzData/data";   }  // 数据目录
     inline QString pathLogs()    { return getAppRootPath() + "/SqzData/log";   }  // 日志目录
     inline QString pathTranslators() { return getAppRootPath() + "/SqzData/translator";   }  // 翻译文件目录

    const QString FileConfig = "app_config.ini";     // INI配置文件名


    // ---------------------- 【GUI】 ----------------------
     QString g_MainWidgetName;   //主窗口名称
     QString g_theme;            // 主题 Light / Dark
     int     g_winWidth;         // 窗口宽度
     int     g_winHeight;        // 窗口高度
     bool    g_isFullScreen;     // 是否全屏
    // ---------------------- 【语言】 ----------------------
     QString g_language;         // 启动语言 简体中文、English...
     QString g_deflanguage = "简体中文";      // 默认语言 （缺少语言文件、切换失败、全部禁用，用默认语言兜底）
     QStringList g_languageList;  // 语言种类列表
     QString g_disLanguage;       //禁用语言

    // ---------------------- 【登录 & 账号】 ----------------------
     QString g_oprUser;          // 操作员账号
     QString g_oprPwd;           // 操作元密码
     QString g_adminUser;        // 管理员账号
     QString g_adminPwd;         // 管理员密码
     QString g_defaultLevel;     // 默认登录权限等级

    // ---------------------- 【网络配置】 ----------------------
     QString   g_serverIP;       // ip
     int   g_serverPort;       // 端口
     int     g_netTimeout;       // 超时 ms
     bool    g_autoConnect;      // 开机自动连接

    // ---------------------- 【数据库配置】 ----------------------
     bool    g_dbEnable;         // 是否启用数据库
     QString g_dbPath;           // 数据库文件路径
    // ---------------------- 【日志配置】 ----------------------
     int     g_maxLogSize = 10*1024*1024;       // 单个日志文件最大存储
     QString g_logFileName = "app_Run";      // 日志文件前缀
     bool    g_enableConsole = true;     //是否开启控制台打印
     bool    g_enableFile = false;        //是否开启打印日志文件

    // 配置加载 & 保存（全局只调用一次）
    // 从INI加载所有配置
     void loadConfig()
    {
        QString iniPath = pathConfig() + "/" + FileConfig;

        QSettings set(iniPath, QSettings::IniFormat);
        set.setIniCodec("UTF-8");

        // === GUI ===
        g_MainWidgetName = set.value("GUI/MainWidgetName", "Widget").toString();
        g_theme        = set.value("GUI/Theme",       "Light").toString();
        g_winWidth     = set.value("GUI/Width",       1280).toInt();
        g_winHeight    = set.value("GUI/Height",       720).toInt();
        g_isFullScreen = set.value("GUI/FullScreen", false).toBool();

        // === 语言===
        g_language     = set.value("GUI/Language",      "简体中文").toString();
        g_deflanguage  = set.value("GUI/DefLanguage",   "简体中文").toString();
        g_languageList = set.value("GUI/LanguageList",   QStringList({"简体中文","English"})).toStringList();
        g_disLanguage  = set.value("GUI/DisLanguage",   "").toString();
        // === 账号 ===
        g_oprUser       = set.value("Account/OprUser", "操作员").toString();
        g_oprPwd        = set.value("Account/OprPwd",  "123456").toString();
        g_adminUser    = set.value("Account/AdminUser", "admin").toString();
        g_adminPwd     = set.value("Account/AdminPwd",  "123456").toString();
        g_defaultLevel = set.value("Account/Level",      "操作员").toString();

        // === 网络 ===
        g_serverIP     = set.value("Network/IP",     "127.0.0.1").toString();
        g_serverPort   = set.value("Network/Port",       8080).toInt();
        g_netTimeout   = set.value("Network/Timeout",   15000).toInt();
        g_autoConnect  = set.value("Network/AutoConnect", true).toBool();

        // === 数据库 ===
        g_dbEnable     = set.value("Database/Enable",  false).toBool();
        g_dbPath       = set.value("Database/Path", pathData()+"/app.db").toString();

        // === 日志 ===
        g_maxLogSize   = set.value("LOG/MaxSize",  10*1024*1024).toInt();
        g_logFileName  = set.value("LOG/FileName",  "app_Run").toString();
        g_enableConsole = set.value("LOG/EnableConsole",  true).toBool();
        g_enableFile    = set.value("LOG/EnableFile",  false).toBool();

        // 合法性校验
        if(g_serverPort <1 || g_serverPort>65535) g_serverPort = 8080;


        logdebug<<g_serverIP<<g_serverPort;
    }

     //保存当前所有配置到INI
     void saveConfig()
    {
        QString iniPath = pathConfig() + "/" + FileConfig;
        QSettings set(iniPath, QSettings::IniFormat);
        set.setIniCodec("UTF-8");

        // GUI
        set.setValue("GUI/MainWidgetName", g_MainWidgetName);
        set.setValue("GUI/Theme",         g_theme);
        set.setValue("GUI/Width",         g_winWidth);
        set.setValue("GUI/Height",        g_winHeight);
        set.setValue("GUI/FullScreen",    g_isFullScreen);
        //语言
        set.setValue("Language/Language",      g_language);
        set.setValue("Language/DefLanguage",   g_deflanguage);
        set.setValue("Language/LanguageList",   g_languageList);
        set.setValue("Language/DisLanguage",   g_disLanguage);

        // 账号
        set.setValue("Account/OprUser", g_oprUser);
        set.setValue("Account/OprPwd",  g_oprPwd);
        set.setValue("Account/AdminUser", g_adminUser);
        set.setValue("Account/AdminPwd",  g_adminPwd);
        set.setValue("Account/Level",     g_defaultLevel);

        // 网络
        set.setValue("Network/IP",        g_serverIP);
        set.setValue("Network/Port",      g_serverPort);
        set.setValue("Network/Timeout",   g_netTimeout);
        set.setValue("Network/AutoConnect",g_autoConnect);

        // 数据库
        set.setValue("Database/Enable",   g_dbEnable);
        set.setValue("Database/Path",     g_dbPath);

        // 日志
        set.setValue("LOG/MaxSize",    g_maxLogSize);
        set.setValue("LOG/FileName",   g_logFileName);
        set.setValue("LOG/EnableConsole",  g_enableConsole);
        set.setValue("LOG/EnableFile",  g_enableFile);

    }


    //工具函数（目录、日志）
   //初始化程序目录：config / data / logs / Translators

     void initDirs()
    {
        QStringList dirList = { pathConfig(), pathData(), pathLogs(),pathTranslators()};
        for(const QString& dir : dirList)
        {
            QDir qDir(dir);
            if(!qDir.exists())
                qDir.mkpath(".");
        }
    }
};

#if defined(GlobalInstance)
#undef GlobalInstance
#endif
#define GlobalInstance GlobalDefine::instance()


#endif // GLOBALDEFINE_H


//=======================================使用方法
//void GlobalDefineTest(){
//    // 1. 初始化目录
//    GlobalDefine::initDirs();

//    // 2. 加载配置
//    GlobalDefine::loadConfig();

//    GlobalDefine::saveConfig(); // ← 这句会自动生成 ini 文件！

//    // 3. 直接使用
//    logdebug << "软件名称：" << GlobalDefine::AppName;
//    logdebug << "当前语言：" << GlobalDefine::g_language;
//    logdebug << "服务器IP：" << GlobalDefine::g_serverIP;
//}


