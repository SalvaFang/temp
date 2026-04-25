#include "showcal.h"
#include "ui_showcal.h"


#include "ElSe.h"
#include "PuRe.h"
#include "utils.h"
#include "eyeimageprocessor.h"
#include <qdebug.h>
#include <QDir>
#include <QMessageBox>
#include"showpoint.h"
#include <QDebug>

// 包含Eigen头文件用于仿射变换计算
#include <Eigen/Dense>
using namespace Eigen;

ShowCal::ShowCal(quint32 pointsnum, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ShowCal)
  , CALIBRATIONPOINTS1(pointsnum)
  , scenecalpoints(pointsnum, cv::Point())
  , pucalipoints(pointsnum)
  , crcalipoints(pointsnum)
  , vectors(pointsnum)

{
    ui->setupUi(this);
    m_CalNum = 0;
    this->setWindowIcon(QIcon("eye.ico"));
    //this->setWindowTitle(QString::fromLocal8Bit("九点标定_第") + QString::number(m_CalNum+1,'f',0) + QString::fromLocal8Bit("图"));
    this->setWindowTitle(QString::number(m_CalNum+1,'f',0));


    sceneLabel = new QLabel(this);
    eyeLabel = new QLabel(this);








    ui->horizontalLayout->addWidget(sceneLabel);
    ui->horizontalLayout->addWidget(eyeLabel);



    sceneLabel->installEventFilter(this);
    eyeLabel->installEventFilter(this);
    //connect(sceneLabel, SIGNAL(clicked()), this, SLOT(updateImage()));
    connect(ui->nextButton, SIGNAL(clicked()), this, SLOT(clicknextButton()));
    connect(ui->computeButton, SIGNAL(clicked()), this, SLOT(clickcomputeButton()));

    char eyeName[128],sceneName[128];
    sprintf((char*)&eyeName[0],".//yanshiCalib//eye_%d.png",m_CalNum);
    sprintf((char*)&sceneName[0],".//yanshiCalib//scene_%d.png",m_CalNum);
    sceneImage = imread(sceneName);
    eyeImage = imread(eyeName);
    pupilDetectionMethod = new PuRe;//new PuRe();ElSe


    // 4) “手动”把对话框调到至少能容下 1280×720 的尺寸
       //    如果你还有按钮在下方，需要加上一点高度留白，比如 +100 像素
    this->resize(1280, 720 + 200);
    this->setMinimumSize(1280, 720 + 100);
    this->update();

}


ShowCal::~ShowCal()
{
    delete sceneLabel;
    delete eyeLabel;


    delete pupilDetectionMethod;
    delete pupilTrackingMethod;
    delete ui;

}



void ShowCal::paintEvent(QPaintEvent *e)
{
    // 使用原始大小显示sceneImage


         QImage image = QImage((uchar*)(sceneImage.data),sceneImage.cols,sceneImage.rows,QImage::Format_RGB888);
         QImage image1 = QImage((uchar*)(eyeImage.data),eyeImage.cols,eyeImage.rows,QImage::Format_RGB888);
         sceneLabel->setPixmap(QPixmap::fromImage(image));
         sceneLabel->resize(image.size());
         sceneLabel->show();
//         eyeLabel->setPixmap(QPixmap::fromImage(image1));
//         eyeLabel->resize(image1.size());
//         eyeLabel->show();
         QDialog::paintEvent(e);


}

/*void ShowCal::updateImage()
{

    char eyeName[128],sceneName[128];
    sprintf((char*)&eyeName[0],"C://Users//Hewie//Desktop//613//yanshiCalib//eye_%d.png",m_CalNum);
    sprintf((char*)&sceneName[0],"C://Users//Hewie//Desktop//613//yanshiCalib//scene_%d.png",m_CalNum);
    sceneImage = imread(sceneName);
    eyeImage = imread(eyeName);
    if(sceneImage.data && eyeImage.data)
    {
        Mat proc;
        cv::resize(eyeImage, proc, Size(0, 0), 0.2, 0.2, 1);
        TrackerParams params;
        params.Radius_Max = 10;
        params.Radius_Min = 8;
        params.Pupil_center = Point2f(0, 0);
        params.Corneal_center=Point2f(0,0);
        findPupilEllipse(proc, params);

        CCircle_detector::compute_ellipse(eyeImage, in, params.Pupil_center,params.Corneal_center);

        vectors[m_CalNum].x = params.Pupil_center.x - params.Corneal_center.x;
        vectors[m_CalNum].y = params.Pupil_center.y - params.Corneal_center.y;
        cvtColor(sceneImage,sceneImage,CV_BGR2RGB);
        cvtColor(eyeImage,eyeImage,CV_BGR2RGB);
        this->update();
    }
}*/

bool ShowCal::eventFilter(QObject *obj, QEvent *event)
{
    char eyeName[128],sceneName[128];
    sprintf((char*)&eyeName[0],".//yanshiCalib//eye_%d.png",m_CalNum);
    sprintf((char*)&sceneName[0],".//yanshiCalib//scene_%d.png",m_CalNum);


    if(qobject_cast<QLabel*>(obj)==sceneLabel&&event->type() == QEvent::MouseButtonPress)
    {
        sceneLabel->setStyleSheet("background-color: rgb(0, 255, 255);");
        m_point = cursor().pos();
        m_point = sceneLabel->mapFromGlobal(m_point);

        // 直接使用场景图像的原始坐标，不进行标准化
        scenecalpoints[m_CalNum].x = m_point.rx();
        scenecalpoints[m_CalNum].y = m_point.ry();

        qDebug() << "场景坐标:" << m_point.rx() << "," << m_point.ry();
        qDebug() << "场景图像尺寸:" << sceneImage.cols << "x" << sceneImage.rows;

        qDebug() << "::"<<scenecalpoints[m_CalNum].x<<":"<<scenecalpoints[m_CalNum].y<<endl;
        sceneImage=imread(sceneName);
        cv::circle(sceneImage,cv::Point(m_point.rx(),m_point.ry()),5,CV_RGB(0,255,0),-1);

        cvtColor(sceneImage,sceneImage,CV_BGR2RGB);

        eyeImage=imread(eyeName);
        EyeData data;
//        Size inputSize;
//        inputSize.width = 640;
//        inputSize.height = 480;
//        data.input = Mat(inputSize, eyeImage.type() );
//        cv::resize(eyeImage, data.input, inputSize);
         data.input=eyeImage.clone();
        if (data.input.channels() > 1) // TODO: make it algorithm dependent
                    cvtColor(data.input, data.input, CV_BGR2GRAY);
        data.pupil = Pupil();
                data.validPupil = false;
        QPointF sROI, eROI;
        sROI = QPointF(0,0);
        eROI = QPointF(1,1);
        Rect userROI = Rect(
                Point(sROI.x() * data.input.cols, sROI.y() * data.input.rows),
                Point( eROI.x() * data.input.cols, eROI.y() * data.input.rows)
            );
        float scalingFactor = 1;
        //scalingFactor = 1.0 / cfg.processingDownscalingFactor;//需要看看

        Mat downscaled;
        cv::resize(data.input(userROI), downscaled, Size(),
               scalingFactor, scalingFactor,
               INTER_AREA);

        Rect coarseROI = {0, 0, downscaled.cols, downscaled.rows };
        coarseROI = PupilDetectionMethod::coarsePupilDetection( downscaled, 0.5f, 60, 40);
        data.coarseROI = Rect(
                             userROI.tl() + coarseROI.tl() / scalingFactor,
                             userROI.tl() + coarseROI.br() / scalingFactor
                        );
        PupilDetectionMethod *pupilDetectionMethod = new PuRe();// new PuRe();
        pupilDetectionMethod->run( downscaled, coarseROI, data.pupil );
        data.pupil.confidence = PupilDetectionMethod::outlineContrastConfidence(downscaled, data.pupil);

        const double MIN_CONFIDENCE = 0.6;
       if (data.pupil.confidence < MIN_CONFIDENCE || data.pupil.center.x <= 0 || data.pupil.center.y <= 0) {
           // 检测失败
           QMessageBox::warning(this, "瞳孔检测失败",
                                QString("无法在此图像中检测到有效的瞳孔。\n(置信度: %1，阈值: %2)\n\n请返回主界面重新采集此点的图像，或尝试重新点击。")
                                .arg(data.pupil.confidence, 0, 'f', 2).arg(MIN_CONFIDENCE));

           // 释放资源并中止当前操作
           delete pupilDetectionMethod;
           return true; // 事件已处理，但我们不进行下一步
       }


        Mat imgRGB;
        cvtColor(data.input, eyeImage, COLOR_GRAY2RGB);
        ellipse(eyeImage,Point2d(data.pupil.center.x ,data.pupil.center.y),Size(data.pupil.size.width/2,data.pupil.size.height/2),data.pupil.angle,0,360,CV_RGB(0,0,255),1,8);
        Point2d point = Point2d(data.pupil.center.x ,data.pupil.center.y );
        circle(eyeImage,point,2,CV_RGB(0,0,255),-1);

        vectors[m_CalNum].x = point.x;//corneal_center.x;
        vectors[m_CalNum].y = point.y;//corneal_center.y;
        qDebug() << ":111:"<<vectors[m_CalNum].x<<":"<<vectors[m_CalNum].y<<endl;
        this->update();

        // 自动下一步或自动计算
        m_CalNum++;
        if(m_CalNum >= CALIBRATIONPOINTS1) {
            // 所有点都选择完毕，自动开始计算
            qDebug() << "所有" << CALIBRATIONPOINTS1 << "个点已选择完毕，自动开始计算...";
            clickcomputeButton();  // 自动调用计算函数
        } else {
            // 还有点未选择，自动切换到下一个点
            qDebug() << "已选择第" << m_CalNum << "个点，自动切换到下一个点...";

            // 重置m_CalNum到正确的索引（因为clicknextButton会再次递增）
            m_CalNum--;
            clicknextButton();  // 自动调用下一步函数
        }

        return true;
    }
    else
    {
        return false;
    }




}
void ShowCal::clicknextButton()
{
    if(m_CalNum == 0)
    {
        //this->updateImage();
    }
    else if(m_CalNum == 9 &&CALIBRATIONPOINTS1==9){
        m_CalNum = 0;
        //this->updateImage();
    }
    m_CalNum++;

    if(m_CalNum == 3&&CALIBRATIONPOINTS1==3){
         m_CalNum = m_CalNum % 3;
    }

    if(m_CalNum == CALIBRATIONPOINTS1){
         m_CalNum = m_CalNum % CALIBRATIONPOINTS1;
    }


    char eyeName[128],sceneName[128];
    sprintf((char*)&eyeName[0],".//yanshiCalib//eye_%d.png",m_CalNum);
    sprintf((char*)&sceneName[0],".//yanshiCalib//scene_%d.png",m_CalNum);


    sceneImage = imread(sceneName);
    eyeImage = imread(eyeName);
    //this->setWindowTitle(QString::fromLocal8Bit("九点标定_第")+QString::number(m_CalNum+1,'f',0)+QString::fromLocal8Bit("图"));
    this->setWindowTitle(QString::number(m_CalNum+1,'f',0));
    this->update();
}

void ShowCal::performAffineCalibration(int pointCount) {
    if (pointCount < 3) {
        QMessageBox::warning(this, "错误", "仿射变换至少需要3个点");
        return;
    }

    // 确保有足够的点可用
    if (vectors.size() < pointCount || scenecalpoints.size() < pointCount) {
        QMessageBox::warning(this, "错误", "标定点数量不足");
        return;
    }

    // 构建最小二乘系统：Ax = bx, Ay = by
    MatrixXd A(pointCount, 3);
    VectorXd bx(pointCount);
    VectorXd by(pointCount);

    for (int i = 0; i < pointCount; ++i) {
        A(i, 0) = 1.0;                // 偏置项
        A(i, 1) = vectors[i].x;       // gaze x
        A(i, 2) = vectors[i].y;       // gaze y

        bx(i) = scenecalpoints[i].x;  // screen x
        by(i) = scenecalpoints[i].y;  // screen y

        qDebug() << "Gaze:" << vectors[i].x << vectors[i].y
                 << " --> Screen:" << scenecalpoints[i].x << scenecalpoints[i].y;
    }

    // 求解仿射参数
    VectorXd x_params = A.colPivHouseholderQr().solve(bx);
    VectorXd y_params = A.colPivHouseholderQr().solve(by);

    // 确保当前目录存在，然后保存仿射矩阵到文件
    QDir().mkpath(".");
    FILE* fp = fopen(".//yanshi_matrix.txt", "w");
    if (fp) {
        // 仿射矩阵写成 3x3 形式，最后一行固定为 (0, 0, 1)
        fprintf(fp, "%.6lf %.6lf %.6lf\n", x_params[1], x_params[2], x_params[0]);
        fprintf(fp, "%.6lf %.6lf %.6lf\n", y_params[1], y_params[2], y_params[0]);
        fprintf(fp, "0.000000 0.000000 1.000000\n");
        fclose(fp);
    } else {
        QMessageBox::critical(this, "错误", "无法保存仿射矩阵到文件");
        return;
    }
    emit calibrationFinished();  // 通知外部
    QMessageBox::information(this, "信息", QString("仿射变换标定完成，已使用 %1 个点").arg(pointCount));

    // 自动关闭窗口
    this->accept();
}

void ShowCal::performPerspectiveCalibration(int pointCount) {
    if (pointCount < 3) {
        QMessageBox::warning(this, "错误", "透视变换至少需要4个点");
        return;
    }

    // 确保有足够的点可用
    if (vectors.size() < pointCount || scenecalpoints.size() < pointCount) {
        QMessageBox::warning(this, "错误", "标定点数量不足");
        return;
    }

    // 使用OpenCV计算透视变换矩阵
    std::vector<cv::Point2f> srcPoints, dstPoints;

    for (int i = 0; i < pointCount; ++i) {
        srcPoints.push_back(cv::Point2f(vectors[i].x, vectors[i].y));
        dstPoints.push_back(cv::Point2f(scenecalpoints[i].x, scenecalpoints[i].y));

        qDebug() << "Gaze:" << vectors[i].x << vectors[i].y
                 << " --> Screen:" << scenecalpoints[i].x << scenecalpoints[i].y;
    }

    // 计算透视变换矩阵（3x3）
    cv::Mat perspectiveMatrix;
    if (pointCount == 4) {
        // 对于4个点，使用精确透视变换
        perspectiveMatrix = cv::getPerspectiveTransform(srcPoints, dstPoints);
    } else {
        // 对于超过4个点，使用最小二乘法求解透视变换（删除RANSAC参数）
        perspectiveMatrix = cv::findHomography(srcPoints, dstPoints, 0);
    }

    if (perspectiveMatrix.empty()) {
        QMessageBox::critical(this, "错误", "透视变换计算失败");
        return;
    }

    // 确保当前目录存在，然后保存透视变换矩阵到文件
    QDir().mkpath(".");
    FILE* fp = fopen(".//yanshi_matrix.txt", "w");
    if (fp) {
        // 保存3x3透视变换矩阵
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                fprintf(fp, "%.8lf", perspectiveMatrix.at<double>(i, j));
                if (j < 2) fprintf(fp, " ");
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
    } else {
        QMessageBox::critical(this, "错误", "无法保存透视变换矩阵到文件");
        return;
    }

    emit calibrationFinished();  // 通知外部
    QMessageBox::information(this, "信息", QString("透视变换标定完成，已使用 %1 个点").arg(pointCount));

    // 自动关闭窗口
    this->accept();
}


void ShowCal::clickcomputeButton()
{
    switch (CALIBRATIONPOINTS1) {
        case 3:
            // 对于3个点，明确调用仿射变换函数
            qDebug() << "3-point calibration detected. Using Affine Transformation.";
            performAffineCalibration(CALIBRATIONPOINTS1);
            break;

        case 4:
        case 5:
        case 9:
        case 12:
        case 16:
            // 对于4个及更多点，使用透视变换
            qDebug() << CALIBRATIONPOINTS1 << "-point calibration detected. Using Perspective Transformation.";
            performPerspectiveCalibration(CALIBRATIONPOINTS1);
            break;

        default:
            QMessageBox::warning(this, "错误", QString("不支持的标定点数量: %1").arg(CALIBRATIONPOINTS1));
            break;
    }
}



