#ifndef CALIBERROR_H
#define CALIBERROR_H

#include <QDialog>
#include <QVector>
#include <QPointF>
#include <QColor>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QTimer>
#include <QQueue>
#include <QDateTime>
#include <opencv2/opencv.hpp>

class CalibError : public QDialog
{
    Q_OBJECT

public:
    explicit CalibError(int width, int height, QWidget *parent = nullptr);
    ~CalibError();

    void setCalibrationData(const QVector<QPointF>& calibrationPoints,
                           const QVector<QVector<QPointF>>& gazePoints,
                           const QVector<QVector<float>>& errors,
                           QColor flashColor,
                           int currentPoint,
                           bool recording);

    // MainWindow中需要的方法
    int getCycleCount() const { return m_cycleCount; }

    // 设置是否完成当前轮次的所有标定点
    void setCalibrationCompleted(bool completed) { m_calibrationCompleted = completed; }

    // 检查是否应该停止下一轮（所有标定点都有数据）
    bool shouldStopNextRound() const;

    // 解锁数据，允许重新标定
    void unlockData() { m_dataLocked = false; }

    // 检查数据是否已锁定
    bool isDataLocked() const { return m_dataLocked; }

    // T模式相关方法
    void setFreeGazeMode(bool enabled);
    bool isFreeGazeMode() const { return m_freeGazeMode; }
    void updateFreeGaze(const cv::Point2f& gazePoint);

    // 强制锁定数据（用于确保数据不被清空）
    void forceDataLock() { m_dataLocked = true; }

    // 重置所有数据，开始新的精确测试
    void resetForNewTest();


protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

signals:
    void keyPressed(QKeyEvent *event);

private:
    QVector<double> m_pointPrecisions; // 存储每个标定点的精确度（标准差，单位：像素）
    bool m_flashing = false;
    void showEvent(QShowEvent *event) override;

    QVector<QPointF> m_calibrationPoints;
    QVector<QVector<QPointF>> m_gazePoints;
    QVector<QVector<float>> m_errors;
    QColor m_flashColor;
    int m_currentPoint;
    bool m_recording;
    int m_cycleCount;  // 添加缺失的成员变量
    bool m_calibrationCompleted;  // 是否完成所有标定点
    bool m_dataLocked;  // 数据锁定状态，防止结束后被清空

    // T模式相关变量
    bool m_freeGazeMode;           // T模式状态
    QPoint m_currentFreeGaze;      // 当前自由注视点
    QQueue<QPoint> m_gazeTrail;    // 注视轨迹队列
    QTimer* m_gazeTrailTimer;      // 轨迹更新定时器
    qint64 m_lastGazeTime;         // 上次注视时间

    void drawFreeGazeTrail(QPainter& painter);
    void addGazeTrailPoint(const QPoint& point);
    void updateGazeTrailOpacity();

    // 存储每个点的准确度和精确度
    QVector<double> m_pointAccuracies;  // 每个点的准确度


    // 备份数据，防止意外丢失
    QVector<QPointF> m_backupCalibrationPoints;
    QVector<QVector<QPointF>> m_backupGazePoints;
    QVector<double> m_backupAccuracies;
    QVector<double> m_backupPrecisions;

private slots:
    void updateGazeTrail();

private:
    static const int FREE_GAZE_TRAIL_MAX_POINTS = 30;  // 最大轨迹点数
    static const int FREE_GAZE_TRAIL_DURATION = 2000;  // 轨迹持续时间（毫秒）
};

#endif // CALIBERROR_H
