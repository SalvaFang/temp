#include "caliberror.h"
#include <QPainter>
#include <QFont>
#include <QDebug>
#include <QTimer>
#include <QDateTime>
CalibError::CalibError(int width, int height, QWidget *parent)
    : QDialog(parent)
    , m_currentPoint(-1)
    , m_recording(false)
    , m_cycleCount(0)  // 初始化cycleCount
    , m_calibrationCompleted(false)  // 初始化为未完成
    , m_dataLocked(false)  // 初始化为未锁定
    , m_freeGazeMode(false)  // 初始化T模式为关闭
    , m_gazeTrailTimer(new QTimer(this))
    , m_lastGazeTime(0)
{
    setGeometry(0, 0, width, height);
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    // 设置T模式的轨迹更新定时器
    m_gazeTrailTimer->setSingleShot(false);
    m_gazeTrailTimer->setInterval(33); // 30 FPS轨迹更新
    connect(m_gazeTrailTimer, &QTimer::timeout, this, &CalibError::updateGazeTrail);
}

CalibError::~CalibError()
{
}

void CalibError::setCalibrationData(const QVector<QPointF>& calibrationPoints,
                                    const QVector<QVector<QPointF>>& gazePoints,
                                    const QVector<QVector<float>>& errors,
                                    QColor flashColor,
                                    int currentPoint,
                                    bool recording)
{
    // 如果数据已锁定，则拒绝更新（保护完整的标定结果）
    if (m_dataLocked) {
//        qDebug() << "CalibError数据已锁定，拒绝更新数据。如需重新开始，请调用resetForNewTest()";
        return;
    }

    // 检查传入的数据：如果是空数据且不是初始状态，拒绝更新
    if (calibrationPoints.isEmpty() && !m_calibrationPoints.isEmpty()) {
        qDebug() << "CalibError收到空标定点数据，拒绝更新";
        return;
    }

    // 允许空的gazePoints（初始状态或部分完成状态）
    if (gazePoints.isEmpty() && calibrationPoints.isEmpty()) {
        qDebug() << "CalibError收到完全空数据，拒绝更新";
        return;
    }

    // 先更新数据
    m_calibrationPoints = calibrationPoints;
    m_gazePoints = gazePoints;
    m_errors = errors;
    m_flashColor = flashColor;
    m_currentPoint = currentPoint;
    m_recording = recording;

    // 检查是否完成了所有标定点（使用更新后的数据）
    if (shouldStopNextRound()) {
        m_dataLocked = true;

        // 创建备份数据
        m_backupCalibrationPoints = m_calibrationPoints;
        m_backupGazePoints = m_gazePoints;

        qDebug() << "CalibError标定完成，数据已锁定并备份";
    }

    // 计算过程在绘制时进行，不需要预先计算和存储

    qDebug() << "CalibError收到数据 - 标定点数:" << calibrationPoints.size()
             << "当前点:" << currentPoint << "注视点数据:" << gazePoints.size()
             << "数据锁定状态:" << m_dataLocked;

    // 添加闪烁效果：500毫秒后隐藏圆环
    if (recording && currentPoint >= 0 && currentPoint < calibrationPoints.size()) {
        m_flashing = true;  // 开始闪烁
        QTimer::singleShot(500, this, [this]() {
            m_flashing = false;  // 停止闪烁，变为普通红点
            update();
        });
    }

    update(); // 触发重绘
}

void CalibError::showEvent(QShowEvent *event)
{
    qDebug() << "CalibError窗口显示，重置状态";
    resetForNewTest(); // 每次窗口显示时清空数据
    QDialog::showEvent(event);
}


void CalibError::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::white);  // 白色背景

    // 如果是T模式，只绘制自由注视轨迹
    if (m_freeGazeMode) {
        drawFreeGazeTrail(painter);

        // 绘制T模式提示信息
        painter.setPen(QColor(50, 50, 50));
        painter.setFont(QFont("Arial", 16));
        painter.drawText(20, 30, "T模式：自由注视模式");
        painter.setFont(QFont("Arial", 12));
        painter.drawText(20, height() - 40, "按T键退出 | 按ESC关闭窗口");
        return;
    }

    // 配置参数（与generateErrorPlot一致）
    const int TEXT_AREA_WIDTH = 320;
    const int TEXT_START_X = 10;
    const int TEXT_START_Y = 30;
    const int LINE_HEIGHT = 22;
    const double MM_PER_PIXEL = 0.125391;
    const double VIEWING_DISTANCE_MM = 500.0;

    // 颜色定义（与generateErrorPlot一致）
    const QColor CIRCLE_COLOR = QColor(255, 200, 100);  // 浅橙色
    const QColor TEXT_COLOR = QColor(50, 50, 50);       // 深灰色
    const QColor SEPARATOR_COLOR = QColor(200, 200, 200); // 分割线颜色
    const QColor METRIC_COLOR = QColor(200, 100, 0);    // 指标文本颜色

    // 如果没有标定点数据且数据未锁定，显示等待信息
    if (m_calibrationPoints.isEmpty() && !m_dataLocked) {
        painter.setPen(TEXT_COLOR);
        painter.setFont(QFont("Arial", 16));
        painter.drawText(width()/2 - 100, height()/2, "等待标定数据...");
        painter.drawText(20, height() - 40, "按空格键开始采样");
        return;
    }

    // 如果数据被锁定但主数据为空，尝试使用备份数据
    if (m_calibrationPoints.isEmpty() && m_dataLocked && !m_backupCalibrationPoints.isEmpty()) {
        // 使用备份数据进行绘制
        QVector<QPointF> tempCalibPoints = m_backupCalibrationPoints;
        QVector<QVector<QPointF>> tempGazePoints = m_backupGazePoints;
        QVector<double> tempAccuracies = m_backupAccuracies;
        QVector<double> tempPrecisions = m_backupPrecisions;

        qDebug() << "CalibError使用备份数据绘制，备份点数:" << tempCalibPoints.size();

        // 临时恢复数据用于绘制
        m_calibrationPoints = tempCalibPoints;
        m_gazePoints = tempGazePoints;
        m_pointAccuracies = tempAccuracies;
        m_pointPrecisions = tempPrecisions;
    }

    // 如果数据被锁定但所有数据都为空，显示锁定状态
    if (m_calibrationPoints.isEmpty() && m_dataLocked) {
        painter.setPen(TEXT_COLOR);
        painter.setFont(QFont("Arial", 16));
        painter.drawText(width()/2 - 100, height()/2, "数据已锁定但丢失");
        painter.drawText(20, height() - 40, "请重新标定");
        return;
    }

    // 绘制分割线（与generateErrorPlot一致）
    painter.setPen(QPen(SEPARATOR_COLOR, 1));
    painter.drawLine(TEXT_AREA_WIDTH, 0, TEXT_AREA_WIDTH, height());



    // 绘制透明误差圆形区域
       painter.setBrush(QBrush(CIRCLE_COLOR, Qt::SolidPattern));
       painter.setPen(Qt::NoPen);
       for (int i = 0; i < m_gazePoints.size() && i < m_calibrationPoints.size(); ++i) {
           if (!m_gazePoints[i].isEmpty()) {
               // 计算平均注视点
               QPointF avgGaze(0, 0);
               for (const QPointF& gp : m_gazePoints[i]) {
                   avgGaze += gp;
               }
               avgGaze /= m_gazePoints[i].size();

               // 确保只有在注视完成后再计算误差
               if (m_gazePoints[i].size() > 0) {
                   // 计算误差
                   float error = QLineF(m_calibrationPoints[i], avgGaze).length();

                   // 绘制透明误差圆（半径为误差大小）
                   // 使用原始坐标，不进行任何加减操作
                   QPointF calibPoint = m_calibrationPoints[i];

                   if (error > 0) {
                       painter.setOpacity(0.3);
                       painter.drawEllipse(calibPoint, error, error);
                       painter.setOpacity(1.0);
                   }
               }
           }
       }

       // 绘制真实标定点（红色圆圈）
       painter.setBrush(QBrush(Qt::red, Qt::SolidPattern));
       painter.setPen(Qt::NoPen);
       for (const auto &pt : m_calibrationPoints) {
           // 使用原始坐标，不进行任何加减操作
           painter.drawEllipse(pt, 5, 5);
       }

       // 绘制预测点（绿色圆圈）
       painter.setBrush(QBrush(Qt::green, Qt::SolidPattern));
          painter.setPen(Qt::NoPen);
          for (int i = 0; i < m_gazePoints.size() && i < m_calibrationPoints.size(); ++i) {
              if (!m_gazePoints[i].isEmpty()) {
                  // 遍历并绘制每个 gaze point（每个 calibration point 对应 20 个点）
                  for (const QPointF& gazePoint : m_gazePoints[i]) {
                      // 使用原始坐标，不进行任何加减操作
                      painter.drawEllipse(gazePoint, 2, 2);  // 小圆点表示每个 gaze point
                  }
              }
          }

       // 绘制误差向量（蓝色箭头）
          painter.setPen(QPen(Qt::blue, 1));
          for (int i = 0; i < m_gazePoints.size() && i < m_calibrationPoints.size(); ++i) {
              if (!m_gazePoints[i].isEmpty()) {
                  // 计算平均注视点
                  QPointF avgGaze(0, 0);
                  for (const QPointF& gp : m_gazePoints[i]) {
                      avgGaze += gp;
                  }
                  avgGaze /= m_gazePoints[i].size();

                  // 使用原始坐标，不进行任何加减操作
                  painter.drawLine(avgGaze, m_calibrationPoints[i]);

            // 简单的箭头头部
            QPointF direction = m_calibrationPoints[i] - avgGaze;
            float length = sqrt(direction.x() * direction.x() + direction.y() * direction.y());
            if (length > 5) {
                direction /= length;
                QPointF arrowHead1 = m_calibrationPoints[i] - direction * 5 + QPointF(-direction.y(), direction.x()) * 2;
                QPointF arrowHead2 = m_calibrationPoints[i] - direction * 5 - QPointF(-direction.y(), direction.x()) * 2;
                painter.drawLine(m_calibrationPoints[i], arrowHead1);
                painter.drawLine(m_calibrationPoints[i], arrowHead2);
            }
        }
    }

          // 当前闪烁点（如果正在记录）
          if (m_recording && m_currentPoint >= 0 && m_currentPoint < m_calibrationPoints.size()) {
              QPointF currentPoint = m_calibrationPoints[m_currentPoint];

              // 绘制黑色圆环
              painter.setBrush(QBrush(Qt::black, Qt::SolidPattern));
              painter.setPen(Qt::NoPen);
              painter.drawEllipse(currentPoint, 15, 15);

              // 绘制中心红色点
              painter.setBrush(QBrush(Qt::red, Qt::SolidPattern));
              painter.setPen(Qt::NoPen);
              painter.drawEllipse(currentPoint, 5, 5);
          }


          // 绘制左侧文本信息
          int currentY = TEXT_START_Y;

          // 标题
          painter.setPen(TEXT_COLOR);
          painter.setFont(QFont("Arial", 14, QFont::Bold));
          painter.drawText(TEXT_START_X, currentY, "Eye Tracking Analysis");
          currentY += LINE_HEIGHT + 10;

          // 分割线
          painter.setPen(QPen(TEXT_COLOR, 1));
          painter.drawLine(TEXT_START_X, currentY - 5, TEXT_AREA_WIDTH - 10, currentY - 5);
          currentY += 10;

          // 每个点的误差和精确度信息
          painter.setFont(QFont("Arial", 9));
          for (int i = 0; i < m_calibrationPoints.size(); ++i) {
              if (i < m_gazePoints.size() && !m_gazePoints[i].isEmpty()) {
                  // 计算平均注视点
                  QPointF avgGaze(0, 0);
                  for (const QPointF& gp : m_gazePoints[i]) {
                      avgGaze += gp;
                  }
                  avgGaze /= m_gazePoints[i].size();

                  // 计算准确度（误差大小，单位：像素和毫米）
                  float errorPixels = QLineF(m_calibrationPoints[i], avgGaze).length();
                  double errorMM = errorPixels * MM_PER_PIXEL;

                  // 计算精确度（注视点到平均注视点的标准差，单位：像素）
                  double sumSquaredDist = 0.0;
                  for (const QPointF& gp : m_gazePoints[i]) {
                      float dist = QLineF(avgGaze, gp).length();
                      sumSquaredDist += dist * dist;
                  }
                  double precisionPixels = m_gazePoints[i].size() > 1 ? sqrt(sumSquaredDist / (m_gazePoints[i].size() - 1)) : 0.0;
                  double precisionMM = precisionPixels * MM_PER_PIXEL;

                  // 存储精确度
                  if (i >= m_pointPrecisions.size()) {
                      m_pointPrecisions.resize(i + 1);
                  }
                  m_pointPrecisions[i] = precisionPixels;

                  // 显示格式：P1: 23.5px/2.93mm (Prec: 5.2px/0.65mm)
                  QString errorText = QString("P%1: %2px/%3mm (Prec: %4px/%5mm)")
                                      .arg(i + 1)
                                      .arg(errorPixels, 0, 'f', 1)
                                      .arg(errorMM, 0, 'f', 2)
                                      .arg(precisionPixels, 0, 'f', 1)
                                      .arg(precisionMM, 0, 'f', 2);

                  painter.setPen(TEXT_COLOR);
                  painter.drawText(TEXT_START_X, currentY, errorText);
                  currentY += LINE_HEIGHT;
              } else {
                  // 没有数据的点
                  QString errorText = QString("P%1: No data").arg(i + 1);
                  painter.setPen(QColor(150, 150, 150)); // 灰色
                  painter.drawText(TEXT_START_X, currentY, errorText);
                  currentY += LINE_HEIGHT;
              }
          }

          // 添加空行
          currentY += 10;

          // 分割线
          painter.setPen(QPen(METRIC_COLOR, 1));
          painter.drawLine(TEXT_START_X, currentY - 5, TEXT_AREA_WIDTH - 10, currentY - 5);
          currentY += 5;

          // 计算总体统计信息
          if (!m_gazePoints.isEmpty()) {
              float totalError = 0;
              double totalPrecision = 0;
              int validPoints = 0;

              for (int i = 0; i < m_gazePoints.size() && i < m_calibrationPoints.size(); ++i) {
                  if (!m_gazePoints[i].isEmpty()) {
                      // 计算平均注视点
                      QPointF avgGaze(0, 0);
                      for (const QPointF& gp : m_gazePoints[i]) {
                          avgGaze += gp;
                      }
                      avgGaze /= m_gazePoints[i].size();

                      // 计算准确度
                      float error = QLineF(m_calibrationPoints[i], avgGaze).length();
                      totalError += error;

                      // 计算精确度
                      double sumSquaredDist = 0.0;
                      for (const QPointF& gp : m_gazePoints[i]) {
                          float dist = QLineF(avgGaze, gp).length();
                          sumSquaredDist += dist * dist;
                      }
                      double precisionPixels = m_gazePoints[i].size() > 1 ? sqrt(sumSquaredDist / (m_gazePoints[i].size() - 1)) : 0.0;
                      totalPrecision += precisionPixels;

                      validPoints++;
                  }
              }

              if (validPoints > 0) {
                  float avgErrorPixels = totalError / validPoints;
                  double avgErrorMM = avgErrorPixels * MM_PER_PIXEL;
                  double avgErrorDegrees = atan(avgErrorMM / VIEWING_DISTANCE_MM) * 180.0 / 3.14159;
                  double avgPrecisionPixels = totalPrecision / validPoints;
                  double avgPrecisionMM = avgPrecisionPixels * MM_PER_PIXEL;

                  // 显示总体准确度
                  painter.setPen(METRIC_COLOR);
                  painter.setFont(QFont("Arial", 10, QFont::Bold));
                  painter.drawText(TEXT_START_X, currentY, "Accuracy (avg error):");
                  currentY += LINE_HEIGHT;

                  painter.setPen(Qt::red);
                  painter.setFont(QFont("Arial", 10));
                  painter.drawText(TEXT_START_X + 10, currentY, QString("%1 px / %2 mm")
                                   .arg(avgErrorPixels, 0, 'f', 1)
                                   .arg(avgErrorMM, 0, 'f', 2));
                  currentY += LINE_HEIGHT;

                  painter.drawText(TEXT_START_X + 10, currentY, QString("%1 degrees")
                                   .arg(avgErrorDegrees, 0, 'f', 3));
                  currentY += LINE_HEIGHT;

                  // 显示总体精确度
                  painter.setPen(METRIC_COLOR);
                  painter.setFont(QFont("Arial", 10, QFont::Bold));
                  painter.drawText(TEXT_START_X, currentY, "Precision (std dev):");
                  currentY += LINE_HEIGHT;

                  painter.setPen(Qt::red);
                  painter.setFont(QFont("Arial", 10));
                  painter.drawText(TEXT_START_X + 10, currentY, QString("%1 px / %2 mm")
                                   .arg(avgPrecisionPixels, 0, 'f', 1)
                                   .arg(avgPrecisionMM, 0, 'f', 2));
                  currentY += LINE_HEIGHT;
              }
          }

          // 底部操作提示
          painter.setPen(Qt::black);
          painter.setFont(QFont("Arial", 10));
          painter.drawText(20, height() - 40, "ESC退出 | 空格继续");


}


bool CalibError::shouldStopNextRound() const
{
    // 检查是否所有标定点都有对应的注视点数据
    if (m_calibrationPoints.size() == 0) {
        return false;
    }

    // 检查每个标定点是否都有注视点数据
    for (int i = 0; i < m_calibrationPoints.size(); ++i) {
        if (i >= m_gazePoints.size() || m_gazePoints[i].isEmpty()) {
            return false;  // 还有标定点没有注视点数据
        }
    }

    return true;  // 所有标定点都有注视点数据，应该停止下一轮
}


void CalibError::resetForNewTest()
{
    qDebug() << "CalibError重置，开始新的精确测试";

    // 解锁数据
    m_dataLocked = false;

    // 清空所有数据
    m_calibrationPoints.clear();
    m_gazePoints.clear();
    m_errors.clear();
    m_pointAccuracies.clear();
    m_pointPrecisions.clear();

    // 清空备份数据
    m_backupCalibrationPoints.clear();
    m_backupGazePoints.clear();
    m_backupAccuracies.clear();
    m_backupPrecisions.clear();

    // 重置状态
    m_currentPoint = -1;
    m_recording = true;
    m_calibrationCompleted = false;
    m_flashing = true;

    // 触发重绘
    update();
}

void CalibError::setFreeGazeMode(bool enabled)
{
    m_freeGazeMode = enabled;
    if (enabled) {
        qDebug() << "CalibError T模式开启：自由注视模式";
        m_gazeTrail.clear();
        m_gazeTrailTimer->start();
    } else {
        qDebug() << "CalibError T模式关闭";
        m_gazeTrailTimer->stop();
        m_gazeTrail.clear();
    }
    update();
}

void CalibError::updateFreeGaze(const cv::Point2f& gazePoint)
{
    if (!m_freeGazeMode) return;

    QPoint qGazePoint(static_cast<int>(gazePoint.x), static_cast<int>(gazePoint.y));
    m_currentFreeGaze = qGazePoint;

    // 添加轨迹点
    addGazeTrailPoint(qGazePoint);

    update(); // 触发重绘
}

void CalibError::addGazeTrailPoint(const QPoint& point)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // 添加新的轨迹点
    m_gazeTrail.enqueue(point);
    m_lastGazeTime = currentTime;

    // 限制轨迹点数量
    while (m_gazeTrail.size() > FREE_GAZE_TRAIL_MAX_POINTS) {
        m_gazeTrail.dequeue();
    }
}

void CalibError::updateGazeTrail()
{
    // 清理过期的轨迹点
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // 如果超过持续时间没有新的注视点，清空轨迹
    if (currentTime - m_lastGazeTime > FREE_GAZE_TRAIL_DURATION) {
        m_gazeTrail.clear();
    }

    update();
}

void CalibError::drawFreeGazeTrail(QPainter& painter)
{
    if (m_gazeTrail.isEmpty()) return;

    painter.setRenderHint(QPainter::Antialiasing);

    // 将队列转换为列表以便遍历
    QList<QPoint> trailList;
    for (const auto& point : m_gazeTrail) {
        trailList.append(point);
    }

    // 绘制轨迹线
    if (trailList.size() > 1) {
        QPen trailPen;
        trailPen.setWidth(3);
        trailPen.setCapStyle(Qt::RoundCap);
        trailPen.setJoinStyle(Qt::RoundJoin);

        for (int i = 1; i < trailList.size(); ++i) {
            const QPoint& prev = trailList[i-1];
            const QPoint& curr = trailList[i];

            // 计算透明度，越新的线段越不透明
            qreal opacity = static_cast<qreal>(i) / trailList.size();
            QColor lineColor(100, 200, 255, static_cast<int>(opacity * 200));
            trailPen.setColor(lineColor);
            painter.setPen(trailPen);

            painter.drawLine(prev, curr);
        }
    }

    // 绘制轨迹点（小圆点）
    for (int i = 0; i < trailList.size(); ++i) {
        const QPoint& point = trailList[i];
        qreal opacity = static_cast<qreal>(i + 1) / trailList.size();

        QColor pointColor(150, 220, 255, static_cast<int>(opacity * 180));
        painter.setBrush(QBrush(pointColor));
        painter.setPen(Qt::NoPen);

        int radius = 3 + static_cast<int>(opacity * 2); // 半径在3-5之间
        painter.drawEllipse(point, radius, radius);
    }

    // 绘制当前注视点（与EyeTrackingDialog完全一致）
    if (!m_currentFreeGaze.isNull()) {
        const int GAZE_INDICATOR_SIZE = 25;  // 与EyeTrackingDialog一致

        // 外圈 - 红色边框
        QColor outerColor = QColor(255, 50, 50, 200);
        QPen pen(outerColor, 3);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(m_currentFreeGaze, GAZE_INDICATOR_SIZE, GAZE_INDICATOR_SIZE);

        // 内圈填充
        QColor innerColor = QColor(255, 100, 100, 80);
        QBrush brush(innerColor);
        painter.setBrush(brush);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(m_currentFreeGaze, GAZE_INDICATOR_SIZE - 8, GAZE_INDICATOR_SIZE - 8);

        // 中心点
        QColor centerColor = QColor(255, 0, 0, 255);
        painter.setBrush(QBrush(centerColor));
        painter.drawEllipse(m_currentFreeGaze, 3, 3);
    }
}

void CalibError::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        qDebug() << "ESC键按下，退出CalibError";
        close();
        return;
    }

    if (event->key() == Qt::Key_T) {
        // 切换T模式
        m_freeGazeMode = !m_freeGazeMode;
        if (m_freeGazeMode) {
            qDebug() << "T模式开启：自由注视模式";
            m_gazeTrail.clear();
            m_gazeTrailTimer->start();
        } else {
            qDebug() << "T模式关闭";
            m_gazeTrailTimer->stop();
            m_gazeTrail.clear();
        }
        update();
        return;
    }

    emit keyPressed(event);  // 发出信号给MainWindow处理
    QDialog::keyPressEvent(event);
}
