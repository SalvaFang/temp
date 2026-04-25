#include "aroverlay.h"
#include <QDebug>
#include <QKeyEvent>
#include <QApplication>
#include <QDesktopWidget>

AROverlay::AROverlay(QWidget *parent)
    : QWidget(parent)
    , m_framePosition(400, 300)
    , m_frameSize(300, 200)
    , m_frameVisible(false)  // 默认隐藏
    , m_frameColor(0, 255, 0, 200)
    , m_crosshairColor(255, 255, 0, 180)
    , m_frameThickness(3)
    , m_updateTimer(nullptr)  // 先初始化为null
{
    qDebug() << "AROverlay构造函数开始";

    try {
        // 创建定时器但不立即启动
        m_updateTimer = new QTimer(this);

        // 设置透明窗口
        setupTransparentWindow();

        // 连接定时器信号
        connect(m_updateTimer, &QTimer::timeout, this, &AROverlay::updateFrame);

        // 延迟启动定时器
        QTimer::singleShot(100, this, [this]() {
            if (m_updateTimer) {
                qDebug() << "启动AR更新定时器";
                m_updateTimer->start(16); // 60fps
            }
        });

        qDebug() << "✅ AROverlay构造函数完成";

    } catch (const std::exception& e) {
        qDebug() << "❌ AROverlay构造函数异常:" << e.what();
        // 清理部分构造的对象
        if (m_updateTimer) {
            delete m_updateTimer;
            m_updateTimer = nullptr;
        }
        throw; // 重新抛出异常
    }
}


AROverlay::~AROverlay()
{
    qDebug() << "AROverlay析构函数开始";

    try {
        // 【关键修复】立即停止定时器，避免回调
        if (m_updateTimer && m_updateTimer->isActive()) {
            m_updateTimer->stop();
            qDebug() << "定时器已停止";
        }

        // 【修复】断开所有信号连接
        if (m_updateTimer) {
            disconnect(m_updateTimer, nullptr, this, nullptr);
        }

        // 【修复】隐藏窗口
        setVisible(false);

        qDebug() << "✅ AROverlay析构函数完成";

    } catch (...) {
        qDebug() << "❌ AROverlay析构函数异常（已忽略）";
        // 析构函数不应抛出异常
    }
}




void AROverlay::setupTransparentWindow()
{
    qDebug() << "设置AR透明悬浮窗口";

    // 【关键】设置窗口属性为透明悬浮窗口
    setWindowFlags(Qt::FramelessWindowHint |           // 无边框
                   Qt::WindowStaysOnTopHint |          // 始终置顶
                   Qt::Tool |                          // 工具窗口（不显示在任务栏）
                   Qt::X11BypassWindowManagerHint);    // 绕过窗口管理器

    // 【关键】设置窗口透明
    setAttribute(Qt::WA_TranslucentBackground, true);   // 透明背景
    setAttribute(Qt::WA_NoSystemBackground, true);      // 无系统背景
    setAttribute(Qt::WA_TransparentForMouseEvents, false); // 可以接收鼠标事件（用于按键）

    // 设置窗口大小为全屏
    QDesktopWidget* desktop = QApplication::desktop();
    QRect screenRect = desktop->screenGeometry();
    setGeometry(screenRect);

    // 设置窗口标题（调试用）
    setWindowTitle("AR Virtual Overlay");

    qDebug() << "AR悬浮窗口设置完成，大小:" << screenRect;
}

void AROverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if (!m_frameVisible) {
        return;
    }

    QPainter painter(this);

    // 设置抗锯齿
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 绘制虚拟框架
    drawVirtualFrame(painter);

    // 绘制十字准线
    drawCrosshair(painter);
}

void AROverlay::drawVirtualFrame(QPainter& painter)
{
    // 设置画笔
    QPen framePen(m_frameColor, m_frameThickness, Qt::SolidLine);
    painter.setPen(framePen);

    // 计算框架矩形
    QRect frameRect(
        static_cast<int>(m_framePosition.x - m_frameSize.width / 2.0f),
        static_cast<int>(m_framePosition.y - m_frameSize.height / 2.0f),
        m_frameSize.width,
        m_frameSize.height
    );

    // 绘制外边框
    painter.drawRect(frameRect);

    // 绘制内边框（双线效果）
    QPen innerPen(QColor(m_frameColor.red(), m_frameColor.green(), m_frameColor.blue(), 100), 1);
    painter.setPen(innerPen);

    QRect innerRect = frameRect.adjusted(m_frameThickness, m_frameThickness,
                                        -m_frameThickness, -m_frameThickness);
    painter.drawRect(innerRect);

    // 【可选】绘制角落标记（AR风格）
    drawCornerMarkers(painter, frameRect);
}

void AROverlay::drawCornerMarkers(QPainter& painter, const QRect& frameRect)
{
    QPen markerPen(QColor(0, 255, 255, 150), 2); // 青色角落标记
    painter.setPen(markerPen);

    int markerSize = 15;

    // 左上角
    painter.drawLine(frameRect.topLeft(),
                    frameRect.topLeft() + QPoint(markerSize, 0));
    painter.drawLine(frameRect.topLeft(),
                    frameRect.topLeft() + QPoint(0, markerSize));

    // 右上角
    painter.drawLine(frameRect.topRight(),
                    frameRect.topRight() + QPoint(-markerSize, 0));
    painter.drawLine(frameRect.topRight(),
                    frameRect.topRight() + QPoint(0, markerSize));

    // 左下角
    painter.drawLine(frameRect.bottomLeft(),
                    frameRect.bottomLeft() + QPoint(markerSize, 0));
    painter.drawLine(frameRect.bottomLeft(),
                    frameRect.bottomLeft() + QPoint(0, -markerSize));

    // 右下角
    painter.drawLine(frameRect.bottomRight(),
                    frameRect.bottomRight() + QPoint(-markerSize, 0));
    painter.drawLine(frameRect.bottomRight(),
                    frameRect.bottomRight() + QPoint(0, -markerSize));
}

void AROverlay::drawCrosshair(QPainter& painter)
{
    QPen crosshairPen(m_crosshairColor, 2);
    painter.setPen(crosshairPen);

    // 十字准线中心
    QPoint center(static_cast<int>(m_framePosition.x), static_cast<int>(m_framePosition.y));

    int crossSize = 20;

    // 水平线
    painter.drawLine(center - QPoint(crossSize, 0), center + QPoint(crossSize, 0));

    // 垂直线
    painter.drawLine(center - QPoint(0, crossSize), center + QPoint(0, crossSize));

    // 中心点
    painter.setPen(QPen(Qt::red, 4));
    painter.drawPoint(center);
}

void AROverlay::setFramePosition(const cv::Point2f& position, const cv::Size& size)
{
    m_framePosition = position;
    m_frameSize = size;
    update(); // 触发重绘
}

void AROverlay::setFrameVisible(bool visible)
{
    m_frameVisible = visible;
    setVisible(visible);
    if (visible) {
        update();
    }
}

void AROverlay::setFrameColor(const QColor& color)
{
    m_frameColor = color;
    update();
}

void AROverlay::updateFrame()
{
    if (m_frameVisible) {
        update(); // 定期更新显示
    }
}

void AROverlay::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Escape:
            setFrameVisible(false);
            qDebug() << "ESC键：关闭AR悬浮窗口";
            break;

        case Qt::Key_Space:
            m_frameVisible = !m_frameVisible;
            setVisible(m_frameVisible);
            qDebug() << "空格键：切换AR框架显示";
            break;

        default:
            QWidget::keyPressEvent(event);
            break;
    }
}
