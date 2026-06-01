#ifndef SqzPluginManager_H
#define SqzPluginManager_H

#include <QObject>
#include <QPluginLoader>
#include <QMap>
#include <QString>
#include "SqzPluginInterface.h"

/**
 * @brief 智能按需插件管理器
 * 核心特性：
 * 1. 预先扫描插件目录，建立 【自定义业务插件名 → 物理文件路径】映射
 * 2. 外部直接用【自己定义的插件名字】按需加载，不用管dll/so文件名
 * 3. 真正懒加载：用一个加载一个，不用不占内存
 * 4. 支持按业务名卸载、查询
 */
class SqzPluginManager : public QObject
{
    Q_OBJECT
public:
    // 全局单例
    static SqzPluginManager& GetInstance();

    // 设置插件根目录
    void setPluginDir(const QString& dir);

    // 【第一步】预扫描：只扫目录、读插件名字，不真正加载常驻内存
    bool scanPluginMeta();

    // 【核心】按【你自定义的业务插件名】按需加载插件
    SqzPluginInterface* loadPluginByBizName(const QString& bizPluginName);

    // 按业务名卸载单个插件
    bool unloadPluginByBizName(const QString& bizPluginName);

    // 卸载所有
    void unloadAllPlugins();

    // 获取已加载插件
    SqzPluginInterface* getLoadedPluginByBizName(const QString& bizPluginName);

private:
    explicit SqzPluginManager(QObject *parent = nullptr);
    ~SqzPluginManager() override;

    SqzPluginManager(const SqzPluginManager&) = delete;
    SqzPluginManager& operator=(const SqzPluginManager&) = delete;

private:
    QString m_pluginDir;

    // 映射1：业务自定义名 → 物理文件全路径（扫描后生成）
    QMap<QString, QString> m_nameToFilePathMap;

    // 映射2：业务自定义名 → 加载器 + 插件实例（真正加载后才存入）
    QMap<QString, QPair<QPluginLoader*, SqzPluginInterface*>> m_loadedPluginMap;
};

#endif
