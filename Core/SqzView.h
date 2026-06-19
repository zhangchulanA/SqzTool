// SqzView.h
#ifndef SqzView_H
#define SqzView_H

/**
 * @class SqzView
 * @brief 所有窗口/视图界面的基类，继承自 QWidget。
 *        提供与 SqzService 同名的通用单例操作接口（Open/Close/Reset/IsExist），
 *        并额外提供窗口专属操作（Hide/Toggle/SetTop/SetSize/SetPos）。
 *        子类必须实现 className() 纯虚函数，并确保类名与注册名称一致。
 *        推荐使用 SqzHub 创建子类实例，避免直接 new。
 */
#include <QWidget>
#include "SqzHub.h"

class SqzView : public QWidget
{
    Q_OBJECT
    friend class SqzHub;
public:
    explicit SqzView(QWidget* parent = nullptr);
    virtual ~SqzView();

    // ========== 通用单例操作（与 SqzService 同名） ==========
    // 创建或激活指定类名的窗口（若已存在则提到最前）
    void Open(const QString& className);
    // 立即销毁指定类名的单例对象
    void Close(const QString& className);
    // 延迟到下一个事件循环销毁指定类名的单例对象（推荐）
    void CloseLater(const QString& className);
    // 重置指定类名的单例（销毁后重建）
    void Reset(const QString& className);
    // 检查指定类名的单例是否已存在
    bool IsExist(const QString& className) const;

    // ========== 界面专属操作（仅 SqzView 提供） ==========
    // 隐藏指定窗口（不销毁对象）
    void Hide(const QString& className);
    // 切换指定窗口的显示/隐藏状态
    void Toggle(const QString& className);
    // 判断指定窗口是否可见
    bool IsVisible(const QString& className) const;
    // 设置指定窗口置顶或取消置顶
    void SetTop(const QString& className, bool topMost);
    // 设置指定窗口大小
    void SetSize(const QString& className, int w, int h);
    // 设置指定窗口位置
    void SetPos(const QString& className, int x, int y);

    // ---------- 快捷操作（操作自身，无需传参） ----------
    // 创建/激活自身窗口（调用 Open(className())）
    void OpenSelf();
    // 销毁自身单例（调用 Close(className())）
    void CloseSelf();
    // 可自行添加更多快捷方法（如 HideSelf、ToggleSelf 等）

protected:
    // 获取子类名称
    virtual QString className() const{
        return QString(metaObject()->className());
    }
    // ========== 新增生命周期回调 ==========
    // 对象首次创建后调用（此时 UI 已建立，但尚未显示）
    virtual void onInit() {}
    // 对象即将被销毁前调用（通过 SqzHub::CloseObj 等触发）
    virtual void onBeforeClose() {}

};

#endif // SqzView_H
