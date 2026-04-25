#ifndef SHOWPOINT_H
#define SHOWPOINT_H

#include <QDialog>
#include <QPainter>
#include <QScreen>
#include <QApplication>
#include <QComboBox>
#include <QVBoxLayout>
#include <QTimer>
#include <opencv/cv.h>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <QApplication>
#include <QDesktopWidget>
using namespace cv;

namespace Ui {
class ShowPoint;
}

class ShowPoint : public QDialog {
    Q_OBJECT

public:

    explicit ShowPoint(int numberOfPoints, QWidget *parent = nullptr);



    ~ShowPoint();

protected:
    void paintEvent(QPaintEvent *e) override;

private slots:
    void updateCalibrationPoints(int index);
    //void updateImage();

private:
    Ui::ShowPoint *ui;
    QTimer theTimer;

private slots:
    void updateFlashing();                   // 控制闪烁节奏

private:
    void generateCalibrationPoints();        // 根据布局生成点

    QComboBox *pointSelector;

    QVector<QPoint> currentPoints;           // 当前所有点的位置
    int numberOfPoints = 9;                  // 点的数量（根据下拉框）
    int currentPointIndex = 0;               // 当前正在显示的点序号
    int flashState = 1;                      // 当前点是亮（1）还是灭（0）
    int flashCount = 0;                      // 每个点闪两次（计数 0~3）




//signals:
//    void dataReady(std::vector<cv::Point> tmp);

};

#endif // SHOWPOINT_H
