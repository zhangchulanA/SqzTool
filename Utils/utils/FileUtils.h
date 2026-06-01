#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QString>
#include <QFileInfoList>
#include <QDir>
#include <QByteArray>

/**
 * @brief 跨平台超级文件工具类
 * @author Qt 5.14.2
 * @details 支持 Windows / Ubuntu 全平台，提供文件/文件夹创建、拷贝、移动、删除、遍历、读写、校验等全功能
 *          静态函数调用，无需实例化，一行代码完成复杂操作
 */
class FileUtils
{
public:
    FileUtils() = default;
    ~FileUtils() = default;

    // ========================== 1. 创建操作 ==========================
    /**
     * @brief 创建单个文件（自动创建父级目录，已存在不报错）
     * @param filePath 文件完整路径
     * @return 成功true 失败false
     */
    static bool createFile(const QString &filePath);

    /**
     * @brief 递归创建多层文件夹（已存在不报错）
     * @param dirPath 文件夹路径
     * @return 成功true 失败false
     */
    static bool createDir(const QString &dirPath);

    // ========================== 2. 拷贝操作 ==========================
    /**
     * @brief 拷贝单个文件（支持覆盖）
     * @param srcFile 源文件
     * @param destFile 目标文件
     * @param overwrite 是否覆盖
     * @return 成功true 失败false
     */
    static bool copyFile(const QString &srcFile, const QString &destFile, bool overwrite = true);

    /**
     * @brief 递归拷贝整个文件夹（包含所有子目录/文件，支持覆盖）
     * @param srcDir 源文件夹
     * @param destDir 目标文件夹
     * @param overwrite 是否覆盖
     * @return 成功true 失败false
     */
    static bool copyDir(const QString &srcDir, const QString &destDir, bool overwrite = true);

    // ========================== 3. 移动操作（新增） ==========================
    /**
     * @brief 移动文件（剪切功能）
     * @param srcFile 源文件
     * @param destFile 目标文件
     * @param overwrite 是否覆盖
     * @return 成功true 失败false
     */
    static bool moveFile(const QString &srcFile, const QString &destFile, bool overwrite = true);

    /**
     * @brief 移动文件夹（剪切整个目录）
     * @param srcDir 源目录
     * @param destDir 目标目录
     * @param overwrite 是否覆盖
     * @return 成功true 失败false
     */
    static bool moveDir(const QString &srcDir, const QString &destDir, bool overwrite = true);

    // ========================== 4. 重命名（新增） ==========================
    /**
     * @brief 重命名文件/文件夹
     * @param oldPath 原路径
     * @param newName 新名称（不含路径）
     * @return 成功true 失败false
     */
    static bool rename(const QString &oldPath, const QString &newName);

    // ========================== 5. 删除操作 ==========================
    /**
     * @brief 删除单个文件
     * @param filePath 文件路径
     * @return 成功true 失败false
     */
    static bool deleteFile(const QString &filePath);

    /**
     * @brief 递归删除文件夹（删除所有内容）
     * @param dirPath 文件夹路径
     * @return 成功true 失败false
     */
    static bool deleteDir(const QString &dirPath);

    /**
     * @brief 清空文件夹（保留文件夹本身，删除所有内容）
     * @param dirPath 文件夹路径
     * @return 成功true 失败false
     */
    static bool clearDir(const QString &dirPath);

    // ========================== 6. 遍历/查询 ==========================
    /**
     * @brief 获取文件列表
     * @param dirPath 目录
     * @param recursive 是否递归子目录
     * @param filters 过滤规则
     * @return 文件信息列表
     */
    static QFileInfoList getFileList(const QString &dirPath,
                                     bool recursive = false,
                                     QDir::Filters filters = QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    /**
     * @brief 根据后缀名筛选文件
     * @param dirPath 目录
     * @param suffixs 后缀列表 如 QStringList()<<"png"<<"jpg"
     * @param recursive 是否递归
     * @return 文件列表
     */
    static QFileInfoList getFilesBySuffix(const QString &dirPath, const QStringList &suffixs, bool recursive = false);

    /**
     * @brief 统计文件夹总大小（字节，递归统计所有文件）
     * @param dirPath 文件夹
     * @return 总大小
     */
    static qint64 getDirSize(const QString &dirPath);

    // ========================== 7. 文件读写（新增） ==========================
    /**
     * @brief 写入文本文件（自动创建目录，覆盖模式）
     * @param filePath 文件路径
     * @param content 文本内容
     * @param codec 编码 默认UTF-8
     * @return 成功true 失败false
     */
    static bool writeTextFile(const QString &filePath, const QString &content, const QString &codec = "UTF-8");

    /**
     * @brief 读取文本文件
     * @param filePath 文件路径
     * @param codec 编码
     * @return 文件内容 失败返回空
     */
    static QString readTextFile(const QString &filePath, const QString &codec = "UTF-8");

    /**
     * @brief 写入二进制文件
     */
    static bool writeBinaryFile(const QString &filePath, const QByteArray &data);

    /**
     * @brief 读取二进制文件
     */
    static QByteArray readBinaryFile(const QString &filePath);

    // ========================== 8. 路径工具（新增） ==========================
    /**
     * @brief 标准化路径（自动适配Windows/Ubuntu）
     * @param path 原路径
     * @return 标准路径
     */
    static QString formatPath(const QString &path);

    /**
     * @brief 获取文件所在目录
     */
    static QString getFileDir(const QString &filePath);

    /**
     * @brief 获取文件名称（带后缀）
     */
    static QString getFileName(const QString &filePath);

    /**
     * @brief 获取文件名称（无后缀）
     */
    static QString getFileNameWithoutSuffix(const QString &filePath);

    /**
     * @brief 获取文件后缀
     */
    static QString getFileSuffix(const QString &filePath);

    // ========================== 9. 校验/判断（新增） ==========================
    /**
     * @brief 判断文件是否存在
     */
    static bool isFileExist(const QString &filePath);

    /**
     * @brief 判断文件夹是否存在
     */
    static bool isDirExist(const QString &dirPath);

    /**
     * @brief 判断路径是否存在（文件/文件夹均可）
     */
    static bool isPathExist(const QString &path);

    /**
     * @brief 获取文件MD5值（用于校验文件完整性）
     */
    static QString getFileMd5(const QString &filePath);

    // ========================== 10. 系统快捷方式（新增） ==========================
    /**
     * @brief 创建桌面快捷方式（Windows/Linux通用）
     * @param filePath 目标文件路径
     * @param linkName 快捷方式名称
     * @return 成功true
     */
    static bool createDesktopLink(const QString &filePath, const QString &linkName);
};

#endif // FILEUTILS_H
