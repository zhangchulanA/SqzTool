// SqzService.cpp
#include "SqzService.h"

SqzService::SqzService(QObject* parent) : QObject(parent) {

}
SqzService::~SqzService() {}

// ---------- 通用单例操作 ----------
void SqzService::CallService(const QString& className) {
    SqzHub::Instance().CreateObject(className);
}

void SqzService::KillService(const QString& className) {
    SqzHub::Instance().CloseObj(className);
}

void SqzService::KillServiceLater(const QString& className) {
    SqzHub::Instance().CloseObjLater(className);
}

void SqzService::RebootService(const QString& className) {
    SqzHub::Instance().ResetObj(className);
}

bool SqzService::HasService(const QString& className) const {
    return SqzHub::Instance().IsExist(className);
}

// ---------- 快捷操作 ----------
void SqzService::CallSelfService() { CallService(className());}

void SqzService::KillSelfService() { KillService(className());}
