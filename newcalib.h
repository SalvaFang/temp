#ifndef NEWCALIB_H
#define NEWCALIB_H

#include<Eigen/Dense>
#include<Eigen/Geometry>
#include <cv.h>
#include <highgui.h>

using namespace Eigen;

class newcalib
{
public:

    MatrixXd A = MatrixXd(12,6);
    MatrixXd B = MatrixXd(12,6);
//    MatrixXd A = MatrixXd(12,5);
//    MatrixXd B = MatrixXd(12,7);

    VectorXd b1 = VectorXd(12,1);
    VectorXd b2 = VectorXd(12,1);

    VectorXd x1 = VectorXd(6,1);
    VectorXd x2 = VectorXd(6,1);
//    VectorXd x1 = VectorXd(5,1);
//    VectorXd x2 = VectorXd(7,1);


    newcalib();
    void cal_calibration(CvPoint2D64f scenecalipoints[],CvPoint2D64f vectors[]);
};

#endif // NEWCALIB_H
