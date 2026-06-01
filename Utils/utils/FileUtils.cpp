#include "FileUtils.h"
#include <QFile>
#include <QDirIterator>
#include <QDebug>
#include <QTextStream>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QFileInfo>

// ========================== 创建 ==========================
bool FileUtils::createFile(const QString &filePath)
{
    if (isFileExist(filePath)) {
        qDebug() << "文件已存在：" << filePath;
        return true;
    }

    QFileInfo info(filePath);
    if (!createDir(info.path())) {
        qDebug() << "创建文件失败：无法创建父目录";
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "创建文件失败：" << file.errorString();
        return false;
    }
    file.close();
    return true;
}

bool FileUtils::createDir(const QString &dirPath)
{
    QDir dir;
    if (dir.exists(dirPath)) return true;
    return dir.mkpath(dirPath);
}

// ========================== 拷贝 ==========================
bool FileUtils::copyFile(const QString &srcFile, const QString &destFile, bool overwrite)
{
    if (!isFileExist(srcFile)) return false;

    QFileInfo destInfo(destFile);
    createDir(destInfo.path());

    if (overwrite && isFileExist(destFile)) QFile::remove(destFile);
    return QFile::copy(srcFile, destFile);
}

bool FileUtils::copyDir(const QString &srcDir, const QString &destDir, bool overwrite)
{
    if (!isDirExist(srcDir)) return false;
    createDir(destDir);

    QDirIterator it(srcDir, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString src = it.next();
        QFileInfo info = it.fileInfo();
        QString rel = src.mid(srcDir.size() + 1);
        QString dst = destDir + "/" + rel;

        if (info.isDir()) createDir(dst);
        else copyFile(src, dst, overwrite);
    }
    return true;
}

// ========================== 移动 ==========================
bool FileUtils::moveFile(const QString &srcFile, const QString &destFile, bool overwrite)
{
    if (!isFileExist(srcFile)) return false;
    if (overwrite && isFileExist(destFile)) deleteFile(destFile);
    return QFile::rename(srcFile, destFile);
}

bool FileUtils::moveDir(const QString &srcDir, const QString &destDir, bool overwrite)
{
    if (!isDirExist(srcDir)) {
        qDebug() << "moveDir 失败：源目录不存在" << srcDir;
        return false;
    }

    // 如果目标目录存在且允许覆盖，先删除它
    if (overwrite && isDirExist(destDir)) {
        if (!deleteDir(destDir)) {
            qDebug() << "moveDir 失败：无法删除已存在的目标目录" << destDir;
            return false;
        }
    }

    // 创建 QDir 对象，调用非静态的 rename 方法
    QDir dir;
    if (!dir.rename(srcDir, destDir)) {
        qDebug() << "moveDir 失败：QDir::rename 返回 false";
        return false;
    }

    return true;
}

// ========================== 重命名 ==========================
bool FileUtils::rename(const QString &oldPath, const QString &newName)
{
    if (!isPathExist(oldPath)) return false;
    QFileInfo info(oldPath);
    QString newPath = info.path() + "/" + newName;
    return QFile::rename(oldPath, newPath);
}

// ========================== 删除 ==========================
bool FileUtils::deleteFile(const QString &filePath)
{
    if (!isFileExist(filePath)) return true;
    return QFile::remove(filePath);
}

bool FileUtils::deleteDir(const QString &dirPath)
{
    if (!isDirExist(dirPath)) return true;
    clearDir(dirPath);
    return QDir(dirPath).rmdir(dirPath);
}

bool FileUtils::clearDir(const QString &dirPath)
{
    QDir dir(dirPath);
    for (auto info : dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        if (info.isDir()) deleteDir(info.filePath());
        else deleteFile(info.filePath());
    }
    return true;
}

// ========================== 遍历 ==========================
QFileInfoList FileUtils::getFileList(const QString &dirPath, bool recursive, QDir::Filters filters)
{
    QFileInfoList list;
    if (!isDirExist(dirPath)) return list;

    if (recursive) {
        QDirIterator it(dirPath, filters, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            list.append(it.fileInfo());
        }
    } else {
        list = QDir(dirPath).entryInfoList(filters);
    }
    return list;
}

QFileInfoList FileUtils::getFilesBySuffix(const QString &dirPath, const QStringList &suffixs, bool recursive)
{
    QFileInfoList all = getFileList(dirPath, recursive, QDir::Files | QDir::NoDotAndDotDot);
    QFileInfoList res;
    for (auto &info : all) {
        if (suffixs.contains(info.suffix().toLower())) res << info;
    }
    return res;
}

qint64 FileUtils::getDirSize(const QString &dirPath)
{
    qint64 size = 0;
    QDirIterator it(dirPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next(); // 先移动迭代器
        size += it.fileInfo().size(); // 再获取当前文件的信息和大小
    }
    return size;
}

// ========================== 读写 ==========================
bool FileUtils::writeTextFile(const QString &filePath, const QString &content, const QString &codec)
{
    if (!createFile(filePath)) return false;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out.setCodec(codec.toUtf8());
    out << content;
    file.close();
    return true;
}

QString FileUtils::readTextFile(const QString &filePath, const QString &codec)
{
    if (!isFileExist(filePath)) return "";
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return "";

    QTextStream in(&file);
    in.setCodec(codec.toUtf8());
    QString content = in.readAll();
    file.close();
    return content;
}

bool FileUtils::writeBinaryFile(const QString &filePath, const QByteArray &data)
{
    if (!createFile(filePath)) return false;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(data);
    file.close();
    return true;
}

QByteArray FileUtils::readBinaryFile(const QString &filePath)
{
    if (!isFileExist(filePath)) return {};
    QFile file(filePath);
    file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll();
    file.close();
    return data;
}

// ========================== 路径工具 ==========================
QString FileUtils::formatPath(const QString &path)
{
    return QDir::cleanPath(path).replace("/", QDir::separator());
}

QString FileUtils::getFileDir(const QString &filePath)
{
    return QFileInfo(filePath).path();
}

QString FileUtils::getFileName(const QString &filePath)
{
    return QFileInfo(filePath).fileName();
}

QString FileUtils::getFileNameWithoutSuffix(const QString &filePath)
{
    return QFileInfo(filePath).baseName();
}

QString FileUtils::getFileSuffix(const QString &filePath)
{
    return QFileInfo(filePath).suffix();
}

// ========================== 校验 ==========================
bool FileUtils::isFileExist(const QString &filePath)
{
    return QFile::exists(filePath) && QFileInfo(filePath).isFile();
}

bool FileUtils::isDirExist(const QString &dirPath)
{
    return QDir(dirPath).exists();
}

bool FileUtils::isPathExist(const QString &path)
{
    return QFile::exists(path);
}

QString FileUtils::getFileMd5(const QString &filePath)
{
    if (!isFileExist(filePath)) return "";
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return "";
    QByteArray hash = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Md5);
    file.close();
    return hash.toHex().toUpper();
}

// ========================== 快捷方式 ==========================
bool FileUtils::createDesktopLink(const QString &filePath, const QString &linkName)
{
    QString desk = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString lnk = desk + "/" + linkName + ".lnk";
#ifdef Q_OS_WIN
    return QFile::copy(filePath, lnk);
#else
    return QFile::link(filePath, lnk);
#endif
}
