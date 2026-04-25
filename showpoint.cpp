//#include "ShowPoint.h"
//#include "ui_ShowPoint.h"
//#include"QFile"
//#include"QTextStream"
//#include <QKeyEvent>

//ShowPoint::ShowPoint(QWidget *parent) :
//    QDialog(parent),
//    ui(new Ui::ShowPoint),
//    numberOfPoints(9)  // 初始化 numberOfPoints 为 9
//{
//    ui->setupUi(this);
//    this->showFullScreen();

//    setFocusPolicy(Qt::StrongFocus);

//    // 设置布局和控件
//    QVBoxLayout *layout = new QVBoxLayout(this);
//    QComboBox *pointSelector = new QComboBox(this);
//    pointSelector->addItem("5 Points");
//    pointSelector->addItem("9 Points");
//    pointSelector->addItem("10 Points");
//    pointSelector->addItem("12 Points");
//    layout->addWidget(pointSelector);
//    // 或者使用 setFixedSize 来固定大小

//     pointSelector->setFixedSize(150, 30);  // (width, height)
//    // 连接下拉框的 currentIndexChanged 信号到槽函数
//    connect(pointSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
//            this, &ShowPoint::updateCalibrationPoints);


//    // 定时器设置
//    theTimer.start(3000);
//    connect(&theTimer, &QTimer::timeout, this, &ShowPoint::updateImage);
//}

//ShowPoint::~ShowPoint() {
//    delete ui;
//}






//void ShowPoint::paintEvent(QPaintEvent *e) {
//    QPainter painter(this);
//    painter.setPen(Qt::black);
//    painter.setBrush(QBrush(Qt::black));

//    // 获取屏幕尺寸
//    QScreen *screen = QApplication::primaryScreen();
//    QRect screenGeometry = screen->geometry();
//    int screenWidth = screenGeometry.width();
//    int screenHeight = screenGeometry.height();

//    // 根据当前选择的标定点数量绘制点
//    switch (numberOfPoints) {
//        case 5: {
//            // 5 点的布局
//            int x[5] = {
//                screenWidth / 2,          // 中心
//                screenWidth / 6,          // 左上
//                (screenWidth * 5) / 6,    // 右上
//                (screenWidth * 5) / 6,     // 右下
//                 screenWidth / 6          // 左下
//            };
//            int y[5] = {
//                screenHeight / 2,         // 中心
//                screenHeight / 6,         // 左上
//                screenHeight / 6,         // 右上
//                (screenHeight * 5) / 6,    // 右下
//                (screenHeight * 5) / 6   // 左下
//            };

//            // 绘制每个点
//            FILE *fp;
//            fp = fopen(".//calibpoint.txt","w+");
//            for (int i = 0; i < 5; i++) {
////                painter.drawEllipse(x[i], y[i], 5, 5);

//            // 设置填充颜色为红色
//                painter.setBrush(Qt::black);

//                // 设置边框为透明（无边框）
//                painter.setPen(Qt::NoPen);

//               // 绘制点
//               painter.drawEllipse(x[i]-50, y[i]-50, 100, 100);

//               painter.setBrush(Qt::red);
//               // 绘制点
//               painter.drawEllipse(x[i]-5, y[i]-5, 10, 10);


//                fprintf( fp, "%d %d \n", x[i],y[i]);

//                }
//            fclose(fp);

//            break;
//        }

//        case 9: {

//        int x[9], y[9];
//        for (int i = 0; i < 3; ++i) {
//            for (int j = 0; j < 3; ++j) {
//                int index = i * 3 + j;
//                x[index] = screenWidth * (j * 0.5 + 0.05) / 1.1; // Positions at ~4.5%, ~50%, ~95.5%
//                y[index] = screenHeight * (i * 0.5 + 0.05) / 1.1; // Positions at ~4.5%, ~50%, ~95.5%
//            }
//        }



//            // 绘制每个点
//            FILE *fp;
//            fp = fopen(".//calibpoint.txt","w+");

//            for (int i = 0; i < 9; i++) {
//                // 设置填充颜色为红色
//                    painter.setBrush(Qt::black);

//                    // 设置边框为透明（无边框）
//                    painter.setPen(Qt::NoPen);

//                   // 绘制点
//                   painter.drawEllipse(x[i]-50, y[i]-50, 100, 100);

//                   painter.setBrush(Qt::red);
//                   // 绘制点
//                   painter.drawEllipse(x[i]-5, y[i]-5, 10, 10);
//                fprintf( fp, "%d %d \n", x[i],y[i]);
//            }

//            fclose(fp);
//            break;
//        }

//        case 10: {
//        painter.setPen(Qt::black);
//        painter.setBrush(QBrush(Qt::black));


//        int centerX = screenWidth / 2;
//        int centerY = screenHeight / 2;

//        // 动态计算缩放比例（保持40%屏幕边距）
//        const float scaleFactor = qMin(screenWidth, screenHeight) * 0.4f / 10.0f;

//        // 12点标定坐标系配置（相对于中心点的归一化坐标）
//        const QVector<QPoint> calibPoints = {
//            // 垂直轴（Y轴）
//            QPoint(0, 10),   QPoint(0, 8),    QPoint(0, 4),
//            QPoint(0, -4),   QPoint(0, -8),   QPoint(0, -10),
//            // 水平轴（X轴）
//            QPoint(-10, 0),  QPoint(-8, 0),   QPoint(-4, 0),
//            QPoint(4, 0),    QPoint(8, 0),    QPoint(10, 0)
//        };

//        // 转换到实际屏幕坐标
//        QVector<QPoint> screenPoints;
//        for (const auto& pt : calibPoints) {
//            int x = centerX + pt.x() * scaleFactor;
//            int y = centerY + pt.y() * scaleFactor;
//            screenPoints.append(QPoint(x, y));
//        }

//        // 绘制标定点
//        FILE *fp = fopen("./calibpoint.txt", "w+");
//        for (const auto& pt : screenPoints) {
//            // 绘制大尺寸黑色背景
//            painter.setBrush(Qt::black);
//            painter.setPen(Qt::NoPen);
//            painter.drawEllipse(pt.x()-25, pt.y()-25, 50, 50);

//            // 绘制小尺寸红色前景
//            painter.setBrush(Qt::red);
//            painter.drawEllipse(pt.x()-5, pt.y()-5, 10, 10);


//        }

//        for (const auto& pt : calibPoints) {

//            // 记录坐标到文件
//            fprintf(fp, "%d %d\n", pt.x(), pt.y());
//        }
//        fclose(fp);

//        // 可选：绘制中心十字线（调试用）
//        painter.setPen(Qt::blue);
//        painter.drawLine(centerX-20, centerY, centerX+20, centerY);
//        painter.drawLine(centerX, centerY-20, centerX, centerY+20);


//        break;
//    }

//        case 12: {
//            // 12 点的 4x3 网格布局

//        int x[12], y[12];
//        for (int i = 0; i < 3; ++i) {
//            for (int j = 0; j < 4; ++j) {
//                int index = i * 4 + j;
//                if (index < 12) {
//                    x[index] = screenWidth * (j * 2 + 1) / 8;  // 修改：横向上每个圆圈的间距相等，且不会碰到边缘
//                    y[index] = screenHeight * (i * 2 + 1) / 6;  // 修改：纵向上每个圆圈的间距相等，且不会碰到边缘
//                }
//            }
//        }
//            // 绘制每个点
//            FILE *fp;
//            fp = fopen(".//calibpoint.txt","w+");
//            for (int i = 0; i < 12; i++) {
//                // 设置填充颜色为红色
//                    painter.setBrush(Qt::black);

//                    // 设置边框为透明（无边框）
//                    painter.setPen(Qt::NoPen);

//                   // 绘制点
//                   painter.drawEllipse(x[i]-50, y[i]-50, 100, 100);

//                   painter.setBrush(Qt::red);
//                   // 绘制点
//                   painter.drawEllipse(x[i]-5, y[i]-5, 10, 10);

//                 fprintf( fp, "%d %d \n", x[i],y[i]);
//            }

//            fclose(fp);
//            break;
//        }

//    }

//}


//void ShowPoint::updateCalibrationPoints(int index) {
//    // 根据下拉框的选择设置标定点数量
//    switch (index) {
//        case 0: // 5 Points
//            numberOfPoints = 5;
//            break;
//        case 1: // 9 Points
//            numberOfPoints = 9;
//            break;
//        case 2: // 9 Points
//            numberOfPoints = 10;
//            break;
//        case 3: // 12 Points
//            numberOfPoints = 12;
//            break;
//    }

//    // 更新显示
//    update();
//}

//void ShowPoint::updateImage() {
//    // 占位符，用于任何图像更新逻辑
//    // 你可以在这里添加你的图像更新逻辑

//}


// ShowPoint.cpp
#include "ShowPoint.h"
#include "ui_ShowPoint.h"
#include <QFile>
#include <QTextStream>
#include <QKeyEvent>
#include <QScreen>
#include <QPainter>
#include <QVBoxLayout>
#include <qdebug.h>
// 替换原来的构造函数
ShowPoint::ShowPoint(int numberOfPoints, QWidget *parent)
    : QDialog(parent), ui(new Ui::ShowPoint), numberOfPoints(numberOfPoints)
{
    ui->setupUi(this);
    this->showFullScreen();
    setFocusPolicy(Qt::StrongFocus);

    generateCalibrationPoints(); // 根据传入的 numberOfPoints 创建点
    theTimer.setInterval(1000);
    connect(&theTimer, &QTimer::timeout, this, &ShowPoint::updateFlashing);
    theTimer.start();
}

ShowPoint::~ShowPoint() {
    delete ui;
}

void ShowPoint::generateCalibrationPoints() {
    currentPoints.clear();

    QRect screenRect = QApplication::desktop()->screenGeometry();
    int w = screenRect.width();
    int h = screenRect.height();

    switch (numberOfPoints) {
    case 3:
        currentPoints = {
            QPoint(w / 6, h / 6),           // 左上
            QPoint(w * 5 / 6, h / 6),       // 右上
            QPoint(w / 2, h * 5 / 6)        // 中下
        };
        break;

    case 4:
        currentPoints = {
            QPoint(w / 6, h / 6),
            QPoint(w * 5 / 6, h / 6),
            QPoint(w * 5 / 6, h * 5 / 6),
            QPoint(w / 6, h * 5 / 6)
        };
        break;
    case 5:
        currentPoints = {
            QPoint(w / 2, h / 2),
            QPoint(w / 6, h / 6),
            QPoint(w * 5 / 6, h / 6),
            QPoint(w * 5 / 6, h * 5 / 6),
            QPoint(w / 6, h * 5 / 6)
        };
        break;
    case 9:
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                currentPoints.append(QPoint(
                    w * (j * 0.5 + 0.05) / 1.1,
                    h * (i * 0.5 + 0.05) / 1.1));
        break;
    case 10: {
        int cx = w / 2, cy = h / 2;
        float s = qMin(w, h) * 0.4f / 10.0f;
        QVector<QPoint> base = {
            {0, -10}, {0, -8}, {0, -4}, {0, 4}, {0, 8}, {0, 10},
            {-10, 0}, {-8, 0}, {-4, 0}, {4, 0}, {8, 0}, {10, 0}
        };
        for (auto pt : base)
            currentPoints.append(QPoint(cx + pt.x() * s, cy + pt.y() * s));
        break;
    }
    case 12:
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 4; ++j)
                currentPoints.append(QPoint(
                    w * (j * 2 + 1) / 8,
                    h * (i * 2 + 1) / 6));
        break;
    case 16:
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                currentPoints.append(QPoint(
                    w * (j * 2 + 1) / 8,
                    h * (i * 2 + 1) / 8));
        break;
    }

        QFile file("calibpoint.txt");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            for (const auto &point : currentPoints) {
                out << point.x() << " " << point.y() << "\n";
            }
            file.close();
        } else {
            qDebug() << "无法打开文件 calibpoint.txt 进行写入";
        }

    currentPointIndex = 0;
    flashCount = 0;
    flashState = 1;
}

void ShowPoint::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (currentPointIndex >= currentPoints.size()) return;

    QPoint pt = currentPoints[currentPointIndex];

    painter.setPen(Qt::NoPen);

    // 黑色圆圈 - 根据闪烁状态显示或隐藏
    if (flashState) {
        painter.setBrush(Qt::black);
        painter.drawEllipse(pt.x() - 20, pt.y() - 20, 40, 40);
    }

    // 红色圆点 - 始终显示，不闪烁
    painter.setBrush(Qt::red);
    painter.drawEllipse(pt.x() - 5, pt.y() - 5, 10, 10);
}

void ShowPoint::updateFlashing() {
    flashState = !flashState;

        if (flashState) {
            flashCount++;
            if (flashCount >= 1) {  // 每个点闪两次（4 个状态：红-黑-红-黑）
                currentPointIndex++;
                flashCount = 0;

                if (currentPointIndex >= currentPoints.size()) {
                    theTimer.stop();  //
                    update();         // 清空屏幕上残留
                    close();
                    return;
                }
            }
        }

        update();
}

void ShowPoint::updateCalibrationPoints(int index) {
    switch (index) {
    case 0: numberOfPoints = 3; break;
    case 1: numberOfPoints = 4; break;
    case 2: numberOfPoints = 5; break;
    case 3: numberOfPoints = 9; break;
    case 4: numberOfPoints = 10; break;
    case 5: numberOfPoints = 12; break;
    case 6: numberOfPoints = 16; break;
    }
    generateCalibrationPoints();
    update();
}
