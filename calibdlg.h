#ifndef CALIBDLG_H
#define CALIBDLG_H

#include <QDialog>

#include"PupilTracker.h"
#include"newcalib.h"

#include "ElSe.h"
#include "utils.h"
#include "eyeimageprocessor.h"

#include<QTableWidget>

#ifndef CALIBRATIONPOINTS2
#define CALIBRATIONPOINTS2 12
#endif
using namespace cv;

namespace Ui {
class calibDlg;
}

class calibDlg : public QDialog
{
    Q_OBJECT

public:
    explicit calibDlg(QWidget *parent = nullptr);
    ~calibDlg();
    void Table2ExcelByHtml(QTableWidget* table, QString title);

private slots:
    void on_calibBtn_clicked();

    CvPoint2D32f Zuixiaoercheng_map_point(CvPoint2D64f p,double map_matrix[6][2]);
    void calibDlg::cal_calibration(CvPoint2D64f scenecalipoints[], CvPoint2D64f vectors[]);

    void on_pushButton_clicked();

signals:
    void calibrationFinished();  // 标定完成信号
public:
    MatrixXd A1 = MatrixXd(12,6);
    MatrixXd B = MatrixXd(12,6);
//    MatrixXd A = MatrixXd(12,5);
//    MatrixXd B = MatrixXd(12,7);

    VectorXd b1 = VectorXd(12);
    VectorXd b2 = VectorXd(12);

    VectorXd x1 = VectorXd(6);
    VectorXd x2 = VectorXd(6);

private:
    Ui::calibDlg *ui;

    int m_CalNum;
    CvPoint2D64f scenecalpoints[CALIBRATIONPOINTS2] ;
    CvPoint2D64f pucalipoints[CALIBRATIONPOINTS2];
    CvPoint2D64f crcalipoints[CALIBRATIONPOINTS2];
    CvPoint2D64f vectors[CALIBRATIONPOINTS2];

    int A[CALIBRATIONPOINTS2][2] = {{0,10},{0,8},{0,5},{-10,0},{-8,0},{-5,0},{5,0},{8,0},{10,0},{0,-5},{0,-8},{0,-10}};

    char eyeName[128],sceneName[128];
    cv::Mat sceneImage,eyeImage;

    double     map_matrix[6][2] = {0};
    double pointMean[CALIBRATIONPOINTS2];
};

#endif // CALIBDLG_H
