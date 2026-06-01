#ifndef TESTWIDGET_H
#define TESTWIDGET_H

#include <QWidget>

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

private:
    Ui::TestWidget *ui;
};

#endif // TESTWIDGET_H
