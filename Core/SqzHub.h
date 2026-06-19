#ifndef SqzHub_H
#define SqzHub_H

#include <functional>
#include <QVariantList>
#include <QWidget>
#include <QStringList>
#include <QCoreApplication>
#include <type_traits>
#include "SqzProp.h"

/**
 * @class SqzHub
 * @brief 通用对象工厂类（生产级单例工厂）- 通过字符串类名创建、管理、销毁各类对象
 *
 * @details
 * SqzHub 是一个全功能的对象工厂，支持通过类名字符串动态创建 QWidget 窗口、
 * QObject 业务类以及普通 C++ 类。它采用单例模式管理所有对象，提供线程安全的
 * 对象池，并自动处理对象的生命周期。
 *
 * 为保证在不知情情况下引入的各个库的注册名字不重复 需要在pro里写上前缀
 * DEFINES += MODULE_PREFIX=\\\"---\\\"  ---为前缀名 建议为项目名
 *
 * ==== 核心特性 ====
 * 1. 纯字符串操作：所有接口使用类名（QString）而非模板或类型，适合脚本化调用。
 * 2. 单例对象池：每个类全局只有一个实例，重复调用返回同一对象。
 * 3. 自动类型识别：注册时通过 std::is_base_of 自动识别 QObject 派生类。
 * 4. 读写锁线程安全：读操作并发无阻塞，写操作互斥执行。
 * 5. UI线程安全：所有窗口操作自动检查主线程，子线程调用返回警告。
 * 6. 自动内存管理：窗口关闭或对象销毁时自动从对象池移除。
 *
 * ==== 使用场景 ====
 * - 动态创建窗口：通过配置文件名打开窗口，无需包含具体头文件。
 * - 全局唯一服务：日志、配置、网络管理等核心单例服务。
 * - 脚本化调用：通过字符串动态创建对象，支持热插拔模块。
 * - 跨模块通信：模块间通过类名获取对象，解耦编译依赖。
 * - 账号切换场景：使用 ResetObj 一键重置用户相关服务。
 * -
 *
 * ==== 注册方式 ====
 * 在每个类的 .cpp 文件末尾添加一行宏即可完成自动注册：
 * - QWidget 窗口类：REGISTER_CLASS_NO_ARG(LoginDialog)
 * - QObject 业务类：REGISTER_CLASS_NO_ARG(UserManager)
 * - 普通 C++ 类：REGISTER_CLASS_NO_ARG(DataCache)
 * - 带参构造类：REGISTER_CLASS_WITH_ARG(MyDialog)
 *
 * ==== 使用示例 ====
 * // 注册类（在 .cpp 末尾）
 * REGISTER_CLASS_NO_ARG(LoginDialog)
 *
 * // 创建/显示窗口
 * SqzHub::Instance().CreateWidget("LoginDialog");
 *
 * // 获取服务单例
 * UserManager* mgr = (UserManager*)SqzHub::Instance().CreateObject("UserManager");
 *
 * // 窗口操作
 * SqzHub::Instance().ToggleWidget("LoginDialog");
 * SqzHub::Instance().HideWidget("LoginDialog");
 * SqzHub::Instance().SetWidgetTop("MainWindow", true);
 *
 * // 对象生命周期管理
 * SqzHub::Instance().CloseObj("TempDialog");
 * SqzHub::Instance().ResetObj("UserManager");
 *
 * // 调试工具
 * SqzHub::Instance().PrintRegClass();
 * int count = SqzHub::Instance().GetInstanceCount();
 *
 * ==== 注意事项 ====
 * - 所有窗口操作必须在主线程调用，内部会自动检查。
 * - 注册宏必须放在 .cpp 文件末尾，且确保该类已完整定义。
 * - 静态库中使用时可能需要额外链接器选项。
 * - 临时对象（CreateTemp）不会自动释放，用完必须手动调用 SafeDelete。
 * - 带参构造类必须提供接收 QVariantList 的构造函数。
 */


// 辅助宏：拼接前缀和类名
#define MAKE_FULL_NAME(Cls) \
    (QString(MODULE_PREFIX).isEmpty() ? QString(#Cls) : QString(MODULE_PREFIX) + "::" + #Cls)


/**
 * @brief 获取全局读写锁（函数内静态局部变量，线程安全，避免静态初始化顺序问题）
 * @return 全局读写锁的引用
 * @note 使用 QReadLocker 进行读操作，QWriteLocker 进行写操作
 */
inline QReadWriteLock& GetFactoryLock()
{
    static QReadWriteLock factoryLock;
    return factoryLock;
}

/**
 * @brief 类元数据结构体，用于存储类的创建函数、销毁函数和类型信息
 * @field creator 创建函数指针，返回新创建的对象指针（void*）
 * @field deleter 销毁函数指针，负责释放对象内存
 * @field isQObject 标记该类是否为 QObject 派生类，用于安全释放
 */
struct ClassMeta
{
    std::function<void*()> creator;
    std::function<void(void* ptr)> deleter;
    bool isQObject = false;
};

/**
 * @brief 带参构造函数类型定义
 * @param args 参数列表（const引用避免拷贝）
 * @return 创建的对象指针
 */
using CreatorWithArg = std::function<void*(const QVariantList& args)>;


class SqzHub : public SqzProp
{
    Q_OBJECT

public:
    // RAII 辅助类：在作用域内临时修改当前线程的前缀
    class PrefixScope {
    public:
        explicit PrefixScope(const QString& prefix);
        ~PrefixScope();
    private:
        QString m_oldPrefix;
    };

    static void SetThreadPrefix(const QString& prefix);   // 设置当前线程前缀
    static QString ThreadPrefix();

private:
    static thread_local QString t_prefix;

    static QString maybeAddThreadPrefix(const QString& className);
public:
    // ==================== 单例入口 ====================

    /**
     * @brief 获取工厂全局单例实例
     * @return SqzHub 实例的引用
     * @usage SqzHub::Instance().CreateWidget("MyDialog");
     */
    static SqzHub& Instance()
    {
        static SqzHub factoryInstance;
        return factoryInstance;
    }

    // ==================== 类注册接口 ====================

    /**
     * @brief 注册无参构造类
     * @param ClassName 类名字符串
     * @param Creator 创建函数 lambda: []()->void*{ return new Cls(); }
     * @param Deleter 销毁函数 lambda: [](void* p){ delete static_cast<Cls*>(p); }
     * @param isQObject 是否为 QObject 派生类（宏自动传入）
     * @usage 通常通过 REGISTER_CLASS_NO_ARG 宏调用
     */
    void RegisterNoArg(const QString& ClassName,
                       std::function<void*()> Creator,
                       std::function<void(void*)> Deleter = nullptr,
                       bool isQObject = false);

    /**
     * @brief 注册带参构造类
     * @param ClassName 类名字符串
     * @param Func 带参构造函数，接收 const QVariantList& 参数
     * @usage 通过 REGISTER_CLASS_WITH_ARG 宏调用
     */
    void RegisterWithArg(const QString& ClassName, CreatorWithArg Func);

    // ==================== 核心创建接口 ====================

    /**
     * @brief 获取或创建窗口单例（仅主线程可用）
     * @param ClassName 已注册的窗口类名
     * @return 窗口指针，失败返回 nullptr
     * @usage QWidget* w = SqzHub::Instance().CreateWidget("LoginDialog");
     */
    QWidget* CreateWidget(const QString& ClassName);

    /**
     * @brief 获取或创建 QObject 业务类单例
     * @param ClassName 已注册的 QObject 类名
     * @return 业务对象指针
     * @usage MyService* svc = (MyService*)SqzHub::Instance().CreateObject("MyService");
     */
    QObject* CreateObject(const QString& ClassName);

    /**
     * @brief 获取或创建纯 C++ 普通类单例
     * @param ClassName 已注册的普通 C++ 类名
     * @return 裸对象指针
     * @warning 如果传入的是 QObject 派生类，会打印警告
     * @usage MyData* data = (MyData*)SqzHub::Instance().CreateRawObj("MyData");
     */
    void* CreateRawObj(const QString& ClassName);
    /**
     * @brief 获取或创建带参数窗口单例（仅主线程可用）
     * @param ClassName 已注册的窗口类名
     * @return 窗口指针，失败返回 nullptr
     * @usage QWidget* w = SqzHub::Instance().CreateWidget("LoginDialog");
     */
    QWidget* CreateWidgetWithArg(const QString& ClassName, const QVariantList& args);
    /**
     * @brief 获取或创建带参数 QObject 业务类单例
     * @param ClassName 已注册的 QObject 类名
     * @return 业务对象指针
     * @usage MyService* svc = (MyService*)SqzHub::Instance().CreateObject("MyService");
     */
    QObject* CreateObjectWithArg(const QString& ClassName, const QVariantList& args);

    // ==================== 对象生命周期管理 ====================

    /**
     * @brief 判断单例对象是否已创建
     * @param ClassName 类名
     * @return true=已存在，false=不存在
     * @usage if(SqzHub::Instance().IsExist("MyDialog")) { ... }
     */
    bool IsExist(const QString& ClassName);

    /**
     * @brief 销毁指定单例对象（立即执行）
     * @param ClassName 类名
     * @usage SqzHub::Instance().CloseObj("LoginDialog");
     */
    void CloseObj(const QString& ClassName);

    /**
     * @brief 延迟销毁对象（下一个事件循环执行）
     * @param ClassName 类名
     * @usage SqzHub::Instance().CloseObjLater("TempDialog");
     */
    void CloseObjLater(const QString& ClassName);
    /**
     * @brief 删除临时对象
     * @param ClassName
     * @param ptr
     */
    void DeleteTemp(const QString& ClassName, void* ptr);
    /**
     * @brief 重置对象（销毁后重新创建）
     * @param ClassName 类名
     * @usage SqzHub::Instance().ResetObj("UserManager");
     */
    void ResetObj(const QString& ClassName);

    /**
     * @brief 创建临时对象（不进入单例池，需手动释放）
     * @param ClassName 类名
     * @return 对象裸指针
     * @usage void* tmp = SqzHub::Instance().CreateTemp("TempData"); SqzHub::SafeDelete(tmp);
     */
    void* CreateTemp(const QString& ClassName);

    /**
     * @brief 安全释放任意裸指针（静态方法）
     * @param Ptr 要释放的指针
     * @param isQObject 是否为 QObject 派生类
     * @usage SqzHub::SafeDelete(ptr);
     */
    static void SafeDelete(void* Ptr, bool isQObject = false, bool immediate = false);

    // ==================== 窗口专属操作接口 ====================

    /**
     * @brief 隐藏指定窗口（不销毁对象）
     * @param ClassName 窗口类名
     * @usage SqzHub::Instance().HideWidget("LoginDialog");
     */
    void HideWidget(const QString& ClassName);

    /**
     * @brief 切换窗口的显示/隐藏状态
     * @param ClassName 窗口类名
     * @usage SqzHub::Instance().ToggleWidget("SettingDialog");
     */
    void ToggleWidget(const QString& ClassName);

    /**
     * @brief 判断窗口是否当前可见
     * @param ClassName 窗口类名
     * @return true=可见，false=隐藏或不存在
     * @usage if(SqzHub::Instance().IsWidgetVisible("MyDialog")) { ... }
     */
    bool IsWidgetVisible(const QString& ClassName);

    /**
     * @brief 设置窗口置顶或取消置顶
     * @param ClassName 窗口类名
     * @param TopMost true=置顶，false=取消置顶
     * @usage SqzHub::Instance().SetWidgetTop("MainWindow", true);
     */
    void SetWidgetTop(const QString& ClassName, bool TopMost);

    /**
     * @brief 设置窗口大小
     * @param ClassName 窗口类名
     * @param W 宽度
     * @param H 高度
     * @usage SqzHub::Instance().SetWidgetSize("MyDialog", 800, 600);
     */
    void SetWidgetSize(const QString& ClassName, int W, int H);

    /**
     * @brief 设置窗口在屏幕上的位置
     * @param ClassName 窗口类名
     * @param X 横坐标
     * @param Y 纵坐标
     * @usage SqzHub::Instance().SetWidgetPos("MyDialog", 100, 200);
     */
    void SetWidgetPos(const QString& ClassName, int X, int Y);

    /**
     * @brief 获取窗口原生指针
     * @param ClassName 窗口类名
     * @return 窗口指针，不存在返回 nullptr
     * @usage MyWidget* w = (MyWidget*)SqzHub::Instance().GetWidgetPtr("MyWidget");
     */
    QWidget* GetWidgetPtr(const QString& ClassName);

    /**
     * @brief 隐藏所有已打开的窗口
     * @usage SqzHub::Instance().HideAllWidget();
     */
    void HideAllWidget();

    // ==================== 类型判断 & 调试工具 ====================

    /**
     * @brief 判断类是否已完成注册
     * @param ClassName 类名
     * @return true=已注册，false=未注册
     * @usage if(!SqzHub::Instance().IsClassReg("MyClass")) { qDebug() << "未注册"; }
     */
    bool IsClassReg(const QString& ClassName);

    /**
     * @brief 判断已创建的实例是否为 QWidget 窗口类型
     * @param ClassName 类名
     * @return true=是窗口，false=不是或不存在
     * @usage if(SqzHub::Instance().IsWidgetObj("MyDialog")) { ... }
     */
    bool IsWidgetObj(const QString& ClassName);

    /**
     * @brief 判断已创建的实例是否为 QObject 类型
     * @param ClassName 类名
     * @return true=是QObject，false=不是或不存在
     * @usage if(SqzHub::Instance().IsQObject("MyService")) { ... }
     */
    bool IsQObject(const QString& ClassName);

    /**
     * @brief 获取所有已创建实例的类名列表
     * @return 类名列表
     * @usage QStringList list = SqzHub::Instance().GetExistClassList();
     */
    QStringList GetExistClassList();

    /**
     * @brief 获取当前已创建的实例总数
     * @return 实例数量
     * @usage int count = SqzHub::Instance().GetInstanceCount();
     */
    int GetInstanceCount();

    /**
     * @brief 打印所有已注册的类名到日志（调试专用）
     * @usage SqzHub::Instance().PrintRegClass();
     */
    void PrintRegClass();

    // ==================== 全局批量操作 ====================

    /**
     * @brief 销毁所有单例对象
     * @usage SqzHub::Instance().CloseAll();
     */
    void CloseAll();

    /**
     * @brief 清空构造器注册表（极少使用）
     * @warning 清空后所有类将无法创建新实例
     * @usage SqzHub::Instance().ClearReg();
     */
    void ClearReg();

    // ==================== 带参创建扩展接口 ====================

    /**
     * @brief 创建带参数的临时 QObject 对象
     * @param ClassName 类名（需用 REGISTER_CLASS_WITH_ARG 注册）
     * @param Args 参数列表
     * @return 对象指针，失败返回 nullptr
     * @usage QVariantList args; args << 100 << 200; QObject* obj = SqzHub::Instance().CreateObjectByArg("MyDialog", args);
     */
    QObject* CreateObjectByArg(const QString& ClassName, const QVariantList& Args);

private:
    explicit SqzHub(QObject *parent = nullptr);
    ~SqzHub() override;
    Q_DISABLE_COPY(SqzHub)

private:
    /**
     * @brief 内部创建核心函数，处理单例池、线程检查、注册和信号连接
     * @param ClassName      类名（已含前缀）
     * @param validator      验证回调，用于检查创建的对象是否满足类型要求（如 qobject_cast 检查）
     * @param isWidget       是否窗口类型，若是则自动 show/raise/activateWindow
     * @return 创建的对象指针（void*），失败返回 nullptr
     */
    void* createInternal(const QString& ClassName,
                         std::function<bool(void*)> validator,
                         bool isWidget);

    QHash<QString, ClassMeta>      m_noArgCreator;
    QHash<QString, CreatorWithArg> m_argCreator;
    QHash<QString, void*>          m_singlePool;
};


#ifdef _MSC_VER
#define FORCE_LINK_THIS(x) __pragma(comment(linker, "/include:" #x))
#else
#define FORCE_LINK_THIS(x) __attribute__((used))
#endif

/**
 * @brief 无参类自动注册宏（支持模块前缀）
 * @param Cls 类名（无需引号）
 * @note 如果定义了 MODULE_PREFIX，则注册的完整类名为 "MODULE_PREFIX::Cls"
 *       否则注册为 "Cls"
 */
#define SQZ_HUB(Cls) \
static void _auto_reg_##Cls() \
{ \
    constexpr bool isQObj = std::is_base_of<QObject, Cls>::value; \
    SqzHub::Instance().RegisterNoArg(MAKE_FULL_NAME(Cls), \
        []()->void*{ return new Cls(); }, \
        [](void* ptr){ delete static_cast<Cls*>(ptr); }, \
        isQObj \
    ); \
} \
FORCE_LINK_THIS(_reg_flag_##Cls) static bool _reg_flag_##Cls = (_auto_reg_##Cls(), true);

/**
 * @brief 带参类自动注册宏（支持模块前缀）
 * @param Cls 类名（无需引号）
 * @note 类的构造函数需接收 QVariantList 参数
 */
#define SQZ_HUB_ARG(Cls) \
static void _auto_reg_arg_##Cls() \
{ \
    SqzHub::Instance().RegisterWithArg(MAKE_FULL_NAME(Cls), [](const QVariantList& Args)->void*{ \
        return new Cls(Args); \
    }); \
} \
FORCE_LINK_THIS(_reg_flag_arg_##Cls) static bool _reg_flag_arg_##Cls = (_auto_reg_arg_##Cls(), true);

#define Sqz   SqzHub::Instance()

#endif // SqzHub_H
