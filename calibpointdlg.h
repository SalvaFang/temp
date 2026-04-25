#ifndef CALIBPOINTDLG_H
#define CALIBPOINTDLG_H

#include <QDialog>
#include<opencv2/calib3d.hpp>
#include<cv.h>

using namespace cv;

namespace Ui {
class calibPointDlg;
}

class calibPointDlg : public QDialog
{
    Q_OBJECT

public:
    explicit calibPointDlg(QWidget *parent = nullptr);
    ~calibPointDlg();

private slots:
    void on_pushButton_clicked();

private:
    Ui::calibPointDlg *ui;

    Point2f point[12];
};

#endif // CALIBPOINTDLG_H
