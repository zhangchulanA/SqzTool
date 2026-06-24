// SqzWidget.h
#ifndef SqzWidget_H
#define SqzWidget_H

/**
 * @class SqzWidget
 * @brief 所有窗口/视图界面的基类，继承自 QWidget。
 *        提供与 SqzService 同名的通用单例操作接），
 *        并额外提供窗口专属操作。
 *        子类必须实现 className() 纯虚函数，并确保类名与注册名称一致。
 *        推荐使用 SqzHub 创建子类实例，避免直接 new。
 */
#include <QWidget>
#include "SqzHub.h"

class SqzWidget : public QWidget
{
    Q_OBJECT
    friend class SqzHub;
public:
    explicit SqzWidget(QWidget* parent = nullptr);
    virtual ~SqzWidget();


    // ========== 通用单例操作（与 SqzWidget/SqzService 同名） ==========

    /// @brief 呼叫视图（创建并激活窗口/服务实例）
    void CallView(const QString& className);

    /// @brief 杀掉视图（立即销毁指定实例）
    void KillView(const QString& className);

    /// @brief 延迟杀掉视图（下一事件循环安全销毁）
    void KillViewLater(const QString& className);

    /// @brief 重启视图（销毁后重新创建实例）
    void RebootView(const QString& className);

    /// @brief 是否有视图（检查指定实例是否存在）
    bool HasView(const QString& className) const;

    // ========== 窗口专属操作 ==========

    /// @brief 停放视图（隐藏窗口但不销毁）
    void ParkView(const QString& className);

    /// @brief 弹出视图（显示窗口并提升到最前）
    void PopView(const QString& className);

    /// @brief 翻转视图（切换窗口的显示/隐藏状态）
    void FlipView(const QString& className);

    /// @brief 视图是否上线（检查窗口是否可见）
    bool IsViewUp(const QString& className) const;

    /// @brief 固定视图（设置窗口置顶或取消置顶）
    void PinView(const QString& className, bool topMost);

    /// @brief 缩放视图（调整窗口大小）
    void ScaleView(const QString& className, int w, int h);

    /// @brief 移动视图（调整窗口位置）
    void MoveView(const QString& className, int x, int y);

    // ========== 快捷操作（操作自身） ==========

    /// @brief 呼叫自身视图
    void CallSelfView();

    /// @brief 杀掉自身视图
    void KillSelfView();

    /// @brief 停放自身视图
    void ParkSelfView();

    /// @brief 弹出自身视图
    void PopSelfView();

protected:
    /**
     * 生命周期回调（由 SqzHub 调用）
     * 不要在构造函数或 onInit() 中调用 CallSelfView() 或 KillSelfView() 等依赖虚函数的方法。
     **/
    /// @brief 对象首次创建后回调
    virtual void onInit() {}

    /// @brief 对象即将销毁前回调
    ///
    virtual void onClose() {}

    /// @brief 获取子类名称（必须实现）
    virtual QString className() const = 0;

    /// @brief 获取qml路径（必须实现）
    virtual QString qmlSource() const = 0;

};

#endif // SqzWidget_H
