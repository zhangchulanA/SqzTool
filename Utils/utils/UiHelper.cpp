#include "UiHelper.h"
#include <QByteArray>
#include <QBuffer>
#include <QPixmap>
#include <QGuiApplication>
#include <QPainter>
#include <QList>
#include <QVBoxLayout>

// ===================== ToastWidget 实现 =====================
ToastWidget::ToastWidget(const QString &msg, TipType type, ToastPos pos, int showMs)
    : QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool),
      m_label(nullptr),
      m_pos(pos),
      m_type(type),
      m_showTime(showMs),
      m_opacityEffect(nullptr),
      m_fadeTimer(nullptr),
      m_delayTimer(nullptr)
{
    // 基础窗口配置：无边框、置顶、背景透明
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowOpacity(1.0);
    setFixedSize(QSize(320, 60));

    // 文本标签
    m_label = new QLabel(msg, this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setWordWrap(true);
    m_label->setStyleSheet("color:#ffffff;font-size:14px;");
    m_label->setContentsMargins(10, 5, 10, 5);

    // 布局填充整个窗口
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_label);

    // 透明度动画效果
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(1.0);
    setGraphicsEffect(m_opacityEffect);

    // 延时定时器：停留指定时间后开始淡出
    m_delayTimer = new QTimer(this);
    m_delayTimer->setSingleShot(true);
    connect(m_delayTimer, &QTimer::timeout, this, &ToastWidget::startFadeOut);

    // 淡出动画定时器
    m_fadeTimer = new QTimer(this);
    m_fadeTimer->setInterval(30);
    connect(m_fadeTimer, &QTimer::timeout, this, [this](){
        qreal opacity = m_opacityEffect->opacity() - 0.05;
        if (opacity <= 0)
        {
            m_fadeTimer->stop();
            this->close();
            this->deleteLater();
        }
        m_opacityEffect->setOpacity(opacity);
    });

    // 启动延时关闭
    m_delayTimer->start(m_showTime);
}

void ToastWidget::showToast()
{
    calcToastPos();
    this->show();
    this->raise();
}

void ToastWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    // 圆角矩形背景，修复：传入m_type而不是m_pos
    QRect rect = this->rect();
    rect.adjust(2, 2, -2, -2);
    painter.setBrush(QColor(getBgColor(m_type)));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect, 8, 8);
}

QString ToastWidget::getBgColor(TipType type)
{
    switch (type)
    {
    case TipType::Success: return "#00b42a";
    case TipType::Warn:    return "#ff7d00";
    case TipType::Error:   return "#f53f3f";
    case TipType::Info:    return "#1677ff";
    default: return "#1677ff";
    }
}

void ToastWidget::calcToastPos()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenRect = screen->availableGeometry();
    QSize toastSize = this->size();
    const int margin = 20; // 距离屏幕边距

    int x = 0, y = 0;
    switch (m_pos)
    {
    case ToastPos::TopLeft:
        x = screenRect.x() + margin;
        y = screenRect.y() + margin;
        break;
    case ToastPos::TopRight:
        x = screenRect.right() - toastSize.width() - margin;
        y = screenRect.y() + margin;
        break;
    case ToastPos::BottomLeft:
        x = screenRect.x() + margin;
        y = screenRect.bottom() - toastSize.height() - margin;
        break;
    case ToastPos::BottomRight:
        x = screenRect.right() - toastSize.width() - margin;
        y = screenRect.bottom() - toastSize.height() - margin;
        break;
    case ToastPos::TopCenter:
        x = screenRect.left() + (screenRect.width() - toastSize.width()) / 2;
        y = screenRect.y() + margin;
        break;
    case ToastPos::BottomCenter:
        x = screenRect.left() + (screenRect.width() - toastSize.width()) / 2;
        y = screenRect.bottom() - toastSize.height() - margin;
        break;
    }
    this->move(x, y);
}

void ToastWidget::startFadeOut()
{
    m_fadeTimer->start();
}

// ===================== UiHelper 静态工具实现 =====================
void UiHelper::centerWindow(QWidget *w, QWidget *parent)
{
    if (!w) return;
    QRect targetRect;
    if (parent && parent->isVisible())
        targetRect = parent->frameGeometry();
    else
        targetRect = QGuiApplication::primaryScreen()->availableGeometry();

    int x = targetRect.x() + (targetRect.width() - w->width()) / 2;
    int y = targetRect.y() + (targetRect.height() - w->height()) / 2;
    w->move(x, y);
}

void UiHelper::setWindowLimit(QWidget *w, int minW, int minH, int maxW, int maxH)
{
    if (!w) return;
    w->setMinimumSize(minW, minH);
    if (maxW > 0 && maxH > 0)
        w->setMaximumSize(maxW, maxH);
}

void UiHelper::showTip(const QString &msg, TipType type, QWidget *parent)
{
    QMessageBox::Icon icon = QMessageBox::Information;
    switch (type)
    {
    case TipType::Success: icon = QMessageBox::Information; break;
    case TipType::Warn:    icon = QMessageBox::Warning; break;
    case TipType::Error:   icon = QMessageBox::Critical; break;
    case TipType::Info:    icon = QMessageBox::Information; break;
    }
    QMessageBox msgBox(parent);
    msgBox.setIcon(icon);
    msgBox.setText(msg);
    msgBox.setWindowTitle("提示");
    msgBox.exec();
}

bool UiHelper::showConfirm(const QString &msg, const QString &title, QWidget *parent)
{
    auto ret = QMessageBox::question(parent, title, msg, QMessageBox::Yes | QMessageBox::No);
    return ret == QMessageBox::Yes;
}

void UiHelper::setWidgetStyle(QWidget *w, int radius, const QString &bgColor, int borderW, const QString &borderColor)
{
    if (!w) return;
    QString qss = QString("background-color:%1;border-radius:%2px;").arg(bgColor).arg(radius);
    if (borderW > 0)
        qss += QString("border:%1px solid %2;").arg(borderW).arg(borderColor);
    w->setStyleSheet(qss);
}

void UiHelper::setWidgetFont(QWidget *w, int fontSize, bool bold)
{
    if (!w) return;
    QFont f = w->font();
    f.setPointSize(fontSize);
    f.setBold(bold);
    w->setFont(f);
}

void UiHelper::clearStyle(QWidget *w)
{
    if (w) w->setStyleSheet("");
}

QImage UiHelper::scaleImage(const QImage &src, int w, int h)
{
    return src.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QString UiHelper::imageToBase64(const QImage &img, const QString &fmt)
{
    QByteArray buf;
    QBuffer buffer(&buf);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, fmt.toLatin1().data());
    return buf.toBase64();
}

QImage UiHelper::base64ToImage(const QString &base64Str)
{
    QByteArray data = QByteArray::fromBase64(base64Str.toUtf8());
    QImage img;
    img.loadFromData(data);
    return img;
}

void UiHelper::setLabelImage(QLabel *lab, const QString &imgPath, bool keepRatio)
{
    if (!lab) return;
    QPixmap pix(imgPath);
    if (pix.isNull()) return;
    if (keepRatio)
        lab->setPixmap(pix.scaled(lab->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    else
        lab->setPixmap(pix.scaled(lab->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

QSize UiHelper::getScreenSize()
{
    return QGuiApplication::primaryScreen()->availableSize();
}

qreal UiHelper::getScreenScale(QWidget *w)
{
    // 默认取主屏
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!w)
        return screen->devicePixelRatio();

    QWidget* topWin = w->window();
    if (!topWin)
        return screen->devicePixelRatio();

    QRect winGeo = topWin->frameGeometry();
    QList<QScreen*> screenList = QGuiApplication::screens();
    // 遍历多屏幕匹配窗口所在屏幕
    for (QScreen* scr : screenList)
    {
        if (scr->geometry().intersects(winGeo))
        {
            screen = scr;
            break;
        }
    }
    return screen->devicePixelRatio();
}

void UiHelper::showToast(const QString &msg, TipType type, ToastPos pos, int showMs)
{
    // 新建悬浮窗，内部自动deleteLater释放无内存泄漏
    ToastWidget *toast = new ToastWidget(msg, type, pos, showMs);
    toast->showToast();
}
