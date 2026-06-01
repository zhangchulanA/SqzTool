#ifndef SQZUDPTEST_H
#define SQZUDPTEST_H
#include "SqzUdpTransfer.h"
#include <QWidget>

namespace Ui {
class SqzUdpTest;
}

class SqzUdpTest : public QWidget
{
    Q_OBJECT

public:
    explicit SqzUdpTest(QWidget *parent = nullptr);
    ~SqzUdpTest();

private slots:
    void on_pushButton_clicked();

private:
    Ui::SqzUdpTest *ui;

        SqzUdpTransfer receiver;
         SqzUdpTransfer sender;
};

#endif // SQZUDPTEST_H
