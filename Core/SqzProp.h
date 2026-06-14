#pragma once

#include <QObject>
#include <QHash>
#include <QReadWriteLock>
#include <QPointer>
#include <QMetaProperty>
#include "SqzLog.h"
#include "RF.h"

class SqzHub;
class SqzProp : public QObject {
    Q_OBJECT

    friend class SqzHub;
public:
    explicit SqzProp(QObject* parent = nullptr);
    ~SqzProp();

    // 注册/注销
    void reg(QObject* obj);
    void unreg(QObject* obj);

    // 查询
    QList<QObject*> all() const;
    QList<QObject*> findByClass(const QString& className) const;
    QHash<QString, QVariant> props(QObject* obj) const;
    QList<QObject*> findByProp(const QString& name, const QVariant& value) const;

    // 操作
    bool setProp(QObject* obj, const QString& name, const QVariant& value);

    // 调试
    void dump() const;

private:
    mutable QReadWriteLock m_lock;
    QHash<QObject*, QPointer<QObject>> m_objects;
};
