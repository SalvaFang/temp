#include "newcalib.h"

newcalib::newcalib()
{

//    VectorXd b1(9,1);
//    VectorXd b2(9,1);

//    VectorXd x1(6,1);
//    VectorXd x2(6,1);



//    MatrixXd A(9,6);
//    MatrixXd B(9,6);
}

//void newcalib::cal_calibration(CvPoint2D64f scenecalipoints[], CvPoint2D64f vectors[]) {
//    const int pointCount = 12;  // 可以根据需要改为动态传参或外部定义

//        if (pointCount < 3) {

//            return;
//        }

//        // 构建最小二乘系统：Ax = bx, Ay = by
//        MatrixXd A(pointCount, 3);
//        VectorXd bx(pointCount);
//        VectorXd by(pointCount);

//        for (int i = 0; i < pointCount; ++i) {
//            A(i, 0) = 1.0;                  // 偏置项
//            A(i, 1) = vectors[i].x;         // gaze x
//            A(i, 2) = vectors[i].y;         // gaze y

//            bx(i) = scenecalipoints[i].x;   // screen x
//            by(i) = scenecalipoints[i].y;   // screen y

//        }

//        // 求解仿射参数
//        VectorXd x_params = A.colPivHouseholderQr().solve(bx);
//        VectorXd y_params = A.colPivHouseholderQr().solve(by);

//        // 保存仿射矩阵到文件
//        FILE* fp = fopen("./matrix.txt", "w");
//        if (fp) {
//            // 仿射矩阵写成 3x3 形式，最后一行固定为 (0, 0, 1)
//            fprintf(fp, "%.6lf %.6lf %.6lf\n", x_params[1], x_params[2], x_params[0]);
//            fprintf(fp, "%.6lf %.6lf %.6lf\n", y_params[1], y_params[2], y_params[0]);
//            fprintf(fp, "0.000000 0.000000 1.000000\n");
//            fclose(fp);

//        } else {

//            return;
//        }
//}




void newcalib::cal_calibration(CvPoint2D64f scenecalipoints[], CvPoint2D64f vectors[]){

    for (int i = 0; i < 12; i++) {


        A(i,0)=1.0;
        A(i,1) = vectors[i].x;
        A(i, 2) = vectors[i].y;
        A(i, 3) = vectors[i].x * vectors[i].y;
        A(i, 4) = vectors[i].x * vectors[i].x;
        A(i, 5) = vectors[i].y * vectors[i].y;



        B(i,0)=1.0;
        B(i,1) = vectors[i].y;
        B(i, 2) = vectors[i].x;
        B(i, 3) = vectors[i].x * vectors[i].y;
        B(i, 4) = vectors[i].y * vectors[i].y;
        B(i, 5) = vectors[i].x * vectors[i].x;




        b1(i) = scenecalipoints[i].x;
        b2(i) = scenecalipoints[i].y;

    }



    x1 = A.colPivHouseholderQr().solve(b1);
    x2 = B.colPivHouseholderQr().solve(b2);

    FILE* fp1;
    fp1 = fopen(".//matrix.txt", "w");
    for (int i = 0; i < 6; ++i) {
        fprintf(fp1, "%lf %lf \n", x1[i], x2[i]);
    }

    fclose(fp1);

}
