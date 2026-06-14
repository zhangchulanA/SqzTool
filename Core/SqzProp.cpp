#include "SqzProp.h"
#include <QDebug>

SqzProp::SqzProp(QObject* parent) : QObject(parent) {}

SqzProp::~SqzProp() = default;

void SqzProp::reg(QObject* obj) {
    if (!obj) return;
    QWriteLocker locker(&m_lock);
    m_objects.insert(obj, obj);
}

void SqzProp::unreg(QObject* obj) {
    if (!obj) return;
    QWriteLocker locker(&m_lock);
    m_objects.remove(obj);
}

QList<QObject*> SqzProp::all() const {
    QReadLocker locker(&m_lock);
    QList<QObject*> list;
    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        if (!it.value().isNull())
            list.append(it.value());
    }
    return list;
}

QList<QObject*> SqzProp::findByClass(const QString& className) const {
    QReadLocker locker(&m_lock);
    QList<QObject*> res;
    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        QObject* obj = it.value();
        if (obj && QString(obj->metaObject()->className()) == className)
            res.append(obj);
    }
    return res;
}

QHash<QString, QVariant> SqzProp::props(QObject* obj) const {
    QHash<QString, QVariant> result;
    if (!obj) return result;
    const QMetaObject* mo = obj->metaObject();
    for (int i = 0; i < mo->propertyCount(); ++i) {
        QMetaProperty prop = mo->property(i);
        if (prop.isReadable()) {
            result[QLatin1String(prop.name())] = prop.read(obj);
        }
    }
    return result;
}

QList<QObject*> SqzProp::findByProp(const QString& name, const QVariant& value) const {
    QReadLocker locker(&m_lock);
    QList<QObject*> res;
    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        QObject* obj = it.value();
        if (!obj) continue;
        if (obj->property(name.toUtf8()) == value)
            res.append(obj);
    }
    return res;
}

bool SqzProp::setProp(QObject* obj, const QString& name, const QVariant& value) {
    QReadLocker locker(&m_lock);
    if (!m_objects.contains(obj))
        return false;
    return obj->setProperty(name.toUtf8(), value);
}

void SqzProp::dump() const {
    QReadLocker locker(&m_lock);
    qDebug() << "=== SqzProp dump ===";
    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        QObject* obj = it.value();
        if (!obj) {
            qDebug() << "  (null)";
            continue;
        }
        qDebug() << obj << "class:" << obj->metaObject()->className();
        auto propMap = props(obj);
        for (auto pit = propMap.begin(); pit != propMap.end(); ++pit) {
            qDebug() << "    " << pit.key() << ":" << pit.value();
        }
    }
}
