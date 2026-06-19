// SqzService.h
#ifndef SqzService_H
#define SqzService_H

/**
 * @class SqzService
 * @brief 所有业务对象（服务、管理器等）的基类，继承自 QObject。
 *        提供与 SqzView 同名的通用单例操作接口（Open/Close/Reset/IsExist），
 *        但不包含界面相关方法。子类必须实现 className() 纯虚函数，
 *        并确保类名与注册名称一致。推荐使用 SqzHub 创建子类实例。
 */
#include <QObject>
#include "SqzHub.h"

class SqzService : public QObject
{
    Q_OBJECT
    friend class SqzHub;
public:
    explicit SqzService(QObject* parent = nullptr);
    virtual ~SqzService();

    // ========== 通用单例操作（与 SqzView 同名） ==========
    // 创建或获取指定类名的业务单例（若已存在则直接返回）
    void Open(const QString& className);
    // 立即销毁指定类名的单例对象
    void Close(const QString& className);
    // 延迟到下一个事件循环销毁指定类名的单例对象（推荐）
    void CloseLater(const QString& className);
    // 重置指定类名的单例（销毁后重建）
    void Reset(const QString& className);
    // 检查指定类名的单例是否已存在
    bool IsExist(const QString& className) const;
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

#endif // SqzService_H
