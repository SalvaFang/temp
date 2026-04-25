//#pragma once
//#ifndef SHOWCAL_H
//#define SHOWCAL_H

//#include <QDialog>
//#include <QPainter>
//#include <QPixmap>
//#include <QLabel>
//#include <QImage>
//#include "cdocalibration.h"
//#include"PupilTracker.h"

//#include <QButtonGroup>
//#include "PupilDetectionMethod.h"
//#include"PupilTracker.h"
//#include "PuReST.h"
//#include "PupilTrackingMethod.h"
//#include"newcalib.h"

//#include <QScrollArea>
////#ifndef CALIBRATIONPOINTS1
////#define CALIBRATIONPOINTS1 3
////#endif

//using namespace cv;

//namespace Ui {
//class ShowCal;
//}

//class ShowCal : public QDialog
//{
//    Q_OBJECT

//public:
//    explicit ShowCal(quint32 pointsnum, QWidget *parent = nullptr);
//    ~ShowCal();

//public slots:
//    //void updateImage();
//    void clicknextButton();
//    void clickcomputeButton();

//signals:
//    void calibrationFinished();  // 标定完成信号

//private:
//    void performAffineCalibration(int pointCount);
//    Ui::ShowCal *ui;
//    int m_CalNum;
//    QPoint m_point;
//    quint32 CALIBRATIONPOINTS1 = 3;
//    std::vector<cv::Point> scenecalpoints;
//    std::vector<CvPoint2D64f> pucalipoints;
//    std::vector<CvPoint2D64f>crcalipoints;
//    std::vector<cv::Point2d> vectors;

////    cv::Point scenecalpoints[CALIBRATIONPOINTS1];
////    CvPoint2D64f pucalipoints[CALIBRATIONPOINTS1];
////    CvPoint2D64f crcalipoints[CALIBRATIONPOINTS1];
////    cv::Point2d vectors[CALIBRATIONPOINTS1];
//    CDoCalibration *DoCalibration;
//    double in[5];

//    cv::Mat sceneImage,eyeImage,cal_eye;
//    QLabel *sceneLabel;
//    QLabel *eyeLabel;

//    PupilTrackingMethod *pupilTrackingMethod = nullptr;
//    PupilDetectionMethod *pupilDetectionMethod = nullptr;
//    TrackedPupil params;
//    Timestamp timestamp;

////    MatrixXd A = MatrixXd(12,6);
////    MatrixXd B = MatrixXd(12,6);
////    VectorXd b1 = VectorXd(12,1);
////    VectorXd b2 = VectorXd(12,1);
////    VectorXd x1 = VectorXd(6,1);
////    VectorXd x2 = VectorXd(6,1);

//    MatrixXd A = MatrixXd(3,2);
//    MatrixXd B = MatrixXd(3,2);
//    VectorXd b1 = VectorXd(3,1);
//    VectorXd b2 = VectorXd(3,1);

//    VectorXd x1 = VectorXd(2,1);
//    VectorXd x2 = VectorXd(2,1);


//protected:
//    void paintEvent(QPaintEvent *e);
//    bool eventFilter(QObject *obj, QEvent *event);

//};

//#endif // SHOWCAL_H


#pragma once
#ifndef SHOWCAL_H
#define SHOWCAL_H

#include <QDialog>
#include <QPainter>
#include <QPixmap>
#include <QLabel>
#include <QImage>

#include"PupilTracker.h"

#include <QButtonGroup>
#include "PupilDetectionMethod.h"
#include"PupilTracker.h"
#include "PuReST.h"
#include "PupilTrackingMethod.h"
#include"newcalib.h"

#include <QScrollArea>
//#ifndef CALIBRATIONPOINTS1
//#define CALIBRATIONPOINTS1 3
//#endif

using namespace cv;

namespace Ui {
class ShowCal;
}

class ShowCal : public QDialog
{
    Q_OBJECT

public:
    explicit ShowCal(quint32 pointsnum, QWidget *parent = nullptr);
    ~ShowCal();

public slots:
    //void updateImage();
    void clicknextButton();
    void clickcomputeButton();

signals:
    void calibrationFinished();  // 标定完成信号

private:
    void performAffineCalibration(int pointCount);
    void performPerspectiveCalibration(int pointCount);
    Ui::ShowCal *ui;
    int m_CalNum;
    QPoint m_point;
    quint32 CALIBRATIONPOINTS1 = 3;
    std::vector<cv::Point> scenecalpoints;
    std::vector<CvPoint2D64f> pucalipoints;
    std::vector<CvPoint2D64f>crcalipoints;
    std::vector<cv::Point2d> vectors;

//    cv::Point scenecalpoints[CALIBRATIONPOINTS1];
//    CvPoint2D64f pucalipoints[CALIBRATIONPOINTS1];
//    CvPoint2D64f crcalipoints[CALIBRATIONPOINTS1];
//    cv::Point2d vectors[CALIBRATIONPOINTS1];

    double in[5];

    cv::Mat sceneImage,eyeImage,cal_eye;
    QLabel *sceneLabel;
    QLabel *eyeLabel;

    PupilTrackingMethod *pupilTrackingMethod = nullptr;
    PupilDetectionMethod *pupilDetectionMethod = nullptr;
    TrackedPupil params;
    Timestamp timestamp;

//    MatrixXd A = MatrixXd(12,6);
//    MatrixXd B = MatrixXd(12,6);
//    VectorXd b1 = VectorXd(12,1);
//    VectorXd b2 = VectorXd(12,1);
//    VectorXd x1 = VectorXd(6,1);
//    VectorXd x2 = VectorXd(6,1);

    MatrixXd A = MatrixXd(3,2);
    MatrixXd B = MatrixXd(3,2);
    VectorXd b1 = VectorXd(3,1);
    VectorXd b2 = VectorXd(3,1);

    VectorXd x1 = VectorXd(2,1);
    VectorXd x2 = VectorXd(2,1);


protected:
    void paintEvent(QPaintEvent *e);
    bool eventFilter(QObject *obj, QEvent *event);

};

#endif // SHOWCAL_H
