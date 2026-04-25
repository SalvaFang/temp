#ifndef GAZEOVERLAYWIDGET_H
#define GAZEOVERLAYWIDGET_H

#include <QWidget>
#include <QPointF>
#include <QQueue>
#include <QPainter>

class GazeOverlayWidget : public QWidget
{
    Q_OBJECT

public:
    // 构造函数不指定父窗口，让它成为一个独立的顶层窗口
    explicit GazeOverlayWidget(QWidget *parent = nullptr);

public slots:
    // 提供一个槽函数来更新数据，方便连接信号
    void updateGazeData(const QPointF &currentGaze, const QQueue<QPointF> &trail);

protected:
    // 重写绘制事件
    void paintEvent(QPaintEvent *event) override;

private:
    // 绘制视线轨迹和指示器的私有函数，使paintEvent更整洁
    void drawGazeTrail(QPainter& painter);
    void drawGazeIndicator(QPainter& painter);

    // 存储从MainWindow传递过来的数据
    QPointF m_currentGaze;
    QQueue<QPointF> m_gazeTrail;
};

#endif // GAZEOVERLAYWIDGET_H
