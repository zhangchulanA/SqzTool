// SqzView.cpp
#include "SqzView.h"

SqzView::SqzView(QWidget* parent) : QWidget(parent) {

}
SqzView::~SqzView() {}

// ---------- 通用单例操作 ----------
void SqzView::Open(const QString& className) {
    SqzHub::Instance().CreateWidget(className);
}

void SqzView::Close(const QString& className) {
    SqzHub::Instance().CloseObj(className);
}

void SqzView::CloseLater(const QString& className) {
    SqzHub::Instance().CloseObjLater(className);
}

void SqzView::Reset(const QString& className) {
    SqzHub::Instance().ResetObj(className);
}

bool SqzView::IsExist(const QString& className) const {
    return SqzHub::Instance().IsExist(className);
}

// ---------- 界面专属操作 ----------
void SqzView::Hide(const QString& className) {
    SqzHub::Instance().HideWidget(className);
}

void SqzView::Toggle(const QString& className) {
    SqzHub::Instance().ToggleWidget(className);
}

bool SqzView::IsVisible(const QString& className) const {
    return SqzHub::Instance().IsWidgetVisible(className);
}

void SqzView::SetTop(const QString& className, bool topMost) {
    SqzHub::Instance().SetWidgetTop(className, topMost);
}

void SqzView::SetSize(const QString& className, int w, int h) {
    SqzHub::Instance().SetWidgetSize(className, w, h);
}

void SqzView::SetPos(const QString& className, int x, int y) {
    SqzHub::Instance().SetWidgetPos(className, x, y);
}

// ---------- 快捷操作 ----------
void SqzView::OpenSelf() {
    Open(className());
}

void SqzView::CloseSelf() {
    Close(className());
}
