#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QObject>
#include "SqzQuickView.h"
class LoginWindow : public SqzQuickView
{
    Q_OBJECT
public:
    explicit LoginWindow(QObject *parent = nullptr);

protected:
    // 必须实现：返回类名（必须与注册宏一致）
    QString className() const override { return "LoginWindow"; }

    // 必须实现：返回 QML 文件路径
    QString qmlSource() const override { return "qrc:/LoginWindow.qml"; }

    // 可选重写：自定义 QQuickView 外观
    void customizeWindow(QQuickWindow* window) override {
        window->setTitle("登录");
        window->resize(400, 300);
    }

    // 可选重写：初始化完成后的逻辑
    void onInit() override {
        qDebug() << "LoginWindow initialized";
    }

    // 可选重写：销毁前的清理
    void onBeforeClose() override {
        qDebug() << "LoginWindow closing";
    }

};

#endif // LOGINWINDOW_H
