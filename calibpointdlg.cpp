#include "calibpointdlg.h"
#include "ui_calibpointdlg.h"
#include<QMessageBox>

calibPointDlg::calibPointDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::calibPointDlg)
{
    ui->setupUi(this);
}

calibPointDlg::~calibPointDlg()
{
    delete ui;
}

void calibPointDlg::on_pushButton_clicked()
{
    if(ui->x1->text().isEmpty() || ui->y1->text().isEmpty() || ui->x2->text().isEmpty() || ui->y2->text().isEmpty() ||
            ui->x3->text().isEmpty() || ui->y3->text().isEmpty() || ui->x4->text().isEmpty() || ui->y4->text().isEmpty() ||
            ui->x5->text().isEmpty() || ui->y5->text().isEmpty() || ui->x6->text().isEmpty() || ui->y6->text().isEmpty() ||
            ui->x7->text().isEmpty() || ui->y7->text().isEmpty() || ui->x8->text().isEmpty() || ui->y8->text().isEmpty() ||
            ui->x9->text().isEmpty() || ui->y9->text().isEmpty() || ui->x10->text().isEmpty() || ui->y10->text().isEmpty()||
            ui->x11->text().isEmpty() || ui->y11->text().isEmpty() || ui->x12->text().isEmpty() || ui->y12->text().isEmpty()){
        QMessageBox::information(this,"infomation","You need to enter all coordinates!");
    }else{

        point[0].x = ui->x1->text().toDouble();
        point[0].y = ui->y1->text().toDouble();

        point[1].x = ui->x2->text().toDouble();
        point[1].y = ui->y2->text().toDouble();

        point[2].x = ui->x3->text().toDouble();
        point[2].y = ui->y3->text().toDouble();

        point[3].x = ui->x4->text().toDouble();
        point[3].y = ui->y4->text().toDouble();

        point[4].x = ui->x5->text().toDouble();
        point[4].y = ui->y5->text().toDouble();

        point[5].x = ui->x6->text().toDouble();
        point[5].y = ui->y6->text().toDouble();

        point[6].x = ui->x7->text().toDouble();
        point[6].y = ui->y7->text().toDouble();

        point[7].x = ui->x8->text().toDouble();
        point[7].y = ui->y8->text().toDouble();

        point[8].x = ui->x9->text().toDouble();
        point[8].y = ui->y9->text().toDouble();

        point[9].x = ui->x10->text().toDouble();
        point[9].y = ui->y10->text().toDouble();

        point[10].x = ui->x11->text().toDouble();
        point[10].y = ui->y11->text().toDouble();

        point[11].x = ui->x12->text().toDouble();
        point[11].y = ui->y12->text().toDouble();

        FILE *fp;
        fp = fopen(".//calibpoint.txt","w+");

        for(int i = 0 ; i < 12; ++i){
            fprintf( fp, "%lf %lf \n", point[i].x,point[i].y);
        }

        fclose(fp);
        QMessageBox::information(this,"infomation","Success!");
    }
}
