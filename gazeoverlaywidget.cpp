#include "gazeoverlaywidget.h"
#include <QPaintEvent>

GazeOverlayWidget::GazeOverlayWidget(QWidget *parent) : QWidget(parent)
{
    // --- 核心设置 ---
    // 1. 设置窗口无边框，并始终保持在所有其他窗口的顶部
    //    Qt::Tool 可防止窗口在任务栏中显示图标
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);

    // 2. 设置背景透明，这是实现覆盖效果的关键
    setAttribute(Qt::WA_TranslucentBackground);

    // 3. 让鼠标事件“穿透”这个窗口，可以操作它下面的窗口
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

void GazeOverlayWidget::updateGazeData(const QPointF &currentGaze, const QQueue<QPointF> &trail)
{
    m_currentGaze = currentGaze;
    m_gazeTrail = trail;
    update(); // 请求重绘，Qt会自动调用paintEvent
}

void GazeOverlayWidget::paintEvent(QPaintEvent *event)
{
    // Q_UNUSED宏防止编译器对未使用参数发出警告
    Q_UNUSED(event);

    // QPainter用于在这个窗口上进行所有2D绘制
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 开启抗锯齿，使绘制更平滑

    // 分别调用绘制函数
    drawGazeTrail(painter);
    drawGazeIndicator(painter);
}

void GazeOverlayWidget::drawGazeTrail(QPainter& painter)
{
    // 检查轨迹点数量是否足够绘制线条
    if (m_gazeTrail.size() < 2) return;

    QPen trailPen;
    trailPen.setCapStyle(Qt::RoundCap);
    trailPen.setJoinStyle(Qt::RoundJoin);

    // --- 修正部分开始 ---
    // 直接遍历QQueue，不再调用不存在的 toList()
    for (int i = 1; i < m_gazeTrail.size(); ++i) {
        // 计算透明度，越旧的线段越透明
        // 注意：QQueue的头部(head)是索引0，是最旧的元素
        qreal opacity = static_cast<qreal>(i) / m_gazeTrail.size();
        QColor lineColor(100, 200, 255, static_cast<int>(opacity * 255));
        trailPen.setColor(lineColor);
        trailPen.setWidth(2 + static_cast<int>(opacity * 3)); // 越新的线越粗
        painter.setPen(trailPen);

        // 使用 at() 或 [] 操作符来访问元素
        painter.drawLine(m_gazeTrail.at(i-1), m_gazeTrail.at(i));
    }
    // --- 修正部分结束 ---
}

void GazeOverlayWidget::drawGazeIndicator(QPainter& painter)
{
    if (m_currentGaze.x() < 0) return;

    const int GAZE_INDICATOR_SIZE = 25;
    QPointF gazePoint = m_currentGaze;

    // 外圈 - 红色边框
    QColor outerColor = QColor(255, 50, 50, 200);
    QPen pen(outerColor, 3);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(gazePoint, GAZE_INDICATOR_SIZE, GAZE_INDICATOR_SIZE);

    // 内圈填充
    QColor innerColor = QColor(255, 100, 100, 80);
    painter.setBrush(QBrush(innerColor));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(gazePoint, GAZE_INDICATOR_SIZE - 8, GAZE_INDICATOR_SIZE - 8);

    // 中心点
    QColor centerColor = QColor(255, 0, 0, 255);
    painter.setBrush(QBrush(centerColor));
    painter.drawEllipse(gazePoint, 3, 3);
}
