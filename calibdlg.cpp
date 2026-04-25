 #include "calibdlg.h"
#include "ui_calibdlg.h"

#include"newcalib.h"
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QAxObject>
#include<QDir>

calibDlg::calibDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::calibDlg)
{
    ui->setupUi(this);

    ui->tableWidget->setRowCount(14);
    ui->tableWidget->setColumnCount(8);
    ui->tableWidget->setHorizontalHeaderLabels(QStringList()<<"No."<<"Initial X"<<"Initial Y"<<"Fitting X"<<"Fitting Y"<<"X-Diff"<<"Y-Diff"<<"Square Root");//设置行头
   // ui->tableWidget->setItem(0,0,new QTableWidgetItem("item1")); //设置表格内容
    //隐藏verticalHeader
    ui->tableWidget->verticalHeader()->hide();
    //设置拉伸最后一列
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    //让表头的文字靠左(Qt::AlignLeft是Qt的一个枚举，描述了对齐方式）
    ui->tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);

    //ui->tableWidget->setStyleSheet(QString( "QTableWidget QHeaderView::section{background:#2a9ee4;font-weight:bold;}"));
    ui->tableWidget->setStyleSheet(QString("QTableWidget QHeaderView::section{background:#2a9ee4;font-weight:bold; border:none; height:35px; color:white;}""QTableWidget{gridline-color:#2aaee4; color:#000; border:none;alternate-background-color:lightblue;}"));
    ui->tableWidget->setAlternatingRowColors(true);  //开启间隔行颜色

    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);


    FILE *fp;

    fp = fopen(".//calibpoint.txt","r");
    double temp[12][2]={0};

    for(int i=0; i<12;i++)
    {

        fscanf( fp, "%lf %lf ", &temp[i][0],&temp[i][1]);

        this->A[i][0]=temp[i][0];
        this->A[i][1]=temp[i][1];
    }

    fclose(fp);

}

calibDlg::~calibDlg()
{
    delete ui;
}






void calibDlg::on_calibBtn_clicked()
{

    EyeData data;
    Size inputSize;
    QPointF sROI, eROI;
    Mat downscaled;
    PupilDetectionMethod *pupilDetectionMethod = new PuRe();// new PuRe();
    Point2d point;
    QDir().mkpath("./calibImage/calibResult");
    FILE *centerFP;
    centerFP = fopen(".//calibImage//calibResult//pupilCenter.txt", "w" );

    for(int i = 0; i < CALIBRATIONPOINTS2; ++i){

        scenecalpoints[i].x = A[i][0];
        scenecalpoints[i].y = A[i][1];
        qDebug() << A[i][0]<<A[i][1] << endl;

        sprintf((char*)&eyeName[0],".//calibImage//eye_%d.png",i);

        qDebug() << eyeName << endl;
        //sprintf((char*)&sceneName[0],".//calibImage//scene_%d.png",i);

        eyeImage=imread(eyeName);

//        inputSize.width = 640;
//        inputSize.height = 480;
//        data.input = Mat(inputSize, eyeImage.type() );

//        cv::resize(eyeImage, data.input, inputSize);
         data.input=eyeImage.clone();

        if (data.input.channels() > 1) // TODO: make it algorithm dependent
                    cvtColor(data.input, data.input, CV_BGR2GRAY);
        data.pupil = Pupil();
                data.validPupil = false;

        sROI = QPointF(0,0);
        eROI = QPointF(1,1);
        Rect userROI = Rect(
                Point(sROI.x() * data.input.cols, sROI.y() * data.input.rows),
                Point( eROI.x() * data.input.cols, eROI.y() * data.input.rows)
            );
        float scalingFactor = 1;
        //scalingFactor = 1.0 / cfg.processingDownscalingFactor;//需要看看


        cv::resize(data.input(userROI), downscaled, Size(),
               scalingFactor, scalingFactor,
               INTER_AREA);

        Rect coarseROI = {0, 0, downscaled.cols, downscaled.rows };
        coarseROI = PupilDetectionMethod::coarsePupilDetection( downscaled, 0.5f, 60, 40);
        data.coarseROI = Rect(
                             userROI.tl() + coarseROI.tl() / scalingFactor,
                             userROI.tl() + coarseROI.br() / scalingFactor
                        );

        pupilDetectionMethod->run( downscaled, coarseROI, data.pupil );
        data.pupil.confidence = PupilDetectionMethod::outlineContrastConfidence(downscaled, data.pupil);
        if (data.pupil.center.x > 0 && data.pupil.center.y > 0) {
            // Upscale
            data.pupil.resize( 1.0 / scalingFactor );

            // User region shift
            data.pupil.shift( userROI.tl() );
            data.validPupil = true;
        }
        point = Point2d(data.pupil.center.x,data.pupil.center.y);

        //cvtColor(data.input, eyeImage, COLOR_GRAY2RGB);
        ellipse(eyeImage,Point2f(data.pupil.center.x ,data.pupil.center.y),Size(data.pupil.size.width/2,data.pupil.size.height/2),data.pupil.angle,0,360,CV_RGB(0,255,0),1,8);
        Point2f point = Point2f(data.pupil.center.x ,data.pupil.center.y );
        circle(eyeImage,point,2,CV_RGB(0,0,255),-1);

        char eyeResultName[128];
        sprintf((char*)&eyeResultName[0],".//calibImage//calibResult//eye_%d_result.png",i);

        imwrite(eyeResultName,eyeImage);

        fprintf(centerFP,"%lf %lf\n",point.x,point.y);


//        cv::Point glintCenter1(-1, -1), glintCenter2(-1, -1);
//         //glint
//                int sx = point.x;  // Assume the pupil is at the image center
//                int sy=point.y;
//                int pupil_diameter = data.pupil.size.width*3;  // Approximate pupil diameter in pixels (adjust based on your setup)
//                int pupil_radius = pupil_diameter / 2;
//                int iris_diameter = 3 * pupil_diameter;  // Estimate iris diameter (3x pupil diameter)
//                int iris_radius = iris_diameter / 2;

//                // Create a mask for the pupil region (inverted, to exclude the pupil)
//                Mat pupil_mask = Mat::zeros(eyeImage.size(), CV_8U);
//                cv::circle(pupil_mask, Point(sx, sy), pupil_radius, Scalar(255), FILLED);

//                Mat iris_mask = pupil_mask; // Invert pupil mask to get iris mask
//                imshow("iris_mask gray", iris_mask);
//                // Thresholding and masking
//                Mat gray;
//                if (eyeImage.channels() > 1) {
//                    cvtColor(eyeImage, gray, COLOR_BGR2GRAY);
//                } else {
//                    gray = eyeImage.clone();
//                }


//                Mat masked_thresh;
//                gray.copyTo(masked_thresh, iris_mask); // Apply iris mask to the thresholded image
//                imshow("masked_thresh Threshold", masked_thresh);

//                Mat thresh;
//                double threshValue = 180; // Fixed threshold value (adjust if needed)
//                threshold(masked_thresh, thresh, threshValue, 255, cv::THRESH_BINARY);
//                imshow("Masked gray", thresh);


//                // Find contours in the thresholded and masked image
//                std::vector<std::vector<cv::Point>> contours;
//                cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);




//                double yTolerance = 3.0; // Max Y difference (pixels)

//                for (size_t i = 0; i < contours.size(); ++i) {
//                    double area1 = cv::contourArea(contours[i]);
//                    if (area1 < 70 || area1 > 200) continue;

//                    double perimeter1 = cv::arcLength(contours[i], true);
//                    double circularity1 = (perimeter1 > 0) ? (4 * CV_PI * area1) / (perimeter1 * perimeter1) : 0.0;
//                    if (circularity1 < 0.65) continue;

//                    qDebug()<<"area1"<<area1<<"cir"<<circularity1<<endl;
//                    cv::Moments m1 = cv::moments(contours[i]);
//                    if (m1.m00 == 0) continue;
//                    cv::Point center1(m1.m10 / m1.m00, m1.m01 / m1.m00);
//                    glintCenter1 = center1;
//                    cv::circle(eyeImage, glintCenter1, 3, cv::Scalar(255, 0, 0), -1);  // single glint

//                }


        vectors[i].x = point.x ;
        vectors[i].y = point.y ;

    }

    fclose(centerFP);

    cal_calibration(scenecalpoints,vectors);



    FILE* fp ;
    fp = fopen(".//matrix.txt", "r" );
    double temp[6][2]={0};

    for(int i=0; i<6;i++)
    {

        fscanf( fp, "%lf %lf ", &temp[i][0],&temp[i][1]);

        this->map_matrix[i][0]=temp[i][0];
        this->map_matrix[i][1]=temp[i][1];
    }

    fclose(fp);

    double meanP = 0.0;

    FILE* fp1 ;
    fp1 = fopen(".//calibImage//caliberror.txt", "w" );

    for(int i = 0 ; i < CALIBRATIONPOINTS2; ++i){
        Point2f curr=Zuixiaoercheng_map_point(vectors[i],map_matrix);
        pointMean[i] = sqrt((A[i][0] - curr.x)*(A[i][0] - curr.x) + (A[i][1] - curr.y)*(A[i][1] - curr.y));
        //() << vectors[i].x << endl;
        qDebug() << curr.x << "......" << curr.y << endl;
        meanP += pointMean[i];
        //text = text.append(QString::number(curr.x,10,4) + " " + QString::number(curr.y,10,4) +" "+ QString::number(pointMean[i],10,4) +"\n");



        ui->tableWidget->setItem(i,0,new QTableWidgetItem(QString::number((i+1))));
        ui->tableWidget->setItem(i,1,new QTableWidgetItem(QString::number(A[i][0],10,6)));
        ui->tableWidget->setItem(i,2,new QTableWidgetItem(QString::number(A[i][1],10,6)));
        ui->tableWidget->setItem(i,3,new QTableWidgetItem(QString::number(curr.x,10,6)));
        ui->tableWidget->setItem(i,4,new QTableWidgetItem(QString::number(curr.y,10,6)));
        ui->tableWidget->setItem(i,5,new QTableWidgetItem(QString::number(scenecalpoints[i].x - curr.x,10,6)));
        ui->tableWidget->setItem(i,6,new QTableWidgetItem(QString::number(scenecalpoints[i].y - curr.y,10,6)));
        ui->tableWidget->setItem(i,7,new QTableWidgetItem(QString::number(pointMean[i],10,6)));

        ui->tableWidget->item(i,0)->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->item(i,1)->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->item(i,2)->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->item(i,3)->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->item(i,4)->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->item(i,5)->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->item(i,6)->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->item(i,7)->setTextAlignment(Qt::AlignCenter);

        fprintf( fp1, "%lf %lf %lf \n", curr.x,curr.y,pointMean[i]);
    }

    ui->tableWidget->setSpan(12,0,2,4);
    QTableWidgetItem  * newItem = new QTableWidgetItem;
    ui->tableWidget->setItem(12, 0, newItem);
    newItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    newItem->setText("Calibration Error");

    ui->tableWidget->setSpan(12,4,2,4);
    QTableWidgetItem  * newItem1 = new QTableWidgetItem;
    ui->tableWidget->setItem(12, 4, newItem1);
    newItem1->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    newItem1->setText(QString::number(meanP/CALIBRATIONPOINTS2,10,6));

    fclose(fp);


}

CvPoint2D32f calibDlg::Zuixiaoercheng_map_point(CvPoint2D64f p, double map_matrix[6][2]){
//    CvPoint2D32f  p2;
//    p2.x =map_matrix[0][0] + map_matrix[1][0] * p.x + map_matrix[2][0] * p.y + map_matrix[3][0] * p.x * p.y + map_matrix[4][0] * p.x * p.x + map_matrix[5][0] * p.y * p.y ;
//    p2.y =map_matrix[0][1] + map_matrix[1][1] * p.y + map_matrix[2][1] * p.x + map_matrix[3][1] * p.x * p.y + map_matrix[4][1] * p.y * p.y + map_matrix[5][1] * p.x * p.x;
//    return p2;

    CvPoint2D32f p2;

    // X坐标变换（保持原有逻辑）
    p2.x = map_matrix[0][0] + map_matrix[1][0] * p.x + map_matrix[2][0] * p.y +
           map_matrix[3][0] * p.x * p.y + map_matrix[4][0] * p.x * p.x + map_matrix[5][0] * p.y * p.y;

    // Y坐标变换（修正：统一坐标使用顺序）
    p2.y = map_matrix[0][1] + map_matrix[1][1] * p.x + map_matrix[2][1] * p.y +
           map_matrix[3][1] * p.x * p.y + map_matrix[4][1] * p.x * p.x + map_matrix[5][1] * p.y * p.y;

    return p2;
}

void calibDlg::cal_calibration(CvPoint2D64f scenecalipoints[], CvPoint2D64f vectors[]){

    for (int i = 0; i < 12; i++) {


        A1(i,0)=1.0;
        A1(i,1) = vectors[i].x;
        A1(i, 2) = vectors[i].y;
        A1(i, 3) = vectors[i].x * vectors[i].y;
        A1(i, 4) = vectors[i].x * vectors[i].x;
        A1(i, 5) = vectors[i].y * vectors[i].y;



//        B(i,0)=1.0;
//        B(i,1) = vectors[i].y;
//        B(i, 2) = vectors[i].x;
//        B(i, 3) = vectors[i].x * vectors[i].y;
//        B(i, 4) = vectors[i].y * vectors[i].y;
//        B(i, 5) = vectors[i].x * vectors[i].x;


        // Y坐标变换矩阵 B（修正：与A1使用相同的坐标顺序）
         B(i,0) = 1.0;
         B(i,1) = vectors[i].x;     // 修正：使用x坐标（原来是y）
         B(i,2) = vectors[i].y;     // 修正：使用y坐标（原来是x）
         B(i,3) = vectors[i].x * vectors[i].y;
         B(i,4) = vectors[i].x * vectors[i].x;  // 修正：x²项（原来是y²）
         B(i,5) = vectors[i].y * vectors[i].y;  // 修正：y²项（原来是x²）


        b1(i) = scenecalipoints[i].x;
        b2(i) = scenecalipoints[i].y;

    }



    x1 = A1.colPivHouseholderQr().solve(b1);
    x2 = B.colPivHouseholderQr().solve(b2);

    FILE* fp1;
    fp1 = fopen(".//matrix.txt", "w");
    for (int i = 0; i < 6; ++i) {
        fprintf(fp1, "%lf %lf \n", x1[i], x2[i]);
    }

    fclose(fp1);
    emit calibrationFinished();  // 通知外部
}

void calibDlg::Table2ExcelByHtml(QTableWidget *table, QString title){
    QString fileName = QFileDialog::getSaveFileName(table, "SAVE",QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),"Excel File(*.xlsx)");
    if (fileName!=""){
        QAxObject *excel = new QAxObject;
        if (excel->setControl("Excel.Application")){
             //连接Excel控件
            excel->dynamicCall("SetVisible (bool Visible)","false");//不显示窗体
            excel->setProperty("DisplayAlerts", false);//不显示任何警告信息。如果为true那么在关闭是会出现类似“文件已修改，是否保存”的提示
            QAxObject *workbooks = excel->querySubObject("WorkBooks");//获取工作簿集合
            workbooks->dynamicCall("Add");//新建一个工作簿
            QAxObject *workbook = excel->querySubObject("ActiveWorkBook");//获取当前工作簿
            QAxObject *worksheet = workbook->querySubObject("Worksheets(int)", 1);

            int i,j;
            //QTablewidget 获取数据的列数
            int colcount=table->columnCount();
            //QTablewidget 获取数据的行数
            int rowscount=table->rowCount();

            QAxObject *cell,*col;
            //标题行
            cell=worksheet->querySubObject("Cells(int,int)", 1, 1);
            cell->dynamicCall("SetValue(const QString&)", title);
            cell->querySubObject("Font")->setProperty("Size", 18);
            //调整行高
            worksheet->querySubObject("Range(const QString&)", "1:1")->setProperty("RowHeight", 30);
            //合并标题行
            QString cellTitle;
            cellTitle.append("A1:");
            cellTitle.append(QChar(colcount - 1 + 'A'));
            cellTitle.append(QString::number(1));
            QAxObject *range = worksheet->querySubObject("Range(const QString&)", cellTitle);
            range->setProperty("WrapText", true);
            range->setProperty("MergeCells", true);
            range->setProperty("HorizontalAlignment", -4108);//xlCenter
            range->setProperty("VerticalAlignment", -4108);//xlCenter
            //列标题
            for(i=0;i<colcount;i++){
                QString columnName;
                columnName.append(QChar(i  + 'A'));
                columnName.append(":");
                columnName.append(QChar(i + 'A'));
                col = worksheet->querySubObject("Columns(const QString&)", columnName);
                col->setProperty("ColumnWidth", table->columnWidth(i)/6);
                cell=worksheet->querySubObject("Cells(int,int)", 2, i+1);
                //QTableWidget 获取表格头部文字信息
                columnName=table->horizontalHeaderItem(i)->text();
                cell->dynamicCall("SetValue(const QString&)", columnName);
                cell->querySubObject("Font")->setProperty("Bold", true);
                cell->querySubObject("Interior")->setProperty("Color",QColor(191, 191, 191));
                cell->setProperty("HorizontalAlignment", -4108);//xlCenter
                cell->setProperty("VerticalAlignment", -4108);//xlCenter
            }
            //数据区
            for(i=0;i<rowscount;i++){
                for (j=0;j<colcount;j++){
                    worksheet->querySubObject("Cells(int,int)", i+3, j+1)->dynamicCall("SetValue(const QString&)", table->item(i,j)?table->item(i,j)->text():"");
                }
            }

            //画框线
            QString lrange;
            lrange.append("A2:");
            lrange.append(colcount - 1 + 'A');
            lrange.append(QString::number(table->rowCount() + 2));
            range = worksheet->querySubObject("Range(const QString&)", lrange);
            range->querySubObject("Borders")->setProperty("LineStyle", QString::number(1));
            range->querySubObject("Borders")->setProperty("Color", QColor(0, 0, 0));

            //调整数据区行高
            QString rowsName;
            rowsName.append("2:");
            rowsName.append(QString::number(table->rowCount() + 2));
            range = worksheet->querySubObject("Range(const QString&)", rowsName);
            range->setProperty("RowHeight", 20);
            workbook->dynamicCall("SaveAs(const QString&)",QDir::toNativeSeparators(fileName));//保存至fileName
            workbook->dynamicCall("Close()");//关闭工作簿
            excel->dynamicCall("Quit()");//关闭excel
            delete excel;
            excel=NULL;
            if (QMessageBox::question(NULL,"Completed","The file has been exported. Do you want to open it now?",QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes){
                QDesktopServices::openUrl(QUrl("file:///" + QDir::toNativeSeparators(fileName)));
            }
        }else{
            QMessageBox::warning(NULL,"Failed","Failed to create excel object,Please install Microsoft Excel",QMessageBox::Apply);
        }
    }
}


void calibDlg::on_pushButton_clicked()
{
    this->Table2ExcelByHtml(ui->tableWidget,"calibration data");
}
