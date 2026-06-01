#include "SystemUtils.h"
#include <QCoreApplication>
#include <QDir>
#include <QSysInfo>
#include <QStandardPaths>
#include <QProcessEnvironment>
#include <QStorageInfo>
#include <QDateTime>
#include <QtGlobal>
#include <QProcess>
#include <QSettings>
#include <QThread>
#include <QLocale>

#ifdef Q_OS_WIN
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600
#include <Windows.h>
#endif
#ifdef Q_OS_LINUX
#include <unistd.h>
#endif




//=============================================================================
// 程序路径
//=============================================================================
QString SystemUtils::appDirPath() {
    return QCoreApplication::applicationDirPath();
}

QString SystemUtils::appFilePath() {
    return QCoreApplication::applicationFilePath();
}

QString SystemUtils::appName() {
    return QCoreApplication::applicationName();
}

QStringList SystemUtils::appArguments() {
    return QCoreApplication::arguments();
}

QString SystemUtils::appDataPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

//=============================================================================
// 系统判断
//=============================================================================
bool SystemUtils::isWindows() {
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

bool SystemUtils::isLinux() {
#ifdef Q_OS_LINUX
    return true;
#else
    return false;
#endif
}

//=============================================================================
// 系统版本
//=============================================================================
QString SystemUtils::systemName() {
    return QSysInfo::prettyProductName();
}

QString SystemUtils::kernelVersion() {
    return QSysInfo::kernelVersion();
}

bool SystemUtils::isWindows10OrHigher() {
#ifdef Q_OS_WIN
    QString ver = QSysInfo::kernelVersion(); // 格式如 10.0.19041
    QStringList parts = ver.split(".");
    if (parts.size() >= 2) {
        int major = parts[0].toInt();
        int minor = parts[1].toInt();
        return (major > 10) || (major == 10 && minor >= 0);
    }
#endif
    return false;
}

bool SystemUtils::isWindows11() {
#ifdef Q_OS_WIN
    // Win11 内核版本 >= 10.0.22000
    QString ver = QSysInfo::kernelVersion();
    QStringList parts = ver.split(".");
    if (parts.size() >= 3) {
        int build = parts[2].toInt();
        return build >= 22000;
    }
#endif
    return false;}

bool SystemUtils::isUbuntu() {
    return systemName().contains("Ubuntu", Qt::CaseInsensitive);
}

bool SystemUtils::isSystemDarkMode() {
#ifdef Q_OS_WIN
    // 需要 #include <QSettings>
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                  QSettings::NativeFormat);
    return reg.value("AppsUseLightTheme", 1).toInt() == 0;
#else
    QString env = getEnv("XDG_CURRENT_DESKTOP").toLower();
    QString theme = getEnv("GTK_THEME").toLower();
    return theme.contains("dark") || env.contains("gnome");
#endif
}

//=============================================================================
// 硬件信息
//=============================================================================
int SystemUtils::cpuCoreCount() {
    // Qt5.14 里是 QThread::idealThreadCount()
    return QThread::idealThreadCount();
}

QString SystemUtils::cpuArchitecture() {
    return QSysInfo::currentCpuArchitecture();
}

qint64 SystemUtils::totalMemoryMB() {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return statex.ullTotalPhys / 1024 / 1024;
#else
    // Linux 下读取 /proc/meminfo
    QFile file("/proc/meminfo");
    if (file.open(QIODevice::ReadOnly)) {
        QString line = file.readLine();
        QRegExp rx("MemTotal:\\s*(\\d+)\\s*kB");
        if (rx.indexIn(line) != -1) {
            return rx.cap(1).toLongLong() / 1024;
        }
    }
    return 0;
#endif
}

qint64 SystemUtils::availableMemoryMB() {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return statex.ullAvailPhys / 1024 / 1024;
#else
    QFile file("/proc/meminfo");
    if (file.open(QIODevice::ReadOnly)) {
        QStringList lines;
        while (!file.atEnd()) lines << file.readLine();
        foreach (QString line, lines) {
            QRegExp rx("MemAvailable:\\s*(\\d+)\\s*kB");
            if (rx.indexIn(line) != -1) {
                return rx.cap(1).toLongLong() / 1024;
            }
        }
    }
    return 0;
#endif
}

//=============================================================================
// 磁盘信息
//=============================================================================
qint64 SystemUtils::diskTotalBytes(const QString &path) {
    QStorageInfo info(path);
    return info.bytesTotal();
}

qint64 SystemUtils::diskFreeBytes(const QString &path) {
    QStorageInfo info(path);
    return info.bytesFree();
}

//=============================================================================
// 计算机 & 用户
//=============================================================================
QString SystemUtils::computerName() {
    return QSysInfo::machineHostName();
}

QString SystemUtils::userName() {
    return QProcessEnvironment::systemEnvironment().value(
        isWindows() ? "USERNAME" : "USER", "unknown");
}

QString SystemUtils::systemLanguage() {
    return QLocale::system().name();
}

//=============================================================================
// 常用目录
//=============================================================================
QString SystemUtils::desktopPath() {
    return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
}

QString SystemUtils::documentPath() {
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

QString SystemUtils::downloadPath() {
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

QString SystemUtils::tempPath() {
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
}

QString SystemUtils::homePath() {
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
}

//=============================================================================
// 环境变量
//=============================================================================
QString SystemUtils::getEnv(const QString &key) {
    return QProcessEnvironment::systemEnvironment().value(key);
}

bool SystemUtils::setEnv(const QString &key, const QString &value) {
    return qputenv(key.toUtf8(), value.toUtf8());
}

//=============================================================================
// 系统运行时长
//=============================================================================
qint64 SystemUtils::systemBootTime() {
    return QDateTime::currentSecsSinceEpoch() - systemUptimeSec();
}

qint64 SystemUtils::systemUptimeSec() {
#ifdef Q_OS_WIN
    return GetTickCount64() / 1000;
#else
    QFile f("/proc/uptime");
    if (f.open(QIODevice::ReadOnly)) {
        QString line = f.readLine();
        return line.split(" ").first().toDouble();
    }
    return 0;
#endif
}

//=============================================================================
// 系统架构
//=============================================================================
bool SystemUtils::is64BitSystem() {
    return QSysInfo::currentCpuArchitecture().contains("x86_64") ||
           QSysInfo::currentCpuArchitecture().contains("arm64");
}

bool SystemUtils::is32BitSystem() {
    return !is64BitSystem();
}

bool SystemUtils::isArmArchitecture() {
    return QSysInfo::currentCpuArchitecture().contains("arm");
}

//=============================================================================
// 管理员权限
//=============================================================================
bool SystemUtils::isRunAsAdmin() {
#ifdef Q_OS_WIN
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, cbSize, &cbSize)) {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    return isAdmin;
#else
    return getuid() == 0;
#endif
}

//=============================================================================
// 关机/重启/注销
//=============================================================================
bool SystemUtils::shutdownSystem() {
#ifdef Q_OS_WIN
    QProcess::startDetached("shutdown -s -t 0");
#else
    QProcess::startDetached("poweroff");
#endif
    return true;
}

bool SystemUtils::rebootSystem() {
#ifdef Q_OS_WIN
    QProcess::startDetached("shutdown -r -t 0");
#else
    QProcess::startDetached("reboot");
#endif
    return true;
}

bool SystemUtils::logoutUser() {
#ifdef Q_OS_WIN
    QProcess::startDetached("shutdown -l");
#else
    QProcess::startDetached("gnome-session-quit --logout --no-prompt");
#endif
    return true;
}
