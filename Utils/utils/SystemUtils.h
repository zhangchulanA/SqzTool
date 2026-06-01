#ifndef SYSTEMUTILS_H
#define SYSTEMUTILS_H

#include <QString>
#include <QDateTime>

/**
 * @brief 系统工具类（）
 * 支持：Windows 7/8/10/11 + Ubuntu 全系列
 * 纯 Qt5.14 API，无第三方依赖，跨平台通用
 *
 * 功能模块：
 * 1. 程序路径与启动
 * 2. 系统类型与版本判断
 * 3. 系统版本详细信息
 * 4. 计算机硬件信息（CPU/内存/磁盘）
 * 5. 用户与主机信息
 * 6. 常用系统目录
 * 7. 环境变量
 * 8. 系统状态（运行时长、暗色模式）
 * 9. 系统架构（64/32位）
 * 10. 系统控制（关机、重启、注销）
 * 11. 管理员权限判断
 */
class SystemUtils
{
public:
    // ==============================================
    // 1. 程序路径与启动（最常用）
    // ==============================================

    /**
     * @brief  获取程序运行目录（末尾不带斜杠）
     * @example C:/MyApp 或 /home/user/MyApp
     */
    static QString appDirPath();

    /**
     * @brief  获取程序完整路径（含exe）
     */
    static QString appFilePath();

    /**
     * @brief  获取程序名称（不含路径）
     */
    static QString appName();

    /**
     * @brief  获取程序启动参数列表
     */
    static QStringList appArguments();

    /**
     * @brief  获取程序数据目录（通用配置目录）
     */
    static QString appDataPath();

    // ==============================================
    // 2. 系统类型判断
    // ==============================================
    static bool isWindows();
    static bool isLinux();

    // ==============================================
    // 3. 系统版本详细判断（超级实用）
    // ==============================================
    static QString systemName();            // 完整名称：Windows11/Ubuntu22.04
    static QString kernelVersion();         // 内核版本
    static bool isWindows10OrHigher();      // Win10及以上
    static bool isWindows11();               // Win11
    static bool isUbuntu();                  // 是否Ubuntu
    static bool isSystemDarkMode();          // 系统是否暗色模式

    // ==============================================
    // 4. 硬件信息
    // ==============================================
    static int cpuCoreCount();               // CPU逻辑核心数
    static QString cpuArchitecture();        // CPU架构：x86_64/arm64
    static qint64 totalMemoryMB();           // 总内存(MB)
    static qint64 availableMemoryMB();       // 可用内存(MB)

    // ==============================================
    // 5. 磁盘信息
    // ==============================================
    static qint64 diskTotalBytes(const QString &path);  // 磁盘总容量
    static qint64 diskFreeBytes(const QString &path);   // 磁盘剩余容量

    // ==============================================
    // 6. 计算机 & 用户信息
    // ==============================================
    static QString computerName();           // 计算机名
    static QString userName();               // 登录用户名
    static QString systemLanguage();         // 系统语言

    // ==============================================
    // 7. 常用系统目录
    // ==============================================
    static QString desktopPath();            // 桌面
    static QString documentPath();           // 文档
    static QString downloadPath();           // 下载
    static QString tempPath();               // 系统临时目录
    static QString homePath();               // 用户目录

    // ==============================================
    // 8. 环境变量
    // ==============================================
    static QString getEnv(const QString &key);
    static bool setEnv(const QString &key, const QString &value);

    // ==============================================
    // 9. 系统状态
    // ==============================================
    static qint64 systemBootTime();          // 开机时间戳
    static qint64 systemUptimeSec();         // 已运行秒数

    // ==============================================
    // 10. 系统架构
    // ==============================================
    static bool is64BitSystem();             // 是否64位系统
    static bool is32BitSystem();
    static bool isArmArchitecture();         // 是否ARM

    // ==============================================
    // 11. 权限 & 管理员
    // ==============================================
    static bool isRunAsAdmin();              // 是否以管理员/root运行

    // ==============================================
    // 12. 系统控制（关机/重启/注销）
    // ==============================================
    static bool shutdownSystem();            // 关机
    static bool rebootSystem();              // 重启
    static bool logoutUser();                // 注销用户
};

#endif
