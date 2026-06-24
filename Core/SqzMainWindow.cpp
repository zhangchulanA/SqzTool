// SqzMainWindow.cpp
#include "SqzMainWindow.h"

SqzMainWindow::SqzMainWindow(QWidget *parent) : QMainWindow(parent) {

}
SqzMainWindow::~SqzMainWindow() {}

// ---------- 通用单例操作 ----------
void SqzMainWindow::CallView(const QString& className) {
    SqzHub::Instance().CreateWidget(className);
}

void SqzMainWindow::KillView(const QString& className) {
    SqzHub::Instance().CloseObj(className);
}

void SqzMainWindow::KillViewLater(const QString& className) {
    SqzHub::Instance().CloseObjLater(className);
}

void SqzMainWindow::RebootView(const QString& className) {
    SqzHub::Instance().ResetObj(className);
}

bool SqzMainWindow::HasView(const QString& className) const {
    return SqzHub::Instance().IsExist(className);
}

// ---------- 界面专属操作 ----------
void SqzMainWindow::ParkView(const QString& className) {
    SqzHub::Instance().HideWidget(className);
}

void SqzMainWindow::PopView(const QString &className)
{
    SqzHub::Instance().ShowWidget(className);
}

void SqzMainWindow::FlipView(const QString& className) {
    SqzHub::Instance().ToggleWidget(className);
}

bool SqzMainWindow::IsViewUp(const QString& className) const {
    return SqzHub::Instance().IsWidgetVisible(className);
}

void SqzMainWindow::PinView(const QString& className, bool topMost) {
    SqzHub::Instance().SetWidgetTop(className, topMost);
}

void SqzMainWindow::ScaleView(const QString& className, int w, int h) {
    SqzHub::Instance().SetWidgetSize(className, w, h);
}

void SqzMainWindow::MoveView(const QString& className, int x, int y) {
    SqzHub::Instance().SetWidgetPos(className, x, y);
}

// ---------- 快捷操作 ----------KillSelfView
// ---------- 快捷操作 ----------
void SqzMainWindow::CallSelfView() { CallView(className()); }
void SqzMainWindow::KillSelfView() { KillView(className()); }
void SqzMainWindow::ParkSelfView() { ParkView(className()); }
void SqzMainWindow::PopSelfView()  { PopView(className()); }
