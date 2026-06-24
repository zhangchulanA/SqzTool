// SqzQuick.cpp
#include "SqzQuick.h"

#include <QQmlComponent>

SqzQuick::SqzQuick(QObject* parent) : QObject(parent) {}
SqzQuick::~SqzQuick() {

}

// ---------- 通用单例操作 ----------
void SqzQuick::CallView(const QString& className) {
    SqzHub::Instance().CreateQuick(className);
}
void SqzQuick::KillView(const QString& className) {
    SqzHub::Instance().CloseObj(className);
}
void SqzQuick::KillViewLater(const QString& className) {
    SqzHub::Instance().CloseObjLater(className);
}
void SqzQuick::RebootView(const QString& className) {
    SqzHub::Instance().ResetObj(className);
}
bool SqzQuick::HasView(const QString& className) const {
    return SqzHub::Instance().IsExist(className);
}

// ---------- 窗口专属操作 ----------
void SqzQuick::ParkView(const QString& className) {
    SqzHub::Instance().HideQuick(className);
}

void SqzQuick::PopView(const QString& className) {
    SqzHub::Instance().ShowQuick(className);
}

void SqzQuick::FlipView(const QString& className) {
    SqzHub::Instance().ToggleQuick(className);
}

bool SqzQuick::IsViewUp(const QString& className) const {
    return SqzHub::Instance().IsQuickVisible(className);
}

void SqzQuick::PinView(const QString& className, bool topMost) {
    SqzHub::Instance().SetQuickTop(className, topMost);
}

void SqzQuick::ScaleView(const QString& className, int w, int h) {
    SqzHub::Instance().SetQuickSize(className, w, h);
}

void SqzQuick::MoveView(const QString& className, int x, int y) {
    SqzHub::Instance().SetQuickPos(className, x, y);
}

// ---------- 快捷操作 ----------
void SqzQuick::CallSelfView() { CallView(className()); }
void SqzQuick::KillSelfView() { KillView(className()); }
void SqzQuick::ParkSelfView() { ParkView(className()); }
void SqzQuick::PopSelfView()  { PopView(className()); }


void SqzQuick::initializeView()
{
    if (m_initialized) return;

    QQmlEngine* engine = SqzHub::Instance().qmlEngine();
    if (!engine) {
        logwarn << "QML engine not available!";
        return;
    }

    QQmlComponent component(engine, QUrl(qmlSource()));
    if (component.isError()) {
        logwarn << "Failed to load QML:" << component.errors();
        return;
    }

    QObject* obj = component.create();
    if (!obj) {
        logwarn << "Failed to create QML object!";
        return;
    }

    m_window = qobject_cast<QQuickWindow*>(obj);
    if (!m_window) {
        logwarn << "QML root is not a QQuickWindow!";
        delete obj;
        return;
    }

    // 设置 C++ 所有权，防止 QML 引擎自动销毁
    QQmlEngine::setObjectOwnership(m_window, QQmlEngine::CppOwnership);

    m_initialized = true;

    onInit();
}
