// SqzService.cpp
#include "SqzService.h"

SqzService::SqzService(QObject* parent) : QObject(parent) {
    setObjectName(className());
}
SqzService::~SqzService() {}

// ---------- 通用单例操作 ----------
void SqzService::Open(const QString& className) {
    SqzHub::Instance().CreateObject(className);
}

void SqzService::Close(const QString& className) {
    SqzHub::Instance().CloseObj(className);
}

void SqzService::CloseLater(const QString& className) {
    SqzHub::Instance().CloseObjLater(className);
}

void SqzService::Reset(const QString& className) {
    SqzHub::Instance().ResetObj(className);
}

bool SqzService::IsExist(const QString& className) const {
    return SqzHub::Instance().IsExist(className);
}

// ---------- 快捷操作 ----------
void SqzService::OpenSelf() {
    Open(className());
}

void SqzService::CloseSelf() {
    Close(className());
}
