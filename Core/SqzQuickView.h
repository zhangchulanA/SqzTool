// SqzQuickView.h
#ifndef SqzQuickView_H
#define SqzQuickView_H

#include <QObject>
#include <QtQuick/QQuickView>
#include "SqzHub.h"

/**
 * @class SqzQuickView
 * @brief QML 窗口界面的逻辑基类，继承自 QObject。
 *        提供与 SqzView 同名的接口（Open/Close/Hide/Show/SetTop 等），
 *        内部持有 QQuickView* 用于实际窗口操作。
 *        子类必须实现 className() 纯虚函数，并注册到 SqzHub。
 *        推荐使用 SqzHub::CreateQmlWidget() 创建单例。
 */
class SqzQuickView : public QObject
{
    Q_OBJECT
    friend class SqzHub;

public:
    explicit SqzQuickView(QObject* parent = nullptr);
    virtual ~SqzQuickView();

    // ========== 通用单例操作（与 SqzView/SqzService 同名） ==========
    void Open(const QString& className);
    void Close(const QString& className);
    void CloseLater(const QString& className);
    void Reset(const QString& className);
    bool IsExist(const QString& className) const;

    // ========== 窗口专属操作 ==========
    void Hide(const QString& className);
    void Show(const QString& className);
    void Toggle(const QString& className);
    bool IsVisible(const QString& className) const;
    void SetTop(const QString& className, bool topMost);
    void SetSize(const QString& className, int w, int h);
    void SetPos(const QString& className, int x, int y);

    // ========== 快捷操作（操作自身） ==========
    void OpenSelf();
    void CloseSelf();
    void HideSelf();
    void ShowSelf();

protected:

    // 生命周期回调（由 SqzHub 调用）
    //不要在构造函数或 onInit() 中调用 OpenSelf() 或 CloseSelf() 等依赖虚函数的方法。
    virtual void onInit() {}
    virtual void onBeforeClose() {}

   // 子类必须实现 className() 和 qmlSource()，且返回固定字符串。
    virtual QString className() const = 0;
    virtual QString qmlSource() const = 0;

    // 子类可以重写此方法以自定义 QQuickView 的行为（例如设置透明、初始大小等）
    virtual void customizeWindow(QQuickWindow* window) {
        // 默认什么也不做
        Q_UNUSED(window);
    }

    // 基类提供的方法，用于初始化内部的 m_view
    void initializeView();

    // 提供对 m_view 的只读访问（如果子类需要查询状态）
    QQuickWindow* window() const { return m_window; }

private:
     QQuickWindow* m_window = nullptr;
    bool m_initialized = false;
};

#endif // SqzQuickView_H
