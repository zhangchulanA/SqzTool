#ifndef SQZVIEWTEST2_H
#define SQZVIEWTEST2_H

#include <QWidget>
#include "SqzView.h"
namespace Ui {
class SqzViewTest2;
}

class SqzViewTest2 : public SqzView
{
    Q_OBJECT

public:
    explicit SqzViewTest2(SqzView *parent = nullptr);
    ~SqzViewTest2();

private:
    Ui::SqzViewTest2 *ui;
};

#endif // SQZVIEWTEST2_H
