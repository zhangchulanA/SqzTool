#ifndef TESTWIDGET_H
#define TESTWIDGET_H

#include <QWidget>
#include "UdpServer.h"
namespace Ui {
class TestWidget;
}

class TestWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TestWidget(QWidget *parent = nullptr);
    ~TestWidget();
    Q_INVOKABLE void M_InitTrUi();
private slots:
    void on_pushButton_clicked();
     void ReceiveUdpData(const QVariantList& list);
private:
    Ui::TestWidget *ui;

    UdpServer * m_udp = nullptr;
};

#endif // TESTWIDGET_H
