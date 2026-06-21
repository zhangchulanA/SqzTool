#ifndef UIHELPER_H
#define UIHELPER_H

#include <QWidget>
#include <QLabel>
#include <QScreen>
#include <QImage>
#include <QDialog>
#include <QMessageBox>
#include <QFont>
#include <QRect>
#include <QTimer>
#include <QObject>
#include <QGraphicsOpacityEffect>

// 弹窗提示类型
enum class TipType
{
    Success,
    Warn,
    Error,
    Info
};

// Toast悬浮窗弹出位置（6个方位）
enum class ToastPos
{
    TopLeft,     // 左上角
    TopRight,    // 右上角
    BottomLeft,  // 左下角
    BottomRight, // 右下角
    TopCenter,   // 顶部居中
    BottomCenter // 底部居中
};

// Toast全局悬浮提示控件（内部私有）
class ToastWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToastWidget(const QString &msg, TipType type, ToastPos pos, int showMs = 2000);
    void showToast();

protected:
    // 窗口透明无边框绘制
    void paintEvent(QPaintEvent *event) override;

private slots:
    // 计算窗口最终坐标
    void calcToastPos();
    // 淡出动画
    void startFadeOut();
private:
    QLabel *m_label;
    ToastPos m_pos;
    TipType m_type;
    int m_showTime;
    QGraphicsOpacityEffect *m_opacityEffect;
    QTimer *m_fadeTimer;
    QTimer *m_delayTimer;

    // 根据TipType获取对应背景色
    QString getBgColor(TipType type);

};

// UI静态工具类，全部静态方法
class UiHelper
{
public:
    // ==================== 窗口通用 ====================
    /// 窗口居中（支持主屏幕/指定父窗口）Qt5.12兼容
    static void centerWindow(QWidget *w, QWidget *parent = nullptr);
    /// 设置窗口最小/最大尺寸
    static void setWindowLimit(QWidget *w, int minW, int minH, int maxW = -1, int maxH = -1);
    /// 弹出标准模态提示框（MessageBox）
    static void showTip(const QString &msg, TipType type = TipType::Info, QWidget *parent = nullptr);
    /// 确认弹窗，返回true=点击确定
    static bool showConfirm(const QString &msg, const QString &title = "提示", QWidget *parent = nullptr);

    // ==================== 控件样式快捷 ====================
    /// 给控件设置圆角+背景色+边框
    static void setWidgetStyle(QWidget *w, int radius, const QString &bgColor,
                               int borderW = 0, const QString &borderColor = "#cccccc");
    /// 一键设置字体大小加粗
    static void setWidgetFont(QWidget *w, int fontSize, bool bold = false);
    /// 清空控件所有样式表
    static void clearStyle(QWidget *w);

    // ==================== 图片处理 ====================
    /// QImage 等比例缩放
    static QImage scaleImage(const QImage &src, int w, int h);
    /// 图片转Base64字符串
    static QString imageToBase64(const QImage &img, const QString &fmt = "png");
    /// Base64转回图片
    static QImage base64ToImage(const QString &base64Str);
    /// Label加载图片自适应填充
    static void setLabelImage(QLabel *lab, const QString &imgPath, bool keepRatio = true);

    // ==================== 屏幕DPI适配（Qt5.12兼容，无widget->screen()） ====================
    /// 获取主屏幕分辨率
    static QSize getScreenSize();
    /// 获取当前窗口所在屏幕缩放系数
    static qreal getScreenScale(QWidget *w = nullptr);

    // ==================== 新增全局悬浮Toast提示 ====================
    /// 弹出无阻塞悬浮Toast
    /// @param msg 提示文本
    /// @param type 提示类型（成功/警告/错误/普通）
    /// @param pos 弹出方位
    /// @param showMs 停留毫秒，默认2000ms
    static void showToast(const QString &msg, TipType type = TipType::Info,
                          ToastPos pos = ToastPos::BottomRight, int showMs = 2000);

private:
    // 禁止实例化静态工具类
    UiHelper() = default;
};

#endif // UIHELPER_H
