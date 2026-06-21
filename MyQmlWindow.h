#ifndef MYQMLWINDOW_H
#define MYQMLWINDOW_H

#include <QObject>
#include "SqzQml.h"
class MyQmlWindow : public SqzQml
{
    Q_OBJECT
public:
    explicit MyQmlWindow(QObject *parent = nullptr);
protected:
    QString className() const override { return "MyQmlWindow"; }
    QString qmlSource() const override { return "qrc:/MyWindow.qml"; }
    void onInit() override {

        }

};

#endif // MYQMLWINDOW_H
