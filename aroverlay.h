#ifndef AROVERLAY_H
#define AROVERLAY_H

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QApplication>
#include <QDesktopWidget>
#include <opencv2/opencv.hpp>

class AROverlay : public QWidget
{
    Q_OBJECT

public:
    explicit AROverlay(QWidget *parent = nullptr);
    ~AROverlay();

    // 公共接口
    void setFramePosition(const cv::Point2f& position, const cv::Size& size);
    void setFrameVisible(bool visible);
    void setFrameColor(const QColor& color);
    void updateFrame();
    void AROverlay::drawCornerMarkers(QPainter& painter, const QRect& frameRect);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    // 虚拟框架参数
    cv::Point2f m_framePosition;
    cv::Size m_frameSize;
    bool m_frameVisible;
    QColor m_frameColor;
    QColor m_crosshairColor;
    int m_frameThickness;

    // 窗口参数
    QTimer* m_updateTimer;

    void setupTransparentWindow();
    void drawVirtualFrame(QPainter& painter);
    void drawCrosshair(QPainter& painter);
};

#endif // AROVERLAY_H
