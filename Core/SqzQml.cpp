// SqzQml.cpp
#include "SqzQml.h"

#include <QQmlComponent>

SqzQml::SqzQml(QObject* parent) : QObject(parent) {}
SqzQml::~SqzQml() {

}

// ---------- 通用单例操作 ----------
void SqzQml::Open(const QString& className) {
    SqzHub::Instance().CreateQmlWidget(className);
}
void SqzQml::Close(const QString& className) {
    SqzHub::Instance().CloseObj(className);
}
void SqzQml::CloseLater(const QString& className) {
    SqzHub::Instance().CloseObjLater(className);
}
void SqzQml::Reset(const QString& className) {
    SqzHub::Instance().ResetObj(className);
}
bool SqzQml::IsExist(const QString& className) const {
    return SqzHub::Instance().IsExist(className);
}

// ---------- 窗口专属操作 ----------
void SqzQml::Hide(const QString& className) {
    QObject* obj = SqzHub::Instance().GetQmlObject(className);
    if (obj) {
        SqzQml* view = qobject_cast<SqzQml*>(obj);
        if (view && view->m_window) {
            view->m_window->hide();
        }
    }
}
void SqzQml::Show(const QString& className) {
    QObject* obj = SqzHub::Instance().GetQmlObject(className);
    if (obj) {
        SqzQml* view = qobject_cast<SqzQml*>(obj);
        if (view && view->m_window) {
            view->m_window->show();
            view->m_window->raise();
            view->m_window->requestActivate();
        }
    }
}
void SqzQml::Toggle(const QString& className) {
    QObject* obj = SqzHub::Instance().GetQmlObject(className);

    if (obj) {
        SqzQml* view = qobject_cast<SqzQml*>(obj);
        bool visible = obj->property("visible").toBool();
        if (visible) {
            if (view && view->m_window)
            view->m_window->hide();
        } else {   
            if (view && view->m_window) {
                view->m_window->show();
                view->m_window->raise();
                view->m_window->requestActivate();
            }
        }
    }
}
bool SqzQml::IsVisible(const QString& className) const {
    QObject* obj = SqzHub::Instance().GetQmlObject(className);
    if (obj) {
        SqzQml* view = qobject_cast<SqzQml*>(obj);
        if (view && view->m_window) {
            return view->m_window->isVisible();
        }
    }
    return false;
}
void SqzQml::SetTop(const QString& className, bool topMost) {
    QObject* obj = SqzHub::Instance().GetQmlObject(className);
    if (obj) {
        SqzQml* view = qobject_cast<SqzQml*>(obj);
        if (view && view->m_window) {
            view->m_window->setFlag(Qt::WindowStaysOnTopHint, topMost);
        }
    }
}
void SqzQml::SetSize(const QString& className, int w, int h) {
    QObject* obj = SqzHub::Instance().GetQmlObject(className);
    if (obj) {
        obj->setProperty("width", w);
        obj->setProperty("height", h);
    }
}
void SqzQml::SetPos(const QString& className, int x, int y) {
    QObject* obj = SqzHub::Instance().GetQmlObject(className);
    if (obj) {
        obj->setProperty("x", x);
        obj->setProperty("y", y);
    }
}

// ---------- 快捷操作 ----------
void SqzQml::OpenSelf() { Open(className()); }
void SqzQml::CloseSelf() { Close(className()); }
void SqzQml::HideSelf() { Hide(className()); }
void SqzQml::ShowSelf() { Show(className()); }


void SqzQml::initializeView()
{
    if (m_initialized) return;

    QQmlEngine* engine = SqzHub::Instance().qmlEngine();
    if (!engine) {
        qWarning() << "QML engine not available!";
        return;
    }

    QQmlComponent component(engine, QUrl(qmlSource()));
    if (component.isError()) {
        qWarning() << "Failed to load QML:" << component.errors();
        return;
    }

    QObject* obj = component.create();
    if (!obj) {
        qWarning() << "Failed to create QML object!";
        return;
    }

    m_window = qobject_cast<QQuickWindow*>(obj);
    if (!m_window) {
        qWarning() << "QML root is not a QQuickWindow!";
        delete obj;
        return;
    }

    // 设置 C++ 所有权，防止 QML 引擎自动销毁
    QQmlEngine::setObjectOwnership(m_window, QQmlEngine::CppOwnership);

    // 调用子类自定义
    customizeWindow(m_window);

    m_initialized = true;
    onInit();
}
