// SqzWidget.cpp
#include "SqzWidget.h"

SqzWidget::SqzWidget(QWidget* parent) : QWidget(parent) {

}
SqzWidget::~SqzWidget() {}

// ---------- 通用单例操作 ----------
void SqzWidget::CallView(const QString& className) {
    SqzHub::Instance().CreateWidget(className);
}

void SqzWidget::KillView(const QString& className) {
    SqzHub::Instance().CloseObj(className);
}

void SqzWidget::KillViewLater(const QString& className) {
    SqzHub::Instance().CloseObjLater(className);
}

void SqzWidget::RebootView(const QString& className) {
    SqzHub::Instance().ResetObj(className);
}

bool SqzWidget::HasView(const QString& className) const {
    return SqzHub::Instance().IsExist(className);
}

// ---------- 界面专属操作 ----------
void SqzWidget::ParkView(const QString& className) {
    SqzHub::Instance().HideWidget(className);
}

void SqzWidget::PopView(const QString &className)
{
    SqzHub::Instance().ShowWidget(className);
}

void SqzWidget::FlipView(const QString& className) {
    SqzHub::Instance().ToggleWidget(className);
}

bool SqzWidget::IsViewUp(const QString& className) const {
    return SqzHub::Instance().IsWidgetVisible(className);
}

void SqzWidget::PinView(const QString& className, bool topMost) {
    SqzHub::Instance().SetWidgetTop(className, topMost);
}

void SqzWidget::ScaleView(const QString& className, int w, int h) {
    SqzHub::Instance().SetWidgetSize(className, w, h);
}

void SqzWidget::MoveView(const QString& className, int x, int y) {
    SqzHub::Instance().SetWidgetPos(className, x, y);
}

// ---------- 快捷操作 ----------KillSelfView
// ---------- 快捷操作 ----------
void SqzWidget::CallSelfView() { CallView(className()); }
void SqzWidget::KillSelfView() { KillView(className()); }
void SqzWidget::ParkSelfView() { ParkView(className()); }
void SqzWidget::PopSelfView()  { PopView(className()); }
