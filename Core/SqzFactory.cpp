#include "SqzFactory.h"


thread_local QString SqzFactory::t_prefix;
// 构造函数
SqzFactory::SqzFactory(QObject *parent) : QObject(parent) {}

// 析构函数：释放所有单例对象
SqzFactory::~SqzFactory()
{
    QWriteLocker locker(&GetFactoryLock());
    for (auto it = m_singlePool.begin(); it != m_singlePool.end(); ++it)
    {
        void* ptr = it.value();
        if (m_noArgCreator.contains(it.key()))
        {
            auto& meta = m_noArgCreator[it.key()];
            if (meta.deleter) meta.deleter(ptr);
            else SafeDelete(ptr, meta.isQObject);
        }
        else SafeDelete(ptr, false);
    }
    m_singlePool.clear();
}


void SqzFactory::SetThreadPrefix(const QString &prefix)
{
    t_prefix = prefix;
}

QString SqzFactory::ThreadPrefix()
{
    return t_prefix;
}

QString SqzFactory::maybeAddThreadPrefix(const QString &className)
{
    if (className.contains("::") || SqzFactory::ThreadPrefix().isEmpty())
        return className;
    return SqzFactory::ThreadPrefix() + "::" + className;
}

// 注册无参类
void SqzFactory::RegisterNoArg(const QString& ClassName,
                               std::function<void*()> Creator,
                               std::function<void(void*)> Deleter,
                               bool isQObject)
{
    QString fullname = maybeAddThreadPrefix(ClassName);
    QWriteLocker locker(&GetFactoryLock());
    if (!m_noArgCreator.contains(fullname))
    {
        ClassMeta meta{std::move(Creator), std::move(Deleter), isQObject};
        m_noArgCreator[fullname] = std::move(meta);
    }
    else logwarn << "[Fac] 重复注册类：" << fullname;
}

// 注册带参类
void SqzFactory::RegisterWithArg(const QString& ClassName, CreatorWithArg Func)
{
        QString fullname = maybeAddThreadPrefix(ClassName);
    QWriteLocker locker(&GetFactoryLock());
    if (!m_argCreator.contains(fullname))
        m_argCreator[fullname] = std::move(Func);
    else logwarn << "[Fac] 重复注册带参类：" << fullname;
}

// 创建窗口单例（主线程专用）
QWidget* SqzFactory::CreateWidget(const QString& ClassName)
{
            QString fullname = maybeAddThreadPrefix(ClassName);
    if (QThread::currentThread() != QCoreApplication::instance()->thread())
    {
        logwarn << "[Fac] 禁止子线程操作UI：" << fullname;
        return nullptr;
    }
    {
        QReadLocker locker(&GetFactoryLock());
        if (m_singlePool.contains(fullname))
        {
            QWidget* w = static_cast<QWidget*>(m_singlePool[fullname]);
            w->show(); w->raise(); w->activateWindow();
            return w;
        }
    }
    ClassMeta meta;
    {
        QReadLocker locker(&GetFactoryLock());
        if (!m_noArgCreator.contains(fullname))
        {
            logwarn << "[Fac] 未注册类：" << fullname;
            return nullptr;
        }
        meta = m_noArgCreator[fullname];
    }
    void* raw = meta.creator();
    QWidget* widget = qobject_cast<QWidget*>(static_cast<QObject*>(raw));
    if (!widget)
    {
        if (meta.deleter) meta.deleter(raw); else SafeDelete(raw, meta.isQObject);
        logwarn << "[Fac] 类型转换失败：" << fullname;
        return nullptr;
    }
    {
        QWriteLocker locker(&GetFactoryLock());
        if (m_singlePool.contains(fullname))
        {
            if (meta.deleter) meta.deleter(raw); else SafeDelete(raw, meta.isQObject);
            QWidget* exist = static_cast<QWidget*>(m_singlePool[fullname]);
            exist->show(); exist->raise(); exist->activateWindow();
            return exist;
        }
        m_singlePool[fullname] = widget;
    }
    connect(widget, &QWidget::destroyed, this, [=](){
        QWriteLocker locker(&GetFactoryLock());
        m_singlePool.remove(fullname);
    });
    widget->show(); widget->raise(); widget->activateWindow();
    return widget;
}

// 创建QObject单例
QObject* SqzFactory::CreateObject(const QString& ClassName)
{
                QString fullname = maybeAddThreadPrefix(ClassName);
    {
        QReadLocker locker(&GetFactoryLock());
        if (m_singlePool.contains(fullname))
            return static_cast<QObject*>(m_singlePool[fullname]);
    }
    ClassMeta meta;
    {
        QReadLocker locker(&GetFactoryLock());
        if (!m_noArgCreator.contains(fullname)) return nullptr;
        meta = m_noArgCreator[fullname];
    }
    void* raw = meta.creator();
    QObject* obj = qobject_cast<QObject*>(static_cast<QObject*>(raw));
    if (!obj)
    {
        if (meta.deleter) meta.deleter(raw); else SafeDelete(raw, meta.isQObject);
        return nullptr;
    }
    {
        QWriteLocker locker(&GetFactoryLock());
        if (m_singlePool.contains(fullname))
        {
            if (meta.deleter) meta.deleter(raw); else SafeDelete(raw, meta.isQObject);
            return static_cast<QObject*>(m_singlePool[fullname]);
        }
        m_singlePool[fullname] = obj;
    }
    connect(obj, &QObject::destroyed, this, [=](){
        QWriteLocker locker(&GetFactoryLock());
        m_singlePool.remove(fullname);
    });
    return obj;
}

// 创建普通类单例
void* SqzFactory::CreateRawObj(const QString& ClassName)
{
       QString fullname = maybeAddThreadPrefix(ClassName);
    {
        QReadLocker locker(&GetFactoryLock());
        if (m_singlePool.contains(fullname))
            return m_singlePool[fullname];
    }
    ClassMeta meta;
    {
        QReadLocker locker(&GetFactoryLock());
        if (!m_noArgCreator.contains(fullname)) return nullptr;
        meta = m_noArgCreator[fullname];
    }
    void* raw = meta.creator();
    if (meta.isQObject)
        logwarn << "[Fac] 警告：" << fullname << "是QObject，建议用CreateObject";
    {
        QWriteLocker locker(&GetFactoryLock());
        if (m_singlePool.contains(fullname))
        {
            if (meta.deleter) meta.deleter(raw); else SafeDelete(raw, meta.isQObject);
            return m_singlePool[fullname];
        }
        m_singlePool[fullname] = raw;
    }
    return raw;
}

// 判断对象是否存在
bool SqzFactory::IsExist(const QString& ClassName)
{
           QString fullname = maybeAddThreadPrefix(ClassName);
    QReadLocker locker(&GetFactoryLock());
    return m_singlePool.contains(fullname);
}

// 立即销毁对象
void SqzFactory::CloseObj(const QString& ClassName)
{
    QString fullname = maybeAddThreadPrefix(ClassName);
    ClassMeta meta;
    void* ptr = nullptr;
    {
        QWriteLocker locker(&GetFactoryLock());
        if (!m_singlePool.contains(fullname)) return;
        ptr = m_singlePool.take(fullname);
        if (m_noArgCreator.contains(fullname))
            meta = m_noArgCreator[fullname];
    }
    if (meta.deleter) meta.deleter(ptr);
    else SafeDelete(ptr, meta.isQObject);
}

// 延迟销毁对象
void SqzFactory::CloseObjLater(const QString& ClassName)
{
    QString fullname = maybeAddThreadPrefix(ClassName);

    QTimer::singleShot(0, this, [=](){ CloseObj(fullname); });
}

// 重置对象（销毁+重建）
void SqzFactory::ResetObj(const QString& ClassName)
{
    QString fullname = maybeAddThreadPrefix(ClassName);

    bool isWidget = false, isQObj = false;
    {
        QReadLocker locker(&GetFactoryLock());
        if (m_singlePool.contains(fullname))
        {
            void* ptr = m_singlePool[fullname];
            QObject* obj = static_cast<QObject*>(ptr);
            if (qobject_cast<QWidget*>(obj)) isWidget = true;
            else if (qobject_cast<QObject*>(obj)) isQObj = true;
        }
    }
    CloseObj(fullname);
    if (isWidget) CreateWidget(fullname);
    else if (isQObj) CreateObject(fullname);
    else CreateRawObj(fullname);
}

// 创建临时对象（不入池）
void* SqzFactory::CreateTemp(const QString& ClassName)
{
    QString fullname = maybeAddThreadPrefix(ClassName);
    QReadLocker locker(&GetFactoryLock());
    if (!m_noArgCreator.contains(fullname)) return nullptr;
    return m_noArgCreator[fullname].creator();
}

// 安全释放裸指针
void SqzFactory::SafeDelete(void* Ptr, bool isQObject)
{
    if (!Ptr) return;
    if (isQObject) reinterpret_cast<QObject*>(Ptr)->deleteLater();
    else delete static_cast<char*>(Ptr);
}

// 隐藏窗口
void SqzFactory::HideWidget(const QString& ClassName)
{
    QString fullname = maybeAddThreadPrefix(ClassName);

    if (QThread::currentThread() != QCoreApplication::instance()->thread())
    { logwarn << "[Fac] 子线程不可操作UI：" << fullname; return; }
    QWidget* w = GetWidgetPtr(fullname);
    if (w) w->hide();
}

// 切换窗口显示状态
void SqzFactory::ToggleWidget(const QString& ClassName)
{
        QString fullname = maybeAddThreadPrefix(ClassName);
    if (QThread::currentThread() != QCoreApplication::instance()->thread())
    { logwarn << "[Fac] 子线程不可操作UI：" << fullname; return; }
    QWidget* w = GetWidgetPtr(fullname);
    if (!w) return;
    if (w->isVisible()) w->hide();
    else { w->show(); w->raise(); }
}

// 判断窗口是否可见
bool SqzFactory::IsWidgetVisible(const QString& ClassName)
{
    QString fullname = maybeAddThreadPrefix(ClassName);

    QWidget* w = GetWidgetPtr(fullname);
    return w ? w->isVisible() : false;
}

// 设置窗口置顶
void SqzFactory::SetWidgetTop(const QString& ClassName, bool TopMost)
{
    QString fullname = maybeAddThreadPrefix(ClassName);

    if (QThread::currentThread() != QCoreApplication::instance()->thread())
    { logwarn << "[Fac] 子线程不可操作UI：" << fullname; return; }
    QWidget* w = GetWidgetPtr(fullname);
    if (w) { w->setWindowFlag(Qt::WindowStaysOnTopHint, TopMost); w->show(); }
}

// 设置窗口大小
void SqzFactory::SetWidgetSize(const QString& ClassName, int W, int H)
{
    QString fullname = maybeAddThreadPrefix(ClassName);

    if (QThread::currentThread() != QCoreApplication::instance()->thread())
    { logwarn << "[Fac] 子线程不可操作UI：" << fullname; return; }
    QWidget* w = GetWidgetPtr(fullname);
    if (w) w->resize(W, H);
}

// 设置窗口位置
void SqzFactory::SetWidgetPos(const QString& ClassName, int X, int Y)
{
    QString fullname = maybeAddThreadPrefix(ClassName);

    if (QThread::currentThread() != QCoreApplication::instance()->thread())
    { logwarn << "[Fac] 子线程不可操作UI：" << fullname; return; }
    QWidget* w = GetWidgetPtr(fullname);
    if (w) w->move(X, Y);
}

// 获取窗口指针
QWidget* SqzFactory::GetWidgetPtr(const QString& ClassName)
{
    QString fullname = maybeAddThreadPrefix(ClassName);

    QReadLocker locker(&GetFactoryLock());
    if (!m_singlePool.contains(fullname)) return nullptr;
    return static_cast<QWidget*>(m_singlePool[fullname]);
}

// 隐藏所有窗口
void SqzFactory::HideAllWidget()
{
    if (QThread::currentThread() != QCoreApplication::instance()->thread())
    { logwarn << "[Fac] 子线程不可操作UI"; return; }
    QReadLocker locker(&GetFactoryLock());
    for (auto ptr : m_singlePool)
    {
        QWidget* w = qobject_cast<QWidget*>(static_cast<QObject*>(ptr));
        if (w) w->hide();
    }
}

// 判断类是否已注册
bool SqzFactory::IsClassReg(const QString& ClassName)
{
    QString fullname = maybeAddThreadPrefix(ClassName);

    QReadLocker locker(&GetFactoryLock());
    return m_noArgCreator.contains(fullname) || m_argCreator.contains(fullname);
}

// 判断实例是否为窗口
bool SqzFactory::IsWidgetObj(const QString& ClassName)
{
    QString fullname = maybeAddThreadPrefix(ClassName);

    QReadLocker locker(&GetFactoryLock());
    if (!m_singlePool.contains(fullname)) return false;
    return qobject_cast<QWidget*>(static_cast<QObject*>(m_singlePool[fullname])) != nullptr;
}

// 判断实例是否为QObject
bool SqzFactory::IsQObject(const QString& ClassName)
{
    QString fullname = maybeAddThreadPrefix(ClassName);

    QReadLocker locker(&GetFactoryLock());
    if (!m_singlePool.contains(fullname)) return false;
    return qobject_cast<QObject*>(static_cast<QObject*>(m_singlePool[fullname])) != nullptr;
}

// 获取所有实例类名列表
QStringList SqzFactory::GetExistClassList()
{
    QReadLocker locker(&GetFactoryLock());
    return m_singlePool.keys();
}

// 获取实例总数
int SqzFactory::GetInstanceCount()
{
    QReadLocker locker(&GetFactoryLock());
    return m_singlePool.size();
}

// 打印已注册类名
void SqzFactory::PrintRegClass()
{
    QReadLocker locker(&GetFactoryLock());
    logdebug << "===== [Fac] 已注册类列表 =====";
    for (auto& key : m_noArgCreator.keys())
        logdebug << key;
}

// 销毁所有单例
void SqzFactory::CloseAll()
{
    QList<void*> deleteList;
    QList<ClassMeta> metaList;
    {
        QWriteLocker locker(&GetFactoryLock());
        for (auto it = m_singlePool.begin(); it != m_singlePool.end(); ++it)
        {
            deleteList.append(it.value());
            if (m_noArgCreator.contains(it.key()))
                metaList.append(m_noArgCreator[it.key()]);
            else
                metaList.append({nullptr, nullptr, false});
        }
        m_singlePool.clear();
    }
    for (int i = 0; i < deleteList.size(); ++i)
    {
        if (metaList[i].deleter) metaList[i].deleter(deleteList[i]);
        else SafeDelete(deleteList[i], metaList[i].isQObject);
    }
}

// 清空注册表
void SqzFactory::ClearReg()
{
    QWriteLocker locker(&GetFactoryLock());
    m_noArgCreator.clear();
    m_argCreator.clear();
}

// 带参创建临时QObject
QObject* SqzFactory::CreateObjectByArg(const QString& ClassName, const QVariantList& Args)
{
    QString fullname = maybeAddThreadPrefix(ClassName);

    QReadLocker locker(&GetFactoryLock());
    if (!m_argCreator.contains(fullname)) return nullptr;
    void* raw = m_argCreator[fullname](Args);
    return qobject_cast<QObject*>(static_cast<QObject*>(raw));
}

SqzFactory::PrefixScope::PrefixScope(const QString &prefix) : m_oldPrefix(t_prefix)
{
    t_prefix = prefix;
}

SqzFactory::PrefixScope::~PrefixScope()
{
     t_prefix = m_oldPrefix;
}
