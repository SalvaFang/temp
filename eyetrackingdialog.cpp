#include "eyetrackingdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QtMath>
#include <QApplication>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QRadialGradient>
#include <QConicalGradient>
#include <QRandomGenerator>
#include <QScreen>
#include <QApplication>
#include <limits>

class TrailOverlay : public QWidget
{
public:
    TrailOverlay(QWidget* parent) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
        // 尝试这些属性组合
        setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_AlwaysStackOnTop);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        static_cast<EyeTrackingDialog*>(parent())->drawGazeTrail(painter);
    }
};




EyeTrackingDialog::EyeTrackingDialog(QWidget *parent)
    : QDialog(parent)
    , m_isGridMode(false)
    , m_gazeUpdateTimer(new QTimer(this))
    , m_gazeScaleAnimation(nullptr)
    , m_gazeFadeAnimation(nullptr)
    , m_gazeAnimationGroup(nullptr)
    , m_gazeTrailTimer(new QTimer(this))
    , m_attractionTimer(new QTimer(this))
    , m_attractionAnimation(nullptr)
    , m_isAttracting(false)
    , m_attractionCompleted(false)
    , m_attractedButtonIndex(-1)
    , m_scaleAnimation(nullptr)
    , m_opacityEffect(nullptr)
    , m_buttonAnimationGroup(nullptr)
    , m_confirmationAnimationGroup(nullptr)
    , m_hoverTimer(new QTimer(this))
    , m_confirmationTimer(new QTimer(this))
    , m_sectorTimeoutTimer(new QTimer(this))
    , m_autoResetTimer(new QTimer(this))
    , m_buttonMoveTimer(new QTimer(this))
    , m_resultTextTimer(new QTimer(this))
    , m_sectorsVisible(false)
    , m_sectorsStable(false)
    , m_currentHoveredButton(-1)
    , m_candidateButton(-1)
    , m_isConfirming(false)
    , m_gazeVisible(true)
    , m_inSectorMode(false)
    , m_resultTextVisible(false)
    , m_resultTextOpacity(1.0)
    , m_confirmedButtonCount(0)
    , m_buttonActivated(false)
    , m_activatedButtonIndex(-1)
    , m_hideGazeAfterTrigger(true)
    , m_gazeFeedbackTimer(new QTimer(this))
    , m_currentGazingButton(-1)
{
    setupUI();
    setupButtons();
    setupGazeIndicator();
    setupResultDisplay();
    initializeButtonMovement();
    // startAllButtonsMoving(); // 禁用按钮运动

    // 配置定时器
    m_hoverTimer->setSingleShot(true);
    m_confirmationTimer->setSingleShot(true);
    m_sectorTimeoutTimer->setSingleShot(true);
    m_autoResetTimer->setSingleShot(true);
    m_resultTextTimer->setSingleShot(true);
    m_attractionTimer->setSingleShot(false);
    m_attractionTimer->setInterval(16); // 60 FPS
    m_gazeUpdateTimer->setSingleShot(false);
    m_gazeUpdateTimer->setInterval(16); // 60 FPS更新
    m_buttonMoveTimer->setSingleShot(false);
    m_buttonMoveTimer->setInterval(50); // 20 FPS移动更新
    m_gazeTrailTimer->setSingleShot(false);
    m_gazeTrailTimer->setInterval(33); // 30 FPS轨迹更新
    m_gazeFeedbackTimer->setSingleShot(false);
    m_gazeFeedbackTimer->setInterval(GAZE_FEEDBACK_UPDATE_INTERVAL); // 60 FPS注视反馈更新

    connect(m_hoverTimer, &QTimer::timeout, this, &EyeTrackingDialog::onHoverTimerTimeout);
    connect(m_confirmationTimer, &QTimer::timeout, this, &EyeTrackingDialog::onConfirmationTimerTimeout);
    connect(m_sectorTimeoutTimer, &QTimer::timeout, this, &EyeTrackingDialog::onSectorTimeoutTimeout);
    connect(m_autoResetTimer, &QTimer::timeout, this, &EyeTrackingDialog::onAutoResetTimeout);
    connect(m_gazeUpdateTimer, &QTimer::timeout, this, &EyeTrackingDialog::updateGazeDisplay);
    connect(m_attractionTimer, &QTimer::timeout, this, &EyeTrackingDialog::updateGazeAttraction);
    connect(m_buttonMoveTimer, &QTimer::timeout, this, &EyeTrackingDialog::updateButtonMovement);
    connect(m_resultTextTimer, &QTimer::timeout, this, &EyeTrackingDialog::hideResultText);
    connect(m_gazeTrailTimer, &QTimer::timeout, this, &EyeTrackingDialog::updateGazeTrail);
    connect(m_gazeFeedbackTimer, &QTimer::timeout, this, [this]() {
        if (m_currentGazingButton >= 0 && m_currentGazingButton < m_gazeFeedbacks.size()) {
            updateGazeFeedback(m_currentGazingButton);
        }
    });

    setWindowTitle("AR眼动交互演示 - Tobii风格轨迹");

    // 设置全屏显示，适合AR眼镜
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    //showFullScreen();

    // 启动定时器
    m_gazeUpdateTimer->start();
    // m_buttonMoveTimer->start(); // 禁用按钮移动
    m_gazeTrailTimer->start();
    m_gazeFeedbackTimer->start(); // 启动注视反馈定时器

    // 设置背景色为深色，适合AR眼镜显示
    setStyleSheet("QDialog { background-color: #0a0a0a; }");

    // 强制确保所有按钮都显示为蓝色
    for (int i = 0; i < m_buttons.size(); ++i) {
        m_buttons[i]->setStyleSheet(getButtonStyleSheet(i, false, false, false));
    }

    // 初始化扇形区域
    m_confirmSector.type = SECTOR_CONFIRM;
    m_confirmSector.visible = false;
    m_confirmSector.radius = SECTOR_RADIUS;
    m_confirmSector.spanAngle = SECTOR_SPAN_ANGLE;

    m_cancelSector.type = SECTOR_CANCEL;
    m_cancelSector.visible = false;
    m_cancelSector.radius = SECTOR_RADIUS;
    m_cancelSector.spanAngle = SECTOR_SPAN_ANGLE;

    // 初始化gaze相关变量
    m_currentGazePoint = QPoint(-100, -100);
    m_smoothedGazePoint = m_currentGazePoint;
    m_displayGazePoint = m_currentGazePoint;
    m_realGazePoint = m_currentGazePoint;
    m_previousGazePoint = m_currentGazePoint;
    m_gazeVelocity = QPoint(0, 0);

    // 初始化注视反馈状态（25个按钮）
    for (int i = 0; i < 25; ++i) {
        GazeFeedback feedback;
        feedback.buttonIndex = i;
        m_gazeFeedbacks.append(feedback);
    }

    // 在构造函数最后（show() 调用之前）添加：
    // 在构造函数中
    m_trailOverlay = new TrailOverlay(this);
    m_trailOverlay->resize(size());
    m_trailOverlay->raise(); // 确保在最顶层
    m_trailOverlay->show();
}

EyeTrackingDialog::~EyeTrackingDialog()
{
    // 停止所有定时器
        if (m_gazeUpdateTimer) {
            m_gazeUpdateTimer->stop();
        }
        if (m_attractionTimer) {
            m_attractionTimer->stop();
        }
        if (m_buttonMoveTimer) {
            m_buttonMoveTimer->stop();
        }
        if (m_gazeTrailTimer) {
            m_gazeTrailTimer->stop();
        }
        if (m_gazeFeedbackTimer) {
            m_gazeFeedbackTimer->stop();
        }
        if (m_hoverTimer) {
            m_hoverTimer->stop();
        }
        if (m_confirmationTimer) {
            m_confirmationTimer->stop();
        }
        if (m_sectorTimeoutTimer) {
            m_sectorTimeoutTimer->stop();
        }
        if (m_autoResetTimer) {
            m_autoResetTimer->stop();
        }
        if (m_resultTextTimer) {
            m_resultTextTimer->stop();
        }

        // 清理动画
        if (m_scaleAnimation) {
            m_scaleAnimation->stop();
            m_scaleAnimation->deleteLater();
            m_scaleAnimation = nullptr;
        }
        if (m_gazeAnimationGroup) {
            m_gazeAnimationGroup->stop();
            m_gazeAnimationGroup->deleteLater();
            m_gazeAnimationGroup = nullptr;
        }
        if (m_buttonAnimationGroup) {
            m_buttonAnimationGroup->stop();
            m_buttonAnimationGroup->deleteLater();
            m_buttonAnimationGroup = nullptr;
        }
        if (m_confirmationAnimationGroup) {
            m_confirmationAnimationGroup->stop();
            m_confirmationAnimationGroup->deleteLater();
            m_confirmationAnimationGroup = nullptr;
        }
        if (m_attractionAnimation) {
            m_attractionAnimation->stop();
            m_attractionAnimation->deleteLater();
            m_attractionAnimation = nullptr;
        }

        // 清理按钮特效动画
        for (auto* animation : m_buttonEffectAnimations) {
            if (animation) {
                animation->stop();
                animation->deleteLater();
            }
        }
        m_buttonEffectAnimations.clear();

        // 清理按钮
        for (QPushButton* button : m_buttons) {
            if (button) {
                button->deleteLater();
            }
        }
        m_buttons.clear();
}

void EyeTrackingDialog::setupUI()
{
    // 不使用传统布局，改为绝对定位以实现均匀随机分布
    // 注意：不能设置layout为nullptr，会导致错误
}


void EyeTrackingDialog::bringToTop(QPushButton* activeButton)
{
    if (!activeButton) return;

    // 保存其他按钮引用
    m_otherButtons.clear();
    for (QPushButton* button : m_buttons) {
        if (button != activeButton) {
            m_otherButtons.append(button);
        }
    }

    // 先把所有按钮降低层级
    for (QPushButton* button : m_otherButtons) {
        button->lower();
    }

    // 置顶当前按钮
    activeButton->raise();

    // 将确定和取消扇形区域置顶（确保它们在按钮之上）
    if (m_confirmSector.visible) {
        // 可以使用类似raise方法来确保它们在最上层
        // 这里假设m_confirmSector与m_cancelSector是QPainterPath或者可以通过绘制等方式管理
        update(); // 触发重绘
    }

    if (m_cancelSector.visible) {
        update(); // 触发重绘
    }

    // 更新绘制界面（确保新的层级被反映）
    update();
}


void EyeTrackingDialog::setupButtons()
{
    // 创建固定网格布局的按钮位置
    QRect screenRect = QApplication::primaryScreen()->geometry();
    createGridLayout(screenRect.width(), screenRect.height(), 25);

    // 创建25个按钮，均匀随机分布，初始颜色相同
    for (int i = 0; i < 25; ++i) {
        QPushButton* button = new QPushButton(QString("功能 %1").arg(i + 1), this);

        // 设置按钮样式，初始颜色相同（蓝色）
        button->setStyleSheet(getButtonStyleSheet(i, false, false, false));

        // 设置按钮大小为圆形
        button->setFixedSize(BUTTON_MIN_SIZE, BUTTON_MIN_SIZE);

        // 设置按钮位置
        button->move(m_buttonPositions[i]);
        button->show();

        m_buttons.append(button);

        // 为每个按钮设置属性，用于识别
        button->setProperty("buttonIndex", i);

        // 创建按钮特效动画组
        m_buttonEffectAnimations.append(nullptr);
    }
}

void EyeTrackingDialog::setupGazeIndicator()
{
    // Gaze指示器将通过paintEvent绘制
    m_gazeVisible = true;
}

void EyeTrackingDialog::setupResultDisplay()
{
    // 结果显示相关初始化
    m_resultText = "";
    m_resultTextColor = Qt::white;
    m_resultTextVisible = false;
}

void EyeTrackingDialog::initializeButtonMovement()
{
    // 初始化按钮移动状态
    m_buttonMovements.clear();
    for (int i = 0; i < 9; ++i) {
        ButtonMovement movement;
        movement.targetPosition = generateNewTargetPosition(i);
        movement.velocity = QPoint(0, 0);
        movement.lastMoveTime = QDateTime::currentMSecsSinceEpoch() - 1000; // 立即开始移动
        movement.isMoving = true; // 一开始就运动
        m_buttonMovements.append(movement);
    }
}

void EyeTrackingDialog::startAllButtonsMoving()
{
    // 让所有按钮开始运动
    for (int i = 0; i < m_buttonMovements.size(); ++i) {
        m_buttonMovements[i].isMoving = true;
        m_buttonMovements[i].targetPosition = generateNewTargetPosition(i);
        m_buttonMovements[i].lastMoveTime = QDateTime::currentMSecsSinceEpoch();
    }
}

QColor EyeTrackingDialog::getButtonColor(int buttonIndex, bool isActivated)
{
    if (isActivated) {
        // 激活状态：使用不同颜色
        QList<QColor> colors = {
            QColor(102, 126, 234),  // 蓝紫色
            QColor(239, 68, 68),    // 红色
            QColor(34, 197, 94),    // 绿色
            QColor(251, 146, 60),   // 橙色
            QColor(168, 85, 247),   // 紫色
            QColor(14, 165, 233),   // 天蓝色
            QColor(245, 101, 101),  // 粉红色
            QColor(132, 204, 22)    // 青绿色
        };
        return colors[buttonIndex % colors.size()];
    } else {
        // 初始状态：所有按钮相同颜色（蓝色）
        return QColor(102, 126, 234);
    }
}

QString EyeTrackingDialog::getButtonStyleSheet(int buttonIndex, bool isHovered, bool isConfirming, bool isActivated)
{
    QColor baseColor = getButtonColor(buttonIndex, isActivated);

    if (isConfirming) {
        // 确认状态：橙色
        return QString(
            "QPushButton {"
            "  background: qradialgradient(cx:0.5, cy:0.5, radius:1, "
            "    fx:0.3, fy:0.3, stop:0 #ff9800, stop:0.7 #f57c00, stop:1 #ef6c00);"
            "  color: white;"
            "  border: 3px solid #e65100;"
            "  padding: 12px;"
            "  border-radius: 45px;"
            "  font-size: 15px;"
            "  font-weight: bold;"
            "  text-align: center;"
            "}"
        );
    } else if (isHovered) {
        // 悬停状态：亮化基础颜色
        int r = qMin(255, baseColor.red() + 50);
        int g = qMin(255, baseColor.green() + 50);
        int b = qMin(255, baseColor.blue() + 50);
        QColor hoverColor(r, g, b);

        return QString(
            "QPushButton {"
            "  background: qradialgradient(cx:0.5, cy:0.5, radius:1, "
            "    fx:0.3, fy:0.3, stop:0 %1, stop:0.7 %2, stop:1 %3);"
            "  color: white;"
            "  border: 3px solid %4;"
            "  padding: 12px;"
            "  border-radius: 45px;"
            "  font-size: 13px;"
            "  font-weight: bold;"
            "  text-align: center;"
            "}"
        ).arg(hoverColor.name())
         .arg(baseColor.name())
         .arg(baseColor.darker(120).name())
         .arg(baseColor.darker(150).name());
    } else {
        // 正常状态 - 实心蓝色填充
        return QString(
            "QPushButton {"
            "  background-color: %1;"
            "  color: white;"
            "  border: 2px solid %2;"
            "  padding: 12px;"
            "  border-radius: 45px;"
            "  font-size: 13px;"
            "  font-weight: bold;"
            "  text-align: center;"
            "}"
            "QPushButton:hover {"
            "  background-color: %3;"
            "  border: 3px solid %4;"
            "}"
        ).arg(baseColor.name())
         .arg(baseColor.darker(150).name())
         .arg(baseColor.lighter(120).name())
         .arg(baseColor.lighter(130).name());
    }
}

void EyeTrackingDialog::createUniformRandomLayout()
{
    m_buttonPositions.clear();

    // 获取实际屏幕尺寸而不是窗口尺寸
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    // 如果窗口已经有有效尺寸，使用窗口尺寸
    if (this->width() > 100 && this->height() > 100) {
        screenWidth = this->width();
        screenHeight = this->height();
    }

    qDebug() << "创建按钮布局，使用尺寸:" << screenWidth << "x" << screenHeight;

    // 增大可用区域，减少边距
    int margin = qMin(SCREEN_MARGIN, 50); // 减少边距
    int availableWidth = screenWidth - 2 * margin - BUTTON_MIN_SIZE;
    int availableHeight = screenHeight - 2 * margin - BUTTON_MIN_SIZE;

    if (availableWidth <= 0) availableWidth = screenWidth * 0.8;
    if (availableHeight <= 0) availableHeight = screenHeight * 0.8;

    QRandomGenerator* generator = QRandomGenerator::global();
    int maxAttempts = 2000; // 增加尝试次数
    int buttonCount = 8;

    // 使用真正的均匀网格分布 - 25个按钮
    createGridLayout(screenWidth, screenHeight, 25);
}

// 新增：固定位置的网格分布
void EyeTrackingDialog::createGridLayout(int screenWidth, int screenHeight, int buttonCount)
{
    m_buttonPositions.clear();

    // 设置网格模式标志
    m_isGridMode = (buttonCount == 25);

    if (buttonCount == 25) {
        // 25个按钮使用5x5网格布局，绝对均匀分布
        int cols = 5;
        int rows = 5;

        // 增大边距，确保所有按钮都在可见区域内
        int margin = BUTTON_MIN_SIZE + 30; // 加大边距避免遮挡

        // 计算网格间距：将剩余空间均匀分配
        int stepX = (screenWidth - 2 * margin) / (cols - 1);
        int stepY = (screenHeight - 2 * margin) / (rows - 1);

        for (int i = 0; i < buttonCount; ++i) {
            int col = i % cols;           // 列索引 0-4
            int row = i / cols;           // 行索引 0-4

            // 绝对均匀分布：不做任何边界修正，严格按照网格计算
            int x = margin + col * stepX;
            int y = margin + row * stepY;

            m_buttonPositions.append(QPoint(x, y));
//            qDebug() << "按钮" << (i+1) << "绝对均匀网格位置:" << QPoint(x, y) << "行" << row << "列" << col;
        }
    } else {
        // 原来的9个按钮的3x3网格布局
        QList<QPoint> fixedPositions = {
            QPoint(400, 200),   // 按钮1: 左上
            QPoint(800, 200),   // 按钮2: 中上
            QPoint(1200, 200),  // 按钮3: 右上
            QPoint(400, 400),   // 按钮4: 左中
            QPoint(800, 400),   // 按钮5: 中心
            QPoint(1200, 400),  // 按钮6: 右中
            QPoint(400, 600),   // 按钮7: 左下
            QPoint(800, 600),   // 按钮8: 中下
            QPoint(1200, 600)   // 按钮9: 右下
        };

        // 改进的屏幕尺寸适配
        for (int i = 0; i < qMin(buttonCount, fixedPositions.size()); ++i) {
            QPoint pos = fixedPositions[i];

            // 改进的尺寸适配：使用智能缩放而非硬编码分辨率
            const int referenceWidth = 1600;
            const int referenceHeight = 800;

            if (screenWidth > 100 && screenHeight > 100) {
                // 计算缩放比例，使用较小的比例以保持纵横比
                double scaleX = static_cast<double>(screenWidth) / referenceWidth;
                double scaleY = static_cast<double>(screenHeight) / referenceHeight;
                double scale = qMin(scaleX, scaleY); // 使用较小的比例保持纵横比

                // 应用缩放
                pos.setX(static_cast<int>(pos.x() * scale));
                pos.setY(static_cast<int>(pos.y() * scale));

                // 如果使用统一缩放后仍有空间，进行居中调整
                if (scale == scaleX && scaleY > scaleX) {
                    // 水平受限，垂直居中
                    int verticalOffset = static_cast<int>((screenHeight - referenceHeight * scale) / 2);
                    pos.setY(pos.y() + verticalOffset);
                } else if (scale == scaleY && scaleX > scaleY) {
                    // 垂直受限，水平居中
                    int horizontalOffset = static_cast<int>((screenWidth - referenceWidth * scale) / 2);
                    pos.setX(pos.x() + horizontalOffset);
                }
            }

            // 改进的边界检查：确保整个按钮都在屏幕内，添加安全边距
            int safeMargin = 10;
            int minX = BUTTON_MIN_SIZE / 2 + safeMargin;
            int maxX = screenWidth - BUTTON_MIN_SIZE / 2 - safeMargin;
            int minY = BUTTON_MIN_SIZE / 2 + safeMargin;
            int maxY = screenHeight - BUTTON_MIN_SIZE / 2 - safeMargin;

            pos.setX(qMax(minX, qMin(pos.x(), maxX)));
            pos.setY(qMax(minY, qMin(pos.y(), maxY)));

            m_buttonPositions.append(pos);
            qDebug() << "按钮" << (i+1) << "修正固定位置:" << pos;
        }
    }
}

// 新增：分区布局方法
void EyeTrackingDialog::createZonedLayout(int screenWidth, int screenHeight, int buttonCount)
{
    // 将屏幕分为多个区域，每个区域放置一个按钮
    int cols = 4;
    int rows = 2;
    int zoneWidth = screenWidth / cols;
    int zoneHeight = screenHeight / rows;

    QRandomGenerator* generator = QRandomGenerator::global();

    for (int i = 0; i < buttonCount; ++i) {
        int zoneX = i % cols;
        int zoneY = i / cols;

        // 在每个区域内随机选择位置，但避开边缘
        int minX = zoneX * zoneWidth + BUTTON_MIN_SIZE;
        int maxX = (zoneX + 1) * zoneWidth - BUTTON_MIN_SIZE;
        int minY = zoneY * zoneHeight + BUTTON_MIN_SIZE;
        int maxY = (zoneY + 1) * zoneHeight - BUTTON_MIN_SIZE;

        // 确保范围有效
        if (maxX <= minX) maxX = minX + BUTTON_MIN_SIZE;
        if (maxY <= minY) maxY = minY + BUTTON_MIN_SIZE;

        int x = minX + generator->bounded(qMax(1, maxX - minX));
        int y = minY + generator->bounded(qMax(1, maxY - minY));

        m_buttonPositions.append(QPoint(x, y));
    }
}


bool EyeTrackingDialog::isPositionValid(const QPoint& pos, const QList<QPoint>& existingPositions)
{
    // 检查与现有位置的最小距离
    for (const QPoint& existingPos : existingPositions) {
        double distance = qSqrt(qPow(pos.x() - existingPos.x(), 2) +
                               qPow(pos.y() - existingPos.y(), 2));
        if (distance < MIN_BUTTON_DISTANCE) {
            return false;
        }
    }
    return true;
}

void EyeTrackingDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);

    // 窗口大小改变时重新计算网格布局
    if (!m_buttons.isEmpty()) {
        createGridLayout(this->width(), this->height(), 25);
        for (int i = 0; i < m_buttons.size() && i < m_buttonPositions.size(); ++i) {
            m_buttons[i]->move(m_buttonPositions[i]);
            if (i < m_buttonMovements.size()) {
                m_buttonMovements[i].targetPosition = generateNewTargetPosition(i);
            }
        }
    }

    // 新增这部分：
    if (m_trailOverlay) {
        m_trailOverlay->resize(size());
    }
}


void EyeTrackingDialog::updateButtonMovement()
{
    // 网格模式下禁用随机移动，保持固定位置
    if (m_isGridMode) {
        return; // 直接返回，不执行任何移动
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    for (int i = 0; i < m_buttons.size() && i < m_buttonMovements.size(); ++i) {
        ButtonMovement& movement = m_buttonMovements[i];
        QPushButton* button = m_buttons[i];

        // 每3-6秒随机改变目标位置（仅非网格模式）
        if (currentTime - movement.lastMoveTime > (3000 + QRandomGenerator::global()->bounded(3000))) {
            movement.targetPosition = generateNewTargetPosition(i);
            movement.lastMoveTime = currentTime;
            movement.isMoving = true;
        }

        // 如果正在移动，更新位置
        if (movement.isMoving) {
            QPoint currentPos = button->pos();
            QPoint diff = movement.targetPosition - currentPos;
            double distance = qSqrt(diff.x() * diff.x() + diff.y() * diff.y());

            if (distance < BUTTON_MOVE_SPEED * 2) {
                // 到达目标位置
                button->move(movement.targetPosition);
                movement.isMoving = false;
                movement.velocity = QPoint(0, 0);
            } else {
                // 继续移动
                double moveX = (diff.x() / distance) * BUTTON_MOVE_SPEED;
                double moveY = (diff.y() / distance) * BUTTON_MOVE_SPEED;

                QPoint newPos = currentPos + QPoint(static_cast<int>(moveX), static_cast<int>(moveY));
                button->move(newPos);
            }
        }
    }
}

QPoint EyeTrackingDialog::generateNewTargetPosition(int buttonIndex)
{
    // 生成新的目标位置，确保与其他按钮保持足够距离
    int screenWidth = this->width();
    int screenHeight = this->height();

    if (screenWidth <= 0) screenWidth = 1920;
    if (screenHeight <= 0) screenHeight = 1080;

    int availableWidth = screenWidth - 2 * SCREEN_MARGIN - BUTTON_MIN_SIZE;
    int availableHeight = screenHeight - 2 * SCREEN_MARGIN - BUTTON_MIN_SIZE;

    if (availableWidth <= 0) availableWidth = 800;
    if (availableHeight <= 0) availableHeight = 600;

    QRandomGenerator* generator = QRandomGenerator::global();
    int maxAttempts = 200; // 增加尝试次数

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        int x = SCREEN_MARGIN + generator->bounded(availableWidth);
        int y = SCREEN_MARGIN + generator->bounded(availableHeight);
        QPoint newPos(x, y);

        // 检查与其他按钮的距离（包括当前位置和目标位置）
        bool validPosition = true;
        for (int j = 0; j < m_buttons.size(); ++j) {
            if (j == buttonIndex) continue;

            // 检查与当前位置的距离
            QPoint currentPos = m_buttons[j]->pos();
            double currentDistance = qSqrt(qPow(newPos.x() - currentPos.x(), 2) +
                                          qPow(newPos.y() - currentPos.y(), 2));

            // 检查与目标位置的距离
            QPoint targetPos = (j < m_buttonMovements.size())
                              ? m_buttonMovements[j].targetPosition
                              : currentPos;
            double targetDistance = qSqrt(qPow(newPos.x() - targetPos.x(), 2) +
                                         qPow(newPos.y() - targetPos.y(), 2));

            // 确保与当前位置和目标位置都保持足够距离
            if (currentDistance < MIN_BUTTON_DISTANCE || targetDistance < MIN_BUTTON_DISTANCE) {
                validPosition = false;
                break;
            }
        }

        if (validPosition) {
            return newPos;
        }
    }

    // 如果找不到合适位置，返回当前位置
    return m_buttons[buttonIndex]->pos();
}

// Gaze轨迹相关函数
void EyeTrackingDialog::addGazeTrailPoint(const QPoint& point)
{
    if (point.x() < 0 || point.y() < 0) return;

    GazeTrailPoint trailPoint;
    trailPoint.position = point;
    trailPoint.timestamp = QDateTime::currentMSecsSinceEpoch();
    trailPoint.opacity = 1.0;

    m_gazeTrail.enqueue(trailPoint);

    // 限制轨迹点数量
    while (m_gazeTrail.size() > GAZE_TRAIL_MAX_POINTS) {
        m_gazeTrail.dequeue();
    }
}

void EyeTrackingDialog::updateGazeTrail() {
    if (m_trailOverlay) {
        m_trailOverlay->raise(); // 确保始终在最顶层
        m_trailOverlay->update(); // 强制重绘
    }
}
void EyeTrackingDialog::updateGazeTrailOpacity()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    for (auto& point : m_gazeTrail) {
        qint64 age = currentTime - point.timestamp;
        if (age < GAZE_TRAIL_DURATION) {
            // 根据时间计算透明度
            point.opacity = 1.0 - (static_cast<double>(age) / GAZE_TRAIL_DURATION);
        } else {
            point.opacity = 0.0;
        }
    }
}

void EyeTrackingDialog::clearOldTrailPoints()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    while (!m_gazeTrail.isEmpty()) {
        const GazeTrailPoint& oldest = m_gazeTrail.head();
        if (currentTime - oldest.timestamp > GAZE_TRAIL_DURATION) {
            m_gazeTrail.dequeue();
        } else {
            break;
        }
    }
}

void EyeTrackingDialog::drawGazeTrail(QPainter& painter)
{
    if (m_gazeTrail.isEmpty()) return;

    painter.setRenderHint(QPainter::Antialiasing);

    // 将队列转换为列表以便遍历
    QList<GazeTrailPoint> trailList;
    for (const auto& point : m_gazeTrail) {
        trailList.append(point);
    }

    // 绘制轨迹线
    if (trailList.size() > 1) {
        QPen trailPen;
        trailPen.setWidth(2);
        trailPen.setCapStyle(Qt::RoundCap);
        trailPen.setJoinStyle(Qt::RoundJoin);

        for (int i = 1; i < trailList.size(); ++i) {
            const GazeTrailPoint& prev = trailList[i-1];
            const GazeTrailPoint& curr = trailList[i];

            if (prev.opacity > 0 && curr.opacity > 0) {
                // 使用平均透明度
                qreal avgOpacity = (prev.opacity + curr.opacity) / 2.0;
                QColor lineColor(100, 200, 255, static_cast<int>(avgOpacity * 255));
                trailPen.setColor(lineColor);
                painter.setPen(trailPen);

                painter.drawLine(prev.position, curr.position);
            }
        }
    }

    // 绘制轨迹点
    for (const GazeTrailPoint& point : m_gazeTrail) {
        if (point.opacity > 0) {
            QColor pointColor(150, 220, 255, static_cast<int>(point.opacity * 200));
            painter.setBrush(QBrush(pointColor));
            painter.setPen(Qt::NoPen);

            int size = static_cast<int>(3 + point.opacity * 2);
            painter.drawEllipse(point.position, size, size);
        }
    }
}


void EyeTrackingDialog::paintEvent(QPaintEvent* event)
{
    QDialog::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制 gaze 指示器
    if (m_gazeVisible && m_displayGazePoint.x() >= 0 && m_displayGazePoint.y() >= 0) {
        // 外圈 - 根据是否被吸附改变颜色
        QColor outerColor = m_isAttracting ? QColor(255, 165, 0, 220) : QColor(255, 50, 50, 200);
        QPen pen(outerColor, 3);
        painter.setPen(pen);
        int outerSize = GAZE_INDICATOR_SIZE;
        painter.drawEllipse(m_displayGazePoint.x() - outerSize/2, m_displayGazePoint.y() - outerSize/2, outerSize, outerSize);

        // 内圈填充
        QColor innerColor = m_isAttracting ? QColor(255, 200, 100, 100) : QColor(255, 100, 100, 80);
        QBrush brush(innerColor);
        painter.setBrush(brush);
        painter.setPen(Qt::NoPen);
        int innerSize = GAZE_INDICATOR_SIZE - 8;
        painter.drawEllipse(m_displayGazePoint.x() - innerSize/2, m_displayGazePoint.y() - innerSize/2, innerSize, innerSize);

        // 中心点
        QColor centerColor = m_isAttracting ? QColor(255, 140, 0, 255) : QColor(255, 0, 0, 255);
        painter.setBrush(QBrush(centerColor));
        painter.drawEllipse(m_displayGazePoint.x() - 3, m_displayGazePoint.y() - 3, 6, 6);

        // 如果正在吸附，绘制连接线
        if (m_isAttracting && m_attractedButtonIndex >= 0 && m_attractedButtonIndex < m_buttons.size()) {
            QPushButton* button = m_buttons[m_attractedButtonIndex];
            QPoint buttonCenter = button->geometry().center();

            QPen linePen(QColor(255, 165, 0, 150), 2, Qt::DashLine);
            painter.setPen(linePen);
            painter.drawLine(m_displayGazePoint, buttonCenter);
        }
    }

    // 绘制按钮（确保按钮处于最上层）
    for (QPushButton* button : m_buttons) {
        button->raise();  // 确保按钮处于最上层
    }

    // 绘制扇形区域（确保绘制在按钮之后）
    if (m_sectorsVisible) {
        drawSectorRegions(painter);  // 扇形区域绘制
    }

    // 绘制注视反馈特效（在扇形之上，确保可见）
    drawGazeFeedback(painter);

    // 绘制结果文本
    if (m_resultTextVisible) {
        drawResultText(painter);
    }

    // 绘制 gaze 轨迹（在 gaze 指示器之前）
    //drawGazeTrail(painter);

    // 绘制按钮中心的红点（最后绘制，确保在最上层）
    for (int i = 0; i < m_buttons.size(); ++i) {
        QPushButton* button = m_buttons[i];
        QPoint buttonCenter = button->geometry().center();

        // 加大红色中心点尺寸
        painter.setBrush(QBrush(QColor(255, 0, 0, 255)));
        painter.setPen(QPen(QColor(255, 255, 255, 255), 2)); // 白色边框，更明显

        // 绘制更大的红点
        int dotSize = 12; // 从8像素增加到12像素
        painter.drawEllipse(buttonCenter.x() - dotSize/2, buttonCenter.y() - dotSize/2, dotSize, dotSize);

        // 调试信息
//        if (i == 0) {
//            qDebug() << "绘制按钮" << i << "红点，中心:" << buttonCenter << "尺寸:" << dotSize;
//        }
    }
}



void EyeTrackingDialog::drawSectorRegions(QPainter& painter)
{
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制确认扇形（右前方，绿色）
    if (m_confirmSector.visible) {
        drawBeautifulSector(painter, m_confirmSector, "✓");
    }

    // 绘制取消扇形（左前方，红色）
    if (m_cancelSector.visible) {
        drawBeautifulSector(painter, m_cancelSector, "✗");
    }


}

void EyeTrackingDialog::drawBeautifulSector(QPainter& painter, const SectorRegion& sector, const QString& icon)
{
    painter.setRenderHint(QPainter::Antialiasing);

    // 检查当前视线是否在这个扇形区域内
    bool gazeInSector = false;
    if (m_gazeVisible) {
        QPoint sectorCenter = sector.center;
        double distance = qSqrt(qPow(m_displayGazePoint.x() - sectorCenter.x(), 2) +
                               qPow(m_displayGazePoint.y() - sectorCenter.y(), 2));
        gazeInSector = (distance <= sector.radius);
    }

    if (sector.type == SECTOR_CONFIRM) {
        // 🎨 现代化确认按钮设计
        drawModernConfirmButton(painter, sector.center, sector.radius, gazeInSector);
    } else {
        // 🎨 现代化取消按钮设计
        drawModernCancelButton(painter, sector.center, sector.radius, gazeInSector);
    }
}


void EyeTrackingDialog::drawResultText(QPainter& painter)
{
    if (m_resultText.isEmpty()) return;

    painter.setRenderHint(QPainter::Antialiasing);

    // 设置字体
    QFont font("Arial", 24, QFont::Bold);
    painter.setFont(font);

    // 设置颜色和透明度
    QColor textColor = m_resultTextColor;
    textColor.setAlphaF(m_resultTextOpacity);
    painter.setPen(QPen(textColor));

    // 计算文本位置（屏幕中央上方）
    QFontMetrics metrics(font);
    QRect textRect = metrics.boundingRect(m_resultText);
    int x = (this->width() - textRect.width()) / 2;
    int y = this->height() / 4;

    // 绘制文本背景
    QRect backgroundRect = textRect.translated(x, y);
    backgroundRect.adjust(-20, -10, 20, 10);
    QColor backgroundColor = Qt::black;
    backgroundColor.setAlphaF(0.7 * m_resultTextOpacity);
    painter.fillRect(backgroundRect, backgroundColor);

    // 绘制文本
    painter.drawText(x, y + textRect.height(), m_resultText);
}

void EyeTrackingDialog::showResultText(const QString& text, const QColor& color)
{
    m_resultText = text;
    m_resultTextColor = color;
    m_resultTextVisible = true;
    m_resultTextOpacity = 1.0;

    // 启动定时器，3秒后隐藏文本
    m_resultTextTimer->start(RESULT_TEXT_DURATION);

    update(); // 触发重绘
}

void EyeTrackingDialog::hideResultText()
{
    m_resultTextVisible = false;
    update(); // 触发重绘
}

void EyeTrackingDialog::updateGazePoint(const cv::Point2f& gaze)
{
    // 将OpenCV坐标转换为Qt坐标
    QPoint newGazePoint = QPoint(static_cast<int>(gaze.x), static_cast<int>(gaze.y));

    // 更新gaze速度（用于方向检测）
    if (m_currentGazePoint.x() >= 0) {
        m_gazeVelocity = newGazePoint - m_currentGazePoint;
    }

    // 平滑处理gaze位置
    if (m_currentGazePoint.x() < 0) {
        m_currentGazePoint = newGazePoint;
        m_smoothedGazePoint = newGazePoint;
    } else {
        m_smoothedGazePoint.setX(static_cast<int>(
            m_smoothedGazePoint.x() * (1.0 - GAZE_SMOOTHING) +
            newGazePoint.x() * GAZE_SMOOTHING));
        m_smoothedGazePoint.setY(static_cast<int>(
            m_smoothedGazePoint.y() * (1.0 - GAZE_SMOOTHING) +
            newGazePoint.y() * GAZE_SMOOTHING));
    }

    m_previousGazePoint = m_currentGazePoint;
    m_currentGazePoint = newGazePoint;
    m_originalGazePoint = m_smoothedGazePoint;
    m_realGazePoint = m_smoothedGazePoint;  // 保存真实的gaze位置
    m_gazeVisible = true; // 启用内部视线绘制

    // 默认使用平滑后的位置作为显示位置
    m_displayGazePoint = m_smoothedGazePoint;

    // 注意：按钮激活逻辑统一在后面的主要检测逻辑中处理，避免重复检查

    // 暂不添加轨迹，等吸附逻辑完成后统一处理

    // 如果处于扇形选择模式
    if (m_inSectorMode && m_sectorsVisible) {
        // 使用保存的真实gaze位置进行扇形检测
        QPoint realGazeForSectorDetection = m_realGazePoint;

        // 扇形模式下，视线坐标完全锁定在激活按钮中心
        if (m_activatedButtonIndex >= 0) {
            QPushButton* activatedButton = m_buttons[m_activatedButtonIndex];
            QPoint buttonCenter = activatedButton->geometry().center();

            // 🔥 在扇形模式下，完全锁定显示和处理坐标到按钮中心
            m_displayGazePoint = buttonCenter;
            // 注意：不修改m_smoothedGazePoint，保持用于扇形检测
        }

        // 使用真实gaze位置检测扇形区域击中
        SectorType hitSector = checkSectorHit(realGazeForSectorDetection);
        if (hitSector != SECTOR_NONE) {
            addGazeTrailPoint(m_displayGazePoint);
            executeSectorAction(hitSector);
            return;
        }

        // 简化的距离检查：如果离开激活按钮太远，直接取消
        if (m_activatedButtonIndex >= 0) {
            QPushButton* activatedButton = m_buttons[m_activatedButtonIndex];
            QPoint activatedCenter = activatedButton->geometry().center();
            double distanceToActivated = qSqrt(qPow(realGazeForSectorDetection.x() - activatedCenter.x(), 2) +
                                              qPow(realGazeForSectorDetection.y() - activatedCenter.y(), 2));

            // 如果离开激活按钮的扇形操作范围（比激活范围稍大），取消
            double cancelDistance = ACTIVATION_RADIUS + SECTOR_ACTIVATED_RADIUS; // 85 + 75 = 160像素
            if (distanceToActivated > cancelDistance) {
                qDebug() << "🚫 扇形模式：离开操作范围，距离:" << distanceToActivated << "取消距离:" << cancelDistance;
                addGazeTrailPoint(m_displayGazePoint);
                resetButtonState();
                return;
            }
        }

        // 🔄 在扇形模式下不再进行按钮吸附
        addGazeTrailPoint(m_displayGazePoint);
        return;
    }

    if (m_isConfirming) {
        addGazeTrailPoint(m_displayGazePoint);
        return;
    }

    // 首先检查是否需要取消当前激活的按钮
    if (m_attractionCompleted && m_activatedButtonIndex >= 0) {
        QPushButton* activatedButton = m_buttons[m_activatedButtonIndex];
        QPoint activatedCenter = activatedButton->geometry().center();

        // 使用真实的gaze位置进行距离计算
        QPoint realGazeForDistanceCheck = m_realGazePoint;
        double distanceToActivated = qSqrt(qPow(realGazeForDistanceCheck.x() - activatedCenter.x(), 2) +
                                          qPow(realGazeForDistanceCheck.y() - activatedCenter.y(), 2));

        // 如果离开激活按钮的激活范围，立即取消（使用更严格的激活范围）
        if (distanceToActivated > ACTIVATION_RADIUS) {
            qDebug() << "🚫 离开激活按钮" << m_activatedButtonIndex << "激活范围，距离:" << distanceToActivated << "立即取消";
            resetButtonState();
            addGazeTrailPoint(m_displayGazePoint);
            return;
        }
    }

    // 查找最近的按钮进行视线吸附（使用真实gaze位置）
    int nearestButton = findNearestButton(m_realGazePoint);

    // 添加调试信息
    if (nearestButton != -1) {
        QPushButton* button = m_buttons[nearestButton];
        QPoint buttonCenter = button->geometry().center();
        double distance = qSqrt(qPow(m_realGazePoint.x() - buttonCenter.x(), 2) +
                               qPow(m_realGazePoint.y() - buttonCenter.y(), 2));
        // qDebug() << "最近按钮" << nearestButton << "距离:" << distance << "按钮半径:" << (BUTTON_MIN_SIZE/2) << "吸附:" << ATTRACTION_START_RADIUS << "激活:" << ACTIVATION_RADIUS << "已完成:" << m_attractionCompleted;
    }

    // 统一的按钮激活/停用逻辑
    int activationButtonIndex;
    bool inActivationZone = isInActivationZone(cv::Point2f(m_realGazePoint.x(), m_realGazePoint.y()), activationButtonIndex);

    // 调试信息
//    static int debugCounter = 0;
//    if (++debugCounter % 30 == 0) { // 每30帧打印一次
//        qDebug() << "🔍 调试信息 - 真实视线位置:" << m_realGazePoint << "激活状态:" << m_buttonActivated << "激活按钮:" << m_activatedButtonIndex;
//    }

    if (inActivationZone) {
        QPushButton* button = m_buttons[activationButtonIndex];
        QPoint buttonCenter = button->geometry().center();
        double distance = qSqrt(qPow(m_realGazePoint.x() - buttonCenter.x(), 2) +
                               qPow(m_realGazePoint.y() - buttonCenter.y(), 2));

        // 启动注视反馈（如果尚未开始或切换到不同按钮）
        if (m_currentGazingButton != activationButtonIndex) {
            startGazeFeedback(activationButtonIndex);
        }

        // 检查注视反馈是否已完成，只有完成后才激活按钮
        bool gazeFeedbackCompleted = false;
        if (activationButtonIndex < m_gazeFeedbacks.size()) {
            gazeFeedbackCompleted = m_gazeFeedbacks[activationButtonIndex].isCompleted;
        }

        // 只有当注视反馈完成后，才执行按钮激活逻辑
        if (gazeFeedbackCompleted) {
            // 激活按钮（如果尚未激活或切换到不同按钮）
            if (!m_buttonActivated || m_activatedButtonIndex != activationButtonIndex) {
                qDebug() << "🎯 [注视完成] 激活按钮" << activationButtonIndex << "距离:" << distance;

                // 激活按钮强制固定
                activateButtonSnap(activationButtonIndex);

                // 启动吸附动画效果
                if (!m_isAttracting || m_attractedButtonIndex != activationButtonIndex) {
                    startGazeAttraction(activationButtonIndex);
                }
                m_attractionCompleted = true;

                showSectorsAfterAttraction(activationButtonIndex);
            }

            // 强制将所有相关视线坐标固定到按钮中心
            m_displayGazePoint = buttonCenter;
            m_attractedGazePoint = buttonCenter;

            // 发射吸附坐标变化信号
            emit attractedGazeChanged(cv::Point2f(buttonCenter.x(), buttonCenter.y()), true);
        } else {
            // 注视反馈尚未完成，显示轻微吸附但不完全固定
            if (!m_isAttracting || m_attractedButtonIndex != activationButtonIndex) {
                startGazeAttraction(activationButtonIndex);
            }

            // 轻微吸附到按钮中心，但不完全固定
            m_attractedGazePoint = calculateAttractedPosition(m_realGazePoint, buttonCenter);
            m_displayGazePoint = m_attractedGazePoint;

            // 发射轻微吸附信号
            emit attractedGazeChanged(cv::Point2f(m_displayGazePoint.x(), m_displayGazePoint.y()), true);
        }

    } else {
        // 不在任何激活区域内 - 停用所有激活状态

        // 停止注视反馈
        if (m_currentGazingButton >= 0) {
            stopAllGazeFeedback();
        }

        if (m_buttonActivated) {
            qDebug() << "🚫 离开所有激活区域，停用按钮激活";
            deactivateButtonSnap();
        }

        if (m_isAttracting) {
            stopGazeAttraction();
            hideSectorRegions();
            // 发射停止吸附信号
            emit attractedGazeChanged(cv::Point2f(m_smoothedGazePoint.x(), m_smoothedGazePoint.y()), false);
        }
    }

    // 处理最近按钮的悬停效果和注视反馈（仅用于视觉反馈）
    if (nearestButton != -1) {

        // 为非激活区域内的按钮也启动注视反馈（更大范围的注视跟踪）
        QPushButton* nearestBtn = m_buttons[nearestButton];
        QPoint nearestCenter = nearestBtn->geometry().center();
        double nearestDistance = qSqrt(qPow(m_realGazePoint.x() - nearestCenter.x(), 2) +
                                      qPow(m_realGazePoint.y() - nearestCenter.y(), 2));

        // 在更大的范围内（ATTRACTION_START_RADIUS）显示注视反馈
        if (nearestDistance <= ATTRACTION_START_RADIUS) {
            if (m_currentGazingButton != nearestButton && !inActivationZone) {
                startGazeFeedback(nearestButton);
            }
        } else if (m_currentGazingButton == nearestButton && !inActivationZone) {
            // 如果离开了注视范围且不在激活区域，停止该按钮的注视反馈
            stopGazeFeedback(nearestButton);
        }

        // 处理悬停逻辑
        if (nearestButton != m_currentHoveredButton) {
            stopHoverEffect();
            m_candidateButton = nearestButton;
            startHoverEffect(nearestButton);
        }
    } else {
        // 停止吸附和悬停
        if (m_isAttracting) {
            stopGazeAttraction();
            hideSectorRegions();
            // 发射停止吸附信号
            emit attractedGazeChanged(cv::Point2f(m_smoothedGazePoint.x(), m_smoothedGazePoint.y()), false);
        }
        stopHoverEffect();
        m_candidateButton = -1;
    }

    // 最后添加轨迹点（使用最终确定的显示位置）
    addGazeTrailPoint(m_displayGazePoint);
}

void EyeTrackingDialog::showSectorsAfterAttraction(int buttonIndex)
{
    if (buttonIndex < 0 || buttonIndex >= m_buttons.size()) {
        return;
    }

    m_inSectorMode = true;
    m_activatedButtonIndex = buttonIndex;

    QPushButton* targetButton = m_buttons[buttonIndex];

    // 🔄 停止对当前按钮的吸附，让视线可以自由移动
    if (m_isAttracting && m_attractedButtonIndex == buttonIndex) {
        stopGazeAttraction();
//        qDebug() << "💡 进入扇形模式，停止按钮吸附以允许视线移动";
    }

    // 激活按钮：放大1.5倍并改变颜色
    targetButton->setFixedSize(BUTTON_ACTIVATED_SIZE, BUTTON_ACTIVATED_SIZE);
    targetButton->setStyleSheet(getButtonStyleSheet(buttonIndex, false, false, true));
    bringToTop(targetButton);  // 置顶当前按钮

    // 创建扇形区域（只保留确定区域，移除取消区域）
    createSectorRegions(buttonIndex);

    // 显示扇形区域（只显示确定区域）
    m_sectorsVisible = true;
    m_confirmSector.visible = true;
    // m_cancelSector.visible = false; // 已在createSectorRegions中设置

    // 启动100毫秒自动取消定时器
    m_autoResetTimer->start(AUTO_RESET_MS);

    targetButton->setProperty("isConfirming", true);

    // 平滑更新，避免闪烁
    update();
}


bool EyeTrackingDialog::isGazeMovingTowardsSectors()
{
    if (!m_sectorsVisible || m_activatedButtonIndex < 0) {
        return true;
    }

    QPushButton* button = m_buttons[m_activatedButtonIndex];
    QPoint buttonCenter = button->geometry().center();

    // 计算视线速度大小
    double velocityMagnitude = qSqrt(m_gazeVelocity.x() * m_gazeVelocity.x() +
                                    m_gazeVelocity.y() * m_gazeVelocity.y());

    // 放宽静止条件，减少误判
    if (velocityMagnitude < DIRECTION_THRESHOLD / 2) {
        return true; // 静止状态，不取消
    }

    // 检查是否正在向确认或取消圆形移动
    QPoint toConfirm = m_confirmSector.center - m_smoothedGazePoint;
    QPoint toCancel = m_cancelSector.center - m_smoothedGazePoint;

    // 计算到扇形的距离
    double distToConfirm = qSqrt(toConfirm.x() * toConfirm.x() + toConfirm.y() * toConfirm.y());
    double distToCancel = qSqrt(toCancel.x() * toCancel.x() + toCancel.y() * toCancel.y());

    // 如果已经很接近扇形区域，保持激活
    if (distToConfirm < SECTOR_RADIUS * 1.5 || distToCancel < SECTOR_RADIUS * 1.5) {
        return true;
    }

    // 计算视线速度与到达各圆形方向的点积
    double dotConfirm = (m_gazeVelocity.x() * toConfirm.x() + m_gazeVelocity.y() * toConfirm.y()) / velocityMagnitude;
    double dotCancel = (m_gazeVelocity.x() * toCancel.x() + m_gazeVelocity.y() * toCancel.y()) / velocityMagnitude;

    // 放宽角度阈值，从45度放宽到60度
    double threshold = qCos(qDegreesToRadians(60.0)); // 60度角度阈值（更宽松）
    return (dotConfirm > threshold) || (dotCancel > threshold);
}



void EyeTrackingDialog::startGazeAttraction(int buttonIndex)
{
    if (buttonIndex < 0 || buttonIndex >= m_buttons.size()) {
        return;
    }

    m_isAttracting = true;
    m_attractionCompleted = false; // 重置吸附完成状态
    m_attractedButtonIndex = buttonIndex;
    m_attractionTimer->start();
}

void EyeTrackingDialog::updateGazeAttraction()
{
    if (!m_isAttracting || m_attractedButtonIndex < 0 || m_attractedButtonIndex >= m_buttons.size()) {
        return;
    }

    QPushButton* button = m_buttons[m_attractedButtonIndex];
    QPoint buttonCenter = button->geometry().center();

    // 计算吸附后的位置（基于真实gaze位置）
    m_attractedGazePoint = calculateAttractedPosition(m_realGazePoint, buttonCenter);
    m_displayGazePoint = m_attractedGazePoint;

    // 发射吸附坐标变化信号
    emit attractedGazeChanged(cv::Point2f(m_displayGazePoint.x(), m_displayGazePoint.y()), m_isAttracting);
}

void EyeTrackingDialog::stopGazeAttraction()
{
    m_isAttracting = false;
    m_attractionCompleted = false; // 重置吸附完成状态
    m_attractedButtonIndex = -1;
    m_attractionTimer->stop();
    m_displayGazePoint = m_smoothedGazePoint;

    // 发射吸附停止信号
    emit attractedGazeChanged(cv::Point2f(m_displayGazePoint.x(), m_displayGazePoint.y()), false);
}

QPoint EyeTrackingDialog::calculateAttractedPosition(const QPoint& gazePos, const QPoint& targetPos)
{
    // 计算从gaze到目标的向量
    QPoint diff = targetPos - gazePos;
    double distance = qSqrt(diff.x() * diff.x() + diff.y() * diff.y());

    if (distance < COMPLETE_SNAP_RADIUS) {
        // 在完全吸附半径内，直接吸附到中心
        return targetPos;
    } else {
        // 在吸附范围内但不在完全吸附半径内，按比例吸附
        double attractionFactor = ATTRACTION_STRENGTH * (ATTRACTION_START_RADIUS - distance) / ATTRACTION_START_RADIUS;
        attractionFactor = qMax(0.0, qMin(1.0, attractionFactor));

        int newX = static_cast<int>(gazePos.x() + diff.x() * attractionFactor);
        int newY = static_cast<int>(gazePos.y() + diff.y() * attractionFactor);

        return QPoint(newX, newY);
    }
}

void EyeTrackingDialog::updateGazeDisplay()
{
    // 如果没有吸附，使用平滑后的gaze位置
    if (!m_isAttracting) {
        m_displayGazePoint = m_smoothedGazePoint;
    }

    // 最后添加轨迹点（使用最终的显示位置）
    addGazeTrailPoint(m_displayGazePoint);

    // 触发重绘
    update();
}

int EyeTrackingDialog::findNearestButton(const QPoint& gazePoint)
{
    if (m_buttons.isEmpty()) {
        return -1;
    }

    int nearestIndex = 0;
    double minDistance = std::numeric_limits<double>::max();

    for (int i = 0; i < m_buttons.size(); ++i) {
        QPushButton* button = m_buttons[i];
        QPoint buttonCenter = button->geometry().center();

        double distance = qSqrt(qPow(gazePoint.x() - buttonCenter.x(), 2) +
                               qPow(gazePoint.y() - buttonCenter.y(), 2));

        if (distance < minDistance) {
            minDistance = distance;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

void EyeTrackingDialog::startHoverEffect(int buttonIndex)
{
    if (buttonIndex < 0 || buttonIndex >= m_buttons.size()) {
        return;
    }

    QPushButton* button = m_buttons[buttonIndex];
    m_currentHoveredButton = buttonIndex;

    // 创建高级特效
    createAdvancedButtonEffect(button, buttonIndex);

    // 添加悬停高亮效果（如果没有被激活）
    if (m_activatedButtonIndex != buttonIndex) {
        button->setStyleSheet(getButtonStyleSheet(buttonIndex, true, false, false));
    }
}

void EyeTrackingDialog::createAdvancedButtonEffect(QPushButton* button, int buttonIndex)
{
    // 停止之前的特效
    if (buttonIndex < m_buttonEffectAnimations.size() && m_buttonEffectAnimations[buttonIndex]) {
        m_buttonEffectAnimations[buttonIndex]->stop();
        m_buttonEffectAnimations[buttonIndex]->deleteLater();
    }

    // 创建新的特效动画组
    QParallelAnimationGroup* effectGroup = new QParallelAnimationGroup(this);

    // 脉冲缩放动画
    QPropertyAnimation* scaleAnim = new QPropertyAnimation(button, "geometry");
    scaleAnim->setDuration(1200);
    scaleAnim->setEasingCurve(QEasingCurve::InOutSine);

    QRect originalGeometry = button->geometry();
    QRect scaledGeometry = originalGeometry;
    int scaleIncrease = 6;
    scaledGeometry.adjust(-scaleIncrease, -scaleIncrease, scaleIncrease, scaleIncrease);

    scaleAnim->setStartValue(originalGeometry);
    scaleAnim->setEndValue(scaledGeometry);
    scaleAnim->setLoopCount(-1);
    scaleAnim->setDirection(QAbstractAnimation::Forward);

    effectGroup->addAnimation(scaleAnim);
    effectGroup->start();

    // 保存动画组引用
    if (buttonIndex < m_buttonEffectAnimations.size()) {
        m_buttonEffectAnimations[buttonIndex] = effectGroup;
    }
}

void EyeTrackingDialog::stopHoverEffect()
{
    if (m_currentHoveredButton >= 0 && m_currentHoveredButton < m_buttons.size()) {
        QPushButton* button = m_buttons[m_currentHoveredButton];

        // 停止特效动画
        if (m_currentHoveredButton < m_buttonEffectAnimations.size() &&
            m_buttonEffectAnimations[m_currentHoveredButton]) {
            m_buttonEffectAnimations[m_currentHoveredButton]->stop();
        }

        // 恢复原始样式（如果没有被激活）
        if (m_activatedButtonIndex != m_currentHoveredButton) {
            button->setStyleSheet(getButtonStyleSheet(m_currentHoveredButton, false, false, false));
        }

        // 恢复原始大小
        button->setFixedSize(BUTTON_MIN_SIZE, BUTTON_MIN_SIZE);
        m_currentHoveredButton = -1;
    }
}

void EyeTrackingDialog::onHoverTimerTimeout()
{
    // 不再需要悬停超时，因为吸附后立即显示扇形
}

void EyeTrackingDialog::startConfirmationProcess(int buttonIndex)
{
    // 这个函数现在由showSectorsImmediately替代
}

void EyeTrackingDialog::createSectorRegions(int buttonIndex)
{
    if (buttonIndex < 0 || buttonIndex >= m_buttons.size()) {
        return;
    }

    QPushButton* button = m_buttons[buttonIndex];
    QPoint buttonCenter = button->geometry().center();

    // 使用激活后的大小计算距离
    int minDistance = BUTTON_ACTIVATED_SIZE/2 + SECTOR_ACTIVATED_RADIUS + 30; // 按钮半径 + 扇形半径 + 安全距离

    // 只创建确认圆形（正下方，最自然的确认位置）
    m_confirmSector.center = QPoint(
        buttonCenter.x(),                    // 水平居中对齐
        buttonCenter.y() + minDistance       // 正下方
    );
    m_confirmSector.radius = SECTOR_ACTIVATED_RADIUS; // 使用放大的半径

    // 取消区域设为不可见（移除取消功能，离开即取消）
    m_cancelSector.visible = false;

    updateSectorPaths();
}


void EyeTrackingDialog::updateSectorPaths()
{
    // 创建确认扇形路径
    m_confirmSector.path = QPainterPath();
    m_confirmSector.path.moveTo(m_confirmSector.center);
    m_confirmSector.path.arcTo(
        m_confirmSector.center.x() - m_confirmSector.radius,
        m_confirmSector.center.y() - m_confirmSector.radius,
        m_confirmSector.radius * 2,
        m_confirmSector.radius * 2,
        m_confirmSector.startAngle,
        m_confirmSector.spanAngle
    );
    m_confirmSector.path.closeSubpath();

    // 创建取消扇形路径
    m_cancelSector.path = QPainterPath();
    m_cancelSector.path.moveTo(m_cancelSector.center);
    m_cancelSector.path.arcTo(
        m_cancelSector.center.x() - m_cancelSector.radius,
        m_cancelSector.center.y() - m_cancelSector.radius,
        m_cancelSector.radius * 2,
        m_cancelSector.radius * 2,
        m_cancelSector.startAngle,
        m_cancelSector.spanAngle
    );
    m_cancelSector.path.closeSubpath();
}
SectorType EyeTrackingDialog::checkSectorHit(const QPoint& point)
{
    // 检查确认圆形区域
    if (m_confirmSector.visible) {
        QPoint confirmCenter = m_confirmSector.center;
        double confirmDistance = qSqrt(qPow(point.x() - confirmCenter.x(), 2) +
                                     qPow(point.y() - confirmCenter.y(), 2));
        if (confirmDistance <= m_confirmSector.radius) {
            return SECTOR_CONFIRM;
        }
    }

    // 检查取消圆形区域
    if (m_cancelSector.visible) {
        QPoint cancelCenter = m_cancelSector.center;
        double cancelDistance = qSqrt(qPow(point.x() - cancelCenter.x(), 2) +
                                    qPow(point.y() - cancelCenter.y(), 2));
        if (cancelDistance <= m_cancelSector.radius) {
            return SECTOR_CANCEL;
        }
    }

    return SECTOR_NONE;
}

void EyeTrackingDialog::executeSectorAction(SectorType sectorType)
{
    if (sectorType == SECTOR_CONFIRM) {
        // 视线进入确认圆形区域，执行确认操作
        qDebug() << "视线进入确认圆形区域";
        onButtonClicked();
    }
    // 取消功能已移除，离开吸附区域自动取消
}


void EyeTrackingDialog::hideSectorRegions()
{
    m_sectorsVisible = false;
    m_confirmSector.visible = false;
    m_cancelSector.visible = false;
    m_inSectorMode = false;
    update();
}

void EyeTrackingDialog::createConfirmationAnimation(QPushButton* button)
{
    // 不再需要确认动画，因为立即显示扇形
}

void EyeTrackingDialog::onAutoResetTimeout()
{
    // 100毫秒自动取消
    showResultText("自动取消", QColor(255, 165, 0));
    resetButtonState();
}

void EyeTrackingDialog::onSectorTimeoutTimeout()
{
    resetButtonState();
}

void EyeTrackingDialog::onConfirmationTimerTimeout()
{
    resetButtonState();
}

void EyeTrackingDialog::onButtonClicked()
{
    QPushButton* confirmingButton = nullptr;
    int buttonIndex = m_activatedButtonIndex;

    if (buttonIndex >= 0 && buttonIndex < m_buttons.size()) {
        confirmingButton = m_buttons[buttonIndex];
        QString buttonText = confirmingButton->text();

        // 增加确认计数
        m_confirmedButtonCount++;

        // 显示确认结果
        showResultText(QString("已确认: %1 (%2/5)").arg(buttonText).arg(m_confirmedButtonCount), QColor(76, 175, 80));

        qDebug() << "按钮被确认:" << buttonText << "(索引:" << buttonIndex << ") 总计:" << m_confirmedButtonCount;

        // 检查是否需要触发误差校正
        if (m_confirmedButtonCount >= ERROR_CORRECTION_THRESHOLD) {
            qDebug() << "触发误差校正 - 已确认" << m_confirmedButtonCount << "个按钮";
            emit errorCorrectionTriggered();

            // 重置计数器
            m_confirmedButtonCount = 0;

            // 显示误差校正提示
            showResultText("误差校正已触发", QColor(255, 165, 0));
        }
    }

    resetButtonState();
}

void EyeTrackingDialog::onGazeAnimationFinished()
{
    // Gaze动画完成后的处理
}

void EyeTrackingDialog::onAttractAnimationFinished()
{
    // 吸附动画完成后的处理
}

void EyeTrackingDialog::animateGazeIndicator()
{
    // 为gaze指示器添加动画效果（如果需要）
}

void EyeTrackingDialog::resetButtonState()
{
    // 如果有激活的按钮，显示取消结果
//    if (m_activatedButtonIndex >= 0 && m_activatedButtonIndex < m_buttons.size() &&
//        !m_resultTextVisible) { // 避免覆盖已显示的确认文本
//        QPushButton* button = m_buttons[m_activatedButtonIndex];
//        showResultText(QString("已取消: %1").arg(button->text()), QColor(244, 67, 54));
//    }



    m_isConfirming = false;
    m_inSectorMode = false;
    m_attractionCompleted = false; // 重置吸附完成状态
    m_confirmationTimer->stop();
    m_hoverTimer->stop();
    m_sectorTimeoutTimer->stop();
    m_autoResetTimer->stop();

    // 【新增】停用按钮激活
    deactivateButtonSnap();

    // 停止吸附
    if (m_isAttracting) {
        stopGazeAttraction();
    }

    // 隐藏扇形区域
    hideSectorRegions();

    // 停止所有动画
    for (int i = 0; i < m_buttonEffectAnimations.size(); ++i) {
        if (m_buttonEffectAnimations[i]) {
            m_buttonEffectAnimations[i]->stop();
        }
    }
    if (m_confirmationAnimationGroup) {
        m_confirmationAnimationGroup->stop();
    }

    // 恢复所有按钮的状态
    for (int i = 0; i < m_buttons.size(); ++i) {
        QPushButton* button = m_buttons[i];
        button->setProperty("isConfirming", false);

        // 恢复原始样式（统一颜色）
        button->setStyleSheet(getButtonStyleSheet(i, false, false, false));

        // 恢复原始大小
        button->setFixedSize(BUTTON_MIN_SIZE, BUTTON_MIN_SIZE);
    }

    m_currentHoveredButton = -1;
    m_candidateButton = -1;
    m_activatedButtonIndex = -1;
    resetZOrder();

}


void EyeTrackingDialog::resetZOrder()
{
    for (QPushButton* button : m_buttons) {
        button->raise();  // 保证所有按钮在同一层级
    }
}

// =====================================================================================
// =                           【新增】偏置校正系统                                    =
// =====================================================================================

void EyeTrackingDialog::activateButtonSnap(int buttonIndex)
{
    if (buttonIndex < 0 || buttonIndex >= m_buttons.size()) {
        return;
    }

    m_buttonActivated = true;
    m_activatedButtonIndex = buttonIndex;

    QPushButton* button = m_buttons[buttonIndex];
    QPoint buttonCenter = button->geometry().center();

    // 立即将显示的视线强制固定到按钮中心
    m_displayGazePoint = buttonCenter;
    m_attractedGazePoint = buttonCenter;

    // 隐藏视线指示器
    if (m_hideGazeAfterTrigger) {
        m_gazeVisible = false;
        qDebug() << "👁️ [视线隐藏] 按钮激活后隐藏视线指示器";
    }

    qDebug() << QString("🎯 [按钮激活] 按钮%1激活，视线强制固定到按钮中心:(%d,%d)")
                .arg(buttonIndex+1)
                .arg(buttonCenter.x()).arg(buttonCenter.y());
}

void EyeTrackingDialog::deactivateButtonSnap()
{
    if (m_buttonActivated) {
        qDebug() << "🚫 [按钮停用] 离开激活区域，恢复原始视线坐标";
    }

    m_buttonActivated = false;
    m_activatedButtonIndex = -1;

    // 恢复显示的视线为真实位置
    m_displayGazePoint = m_realGazePoint;
    m_attractedGazePoint = m_realGazePoint;

    // 恢复视线指示器显示
    if (m_hideGazeAfterTrigger) {
        m_gazeVisible = true;
        qDebug() << "👁️ [视线恢复] 离开激活区域，恢复视线指示器显示";
    }
}

bool EyeTrackingDialog::isInActivationZone(const cv::Point2f& gazePoint, int& buttonIndex)
{
    static bool s_lastWasInZone = false;
    static int s_lastButtonIndex = -1;

    // 检查视线是否在任何按钮的激活区域内
    for (int i = 0; i < m_buttons.size(); ++i) {
        QPushButton* button = m_buttons[i];
        QPoint buttonCenter = button->geometry().center();

        double distance = qSqrt(qPow(gazePoint.x - buttonCenter.x(), 2) +
                               qPow(gazePoint.y - buttonCenter.y(), 2));

        if (distance <= ACTIVATION_RADIUS) {
            buttonIndex = i;

            // 只在状态改变时打印
            if (!s_lastWasInZone || s_lastButtonIndex != i) {
                qDebug() << "✅ 视线进入激活区域 - 按钮" << i << "距离:" << distance << "阈值:" << ACTIVATION_RADIUS;
            }
            s_lastWasInZone = true;
            s_lastButtonIndex = i;
            return true;
        }
    }

    buttonIndex = -1;

    // 离开激活区域
    if (s_lastWasInZone) {
        qDebug() << "❌ 视线离开所有激活区域";
        s_lastWasInZone = false;
        s_lastButtonIndex = -1;
    }

    return false;
}


// =====================================================================================
// =                           【新增】注视反馈特效系统                                =
// =====================================================================================

void EyeTrackingDialog::startGazeFeedback(int buttonIndex)
{
    if (buttonIndex < 0 || buttonIndex >= m_gazeFeedbacks.size()) {
        return;
    }

    // 停止其他所有按钮的注视反馈
    stopAllGazeFeedback();

    GazeFeedback& feedback = m_gazeFeedbacks[buttonIndex];
    if (feedback.isGazing) {
        return; // 已经在注视中
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    feedback.buttonIndex = buttonIndex;
    feedback.startTime = currentTime;
    feedback.currentTime = currentTime;
    feedback.progress = 0.0f;
    feedback.isGazing = true;
    feedback.isCompleted = false;

    m_currentGazingButton = buttonIndex;

    qDebug() << "🎯 [注视反馈] 开始注视按钮" << buttonIndex << "进度跟踪";
}

void EyeTrackingDialog::updateGazeFeedback(int buttonIndex)
{
    if (buttonIndex < 0 || buttonIndex >= m_gazeFeedbacks.size()) {
        return;
    }

    GazeFeedback& feedback = m_gazeFeedbacks[buttonIndex];
    if (!feedback.isGazing) {
        return;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    feedback.currentTime = currentTime;

    // 计算进度 (0.0 - 1.0)
    qint64 elapsed = currentTime - feedback.startTime;
    feedback.progress = qMin(1.0f, static_cast<float>(elapsed) / static_cast<float>(GAZE_FEEDBACK_DURATION));

    // 检查是否完成
    if (feedback.progress >= 1.0f && !feedback.isCompleted) {
        feedback.isCompleted = true;
        qDebug() << "✅ [注视反馈] 按钮" << buttonIndex << "注视完成！准备激活...";

        // 注意：激活逻辑现在由主循环中的 gazeFeedbackCompleted 检查处理
        // 这里只标记完成状态，实际激活在下一帧的主逻辑中执行
    }

    // 触发重绘
    update();
}

void EyeTrackingDialog::stopGazeFeedback(int buttonIndex)
{
    if (buttonIndex < 0 || buttonIndex >= m_gazeFeedbacks.size()) {
        return;
    }

    GazeFeedback& feedback = m_gazeFeedbacks[buttonIndex];
    if (!feedback.isGazing) {
        return;
    }

    feedback.isGazing = false;
    feedback.progress = 0.0f;
    feedback.isCompleted = false;

    if (m_currentGazingButton == buttonIndex) {
        m_currentGazingButton = -1;
    }

    qDebug() << "🚫 [注视反馈] 停止注视按钮" << buttonIndex;
}

void EyeTrackingDialog::stopAllGazeFeedback()
{
    for (int i = 0; i < m_gazeFeedbacks.size(); ++i) {
        if (m_gazeFeedbacks[i].isGazing) {
            stopGazeFeedback(i);
        }
    }
    m_currentGazingButton = -1;
}

QColor EyeTrackingDialog::getProgressColor(float progress)
{
    // 更鲜艳的渐变颜色：从橙色 -> 黄色 -> 绿色 -> 青色
    if (progress < 0.33f) {
        // 橙色到黄色
        float ratio = progress * 3.0f; // 0.0 - 1.0
        return QColor(
            static_cast<int>(255 * (1.0f - ratio) + 255 * ratio),       // R: 255 -> 255
            static_cast<int>(150 * (1.0f - ratio) + 255 * ratio),       // G: 150 -> 255
            static_cast<int>(0 * (1.0f - ratio) + 0 * ratio),           // B: 0 -> 0
            240 // 更不透明
        );
    } else if (progress < 0.66f) {
        // 黄色到绿色
        float ratio = (progress - 0.33f) * 3.0f; // 0.0 - 1.0
        return QColor(
            static_cast<int>(255 * (1.0f - ratio) + 0 * ratio),         // R: 255 -> 0
            static_cast<int>(255 * (1.0f - ratio) + 255 * ratio),       // G: 255 -> 255
            static_cast<int>(0 * (1.0f - ratio) + 0 * ratio),           // B: 0 -> 0
            240
        );
    } else {
        // 绿色到青色
        float ratio = (progress - 0.66f) * 3.0f; // 0.0 - 1.0
        return QColor(
            static_cast<int>(0 * (1.0f - ratio) + 0 * ratio),           // R: 0 -> 0
            static_cast<int>(255 * (1.0f - ratio) + 255 * ratio),       // G: 255 -> 255
            static_cast<int>(0 * (1.0f - ratio) + 255 * ratio),         // B: 0 -> 255
            240
        );
    }
}

void EyeTrackingDialog::drawProgressRing(QPainter& painter, const QPoint& center, int radius, float progress, const QColor& color)
{
    if (progress <= 0.0f) return;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(color);
    pen.setWidth(PROGRESS_RING_WIDTH);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    // 计算弧形角度 (Qt使用1/16度作为单位)
    int startAngle = 90 * 16;  // 从顶部开始
    int spanAngle = static_cast<int>(-progress * 360 * 16); // 顺时针方向

    QRect rect(center.x() - radius, center.y() - radius, radius * 2, radius * 2);
    painter.drawArc(rect, startAngle, spanAngle);

    painter.restore();
}

void EyeTrackingDialog::drawGazeFeedback(QPainter& painter)
{
    for (int i = 0; i < m_gazeFeedbacks.size(); ++i) {
        const GazeFeedback& feedback = m_gazeFeedbacks[i];

        if (!feedback.isGazing || i >= m_buttons.size()) {
            continue;
        }

        QPushButton* button = m_buttons[i];
        QPoint buttonCenter = button->geometry().center();

        // 获取进度颜色
        QColor progressColor = getProgressColor(feedback.progress);

        // 绘制进度环
        drawProgressRing(painter, buttonCenter, PROGRESS_RING_OUTER_RADIUS, feedback.progress, progressColor);

        // 绘制背景环（淡灰色，显示总体进度框架）
        QPen backgroundPen(QColor(120, 120, 120, 150)); // 更明亮，更可见
        backgroundPen.setWidth(3); // 稍微粗一点
        backgroundPen.setCapStyle(Qt::RoundCap);
        painter.setPen(backgroundPen);
        painter.setBrush(Qt::NoBrush);
        QRect backgroundRect(buttonCenter.x() - PROGRESS_RING_OUTER_RADIUS,
                           buttonCenter.y() - PROGRESS_RING_OUTER_RADIUS,
                           PROGRESS_RING_OUTER_RADIUS * 2,
                           PROGRESS_RING_OUTER_RADIUS * 2);
        painter.drawEllipse(backgroundRect);

        // 显示进度百分比文本（可选）
        if (feedback.progress > 0.1f) { // 只在进度超过10%时显示
            painter.save();
            painter.setPen(QPen(progressColor));
            painter.setFont(QFont("Arial", 10, QFont::Bold));
            QString progressText = QString("%1%").arg(static_cast<int>(feedback.progress * 100));
            QRect textRect(buttonCenter.x() - 15, buttonCenter.y() - 8, 30, 16);
            painter.drawText(textRect, Qt::AlignCenter, progressText);
            painter.restore();
        }
    }
}

// =====================================================================================
// =                           【新增】现代化确认按钮设计                              =
// =====================================================================================

void EyeTrackingDialog::drawModernConfirmButton(QPainter& painter, const QPoint& center, int radius, bool isHovered)
{
    painter.setRenderHint(QPainter::Antialiasing);

    // 🎮 游戏UI设计：科幻圆形 + 发光效果 + 六边形元素

    // 1. 外层发光圆环（科幻感）
    if (isHovered) {
        for (int i = 3; i >= 1; i--) {
            int glowRadius = radius + i * 8;
            int alpha = 80 - i * 20;
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor(0, 255, 127, alpha), 3));
            painter.drawEllipse(center, glowRadius, glowRadius);
        }
    }

    // 2. 主圆形背景（深色半透明）
    QRadialGradient bgGradient(center, radius);
    if (isHovered) {
        bgGradient.setColorAt(0, QColor(40, 40, 40, 200));
        bgGradient.setColorAt(0.7, QColor(20, 20, 20, 220));
        bgGradient.setColorAt(1, QColor(0, 0, 0, 240));
    } else {
        bgGradient.setColorAt(0, QColor(30, 30, 30, 180));
        bgGradient.setColorAt(0.7, QColor(15, 15, 15, 200));
        bgGradient.setColorAt(1, QColor(0, 0, 0, 220));
    }

    painter.setBrush(QBrush(bgGradient));
    painter.setPen(QPen(QColor(0, 255, 127, isHovered ? 255 : 150), 2));
    painter.drawEllipse(center, radius, radius);

    // 3. 内层发光圆形
    int innerRadius = radius * 0.7;
    QRadialGradient innerGlow(center, innerRadius);
    if (isHovered) {
        innerGlow.setColorAt(0, QColor(0, 255, 127, 100));
        innerGlow.setColorAt(0.5, QColor(0, 200, 100, 60));
        innerGlow.setColorAt(1, QColor(0, 150, 75, 20));
    } else {
        innerGlow.setColorAt(0, QColor(0, 200, 100, 60));
        innerGlow.setColorAt(0.5, QColor(0, 150, 75, 30));
        innerGlow.setColorAt(1, QColor(0, 100, 50, 10));
    }

    painter.setBrush(QBrush(innerGlow));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, innerRadius, innerRadius);

    // 4. 科幻风格的对勾图标
    painter.setPen(QPen(QColor(0, 255, 127), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    int checkSize = radius * 0.5;
    QPoint checkStart(center.x() - checkSize/3, center.y());
    QPoint checkMiddle(center.x() - checkSize/6, center.y() + checkSize/3);
    QPoint checkEnd(center.x() + checkSize/2, center.y() - checkSize/3);

    painter.drawLine(checkStart, checkMiddle);
    painter.drawLine(checkMiddle, checkEnd);

    // 5. 装饰性的科幻元素（角落的小线条）
    painter.setPen(QPen(QColor(0, 255, 127, isHovered ? 200 : 120), 2));
    int decorRadius = radius + 15;

    // 绘制四个角的装饰线
    for (int angle = 45; angle < 360; angle += 90) {
        float rad = angle * M_PI / 180.0;
        QPoint decorStart(center.x() + cos(rad) * decorRadius,
                         center.y() + sin(rad) * decorRadius);
        QPoint decorEnd(center.x() + cos(rad) * (decorRadius + 10),
                       center.y() + sin(rad) * (decorRadius + 10));
        painter.drawLine(decorStart, decorEnd);
    }

    // 6. 悬停时的能量波纹
    if (isHovered) {
        static int waveOffset = 0; // 简单的动画偏移
        waveOffset = (waveOffset + 1) % 100;

        painter.setBrush(Qt::NoBrush);
        for (int i = 0; i < 2; i++) {
            int waveRadius = radius + 20 + (waveOffset + i * 50) % 100;
            int alpha = 100 - ((waveOffset + i * 50) % 100);
            painter.setPen(QPen(QColor(0, 255, 127, alpha), 1));
            painter.drawEllipse(center, waveRadius, waveRadius);
        }
    }
}

void EyeTrackingDialog::drawModernCancelButton(QPainter& painter, const QPoint& center, int radius, bool isHovered)
{
    painter.setRenderHint(QPainter::Antialiasing);

    // 🎮 游戏UI设计：科幻圆形 + 红色警告效果

    // 1. 外层警告发光圆环
    if (isHovered) {
        for (int i = 3; i >= 1; i--) {
            int glowRadius = radius + i * 8;
            int alpha = 80 - i * 20;
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor(255, 50, 50, alpha), 3));
            painter.drawEllipse(center, glowRadius, glowRadius);
        }
    }

    // 2. 主圆形背景（深色半透明）
    QRadialGradient bgGradient(center, radius);
    if (isHovered) {
        bgGradient.setColorAt(0, QColor(60, 20, 20, 200));
        bgGradient.setColorAt(0.7, QColor(40, 10, 10, 220));
        bgGradient.setColorAt(1, QColor(20, 0, 0, 240));
    } else {
        bgGradient.setColorAt(0, QColor(40, 20, 20, 180));
        bgGradient.setColorAt(0.7, QColor(25, 10, 10, 200));
        bgGradient.setColorAt(1, QColor(10, 0, 0, 220));
    }

    painter.setBrush(QBrush(bgGradient));
    painter.setPen(QPen(QColor(255, 70, 70, isHovered ? 255 : 150), 2));
    painter.drawEllipse(center, radius, radius);

    // 3. 内层发光圆形
    int innerRadius = radius * 0.7;
    QRadialGradient innerGlow(center, innerRadius);
    if (isHovered) {
        innerGlow.setColorAt(0, QColor(255, 70, 70, 100));
        innerGlow.setColorAt(0.5, QColor(200, 50, 50, 60));
        innerGlow.setColorAt(1, QColor(150, 30, 30, 20));
    } else {
        innerGlow.setColorAt(0, QColor(200, 60, 60, 60));
        innerGlow.setColorAt(0.5, QColor(150, 40, 40, 30));
        innerGlow.setColorAt(1, QColor(100, 20, 20, 10));
    }

    painter.setBrush(QBrush(innerGlow));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, innerRadius, innerRadius);

    // 4. 科幻风格的叉号图标
    painter.setPen(QPen(QColor(255, 100, 100), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    int crossSize = radius * 0.5;
    QPoint crossTopLeft(center.x() - crossSize/2, center.y() - crossSize/2);
    QPoint crossTopRight(center.x() + crossSize/2, center.y() - crossSize/2);
    QPoint crossBottomLeft(center.x() - crossSize/2, center.y() + crossSize/2);
    QPoint crossBottomRight(center.x() + crossSize/2, center.y() + crossSize/2);

    painter.drawLine(crossTopLeft, crossBottomRight);
    painter.drawLine(crossTopRight, crossBottomLeft);

    // 5. 装饰性的警告元素（三角形警告标志）
    painter.setPen(QPen(QColor(255, 70, 70, isHovered ? 200 : 120), 2));
    int decorRadius = radius + 15;

    // 绘制四个角的警告三角形
    for (int angle = 45; angle < 360; angle += 90) {
        float rad = angle * M_PI / 180.0;
        QPoint decorCenter(center.x() + cos(rad) * decorRadius,
                          center.y() + sin(rad) * decorRadius);

        // 绘制小三角形
        QPolygon triangle;
        triangle << QPoint(decorCenter.x(), decorCenter.y() - 5)
                << QPoint(decorCenter.x() - 4, decorCenter.y() + 3)
                << QPoint(decorCenter.x() + 4, decorCenter.y() + 3);
        painter.drawPolygon(triangle);
    }

    // 6. 悬停时的危险波纹
    if (isHovered) {
        static int waveOffset = 0; // 简单的动画偏移
        waveOffset = (waveOffset + 2) % 100; // 红色波纹更快

        painter.setBrush(Qt::NoBrush);
        for (int i = 0; i < 2; i++) {
            int waveRadius = radius + 20 + (waveOffset + i * 50) % 100;
            int alpha = 120 - ((waveOffset + i * 50) % 100);
            painter.setPen(QPen(QColor(255, 50, 50, alpha), 1));
            painter.drawEllipse(center, waveRadius, waveRadius);
        }
    }
}
