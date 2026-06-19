#ifndef SQZVIEWTEST_H
#define SQZVIEWTEST_H

#include <QWidget>
#include "SqzView.h"
namespace Ui {
class SqzViewTest;
}

class SqzViewTest : public SqzView
{
    Q_OBJECT

public:
    explicit SqzViewTest(SqzView *parent = nullptr);
    ~SqzViewTest();

private:
    Ui::SqzViewTest *ui;
};

#endif // SQZVIEWTEST_H
