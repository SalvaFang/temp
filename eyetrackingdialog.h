#ifndef EYETRACKINGDIALOG_H
#define EYETRACKINGDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QGridLayout>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QFont>
#include <QDateTime>
#include <QQueue>
#include <QResizeEvent>
#include <opencv2/opencv.hpp>

class TrailOverlay;

// 扇形区域类型
enum SectorType {
    SECTOR_NONE = 0,
    SECTOR_CONFIRM = 1,  // 确认扇形
    SECTOR_CANCEL = 2    // 取消扇形
};

// 扇形区域结构
struct SectorRegion {
    QPoint center;          // 扇形中心点（与按钮中心相同）
    int radius;             // 扇形半径（比按钮半径大）
    int startAngle;         // 起始角度（度）
    int spanAngle;          // 扇形角度范围（度）
    SectorType type;        // 扇形类型
    bool visible;           // 是否可见
    QPainterPath path;      // 扇形路径，用于碰撞检测
};

// 按钮移动状态
struct ButtonMovement {
    QPoint targetPosition;  // 目标位置
    QPoint velocity;        // 移动速度
    qint64 lastMoveTime;    // 上次移动时间
    bool isMoving;          // 是否正在移动
};

// Gaze轨迹点
struct GazeTrailPoint {
    QPoint position;        // 位置
    qint64 timestamp;       // 时间戳
    qreal opacity;          // 透明度
};

// 注视反馈状态
struct GazeFeedback {
    int buttonIndex;        // 按钮索引
    qint64 startTime;       // 注视开始时间
    qint64 currentTime;     // 当前时间
    float progress;         // 注视进度 [0.0-1.0]
    bool isGazing;          // 是否正在注视
    bool isCompleted;       // 是否已完成
    QPropertyAnimation* progressAnimation;  // 进度动画

    GazeFeedback() : buttonIndex(-1), startTime(0), currentTime(0),
                    progress(0.0f), isGazing(false), isCompleted(false), progressAnimation(nullptr) {}
};

class EyeTrackingDialog : public QDialog
{
    Q_OBJECT

signals:
    // 吸附坐标变化信号
    void attractedGazeChanged(const cv::Point2f& attractedGaze, bool isAttracting);

    // 误差校正触发信号
    void errorCorrectionTriggered();

public:
    explicit EyeTrackingDialog(QWidget *parent = nullptr);
    ~EyeTrackingDialog();

    void updateGazePoint(const cv::Point2f& gaze);

    // 获取吸附信息的接口
    bool isAttracting() const { return m_isAttracting; }
    cv::Point2f getAttractedGaze() const { return cv::Point2f(m_displayGazePoint.x(), m_displayGazePoint.y()); }
    cv::Point2f getOriginalGaze() const { return cv::Point2f(m_originalGazePoint.x(), m_originalGazePoint.y()); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onHoverTimerTimeout();
    void onConfirmationTimerTimeout();
    void onSectorTimeoutTimeout();
    void onAutoResetTimeout();
    void onButtonClicked();
    void resetButtonState();
    void updateGazeDisplay();
    void onGazeAnimationFinished();
    void onAttractAnimationFinished();
    void updateButtonMovement();
    void hideResultText();
    void updateGazeTrail();


public:
    // 确保这个方法在 public 部分
    void drawGazeTrail(QPainter& painter);


private:
    void setupUI();
    void setupButtons();
    void setupGazeIndicator();
    void setupResultDisplay();
    void initializeButtonMovement();
    void createUniformRandomLayout();
    bool isPositionValid(const QPoint& pos, const QList<QPoint>& existingPositions);
    int findNearestButton(const QPoint& gazePoint);
    void startHoverEffect(int buttonIndex);
    void stopHoverEffect();
    void startConfirmationProcess(int buttonIndex);
    void animateGazeIndicator();
    void createButtonPulseEffect(QPushButton* button);
    void createConfirmationAnimation(QPushButton* button);

    // 视线吸附相关函数
    void startGazeAttraction(int buttonIndex);
    void updateGazeAttraction();
    void stopGazeAttraction();
    QPoint calculateAttractedPosition(const QPoint& gazePos, const QPoint& targetPos);

    // 按钮激活相关函数
    void activateButtonSnap(int buttonIndex);
    void deactivateButtonSnap();
    bool isInActivationZone(const cv::Point2f& gazePoint, int& buttonIndex);

    // 扇形区域相关函数
    void createSectorRegions(int buttonIndex);
    void drawSectorRegions(QPainter& painter);
    SectorType checkSectorHit(const QPoint& point);
    void updateSectorPaths();
    void hideSectorRegions();
    void executeSectorAction(SectorType sectorType);
    void showSectorsAfterAttraction(int buttonIndex);
    void drawBeautifulSector(QPainter& painter, const SectorRegion& sector, const QString& icon);
    void drawModernConfirmButton(QPainter& painter, const QPoint& center, int radius, bool isHovered);
    void drawModernCancelButton(QPainter& painter, const QPoint& center, int radius, bool isHovered);

    // 方向检测相关函数
    void updateGazeDirection();
    bool isGazeMovingTowardsSectors();
    void resetGazeDirection();
    void createGridLayout(int screenWidth, int screenHeight, int buttonCount);
    void createZonedLayout(int screenWidth, int screenHeight, int buttonCount);
    // 按钮移动相关函数
    void updateButtonPositions();
    QPoint generateNewTargetPosition(int buttonIndex);
    void startAllButtonsMoving();

    // 结果显示相关函数
    void showResultText(const QString& text, const QColor& color);
    void drawResultText(QPainter& painter);

    // Gaze轨迹相关函数
    void addGazeTrailPoint(const QPoint& point);
    void updateGazeTrailOpacity();

    void clearOldTrailPoints();

    // 按钮颜色和特效
    QColor getButtonColor(int buttonIndex, bool isActivated = false);
    QString getButtonStyleSheet(int buttonIndex, bool isHovered = false, bool isConfirming = false, bool isActivated = false);
    void createAdvancedButtonEffect(QPushButton* button, int buttonIndex);

    // 注视反馈特效相关函数
    void startGazeFeedback(int buttonIndex);
    void updateGazeFeedback(int buttonIndex);
    void stopGazeFeedback(int buttonIndex);
    void stopAllGazeFeedback();
    void drawGazeFeedback(QPainter& painter);
    void updateGazeProgress(int buttonIndex, float progress);
    QColor getProgressColor(float progress);
    void drawProgressRing(QPainter& painter, const QPoint& center, int radius, float progress, const QColor& color);

    // UI组件
    QList<QPushButton*> m_buttons;
    QList<QPoint> m_buttonPositions;  // 均匀随机分布的按钮位置
    QList<ButtonMovement> m_buttonMovements; // 按钮移动状态
    bool m_isGridMode;                // 是否为网格模式（用于禁用随机移动）

    // Gaze显示相关
    QTimer* m_gazeUpdateTimer;
    QPropertyAnimation* m_gazeScaleAnimation;
    QPropertyAnimation* m_gazeFadeAnimation;
    QParallelAnimationGroup* m_gazeAnimationGroup;
    TrailOverlay* m_trailOverlay = nullptr;
    // Gaze轨迹相关
    QQueue<GazeTrailPoint> m_gazeTrail;  // Gaze轨迹点队列
    QTimer* m_gazeTrailTimer;            // 轨迹更新定时器

    // 视线吸附相关
    QTimer* m_attractionTimer;
    QPropertyAnimation* m_attractionAnimation;
    bool m_isAttracting;
    bool m_attractionCompleted;      // 吸附是否完成
    int m_attractedButtonIndex;
    QPoint m_originalGazePoint;
    QPoint m_attractedGazePoint;

    // 动画和效果
    QPropertyAnimation* m_scaleAnimation;
    QGraphicsOpacityEffect* m_opacityEffect;
    QParallelAnimationGroup* m_buttonAnimationGroup;
    QSequentialAnimationGroup* m_confirmationAnimationGroup;
    QList<QParallelAnimationGroup*> m_buttonEffectAnimations; // 每个按钮的特效动画

    // 定时器
    QTimer* m_hoverTimer;
    QTimer* m_confirmationTimer;
    QTimer* m_sectorTimeoutTimer;
    QTimer* m_autoResetTimer;        // 100毫秒自动取消定时器
    QTimer* m_buttonMoveTimer;       // 按钮移动定时器
    QTimer* m_resultTextTimer;       // 结果文本显示定时器

    // 扇形区域
    SectorRegion m_confirmSector;  // 确认扇形
    SectorRegion m_cancelSector;   // 取消扇形
    bool m_sectorsVisible;         // 扇形是否可见
    bool m_sectorsStable;          // 扇形是否稳定（防闪烁）

    // 状态变量
    QPoint m_currentGazePoint;
    QPoint m_smoothedGazePoint;
    QPoint m_displayGazePoint;     // 实际显示的gaze位置（可能被吸附）
    QPoint m_realGazePoint;        // 真实的gaze位置（用于距离检测，不受吸附影响）
    QPoint m_previousGazePoint;    // 上一帧的gaze位置，用于方向检测
    QPoint m_gazeVelocity;         // gaze移动速度
    int m_currentHoveredButton;
    int m_candidateButton;
    bool m_isConfirming;
    bool m_gazeVisible;
    bool m_inSectorMode;

    // 结果显示相关
    QString m_resultText;          // 结果文本
    QColor m_resultTextColor;      // 结果文本颜色
    bool m_resultTextVisible;      // 结果文本是否可见
    qreal m_resultTextOpacity;     // 结果文本透明度

    // 误差校正相关
    int m_confirmedButtonCount;    // 已确认的按钮数量
    static const int ERROR_CORRECTION_THRESHOLD = 5;  // 触发误差校正的按钮数量

    // 按钮激活状态管理
    bool m_buttonActivated;           // 是否有按钮被激活
    int m_activatedButtonIndex;       // 当前激活的按钮索引
    bool m_hideGazeAfterTrigger;      // 触发按钮后隐藏视线指示器

    // 注视反馈状态管理
    QList<GazeFeedback> m_gazeFeedbacks;    // 每个按钮的注视反馈状态
    QTimer* m_gazeFeedbackTimer;            // 注视反馈更新定时器
    int m_currentGazingButton;              // 当前注视的按钮索引

    int m_topZValue;
    QList<QPushButton*> m_otherButtons;

    void bringToTop(QPushButton* activeButton);
    void resetZOrder();

    // AR眼镜优化常量
    static const int HOVER_THRESHOLD_MS = 800;        // 悬停时间
    static const int CONFIRMATION_TIMEOUT_MS = 1000;  // 确认超时时间
    static const int SECTOR_TIMEOUT_MS = 1000;        // 扇形区域超时时间
    static const int AUTO_RESET_MS = 1500;             // 自动取消时间（100毫秒）
    // 商业级范围设计（放大范围，便于操作）
    static const int ATTRACTION_START_RADIUS = 190;     // 开始吸附距离（2.0倍按钮半径）
    static const int ACTIVATION_RADIUS = 185;           // 激活/触发距离（1.9倍按钮半径）
    static const int COMPLETE_SNAP_RADIUS = 140;        // 完全吸附距离（0.9倍按钮半径）
    static const int SEARCH_RADIUS = 150;              // 搜索按钮的最大距离

    static const int GAZE_INDICATOR_SIZE = 25;        // Gaze指示器大小
    static const int BUTTON_MIN_SIZE = 90;            // 按钮最小尺寸
    static const int BUTTON_ACTIVATED_SIZE = 135;      // 激活后按钮大小（1.5倍）
    static const int MIN_BUTTON_DISTANCE = 300;       // 按钮间最小距离
    static const int SECTOR_RADIUS = 50;             // 扇形半径（比按钮大）
    static const int SECTOR_ACTIVATED_RADIUS = 75;    // 激活后扇形半径（1.5倍）
    static const int SECTOR_SPAN_ANGLE = 60;          // 扇形角度范围
    static const int SCREEN_MARGIN = 100;             // 屏幕边距
    static const int BUTTON_MOVE_SPEED = 2;           // 按钮移动速度（像素/帧）
    static const int RESULT_TEXT_DURATION = 1000;     // 结果文本显示时间
    static const int GAZE_TRAIL_MAX_POINTS = 10;      // 最大轨迹点数
    static const int GAZE_TRAIL_DURATION = 1000;      // 轨迹持续时间（毫秒）
    static constexpr double SCALE_FACTOR = 1.5;      // 缩放因子
    static constexpr double GAZE_SMOOTHING = 0.4;     // Gaze平滑因子
    static constexpr double ATTRACTION_STRENGTH = 1.3; // 吸附强度（增强，更明显的吸附效果）
    static constexpr double DIRECTION_THRESHOLD = 10.0; // 方向检测阈值

    // 注视反馈相关常量
    static const int GAZE_FEEDBACK_DURATION = 500;   // 注视反馈完成时间（毫秒）
    static const int GAZE_FEEDBACK_UPDATE_INTERVAL = 16; // 注视反馈更新间隔（毫秒，约60FPS）
    static const int PROGRESS_RING_WIDTH = 8;          // 进度环宽度
    static const int PROGRESS_RING_OUTER_RADIUS = 80;  // 进度环外径（比按钮大，更显眼）
    static const int PROGRESS_RING_INNER_RADIUS = 72;  // 进度环内径
};

#endif // EYETRACKINGDIALOG_H

