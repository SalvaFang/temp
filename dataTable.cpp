#include "dataTable.h"
#include "worker.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QHeaderView>
#include <QApplication>
#include <QDateTime>
#include <QLocale>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QStandardPaths>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDialog>
#include <QScrollArea>
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QTextEdit>
#include <QTimer>
#include <QDesktopWidget>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QEvent>
#include <QSettings>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDebug>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <opencv2/opencv.hpp>

// 确保M_PI常量可用
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// FeatureDataCache类实现
void FeatureDataCache::updateFromProcessingResult(const ProcessingResult& result) {
    m_latestResult = result;
    m_updateTime = QDateTime::currentDateTime();
    m_dataValid = true;
}

QString FeatureDataCache::getFeatureValue(const QString& featureName) const {
    if (!m_dataValid) return "0";

    // 直接从ProcessingResult获取最新数据，而不是通过UI
    if (featureName == "视线屏幕X(px)") {
        return QString::number(m_latestResult.gazeScreenCoords.x, 'f', 2);
    } else if (featureName == "视线屏幕Y(px)") {
        return QString::number(m_latestResult.gazeScreenCoords.y, 'f', 2);
    } else if (featureName == "当前状态持续时长(ms)") {
        // 使用Worker计算的精确状态持续时间
        qint64 duration = 0;
        if (m_latestResult.eyeMetrics.stateStartTime > 0) {
            duration = m_latestResult.eyeMetrics.timestamp - m_latestResult.eyeMetrics.stateStartTime;
        }
        return QString::number(duration);
    } else if (featureName == "注视稳定性") {
        // 使用视线速度的倒数作为稳定性近似值
        float stability = (m_latestResult.gazeSpeed > 0) ? (1000.0f / m_latestResult.gazeSpeed) : 1.0f;
        return QString::number(stability, 'f', 3);
    } else if (featureName == "眼动类型") {
        // 根据视线速度判断眼动类型
        return (m_latestResult.gazeSpeed > 100) ? "扫视" : "注视";
    } else if (featureName == "gaze_valid") {
        return m_latestResult.gazeValid ? "true" : "false";
    }
    return "0";
}

bool FeatureDataCache::isDataFresh(int maxAgeMs) const {
    return m_dataValid && m_updateTime.msecsTo(QDateTime::currentDateTime()) < maxAgeMs;
}

// TaskUIManager类实现
TaskUIManager::TaskType TaskUIManager::getTaskType(const QString& taskName) {
    if (taskName.contains("网格点选范式") || taskName.contains("交互意图识别")) {
        return TARGET_SEARCH; // 复用TARGET_SEARCH类型
    }
    if (taskName.contains("目标搜索范式")) return TARGET_SEARCH;
    else if (taskName.contains("疲劳检测")) return FATIGUE_MONITOR;
    else if (taskName.contains("工作负荷")) return WORKLOAD_MULTITASK;
    return UNKNOWN;
}

TaskUIManager::TaskUIState TaskUIManager::getUIStateForTask(const QString& taskName) {
    TaskUIState state;
    TaskType type = getTaskType(taskName);

    switch (type) {
        // GRID_SELECTION case 已删除（范式相关）

        case TARGET_SEARCH:
            state.paradigmSettingsEnabled = true;
            state.settingsTooltip = "配置交互意图识别实验参数";
            state.statusText = "交互意图识别实验已就绪";
            state.taskDescription = "网格点选范式 - 选择意图vs无意图浏览";
            state.startButtonEnabled = true;
            break;

        default:
            state.paradigmSettingsEnabled = false;
            state.settingsTooltip = "当前任务不支持参数配置";
            state.statusText = "请选择支持的实验任务";
            state.taskDescription = "未知任务类型";
            state.startButtonEnabled = false;
            break;
    }

    return state;
}

// 常量定义 (保持不变)
const QString DataTable::TIME = "时间戳";
const QString DataTable::FRAME_NUMBER = "帧号";
const QString DataTable::CAMERA_FPS = "相机帧率(fps)";
const QString DataTable::PUPIL_FPS = "瞳孔跟踪帧率(fps)";
const QString DataTable::PUPIL_CENTER_X = "瞳孔中心X(px)";
const QString DataTable::PUPIL_CENTER_Y = "瞳孔中心Y(px)";
const QString DataTable::PUPIL_MAJOR = "瞳孔长轴(px)";
const QString DataTable::PUPIL_MINOR = "瞳孔短轴(px)";
const QString DataTable::PUPIL_WIDTH = "瞳孔宽度(px)";
const QString DataTable::PUPIL_HEIGHT = "瞳孔高度(px)";
const QString DataTable::PUPIL_DIAMETER = "瞳孔直径(px)";
const QString DataTable::PUPIL_UNDIST_DIAMETER = "瞳孔未失真直径(px)";
const QString DataTable::PUPIL_PHYSICAL_DIAMETER = "瞳孔物理直径(mm)";
const QString DataTable::PUPIL_CONFIDENCE = "瞳孔置信度";
const QString DataTable::PUPIL_OUTLINE_CONFIDENCE = "瞳孔轮廓置信度";
const QString DataTable::PUPIL_CIRCUMFERENCE = "瞳孔周长(px)";
const QString DataTable::PUPIL_RATIO = "瞳孔轴比";
const QString DataTable::EYE_OPENNESS = "眼部开合度";
const QString DataTable::BLINK_STATE = "眨眼状态";
const QString DataTable::PUPIL_VELOCITY_X = "瞳孔速度X(px/s)";
const QString DataTable::PUPIL_VELOCITY_Y = "瞳孔速度Y(px/s)";
const QString DataTable::PUPIL_SPEED = "瞳孔速度(px/s)";
const QString DataTable::PUPIL_ACCELERATION_X = "瞳孔加速度X(px/s²)";
const QString DataTable::PUPIL_ACCELERATION_Y = "瞳孔加速度Y(px/s²)";
const QString DataTable::PUPIL_SIZE_CHANGE = "瞳孔尺寸变化(%/s)";
const QString DataTable::EYE_MOVEMENT_TYPE = "眼动类型";
const QString DataTable::FIXATION_STABILITY = "注视稳定性";

// 新增扩展特征常量定义
const QString DataTable::GAZE_SCREEN_X = "视线屏幕X(px)";
const QString DataTable::GAZE_SCREEN_Y = "视线屏幕Y(px)";
const QString DataTable::GAZE_VELOCITY_X = "视线速度X(px/s)";
const QString DataTable::GAZE_VELOCITY_Y = "视线速度Y(px/s)";
const QString DataTable::GAZE_SPEED = "视线速度(px/s)";
const QString DataTable::CURRENT_STATE_DURATION = "当前状态持续时长(ms)";
const QString DataTable::SACCADE_AMPLITUDE = "扫视幅度(px)";
const QString DataTable::SCANPATH_CONVEX_HULL_AREA = "扫视路径凸包面积(px²)";
const QString DataTable::FIXATION_DENSITY = "注视密度(1/px)";
const QString DataTable::SACCADE_ENTROPY = "扫视熵(bits)";
const QString DataTable::MAIN_SEQUENCE_SLOPE = "主序关系斜率(px/s/px)";
const QString DataTable::SACCADE_PEAK_VELOCITY = "扫视速度峰值(px/s)";
const QString DataTable::BLINK_FREQUENCY = "眨眼频率(/min)";
const QString DataTable::ATTENTION_FOCUS = "注意力集中度";
const QString DataTable::INTENTION_LABEL = "交互意图";
const QString DataTable::FATIGUE_LEVEL = "疲劳等级(0-4)";
const QString DataTable::COGNITIVE_WORKLOAD = "认知负荷(0-4)";

// 新增5个特征常量定义
const QString DataTable::MEAN_PUPIL_DIAMETER = "平均瞳孔直径(px)";
const QString DataTable::PUPIL_RELATIVE_DEVIATION = "瞳孔相对偏差(%)";
const QString DataTable::BLINK_DURATION = "眨眼时长(ms)";
const QString DataTable::MEAN_FIXATION_DURATION = "平均注视时长(ms)";
const QString DataTable::FIXATION_RATE = "注视率(次/秒)";

DataTable::DataTable(bool stereoMode, QWidget *parent) : QWidget(nullptr), stereoMode(stereoMode) {
    // 忽略parent参数，确保窗口完全独立
    Q_UNUSED(parent);

    setWindowTitle("眼动数据分析 - 右侧面板");

    // 设置为完全独立的可调整大小的浮动窗口
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowTitleHint |
                   Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_DeleteOnClose, false); // 防止意外关闭时删除对象

    // 确保窗口可以调整大小
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 初始化窗口状态记忆
    m_settingsFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/EyeTracking/DataTableSettings.ini";
    QDir().mkpath(QFileInfo(m_settingsFile).absolutePath());

    // 简化的窗口管理 - 移除复杂停靠功能，仅保留基本调整大小和位置记忆

    this->updateDelay = 4; // UI更新频率控制在约60FPS


    // 初始化实验状态
    m_isExperimentRunning = false;
    m_isRecordingData = false;
    m_currentTaskIndex = 0;
    m_currentExperimentType = "";
    m_expStartTime = QDateTime();
    m_experimentTimer = new QTimer(this);
    connect(m_experimentTimer, &QTimer::timeout, this, &DataTable::updateExperimentStatus);

    // 初始化新实验流程状态机
    m_currentPhase = PHASE_IDLE;
    m_currentRound = 0;
    m_totalRounds = 0;
    m_selectionCount = 0;
    m_targetSelectionCount = 10;
    m_phaseTimer = new QTimer(this);
    m_phaseTimer->setSingleShot(true); // 在构造函数中一次性设置为单次触发
    connect(m_phaseTimer, &QTimer::timeout, this, &DataTable::onPhaseTimeout);

    // 初始化选择监控定时器
    m_selectionMonitorTimer = new QTimer(this);
    m_selectionMonitorTimer->setInterval(500); // 每500ms检查一次
    connect(m_selectionMonitorTimer, &QTimer::timeout, this, &DataTable::checkSelectionProgress);

    // 初始化选择监控状态
    m_selectionStartTime = QDateTime::currentDateTime();
    m_lastSelectionCount = 0;

    // 初始化ProcessingResult缓存
    m_hasValidProcessingResult = false;

    // 初始化权威状态机
    m_currentState = STATE_UNKNOWN;
    m_stateStartTime = QDateTime::currentDateTime();
    m_rawState = STATE_UNKNOWN;
    m_rawStateStartTime = QDateTime::currentDateTime();
    m_fixationCenter = cv::Point2f(-1, -1);
    m_blinkCount = 0;
    m_lastBlinkResetTime = QDateTime::currentDateTime();

    // 初始化扫视幅度相关状态
    m_saccadeStartPosition = cv::Point2f(-1, -1);
    m_currentSaccadeAmplitude = 0.0f;

    // 初始化扫视路径凸包面积
    m_currentConvexHullArea = 0.0f;

    // 初始化注视密度
    m_currentFixationDensity = 0.0f;
    m_previousFixationDensity = -1.0f;  // 初始化为负值，表示未初始化

    // 初始化新增3个高级特征状态变量
    m_currentSaccadeEntropy = 0.0f;
    m_previousSaccadeEntropy = -1.0f;  // 初始化为负值，表示未初始化
    m_currentMainSequenceSlope = 0.0f;
    m_previousMainSequenceSlope = -1.0f;  // 初始化为负值，表示未初始化
    m_currentSaccadePeakVelocity = 0.0f;
    m_isInSaccade = false;
    m_saccadeStartTime = QDateTime::currentDateTime();

    // 初始化新增4个特征的状态变量
    m_currentFixationRate = 0.0f;
    m_wasInFixation = false;

    // 初始化标签记录系统
    m_lastRecordedIntention = -1;
    m_lastRecordedFatigue = -1;
    m_lastRecordedWorkload = -1;

    // 初始化实时导出
    m_realTimeExportEnabled = false;
    m_realTimeExportFile = nullptr;
    m_realTimeExportStream = nullptr;
    m_exportedRecordCount = 0;
    m_realTimeExportTimer = new QTimer(this);
    m_realTimeExportTimer->setInterval(1000); // 每秒导出一次
    connect(m_realTimeExportTimer, &QTimer::timeout, this, &DataTable::onRealTimeExportTimer);

    // 创建右键菜单
    tableContextMenu = new QMenu(this);
    tableContextMenu->addAction(new QAction("Plot Value", this));
    connect(tableContextMenu, &QMenu::triggered, this, &DataTable::onContextMenuClick);

    // 设置Tab界面（这里会设置tableView和tableModel）
    setupTabInterface();

    // 设置默认位置在右侧
    positionWindowOnRightSide();

    // 加载保存的窗口几何信息
    loadWindowGeometry();

    timer.start();
}

DataTable::~DataTable() {
    // 【修复】确保析构时所有定时器和信号都被正确清理，防止内存泄漏和崩溃
    if (m_experimentTimer) {
        m_experimentTimer->stop();
        m_experimentTimer->disconnect();
    }
    if (m_phaseTimer) {
        m_phaseTimer->stop();
        m_phaseTimer->disconnect();
    }
    if (m_selectionMonitorTimer) {
        m_selectionMonitorTimer->stop();
        m_selectionMonitorTimer->disconnect();
    }
    if (m_realTimeExportTimer) {
        m_realTimeExportTimer->stop();
        m_realTimeExportTimer->disconnect();
    }

    // 保存缓存中的数据，防止数据丢失
    if (m_realTimeExportEnabled && !m_dataBuffer.isEmpty()) {
        qDebug() << "析构时保存" << m_dataBuffer.size() << "条缓存数据";
        flushDataBuffer();
    }

    // 关闭实时导出文件
    if (m_realTimeExportFile) {
        m_realTimeExportFile->close();
        delete m_realTimeExportStream;
        delete m_realTimeExportFile;
        m_realTimeExportStream = nullptr;
        m_realTimeExportFile = nullptr;
    }

    // 保存窗口几何信息
    saveWindowGeometry();
}

void DataTable::makeWindowIndependent() {
    // 公共接口：强制窗口独立显示
    setParent(nullptr);
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    show();
}

QSize DataTable::sizeHint() const {
    // 返回基于内容的合适尺寸，更紧凑的布局
    QDesktopWidget* desktop = QApplication::desktop();
    QRect screenGeometry = desktop->screenGeometry();

    // 计算实际需要的高度：各组件的高度之和，消除多余空白
    int experimentControlHeight = 140;
    int tabWidgetHeight = 400; // 进一步缩小特征表格高度
    int labelInputHeight = 160;
    int exportControlHeight = 70;
    int marginsAndSpacing = 55; // 适度增加边距和间距

    int estimatedContentHeight = experimentControlHeight + tabWidgetHeight + labelInputHeight + exportControlHeight + marginsAndSpacing;
    int reasonableHeight = qMin(estimatedContentHeight, screenGeometry.height() - 100);

    return QSize(450, reasonableHeight);
}

void DataTable::customMenuRequested(QPoint pos){
    // 获取发送信号的QTableView
    QTableView* senderTable = qobject_cast<QTableView*>(sender());
    if (!senderTable) return;

    // 获取对应的模型
    QStandardItemModel* senderModel = qobject_cast<QStandardItemModel*>(senderTable->model());
    if (!senderModel) return;

    int row = senderTable->verticalHeader()->logicalIndexAt(pos);
    if (row >= 0 && row < senderModel->rowCount()) {
        QStandardItem* headerItem = senderModel->verticalHeaderItem(row);
        if (headerItem && !headerItem->text().isEmpty()) {
            tableContextMenu->actions().first()->setData(headerItem->text());
            tableContextMenu->popup(senderTable->verticalHeader()->viewport()->mapToGlobal(pos));
        }
    }
}

// ==================== 核心数据更新逻辑 (最终修正版) ====================

void DataTable::onProcessingResult(const ProcessingResult &result) {
    // 【修复】恢复节流控制，防止高频率更新导致UI阻塞
    if (timer.elapsed() < updateDelay) {
        // 但仍需保存数据用于实时导出和状态机更新
        m_lastProcessingResult = result;
        m_hasValidProcessingResult = true;

        // 更新权威状态机（不受节流影响）
        updateEyeState(result);

        // 在实验进行中仍需记录数据
        if (m_isExperimentRunning && m_isRecordingData) {
            addDataToBuffer(result);
        }
        return;
    }
    timer.restart();

    // 1. 更新时间戳
    tableModel->setItem(0, 0, new QStandardItem(QLocale::system().toString(QDateTime::fromMSecsSinceEpoch(result.eyeMetrics.timestamp), "hh:mm:ss.zzz")));

    // 2. 准备数据源：根据瞳孔是否有效，决定使用真实数据还是“零数据”
    // 【修复】直接使用Worker已经处理好的数据，无需重复判断和清理
    // Worker层已经确保了无效瞳孔时所有参数都是0值，这里直接使用即可
    setPupilData(result.pupilData, 0);
    setEyeMetricsData(result.eyeMetrics, 0);

    // 更新权威状态机 - 这是所有扫视特征计算的核心
    updateEyeState(result);

    // 【新增】更新特征数据缓存，供范式使用
    m_featureCache.updateFromProcessingResult(result);

    // 【修改】实验运行时更新数据计数（用于范式进度显示）
    if (m_isExperimentRunning && m_isRecordingData) {
        // 使用现有的m_exportedRecordCount来统计
        // 在实时导出时会自动增加，这里无需额外处理
    }

    // 更新Tab界面的特征数据
    updateFeatureTabs(result);

    // 2.5 更新新增的基础特征 - 平均瞳孔直径
    updateMeanPupilDiameter(result.pupilData);

    // 3. 更新帧率数据
    // 索引2: CAMERA_FPS - 摄像头采集帧率 (从摄像头配置获取)
    tableModel->setItem(2, 0, new QStandardItem(QString::number(result.eyeMetrics.cameraFps, 'f', 1)));
    // 索引3: PUPIL_FPS - 瞳孔跟踪处理帧率 (Worker实际处理帧率)
    tableModel->setItem(3, 0, new QStandardItem(QString::number(result.eyeMetrics.frameRate, 'f', 1)));

    // 4. 始终保存最后一次的ProcessingResult（用于瞬时导出）
    m_lastProcessingResult = result;
    m_hasValidProcessingResult = true;

    // 5. 只在实验进行中才记录数据到缓存（避免未开始实验时积累无用数据）
    if (m_isExperimentRunning && m_isRecordingData) {
        addDataToBuffer(result);
    }
}

void DataTable::onCameraFramecount(int framecount) {
    // 这个槽函数独立更新总帧数
    tableModel->setItem(1, 0, new QStandardItem(QString::number(framecount)));
}

// 废弃的槽函数，保留空实现以确保二进制兼容性
void DataTable::onPupilData(quint64, const PupilData&, const QString&) {}
void DataTable::onStereoPupilData(quint64, const PupilData&, const PupilData&, const QString&) {}
void DataTable::onCameraFPS(double) {}
void DataTable::onProcessingFPS(double) {}

// ==================== 辅助函数 ====================

void DataTable::setPupilData(const PupilData &pupil, int column) {
    if(column > (stereoMode ? 1 : 0)) return;
    tableModel->setItem(4, column, new QStandardItem(QString::number(pupil.center.x, 'f', 2)));
    tableModel->setItem(5, column, new QStandardItem(QString::number(pupil.center.y, 'f', 2)));
    tableModel->setItem(6, column, new QStandardItem(QString::number(pupil.majorAxis(), 'f', 2)));
    tableModel->setItem(7, column, new QStandardItem(QString::number(pupil.minorAxis(), 'f', 2)));
    tableModel->setItem(8, column, new QStandardItem(QString::number(pupil.width(), 'f', 2)));
    tableModel->setItem(9, column, new QStandardItem(QString::number(pupil.height(), 'f', 2)));
    tableModel->setItem(10, column, new QStandardItem(QString::number(pupil.diameter(), 'f', 2)));
    tableModel->setItem(11, column, new QStandardItem(QString::number(pupil.undistortedDiameter, 'f', 2)));
    tableModel->setItem(12, column, new QStandardItem(QString::number(pupil.physicalDiameter, 'f', 2)));
    tableModel->setItem(13, column, new QStandardItem(QString::number(pupil.confidence, 'f', 3)));
    tableModel->setItem(14, column, new QStandardItem(QString::number(pupil.outline_confidence, 'f', 3)));
    tableModel->setItem(15, column, new QStandardItem(QString::number(pupil.circumference(), 'f', 2)));

    double ratio = 0.0;
    if (pupil.majorAxis() > 1e-6) { // 增加对除零的保护
        // 【修复】确保浮点数除法，避免整数除法导致的0值
        ratio = (double)pupil.minorAxis() / (double)pupil.majorAxis();
    }

    // 【调试】每30帧输出一次瞳孔轴比计算过程，监控修复效果
//    static int ratioDebugCount = 0;
//    if (++ratioDebugCount % 30 == 0) {
//        qDebug() << "[DataTable::setPupilData] 瞳孔轴比修复:"
//                 << "ratio=" << ratio
//                 << " majorAxis=" << pupil.majorAxis()
//                 << " minorAxis=" << pupil.minorAxis()
//                 << " size.width=" << pupil.size.width
//                 << " size.height=" << pupil.size.height;
//    }

    tableModel->setItem(16, column, new QStandardItem(QString::number(ratio, 'f', 3)));
}

void DataTable::setEyeMetricsData(const EyeMetrics &metrics, int column) {
    if(column > (stereoMode ? 1 : 0)) return;

    // 【关键】每60帧输出一次UI更新调试信息，监控修复效果
//    static int uiUpdateCount = 0;
//    if (++uiUpdateCount % 60 == 0) {
//        qDebug() << "[DataTable::setEyeMetricsData] 修复后的UI更新值:"
//                 << "speed=" << metrics.pupilSpeed
//                 << " sizeChange=" << metrics.pupilSizeChange
//                 << " velocity=(" << metrics.pupilVelocity.x << "," << metrics.pupilVelocity.y << ")";
//    }

    tableModel->setItem(17, column, new QStandardItem(QString::number(metrics.eyeOpenness, 'f', 3)));
    tableModel->setItem(18, column, new QStandardItem(QString::number(metrics.blinkState, 'f', 3)));

    // 眼动类型显示为temp01格式：与时序特征保持一致
    QString eyeMovementText;
    switch(metrics.eyeMovementType) {
        case 0: eyeMovementText = "0-扫视"; break;
        case 1: eyeMovementText = "1-注视"; break;
        case 3: eyeMovementText = "3-眨眼/无效"; break;
        default: eyeMovementText = "未知"; break;
    }

    // 【调试】每30帧输出一次眼动类型更新，监控UI显示
//    static int eyeTypeDebugCounter = 0;
//    if (++eyeTypeDebugCounter % 30 == 0) {
//        qDebug() << QString("👁️ [UI更新] 眼动类型: %1(%2) -> UI显示: %3")
//                    .arg(metrics.eyeMovementType).arg(metrics.eyeMovementType == 0 ? "扫视" :
//                         metrics.eyeMovementType == 1 ? "注视" : "其他").arg(eyeMovementText);
//    }

    tableModel->setItem(19, column, new QStandardItem(eyeMovementText));

    tableModel->setItem(20, column, new QStandardItem(QString::number(metrics.fixationStability, 'f', 3)));
}

void DataTable::onContextMenuClick(QAction *action) {
    QString value = action->data().toString();
    if (!value.isEmpty()) {
        emit createGraphPlot(value);
    }
}

// ==================== Tab界面实现 ====================

void DataTable::setupTabInterface() {
    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 创建Tab Widget - 允许自由调整大小
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_tabWidget->setMaximumHeight(680); // 限制TabWidget的最大高度
    // 移除高度限制，允许用户自由调整窗口高度

    // 创建四个Tab页面的TableView和Model
    setupBasicTab();
    setupMotionTab();
    setupTemporalTab();
    setupBehaviorTab();

    // 创建导出控制面板 - 改善样式和间距
    QHBoxLayout* exportLayout = new QHBoxLayout();
    exportLayout->setSpacing(20); // 增加水平间距

    m_exportButton = new QPushButton("导出数据", this);
    m_exportButton->setMinimumHeight(40); // 增加按钮高度
    m_exportButton->setStyleSheet("background-color: #28a745; color: white; font-weight: bold; padding: 8px 16px; border-radius: 6px; font-size: 14px;");

    m_featureSelectionButton = new QPushButton("选择特征", this);
    m_featureSelectionButton->setMinimumHeight(40); // 增加按钮高度
    m_featureSelectionButton->setStyleSheet("background-color: #6c757d; color: white; font-weight: bold; padding: 8px 16px; border-radius: 6px; font-size: 14px;");

    // 添加清空数据按钮
    QPushButton* clearDataButton = new QPushButton("清空数据", this);
    clearDataButton->setMinimumHeight(40);
    clearDataButton->setStyleSheet("background-color: #dc3545; color: white; font-weight: bold; padding: 8px 16px; border-radius: 6px; font-size: 14px;");
    connect(clearDataButton, &QPushButton::clicked, this, &DataTable::clearDataBuffer);

    // 导出数据类型选择
    m_exportDataTypeLabel = new QLabel("导出类型:", this);
    m_exportDataTypeLabel->setStyleSheet("font-size: 14px; font-weight: bold;");

    m_exportDataTypeComboBox = new QComboBox(this);
    m_exportDataTypeComboBox->addItem("瞬时数据", "instantaneous");
    m_exportDataTypeComboBox->addItem("实时数据", "realtime");
    m_exportDataTypeComboBox->setCurrentIndex(1); // 默认选择"实时数据"
    m_exportDataTypeComboBox->setMinimumHeight(40);
    m_exportDataTypeComboBox->setStyleSheet("font-size: 14px; padding: 8px;");

    m_statusLabel = new QLabel("就绪", this);
    m_statusLabel->setStyleSheet("font-size: 13px; color: #28a745; font-weight: bold;");

    exportLayout->addWidget(m_exportButton);
    exportLayout->addWidget(m_featureSelectionButton);
    exportLayout->addWidget(clearDataButton);
    exportLayout->addWidget(m_exportDataTypeLabel);
    exportLayout->addWidget(m_exportDataTypeComboBox);
    exportLayout->addStretch();
    exportLayout->addWidget(m_statusLabel);

    // 连接信号
    connect(m_exportButton, &QPushButton::clicked, this, &DataTable::showExportOptionsDialog);
    connect(m_featureSelectionButton, &QPushButton::clicked, this, &DataTable::onFeatureSelectionClicked);

    // 初始化特征列表
    initializeFeatureList();

    // 初始化特征提取器映射表
    initializeFeatureExtractors();

    // 创建实验控制控件
    setupExperimentControlWidget();

    // 创建标签输入控件
    setupLabelInputWidget();

    // 添加到主布局 - 微调间距和拉伸因子，适度减少空白
    mainLayout->setSpacing(15); // 增加垂直间距
    mainLayout->setContentsMargins(10, 10, 10, 10); // 增加边距

    mainLayout->addWidget(m_experimentControlWidget, 0); // 不拉伸
    mainLayout->addWidget(m_tabWidget, 1); // 适度拉伸，允许占用主要空间
    mainLayout->addWidget(m_labelInputWidget, 0); // 不拉伸
    mainLayout->addLayout(exportLayout, 0); // 不拉伸

    setLayout(mainLayout);
}

void DataTable::setupBasicTab() {
    m_basicTable = new QTableView();
    m_basicModel = new QStandardItemModel(23, stereoMode ? 2 : 1, this);  // 23个基础特征

    // 使用原有的完整特征列表，确保兼容性
    const QStringList basicHeaders = QStringList()
        << TIME << FRAME_NUMBER << CAMERA_FPS << PUPIL_FPS << PUPIL_CENTER_X << PUPIL_CENTER_Y
        << PUPIL_MAJOR << PUPIL_MINOR << PUPIL_WIDTH << PUPIL_HEIGHT << PUPIL_DIAMETER
        << PUPIL_UNDIST_DIAMETER << PUPIL_PHYSICAL_DIAMETER << PUPIL_CONFIDENCE
        << PUPIL_OUTLINE_CONFIDENCE << PUPIL_CIRCUMFERENCE << PUPIL_RATIO << EYE_OPENNESS
        << BLINK_STATE << EYE_MOVEMENT_TYPE << FIXATION_STABILITY << MEAN_PUPIL_DIAMETER << PUPIL_RELATIVE_DEVIATION;

    for(int i = 0; i < basicHeaders.size(); ++i) {
        m_basicModel->setHeaderData(i, Qt::Vertical, basicHeaders[i]);
    }

    setupTableView(m_basicTable, m_basicModel, "基础特征");
}

void DataTable::setupMotionTab() {
    m_motionTable = new QTableView();
    m_motionModel = new QStandardItemModel(17, stereoMode ? 2 : 1, this); // 17个真实运动特征

    const QStringList motionHeaders = QStringList()
        << GAZE_SCREEN_X << GAZE_SCREEN_Y << GAZE_VELOCITY_X << GAZE_VELOCITY_Y << GAZE_SPEED
        << PUPIL_VELOCITY_X << PUPIL_VELOCITY_Y << PUPIL_SPEED
        << PUPIL_ACCELERATION_X << PUPIL_ACCELERATION_Y << PUPIL_SIZE_CHANGE
        << SACCADE_AMPLITUDE << SCANPATH_CONVEX_HULL_AREA << FIXATION_DENSITY
        << SACCADE_ENTROPY << MAIN_SEQUENCE_SLOPE << SACCADE_PEAK_VELOCITY;

    for(int i = 0; i < motionHeaders.size(); ++i) {
        m_motionModel->setHeaderData(i, Qt::Vertical, motionHeaders[i]);
    }

    setupTableView(m_motionTable, m_motionModel, "运动特征");
}

void DataTable::setupTemporalTab() {
    m_temporalTable = new QTableView();
    m_temporalModel = new QStandardItemModel(6, stereoMode ? 2 : 1, this);  // 6个时序特征

    const QStringList temporalHeaders = QStringList()
        << CURRENT_STATE_DURATION << BLINK_FREQUENCY
        << BLINK_DURATION << MEAN_FIXATION_DURATION << "扫视频率(/s)" << FIXATION_RATE;

    for(int i = 0; i < temporalHeaders.size(); ++i) {
        m_temporalModel->setHeaderData(i, Qt::Vertical, temporalHeaders[i]);
    }

    setupTableView(m_temporalTable, m_temporalModel, "时序特征");
}

void DataTable::setupBehaviorTab() {
    m_behaviorTable = new QTableView();
    m_behaviorModel = new QStandardItemModel(4, stereoMode ? 2 : 1, this);  // 减少到4个特征

    const QStringList behaviorHeaders = QStringList()
        << "多任务能力" << INTENTION_LABEL << FATIGUE_LEVEL << COGNITIVE_WORKLOAD;

    for(int i = 0; i < behaviorHeaders.size(); ++i) {
        m_behaviorModel->setHeaderData(i, Qt::Vertical, behaviorHeaders[i]);
    }

    setupTableView(m_behaviorTable, m_behaviorModel, "行为特征");
}

void DataTable::setupLabelInputWidget() {
    m_labelInputWidget = new QWidget(this);
    m_labelInputWidget->setFixedHeight(260); // 进一步增加高度到260像素，防止下边界被遮挡

    QVBoxLayout* mainLayout = new QVBoxLayout(m_labelInputWidget);
    mainLayout->setSpacing(15); // 增加垂直间距

    // 标题
    QLabel* titleLabel = new QLabel("数据集标签输入", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: #2E86AB;");
    mainLayout->addWidget(titleLabel);

    // 第一行：交互意图
    QHBoxLayout* intentionLayout = new QHBoxLayout();
    intentionLayout->setSpacing(20); // 增加水平间距

    QLabel* intentionLabel = new QLabel("交互意图:", this);
    intentionLabel->setMinimumHeight(32);
    intentionLayout->addWidget(intentionLabel);

    m_intentionComboBox = new QComboBox(this);
    m_intentionComboBox->setMinimumHeight(32); // 增加下拉框高度
    m_intentionComboBox->addItems(QStringList() << "浏览" << "点击准备" << "阅读" << "搜索" << "选择" << "无明确意图");
    m_intentionComboBox->setCurrentIndex(0);
    intentionLayout->addWidget(m_intentionComboBox);
    intentionLayout->addStretch();

    // 事件标记按钮 - 改善样式和高度
    m_markEventButton = new QPushButton("标记重要事件", this);
    m_markEventButton->setMinimumHeight(20);
    m_markEventButton->setStyleSheet("background-color: #FF6B35; color: white; font-weight: bold; padding: 3px 8px; border-radius: 3px; font-size: 10px;");
    intentionLayout->addWidget(m_markEventButton);
    mainLayout->addLayout(intentionLayout);

    // 第二行：疲劳等级和认知负荷
    mainLayout->addSpacing(35); // 加大上边距，避免滑动控件上边缘被遮挡
    QHBoxLayout* sliderLayout = new QHBoxLayout();

    // 疲劳等级滑块
    sliderLayout->addWidget(new QLabel("疲劳等级:", this));
    m_fatigueSlider = new QSlider(Qt::Horizontal, this);
    m_fatigueSlider->setRange(0, 4);
    m_fatigueSlider->setValue(0);
    m_fatigueSlider->setTickPosition(QSlider::TicksBelow);
    m_fatigueSlider->setTickInterval(1);
    m_fatigueSlider->setMinimumHeight(40); // 增加滑块高度，防止上边缘被遮挡
    sliderLayout->addWidget(m_fatigueSlider);
    m_fatigueLabel = new QLabel("0 (清醒)", this);
    m_fatigueLabel->setMinimumWidth(60);
    sliderLayout->addWidget(m_fatigueLabel);

    sliderLayout->addSpacing(20);

    // 认知负荷滑块
    sliderLayout->addWidget(new QLabel("认知负荷:", this));
    m_workloadSlider = new QSlider(Qt::Horizontal, this);
    m_workloadSlider->setRange(0, 4);
    m_workloadSlider->setValue(0);
    m_workloadSlider->setTickPosition(QSlider::TicksBelow);
    m_workloadSlider->setTickInterval(1);
    m_workloadSlider->setMinimumHeight(40); // 增加滑块高度，防止上边缘被遮挡
    sliderLayout->addWidget(m_workloadSlider);
    m_workloadLabel = new QLabel("0 (轻松)", this);
    m_workloadLabel->setMinimumWidth(60);
    sliderLayout->addWidget(m_workloadLabel);

    mainLayout->addLayout(sliderLayout);
    mainLayout->addSpacing(20); // 增加下边距

    // 第三行：事件备注
    QHBoxLayout* notesLayout = new QHBoxLayout();
    notesLayout->addWidget(new QLabel("事件备注:", this));
    m_eventNotesEdit = new QTextEdit(this);
    m_eventNotesEdit->setFixedHeight(60); // 关键修改：使用 setFixedHeight
    m_eventNotesEdit->setPlaceholderText("输入当前任务或重要事件描述...");
    notesLayout->addWidget(m_eventNotesEdit);
    mainLayout->addLayout(notesLayout);
    mainLayout->addSpacing(80); // 进一步增加下边距，防止编辑框下边界被遮挡
    // 初始化当前值
    m_currentIntention = 0;
    m_currentFatigue = 0;
    m_currentWorkload = 0;
    m_currentNotes = "";

    // 连接信号槽
    connect(m_intentionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataTable::onIntentionChanged);
    connect(m_fatigueSlider, &QSlider::valueChanged, this, &DataTable::onFatigueChanged);
    connect(m_workloadSlider, &QSlider::valueChanged, this, &DataTable::onWorkloadChanged);
    connect(m_markEventButton, &QPushButton::clicked, this, &DataTable::onMarkEvent);
    connect(m_eventNotesEdit, &QTextEdit::textChanged, this, &DataTable::onEventNotesChanged);
}

void DataTable::setupExperimentControlWidget() {
    m_experimentControlWidget = new QWidget(this);
    m_experimentControlWidget->setFixedHeight(216); // 调整高度以容纳81px间距 (180+36)
    m_experimentControlWidget->setStyleSheet("QWidget { background-color: #f0f0f0; border: 1px solid #ccc; border-radius: 5px; }");

    QVBoxLayout* mainLayout = new QVBoxLayout(m_experimentControlWidget);
    mainLayout->setSpacing(0); // 使用手动间距控制
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 标题
    QLabel* titleLabel = new QLabel("数据集实验控制", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #1E5A8B;");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop); // 左上角对齐
    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(10); // 标题后间距

    // === 第一行：实验类型选择 + 参数设置 ===
    QHBoxLayout* row1Layout = new QHBoxLayout();
    row1Layout->setContentsMargins(0, 5, 0, 5); // 行内边距
    row1Layout->setAlignment(Qt::AlignVCenter); // 整体垂直居中对齐

    QLabel* typeLabel = new QLabel("实验类型:", this);
    typeLabel->setStyleSheet("font-weight: bold; color: #333; font-size: 14px; padding: 8px; border: 1px solid #ccc; border-radius: 6px; background-color: #f8f9fa;");
    typeLabel->setFixedWidth(90);
    typeLabel->setFixedHeight(40); // 与选择列表和按钮统一高度
    typeLabel->setAlignment(Qt::AlignCenter); // 居中对齐

    m_experimentTypeComboBox = new QComboBox(this);
    m_experimentTypeComboBox->setFixedHeight(40); // 统一高度为40
    m_experimentTypeComboBox->setFixedWidth(187); // 宽度增加1/3：140*4/3≈187
    m_experimentTypeComboBox->setStyleSheet("font-size: 14px; padding: 8px; border: 1px solid #ccc; border-radius: 6px;"); // 统一字体和边框样式
    m_experimentTypeComboBox->addItems(QStringList() << "交互意图识别" << "疲劳检测" << "认知负荷评估" << "注意力分析" << "自定义实验");

    // 参数设置按钮移到第一行
    m_paradigmSettingsButton = new QPushButton("参数设置", this);
    m_paradigmSettingsButton->setMinimumHeight(40);
    m_paradigmSettingsButton->setStyleSheet("background-color: #6c757d; color: white; font-weight: bold; padding: 8px 16px; border-radius: 6px; font-size: 14px;"); // 灰色
    m_paradigmSettingsButton->setEnabled(true); // 启用按钮
    m_paradigmSettingsButton->setToolTip("配置实验范式参数");

    row1Layout->addWidget(typeLabel);
    row1Layout->addWidget(m_experimentTypeComboBox);
    row1Layout->addSpacing(25); // 加大间距从15到25
    row1Layout->addWidget(m_paradigmSettingsButton);
    row1Layout->addStretch();

    mainLayout->addLayout(row1Layout);
    mainLayout->addSpacing(15); // 行间距

    // === 第二行：控制按钮组 ===
    QHBoxLayout* row2Layout = new QHBoxLayout();
    row2Layout->setContentsMargins(0, 8, 0, 8); // 行内边距
    row2Layout->setSpacing(25); // 增大按钮间距从15到25

    m_startExperimentButton = new QPushButton("开始", this);
    m_startExperimentButton->setMinimumHeight(40);
    m_startExperimentButton->setStyleSheet("background-color: #007bff; color: white; font-weight: bold; padding: 8px 16px; border-radius: 6px; font-size: 14px;"); // 蓝色

    m_pauseExperimentButton = new QPushButton("暂停", this);
    m_pauseExperimentButton->setMinimumHeight(40);
    m_pauseExperimentButton->setStyleSheet("background-color: #ffc107; color: white; font-weight: bold; padding: 8px 16px; border-radius: 6px; font-size: 14px;"); // 黄色
    m_pauseExperimentButton->setEnabled(false);

    m_stopExperimentButton = new QPushButton("停止", this);
    m_stopExperimentButton->setMinimumHeight(40);
    m_stopExperimentButton->setStyleSheet("background-color: #dc3545; color: white; font-weight: bold; padding: 8px 16px; border-radius: 6px; font-size: 14px;"); // 红色
    m_stopExperimentButton->setEnabled(false);

    m_nextTaskButton = new QPushButton("下一任务", this);
    m_nextTaskButton->setMinimumHeight(40);
    m_nextTaskButton->setStyleSheet("background-color: #17a2b8; color: white; font-weight: bold; padding: 8px 16px; border-radius: 6px; font-size: 14px;"); // 青色
    m_nextTaskButton->setEnabled(false);

    row2Layout->addWidget(m_startExperimentButton);
    row2Layout->addWidget(m_pauseExperimentButton);
    row2Layout->addWidget(m_stopExperimentButton);
    row2Layout->addWidget(m_nextTaskButton);
    row2Layout->addStretch();

    mainLayout->addLayout(row2Layout);
    mainLayout->addSpacing(81); // 按钮与状态信息间距调整为1.8倍 (45x1.8=81)

    // === 第三行：状态显示 + Trial进度 ===
    QHBoxLayout* row3Layout = new QHBoxLayout();
    row3Layout->setContentsMargins(0, 5, 0, 5); // 行内边距
    row3Layout->setSpacing(20); // 状态标签间距

    m_experimentStatusLabel = new QLabel("状态: 未开始", this);
    m_experimentStatusLabel->setStyleSheet("font-weight: bold; color: #666; font-size: 11px;");
    m_experimentStatusLabel->setFixedWidth(80);

    m_currentTaskLabel = new QLabel("当前任务: 无", this);
    m_currentTaskLabel->setStyleSheet("color: #333; font-size: 11px;");
    m_currentTaskLabel->setFixedWidth(120);

    m_timeElapsedLabel = new QLabel("用时: 00:00:00", this);
    m_timeElapsedLabel->setStyleSheet("color: #333; font-size: 11px;");
    m_timeElapsedLabel->setFixedWidth(100);

    // 添加Trial进度显示到第三行
    m_trialProgressLabel = new QLabel("等待开始", this);
    m_trialProgressLabel->setStyleSheet("font-weight: bold; color: #3498db; font-size: 11px;");
    m_trialProgressLabel->setFixedWidth(120);

    row3Layout->addWidget(m_experimentStatusLabel);
    row3Layout->addWidget(m_currentTaskLabel);
    row3Layout->addWidget(m_timeElapsedLabel);
    row3Layout->addWidget(m_trialProgressLabel);
    row3Layout->addStretch();

    mainLayout->addLayout(row3Layout);


    // 连接信号槽
    connect(m_startExperimentButton, &QPushButton::clicked, this, &DataTable::startExperiment);
    connect(m_pauseExperimentButton, &QPushButton::clicked, this, &DataTable::pauseExperiment);
    connect(m_stopExperimentButton, &QPushButton::clicked, this, &DataTable::stopExperiment);
    connect(m_nextTaskButton, &QPushButton::clicked, this, &DataTable::nextTask);
    connect(m_experimentTypeComboBox, &QComboBox::currentTextChanged, this, &DataTable::onExperimentTypeChanged);
    connect(m_paradigmSettingsButton, &QPushButton::clicked, this, &DataTable::onParadigmSettingsClicked);
}

// dataTable.cpp

void DataTable::setupTableView(QTableView* table, QStandardItemModel* model, const QString& tabName) {
    // --- 1. 设置模型和基本属性 ---
    // 删除列标题设置，不显示"Main Value"/"主值"等列标题
    table->setModel(model);
    table->setContextMenuPolicy(Qt::CustomContextMenu);

    // 隐藏水平标题栏
    table->horizontalHeader()->setVisible(false);

    // 设置非常小的字体
    QFont tableFont = table->font();
    tableFont.setPointSize(8); // 设置很小的字体
    table->setFont(tableFont);

    // 设置垂直表头（行标题）的字体也很小
    table->verticalHeader()->setStyleSheet("QHeaderView::section { font-size: 14px; }");
    table->resizeRowsToContents(); // 自动调整行高以适应内容
    // 手动设置更小的行高，减小1/8
    for (int i = 0; i < table->model()->rowCount(); ++i) {
        int currentHeight = table->rowHeight(i);
        table->setRowHeight(i, currentHeight * 7 / 8); // 减小1/8
    }

    // 设置表格的最大高度限制
    table->setMaximumHeight(680); // 480再增大一半: 480 * 1.5 = 720像素

    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // 列宽填充

    // 3. 【重要】让垂直表头（行标题）的宽度也能自动适应内容
    table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    //
    // =======================================================

    // --- 连接信号和初始化数据 ---
    connect(table, &QTableView::customContextMenuRequested, this, &DataTable::customMenuRequested);

    for (int row = 0; row < model->rowCount(); ++row) {
        model->setItem(row, 0, new QStandardItem("0.0"));
        if (stereoMode) {
            model->setItem(row, 1, new QStandardItem("0.0"));
        }
    }

    m_tabWidget->addTab(table, tabName);

    if (tabName == "基础特征") {
        tableView = table;
        tableModel = model;
    }
}

void DataTable::updateFeatureTabs(const ProcessingResult &result) {
    // 注意：基础特征Tab的数据更新已经在onProcessingResult的setPupilData和setEyeMetricsData中完成
    // 但仍需要调用updateBasicFeatures来发射扩展特征的信号

    // 【修复】加入异常处理，防止特征计算出现异常导致UI阻塞
    try {
        // 基础特征信号发射
        updateBasicFeatures(result);

        // 时序特征更新
        updateTemporalFeatures(result);

        // 运动特征更新
        updateMotionFeatures(result);

        // 行为特征更新
        updateBehaviorFeatures(result);
    } catch (const std::exception& e) {
        qDebug() << "【特征计算异常】" << e.what();
        // 即使出现异常，也不应该影响UI线程，继续执行
    } catch (...) {
        qDebug() << "【特征计算异常】未知异常";
    }
}

void DataTable::updateBasicFeatures(const ProcessingResult &result) {
    Q_UNUSED(result); // 抑制未使用参数警告
    // 基础特征Tab的数据更新由原有的setPupilData和setEyeMetricsData方法处理
    // 基础特征Tab中的所有特征都在GraphPlot的ProcessingResult处理中已经支持
    // 这里不需要额外的信号发射
}

void DataTable::updateMotionFeatures(const ProcessingResult &result) {
    const EyeMetrics& metrics = result.eyeMetrics;
    qint64 timestamp = result.eyeMetrics.timestamp;

    // 状态机调试已禁用

    // 获取运动特征头部列表，只保留真实可计算的特征
    const QStringList motionHeaders = QStringList()
        << GAZE_SCREEN_X << GAZE_SCREEN_Y << GAZE_VELOCITY_X << GAZE_VELOCITY_Y << GAZE_SPEED
        << PUPIL_VELOCITY_X << PUPIL_VELOCITY_Y << PUPIL_SPEED
        << PUPIL_ACCELERATION_X << PUPIL_ACCELERATION_Y << PUPIL_SIZE_CHANGE
        << SACCADE_AMPLITUDE << SCANPATH_CONVEX_HULL_AREA << FIXATION_DENSITY
        << SACCADE_ENTROPY << MAIN_SEQUENCE_SLOPE << SACCADE_PEAK_VELOCITY;

    // 计算所有运动特征值
    QVector<double> motionValues(17); // 17个真实特征

    // 真实的标定后视线屏幕坐标和速度
    if (result.gazeValid) {
        motionValues[0] = result.gazeScreenCoords.x;     // GAZE_SCREEN_X - 真实标定坐标
        motionValues[1] = result.gazeScreenCoords.y;     // GAZE_SCREEN_Y - 真实标定坐标
        motionValues[2] = result.gazeVelocity.x;         // GAZE_VELOCITY_X - 真实视线速度
        motionValues[3] = result.gazeVelocity.y;         // GAZE_VELOCITY_Y - 真实视线速度
        motionValues[4] = result.gazeSpeed;              // GAZE_SPEED - 真实视线速度幅度
    } else {
        // 未标定时使用无效值，不进行瞎编的线性变换
        motionValues[0] = 0.0f; // GAZE_SCREEN_X - 未标定
        motionValues[1] = 0.0f; // GAZE_SCREEN_Y - 未标定
        motionValues[2] = 0.0f; // GAZE_VELOCITY_X - 未标定
        motionValues[3] = 0.0f; // GAZE_VELOCITY_Y - 未标定
        motionValues[4] = 0.0f; // GAZE_SPEED - 未标定
    }

    // 瞳孔运动特征
    motionValues[5] = metrics.pupilVelocity.x;          // PUPIL_VELOCITY_X
    motionValues[6] = metrics.pupilVelocity.y;          // PUPIL_VELOCITY_Y
    motionValues[7] = metrics.pupilSpeed;               // PUPIL_SPEED
    motionValues[8] = metrics.pupilAcceleration.x;      // PUPIL_ACCELERATION_X
    motionValues[9] = metrics.pupilAcceleration.y;      // PUPIL_ACCELERATION_Y
    motionValues[10] = metrics.pupilSizeChange;         // PUPIL_SIZE_CHANGE

    // 先更新所有需要计算的特征，再赋值到motionValues
    // 这些函数必须在motionValues赋值前调用，确保数据是最新的

    // 更新扫视路径凸包面积
    updateConvexHullArea(result);

    // 更新注视密度
    updateFixationDensity(result);

    // 扫视检测已移至权威状态机中处理

    // 更新扫视熵
    updateSaccadeEntropy(result);

    // 更新主序关系
    updateMainSequenceRelationship(result);

    // 更新扫视速度峰值
    updateSaccadePeakVelocity(result);

    // 扫视幅度（基于完整扫视事件计算起点到终点距离）
    // 注意：这个值由状态机在updateEyeState中更新，所以在这里直接使用即可
    motionValues[11] = m_currentSaccadeAmplitude;

    // 现在将更新后的值赋给motionValues
    // 现在按照motionHeaders的正确顺序赋值：
    // 11: SACCADE_AMPLITUDE (已在前面赋值)
    // 12: SCANPATH_CONVEX_HULL_AREA
    motionValues[12] = m_currentConvexHullArea;
    // 13: FIXATION_DENSITY
    motionValues[13] = m_currentFixationDensity;
    // 14: SACCADE_ENTROPY
    motionValues[14] = m_currentSaccadeEntropy;
    // 15: MAIN_SEQUENCE_SLOPE
    motionValues[15] = m_currentMainSequenceSlope;
    // 16: SACCADE_PEAK_VELOCITY
    motionValues[16] = m_currentSaccadePeakVelocity;

    // 基于真实数据的运动特征计算
    // 记录当前帧到历史数据
    HistoricalData currentFrame;
    currentFrame.timestamp = QDateTime::currentDateTime();
    currentFrame.gazeCoords = result.gazeValid ? result.gazeScreenCoords : cv::Point2f(-1, -1);
    currentFrame.pupilCenter = result.pupilData.center;
    currentFrame.pupilDiameter = result.pupilData.diameter();
    currentFrame.isValid = result.pupilFound && result.gazeValid;

    m_gazeHistory.append(currentFrame);
    if (m_gazeHistory.size() > MAX_HISTORY_SIZE) {
        m_gazeHistory.removeFirst();
    }

    // 微扫视计数（基于小幅度的快速眼动，使用相对阈值）
    int microsaccadeCount = 0;
    if (m_gazeHistory.size() >= 10 && result.gazeValid) {
        // 计算平均视线跳跃距离作为基准
        float totalDistance = 0;
        int validPairs = 0;
        for (int i = m_gazeHistory.size() - 10; i < m_gazeHistory.size() - 1; i++) {
            if (m_gazeHistory[i].isValid && m_gazeHistory[i+1].isValid) {
                float distance = std::sqrt(std::pow(m_gazeHistory[i+1].gazeCoords.x - m_gazeHistory[i].gazeCoords.x, 2) +
                                    std::pow(m_gazeHistory[i+1].gazeCoords.y - m_gazeHistory[i].gazeCoords.y, 2));
                totalDistance += distance;
                validPairs++;
            }
        }
        if (validPairs > 0) {
            float avgDistance = totalDistance / validPairs;
            float microThreshold = avgDistance * 0.3f; // 微扫视：平均移动距离的30%以下
            float maxMicroThreshold = avgDistance * 0.8f; // 但不超过80%

            // 重新统计微扫视
            for (int i = m_gazeHistory.size() - 10; i < m_gazeHistory.size() - 1; i++) {
                if (m_gazeHistory[i].isValid && m_gazeHistory[i+1].isValid) {
                    float distance = std::sqrt(std::pow(m_gazeHistory[i+1].gazeCoords.x - m_gazeHistory[i].gazeCoords.x, 2) +
                                        std::pow(m_gazeHistory[i+1].gazeCoords.y - m_gazeHistory[i].gazeCoords.y, 2));
                    if (distance > microThreshold && distance < maxMicroThreshold) {
                        microsaccadeCount++;
                    }
                }
            }
        }
    }

    // 更新界面显示并发射信号
    for (int i = 0; i < motionHeaders.size() && i < motionValues.size(); ++i) {
        // 运动特征全部显示为2位小数
        m_motionModel->setItem(i, 0, new QStandardItem(QString::number(motionValues[i], 'f', 2)));

        // 发射信号供GraphPlot使用
        emit extendedFeatureData(motionHeaders[i], motionValues[i], timestamp);
    }

}

void DataTable::updateTemporalFeatures(const ProcessingResult &result) {
    qint64 timestamp = result.eyeMetrics.timestamp;

    // 获取时序特征头部列表，只保留对疲劳检测、工作负荷建模有意义的特征
    const QStringList temporalHeaders = QStringList()
        << CURRENT_STATE_DURATION << BLINK_FREQUENCY
        << BLINK_DURATION << MEAN_FIXATION_DURATION << "扫视频率(/s)" << FIXATION_RATE;

    // 计算所有时序特征值 (6个特征)
    QVector<double> temporalValues(6);

    // 【修复】移除冗余的旧状态机逻辑，避免覆盖updateEyeState中基于I-VD算法的状态判断
    // 时序特征现在完全依赖于updateEyeState和handleStateTransition的权威状态机

    QDateTime currentTime = QDateTime::currentDateTime();

    // 计算各种时序特征

    // 0. 当前状态持续时长 - 直接使用Worker计算的状态持续时间
    float currentStateDuration = 0;
    if (result.eyeMetrics.stateStartTime > 0) {
        // 使用Worker传递的精确状态开始时间
        currentStateDuration = result.eyeMetrics.timestamp - result.eyeMetrics.stateStartTime;
        // 确保时长为正数
        currentStateDuration = qMax(currentStateDuration, 0.0f);
    }
    temporalValues[0] = currentStateDuration;               // CURRENT_STATE_DURATION - 当前状态持续时长

    // 1. 眨眼频率 - 每分钟眨眼次数
    qint64 timeSinceLastReset = m_lastBlinkResetTime.msecsTo(currentTime);
    float blinkFrequency = 0;
    if (timeSinceLastReset > 60000) { // 每分钟重置一次
        blinkFrequency = (m_blinkCount * 60000.0f) / timeSinceLastReset;
        m_blinkCount = 0;
        m_lastBlinkResetTime = currentTime;
    } else if (timeSinceLastReset > 3000) { // 至少3秒后开始计算
        blinkFrequency = (m_blinkCount * 60000.0f) / timeSinceLastReset;
    } else {
        // 前3秒数据不足，不进行估算
        blinkFrequency = 0; // 数据不足时为0
    }
    temporalValues[1] = blinkFrequency;                     // BLINK_FREQUENCY - 真实计算

    // 2. 眨眼时长 - 由updateBlinkDuration()函数单独计算和更新
    // temporalValues[2] 将由updateBlinkDuration()函数直接更新到m_temporalModel

    // 3. 平均注视时长 - 由updateMeanFixationDuration()函数单独计算和更新
    // temporalValues[3] 将由updateMeanFixationDuration()函数直接更新到m_temporalModel

    // 4. 扫视频率 - 每秒扫视次数
    float saccadeFrequency = 0;
    if (m_recentSaccadeDurations.size() > 0 && timeSinceLastReset > 3000) {
        saccadeFrequency = (m_recentSaccadeDurations.size() * 1000.0f) / timeSinceLastReset;
    } else {
        saccadeFrequency = 0; // 数据不足时为0
    }
    temporalValues[4] = saccadeFrequency;                   // 扫视频率(/s) - 真实计算

    // 5. 注视率 - 由updateFixationRate()函数计算
    temporalValues[5] = m_currentFixationRate;              // FIXATION_RATE - 滑动窗口计数法


    // 更新界面显示并发射信号
    for (int i = 0; i < temporalHeaders.size() && i < temporalValues.size(); ++i) {
        // 跳过索引2、3和5，它们由独立函数处理
        if (i == 2 || i == 3 || i == 5) {
            continue; // updateBlinkDuration、updateMeanFixationDuration和updateFixationRate会直接更新这些项
        }

        // 时序特征全部显示为1位小数
        m_temporalModel->setItem(i, 0, new QStandardItem(QString::number(temporalValues[i], 'f', 1)));

        // 发射信号供GraphPlot使用
        emit extendedFeatureData(temporalHeaders[i], temporalValues[i], timestamp);
    }

    // 更新新增的时序特征
    updateBlinkDuration(result);
    updateMeanFixationDuration();
    updateFixationRate(result);
}

void DataTable::updateBehaviorFeatures(const ProcessingResult &result) {
    qint64 timestamp = result.eyeMetrics.timestamp;

    // 4个真实行为特征数据定义
    const QStringList behaviorHeaders = QStringList()
        << "多任务能力" << INTENTION_LABEL << FATIGUE_LEVEL << COGNITIVE_WORKLOAD;

    QVector<double> behaviorValues(4);  // 减少到4个特征

    // 0. 多任务能力 - 基于注视稳定性转换为1-10评分
    behaviorValues[0] = 1 + (result.eyeMetrics.fixationStability * 9);

    // 1. 交互意图标签 (数据集标签)
    behaviorValues[1] = m_currentIntention;

    // 2. 疲劳等级 (数据集标签)
    behaviorValues[2] = m_currentFatigue;

    // 3. 认知负荷 (数据集标签)
    behaviorValues[3] = m_currentWorkload;

    // 更新界面显示并发射信号
    for (int i = 0; i < behaviorHeaders.size() && i < behaviorValues.size(); ++i) {
        // 更新显示
        if (i == 1) {
            // 交互意图标签显示文字
            QStringList intentions = QStringList() << "浏览" << "点击准备" << "阅读" << "搜索" << "选择" << "无明确意图";
            int intentionIndex = (int)behaviorValues[i];
            QString currentIntention = intentions[intentionIndex % intentions.size()];
            m_behaviorModel->setItem(i, 0, new QStandardItem(currentIntention));
        } else if (i == 0) {
            // 多任务能力显示整数评分
            m_behaviorModel->setItem(i, 0, new QStandardItem(QString::number(behaviorValues[i], 'f', 0)));
        } else {
            // 其他特征显示2位小数
            m_behaviorModel->setItem(i, 0, new QStandardItem(QString::number(behaviorValues[i], 'f', 2)));
        }

        // 发射信号供GraphPlot使用
        emit extendedFeatureData(behaviorHeaders[i], behaviorValues[i], timestamp);
    }
}

void DataTable::exportData() {
    QString fileName = QFileDialog::getSaveFileName(this,
        "导出眼动特征数据",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/eyetracking_features.csv",
        "CSV files (*.csv);;JSON files (*.json);;All files (*.*)");

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // 【修复】使用非阻塞错误提示
        m_statusLabel->setText("❌ 导出失败: 无法创建文件");
        QTimer::singleShot(100, this, [this, fileName]() {
            auto* msgBox = new QMessageBox(QMessageBox::Warning,
                                          "导出失败",
                                          "无法创建文件: " + fileName,
                                          QMessageBox::Ok, this);
            msgBox->setModal(false);
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->show();
        });
        return;
    }

    QTextStream out(&file);

    if (fileName.endsWith(".json")) {
        exportToJSON(out);
    } else {
        exportToCSV(out);
    }

    file.close();
    m_statusLabel->setText("✅ 数据已导出: " + QFileInfo(fileName).fileName());

    // 【修复】使用非阻塞消息提示，避免UI冻结
    QTimer::singleShot(100, this, [this, fileName]() {
        auto* msgBox = new QMessageBox(QMessageBox::Information,
                                      "导出成功",
                                      "数据已成功导出到: " + fileName,
                                      QMessageBox::Ok, this);
        msgBox->setModal(false);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->show();
    });
}

void DataTable::exportToCSV(QTextStream& out) {
    // 提示用户手动导出也支持特征选择
    if (m_selectedFeatures.isEmpty()) {
        // 如果没有选择特征，使用默认的全部导出
        exportOriginalCSV(out);
        return;
    }

    // 使用自定义特征选择的CSV导出
    out << generateCustomCSVHeader();

    // 检查是否有历史数据缓存
    if (!m_dataBuffer.isEmpty()) {
        // 导出历史数据缓存中的选中特征
        for (const DataRecord& record : m_dataBuffer) {
            QString timestamp = record.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
            out << timestamp;

            // 写入实验相关字段（如果启用）
            if (m_isExperimentRunning || !record.experimentType.isEmpty()) {
                if (m_selectedFeatures.value("交互意图", false)) {
                    QStringList intentions = QStringList() << "浏览" << "点击准备" << "阅读" << "搜索" << "选择" << "无明确意图";
                    QString intentionText = (record.intention < intentions.size()) ? intentions[record.intention] : "未知";
                    out << "," << intentionText;
                }
                if (m_selectedFeatures.value("疲劳等级", false)) {
                    out << "," << record.fatigue;
                }
                if (m_selectedFeatures.value("认知负荷", false)) {
                    out << "," << record.workload;
                }
                if (m_selectedFeatures.value("实验备注", false)) {
                    QString notes = record.notes;
                    out << "," << notes.replace(",", ";");
                }
                out << "," << record.experimentType;
                out << "," << record.currentTask;
                out << "," << record.experimentStatus;
            }

            // 写入选中的特征数据
            for (const FeatureConfig& feature : m_availableFeatures) {
                if (m_selectedFeatures.value(feature.name, false)) {
                    QString value = record.calculatedFeatures.value(feature.name, "0");
                    out << "," << value;
                }
            }
            out << "\n";
        }
        return;
    }

    // 回退到当前快照导出（如果没有历史缓存）
    QString currentTimestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    out << currentTimestamp;

    // 写入实验相关字段（如果启用）
    if (m_isExperimentRunning || !m_currentExperimentType.isEmpty()) {
        if (m_selectedFeatures.value("交互意图", false)) {
            QStringList intentions = QStringList() << "浏览" << "点击准备" << "阅读" << "搜索" << "选择" << "无明确意图";
            QString intentionText = (m_currentIntention < intentions.size()) ? intentions[m_currentIntention] : "未知";
            out << "," << intentionText;
        }
        if (m_selectedFeatures.value("疲劳等级", false)) {
            out << "," << m_currentFatigue;
        }
        if (m_selectedFeatures.value("认知负荷", false)) {
            out << "," << m_currentWorkload;
        }
        if (m_selectedFeatures.value("实验备注", false)) {
            QString notes = m_currentNotes;
            out << "," << notes.replace(",", ";");
        }
        out << "," << m_currentExperimentType;
        QString currentTask = (m_currentTaskIndex < m_taskList.size()) ? m_taskList[m_currentTaskIndex] : "无任务";
        out << "," << currentTask;
        out << "," << (m_isRecordingData ? "记录中" : "已暂停");
    }

    // 按照特征列表顺序写入选中的特征数据
    for (const FeatureConfig& feature : m_availableFeatures) {
        if (!m_selectedFeatures.value(feature.name, false) || feature.category == "实验特征") {
            continue;
        }

        // 对于手动导出，使用UI界面的当前值
        QString value = getFeatureValueFromUI(feature.name);
        out << "," << value;
    }

    out << "\n";
}

void DataTable::exportOriginalCSV(QTextStream& out) {
    // 检查是否有历史数据缓存
    if (!m_dataBuffer.isEmpty()) {
        // 导出历史数据缓存（更有价值的数据）
        out << "时间戳,实验类型,当前任务,实验状态,特征名称,特征值,交互意图,疲劳等级,认知负荷,备注\n";

        for (const DataRecord& record : m_dataBuffer) {
            QString timestamp = record.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");

            // 为每个特征写一行
            for (auto it = record.calculatedFeatures.begin(); it != record.calculatedFeatures.end(); ++it) {
                out << timestamp << ","
                    << record.experimentType << ","
                    << record.currentTask << ","
                    << record.experimentStatus << ","
                    << it.key() << ","
                    << it.value() << ","
                    << record.intention << ","
                    << record.fatigue << ","
                    << record.workload << ","
                    << record.notes << "\n";
            }
        }
        return;
    }

    // 回退到原有的导出逻辑（当前快照）
    if (m_isExperimentRunning || !m_currentExperimentType.isEmpty()) {
        out << "实验类型,当前任务,实验状态,时间戳,特征分类,特征名称,主值,副值\n";
    } else {
        out << "时间戳,特征分类,特征名称,主值,副值\n";
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    // 导出基础特征
    exportModelToCSV(out, m_basicModel, "基础特征", timestamp);

    // 导出运动特征
    exportModelToCSV(out, m_motionModel, "运动特征", timestamp);

    // 导出时序特征
    exportModelToCSV(out, m_temporalModel, "时序特征", timestamp);

    // 导出行为特征
    exportModelToCSV(out, m_behaviorModel, "行为特征", timestamp);

    // 导出标签事件记录
    if (!m_labelEvents.isEmpty()) {
        out << "\n# 标签变更事件记录\n";
        if (m_isExperimentRunning || !m_currentExperimentType.isEmpty()) {
            out << "实验类型,当前任务,实验状态,时间戳,事件类型,旧值,新值,描述\n";
        } else {
            out << "时间戳,事件类型,旧值,新值,描述\n";
        }

        for (const LabelEvent& event : m_labelEvents) {
            QString eventTimestamp = event.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
            if (m_isExperimentRunning || !m_currentExperimentType.isEmpty()) {
                QString currentTask = (m_currentTaskIndex < m_taskList.size()) ?
                                     m_taskList[m_currentTaskIndex] : "无任务";
                QString experimentStatus = m_isRecordingData ? "记录中" : "已暂停";

                out << m_currentExperimentType << "," << currentTask << "," << experimentStatus << ","
                    << eventTimestamp << "," << event.labelType << ","
                    << event.oldValue.toString() << "," << event.newValue.toString() << ","
                    << event.description << "\n";
            } else {
                out << eventTimestamp << "," << event.labelType << ","
                    << event.oldValue.toString() << "," << event.newValue.toString() << ","
                    << event.description << "\n";
            }
        }
    }
}

void DataTable::exportModelToCSV(QTextStream& out, QStandardItemModel* model,
                                const QString& category, const QString& timestamp) {
    for (int row = 0; row < model->rowCount(); ++row) {
        QString featureName = model->headerData(row, Qt::Vertical).toString();
        QString mainValue = model->item(row, 0) ? model->item(row, 0)->text() : "0.0";
        QString secValue = (stereoMode && model->item(row, 1)) ? model->item(row, 1)->text() : "";

        if (m_isExperimentRunning || !m_currentExperimentType.isEmpty()) {
            // 实验模式：包含实验信息
            QString currentTask = (m_currentTaskIndex < m_taskList.size()) ?
                                 m_taskList[m_currentTaskIndex] : "无任务";
            QString experimentStatus = m_isRecordingData ? "记录中" : "已暂停";

            out << m_currentExperimentType << "," << currentTask << "," << experimentStatus << ","
                << timestamp << "," << category << "," << featureName << ","
                << mainValue << "," << secValue << "\n";
        } else {
            // 普通模式：仅基本信息
            out << timestamp << "," << category << "," << featureName << ","
                << mainValue << "," << secValue << "\n";
        }
    }
}

void DataTable::exportToJSON(QTextStream& out) {
    out << "{\n";
    out << "  \"timestamp\": \"" << QDateTime::currentDateTime().toString(Qt::ISODate) << "\",\n";
    out << "  \"features\": {\n";

    exportModelToJSON(out, m_basicModel, "basic_features", true);
    exportModelToJSON(out, m_motionModel, "motion_features", false);
    exportModelToJSON(out, m_temporalModel, "temporal_features", false);
    exportModelToJSON(out, m_behaviorModel, "behavior_features", false);

    out << "  }\n";
    out << "}\n";
}

void DataTable::exportModelToJSON(QTextStream& out, QStandardItemModel* model,
                                 const QString& category, bool isFirst) {
    if (!isFirst) out << ",\n";
    out << "    \"" << category << "\": {\n";

    for (int row = 0; row < model->rowCount(); ++row) {
        QString featureName = model->headerData(row, Qt::Vertical).toString();
        QString mainValue = model->item(row, 0) ? model->item(row, 0)->text() : "0.0";

        out << "      \"" << featureName << "\": \"" << mainValue << "\"";
        if (row < model->rowCount() - 1) out << ",";
        out << "\n";
    }

    out << "    }";
}


// ===================== 标签输入控制槽函数 =====================

void DataTable::onIntentionChanged(int index) {
    // 记录标签变更事件
    recordLabelChange("intention", m_lastRecordedIntention, index,
                     QString("意图从%1变更为%2").arg(m_lastRecordedIntention).arg(index));

    m_currentIntention = index;
    m_lastRecordedIntention = index;

    QStringList intentions = QStringList() << "浏览" << "点击准备" << "阅读" << "搜索" << "选择" << "无明确意图";
    QString intentionText = (index < intentions.size()) ? intentions[index] : "未知";
    m_statusLabel->setText(QString("交互意图已更新: %1").arg(intentionText));
}

void DataTable::onFatigueChanged(int value) {
    // 记录标签变更事件
    recordLabelChange("fatigue", m_lastRecordedFatigue, value,
                     QString("疲劳等级从%1变更为%2").arg(m_lastRecordedFatigue).arg(value));

    m_currentFatigue = value;
    m_lastRecordedFatigue = value;

    QStringList fatigueLabels = QStringList() << "0 (清醒)" << "1 (轻微疲劳)" << "2 (中度疲劳)" << "3 (明显疲劳)" << "4 (严重疲劳)";
    QString fatigueText = (value < fatigueLabels.size()) ? fatigueLabels[value] : "未知";
    m_fatigueLabel->setText(fatigueText);
    m_statusLabel->setText(QString("疲劳等级已更新: %1").arg(fatigueText));
}

void DataTable::onWorkloadChanged(int value) {
    // 记录标签变更事件
    recordLabelChange("workload", m_lastRecordedWorkload, value,
                     QString("认知负荷从%1变更为%2").arg(m_lastRecordedWorkload).arg(value));

    m_currentWorkload = value;
    m_lastRecordedWorkload = value;

    QStringList workloadLabels = QStringList() << "0 (轻松)" << "1 (较轻)" << "2 (中等)" << "3 (较重)" << "4 (很重)";
    QString workloadText = (value < workloadLabels.size()) ? workloadLabels[value] : "未知";
    m_workloadLabel->setText(workloadText);
    m_statusLabel->setText(QString("认知负荷已更新: %1").arg(workloadText));
}

void DataTable::onMarkEvent() {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString eventInfo = QString("[%1] 意图:%2, 疲劳:%3, 负荷:%4, 备注:%5")
                        .arg(timestamp)
                        .arg(m_intentionComboBox->currentText())
                        .arg(m_currentFatigue)
                        .arg(m_currentWorkload)
                        .arg(m_currentNotes.isEmpty() ? "无" : m_currentNotes);

    // 可以记录到日志文件或显示确认
    m_statusLabel->setText("重要事件已标记!");

    // 临时显示事件信息（3秒后恢复）
    QTimer::singleShot(3000, [this]() {
        m_statusLabel->setText("就绪");
    });

    qDebug() << "标记事件:" << eventInfo;
}

void DataTable::onEventNotesChanged() {
    m_currentNotes = m_eventNotesEdit->toPlainText();
}

// ===================== 实验控制槽函数 =====================

void DataTable::startExperiment() {
    qDebug() << "startExperiment() 被调用";
    if (m_isExperimentRunning) return;

    // 检查是否为自动化交互实验
    m_currentExperimentType = m_experimentTypeComboBox->currentText();
    if (m_currentExperimentType == "交互意图识别") {
        // 检查是否已配置参数
        if (m_expSettings.roundCount <= 0) {
            // 【修复】使用非阻塞警告提示
        m_experimentStatusLabel->setText("⚠️ 警告：请先配置实验参数！");
        QTimer::singleShot(100, this, [this]() {
            auto* msgBox = new QMessageBox(QMessageBox::Warning,
                                          "警告",
                                          "请先点击'参数设置'配置实验参数！",
                                          QMessageBox::Ok, this);
            msgBox->setModal(false);
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->show();
        });
            return;
        }
        // 启动自动化实验流程
        startAutomatedExperiment();
        return; // 自动化实验不需要执行后续的常规实验逻辑
    }

    // 加载对应实验类型的任务列表（非自动化实验）
    loadTasksForExperiment(m_currentExperimentType);

    // 设置实验状态
    m_isExperimentRunning = true;
    m_isRecordingData = true;
    m_expStartTime = QDateTime::currentDateTime();
    m_currentTaskIndex = 0;

    // 不再自动启动实时导出，用户需要手动点击导出按钮选择路径

    // 更新UI状态
    m_startExperimentButton->setEnabled(false);
    m_pauseExperimentButton->setEnabled(true);
    m_stopExperimentButton->setEnabled(true);
    m_nextTaskButton->setEnabled(true);
    m_experimentTypeComboBox->setEnabled(false);

    // 设置标签输入为默认状态并初始化记录系统
    m_intentionComboBox->setCurrentIndex(0);
    m_fatigueSlider->setValue(0);
    m_workloadSlider->setValue(0);
    m_eventNotesEdit->setText(QString("开始%1实验").arg(m_currentExperimentType));

    // 初始化标签记录基准值
    m_lastRecordedIntention = 0;
    m_lastRecordedFatigue = 0;
    m_lastRecordedWorkload = 0;
    m_currentIntention = 0;
    m_currentFatigue = 0;
    m_currentWorkload = 0;

    // 记录实验开始事件
    recordLabelChange("experiment", "未开始", m_currentExperimentType,
                     QString("开始%1实验").arg(m_currentExperimentType));

    // 启动定时器（每秒更新一次）
    m_experimentTimer->start(1000);

    // 更新状态显示
    m_experimentStatusLabel->setText("状态: 实验进行中");
    if (!m_taskList.isEmpty()) {
        m_currentTaskLabel->setText(QString("当前任务: %1").arg(m_taskList[0]));
    }

    // 【新增】初始化范式进度显示
    m_trialProgressLabel->setText("Trial 0/0 | 0条");

    // 【新增】根据实验类型启用范式设置按钮
    updateUIForTask(m_currentExperimentType);

    // 【重构】使用TaskUIManager进行范式启动
    qDebug() << "任务列表大小:" << m_taskList.size() << "当前索引:" << m_currentTaskIndex;
    if (m_currentTaskIndex >= 0 && m_currentTaskIndex < m_taskList.size()) {
        QString currentTask = m_taskList[m_currentTaskIndex];
        qDebug() << "当前任务:" << currentTask;
        TaskUIManager::TaskType taskType = TaskUIManager::getTaskType(currentTask);
        qDebug() << "任务类型:" << (int)taskType;

        switch (taskType) {
            // GRID_SELECTION case 已删除（范式相关）

            case TaskUIManager::TARGET_SEARCH:
                m_experimentStatusLabel->setText("目标搜索范式暂未实现");
                qWarning() << "尝试启动未实现的目标搜索范式";
                break;

            default:
                m_experimentStatusLabel->setText("未知的实验任务类型");
                qWarning() << QString("未知的任务类型: %1").arg(currentTask);
                break;
        }
    }

    qDebug() << "实验开始:" << m_currentExperimentType << "共" << m_taskList.size() << "个任务";
}

void DataTable::pauseExperiment() {
    if (!m_isExperimentRunning) return;

    if (m_isRecordingData) {
        // 暂停记录和自动流程
        m_isRecordingData = false;
        m_pauseExperimentButton->setText("继续");
        m_experimentStatusLabel->setText("状态: 已暂停");
        m_eventNotesEdit->setText("实验已暂停");
        m_statusLabel->setText(QString("数据记录已暂停 - 已记录 %1 条数据").arg(m_dataBuffer.size()));

        // 暂停自动实验定时器
        m_phaseTimer->stop();
        m_selectionMonitorTimer->stop();
    } else {
        // 继续记录和自动流程
        m_isRecordingData = true;
        m_pauseExperimentButton->setText("暂停");
        m_experimentStatusLabel->setText("状态: 实验进行中");
        m_eventNotesEdit->setText("实验已继续");
        m_statusLabel->setText("正在记录实验数据...");

        // 恢复自动实验定时器（根据当前阶段）
        if (m_currentPhase == PHASE_BROWSE) {
            m_phaseTimer->start(m_expSettings.browseTimeSeconds * 1000);
        } else if (m_currentPhase == PHASE_SELECTION) {
            m_phaseTimer->start(m_expSettings.selectionTimeSeconds * 1000);
            m_selectionMonitorTimer->start();
        }
    }
}

void DataTable::stopExperiment() {
    if (!m_isExperimentRunning) return;

    // 记录实验结束事件
    recordLabelChange("experiment", m_currentExperimentType, "已结束",
                     QString("结束%1实验").arg(m_currentExperimentType));

    // 【修复】先断开所有定时器信号，防止在停止过程中触发
    m_experimentTimer->disconnect();
    m_phaseTimer->disconnect();
    m_selectionMonitorTimer->disconnect();

    // 停止实验和所有定时器
    m_isExperimentRunning = false;
    m_isRecordingData = false;
    m_experimentTimer->stop();
    m_phaseTimer->stop();
    m_selectionMonitorTimer->stop();

    // 重置自动实验状态
    if (m_currentPhase != PHASE_IDLE) {
        m_currentPhase = PHASE_IDLE;
        m_currentRound = 0;
        m_selectionCount = 0;
        m_lastSelectionCount = 0;

        // 隐藏交互界面
        emit requestHideInteractionDialog();
    }

    // 重置UI状态
    m_startExperimentButton->setEnabled(true);
    m_pauseExperimentButton->setEnabled(false);
    m_stopExperimentButton->setEnabled(false);
    m_nextTaskButton->setEnabled(false);
    m_experimentTypeComboBox->setEnabled(true);
    m_pauseExperimentButton->setText("暂停");

    // 更新状态显示
    m_experimentStatusLabel->setText("状态: 实验已结束");
    m_currentTaskLabel->setText("当前任务: 无");

    // 【新增】重置范式相关显示
    m_trialProgressLabel->setText("等待开始");

    // 范式相关信号已删除

    // 实验结束，不再自动导出，用户可手动导出数据
    m_eventNotesEdit->setText(QString("实验已结束，共记录 %1 条数据，可手动导出").arg(m_dataBuffer.size()));

    // 重新启用参数设置按钮
    updateUIForTask(m_currentExperimentType);

    // 【修复】重新连接定时器信号，准备下次实验
    connect(m_experimentTimer, &QTimer::timeout, this, &DataTable::updateExperimentStatus);
    connect(m_phaseTimer, &QTimer::timeout, this, &DataTable::onPhaseTimeout);
    connect(m_selectionMonitorTimer, &QTimer::timeout, this, &DataTable::checkSelectionProgress);

    qDebug() << "实验结束，共用时:" << m_expStartTime.secsTo(QDateTime::currentDateTime()) << "秒"
             << "，共记录" << m_dataBuffer.size() << "条数据";
}

void DataTable::nextTask() {
    if (!m_isExperimentRunning || m_taskList.isEmpty()) return;

    // 记录前一个任务结束事件
    if (m_currentTaskIndex >= 0 && m_currentTaskIndex < m_taskList.size()) {
        QString prevTask = m_taskList[m_currentTaskIndex];
        recordLabelChange("task_switch", prevTask, "任务结束",
                         QString("任务%1结束: %2").arg(m_currentTaskIndex+1).arg(prevTask));
    }

    m_currentTaskIndex++;
    if (m_currentTaskIndex >= m_taskList.size()) {
        // 所有任务完成，自动结束实验
        m_currentTaskLabel->setText("所有任务已完成");
        recordLabelChange("task_switch", "进行中", "全部完成", "所有实验任务已完成");
        QTimer::singleShot(2000, this, &DataTable::stopExperiment);
        return;
    }

    // 记录新任务开始事件
    QString currentTask = m_taskList[m_currentTaskIndex];
    recordLabelChange("task_switch", "无任务", currentTask,
                     QString("开始任务%1: %2").arg(m_currentTaskIndex+1).arg(currentTask));

    // 更新当前任务显示
    m_currentTaskLabel->setText(QString("当前任务: %1").arg(currentTask));
    m_eventNotesEdit->setText(QString("开始任务 %1/%2: %3")
                             .arg(m_currentTaskIndex + 1)
                             .arg(m_taskList.size())
                             .arg(currentTask));

    // 【重构】使用统一方法更新UI状态，消除重复逻辑
    updateUIForTask(currentTask);

    // 重置标签为任务开始状态
    m_intentionComboBox->setCurrentIndex(0);

    qDebug() << "切换到任务" << (m_currentTaskIndex + 1) << ":" << currentTask;
}

void DataTable::onExperimentTypeChanged(const QString& type) {
    if (!m_isExperimentRunning) {
        m_currentExperimentType = type;
        // 预览任务列表
        loadTasksForExperiment(type);
        if (!m_taskList.isEmpty()) {
            m_currentTaskLabel->setText(QString("预计任务数: %1").arg(m_taskList.size()));
        }
    }
}

void DataTable::loadTasksForExperiment(const QString& experimentType) {
    m_taskList.clear();

    if (experimentType == "交互意图识别") {
        m_taskList << "网格点选范式 - 选择意图vs无意图 (25分钟)"
                   << "目标搜索范式 - 搜索意图vs浏览意图 (20分钟)";
    } else if (experimentType == "疲劳检测") {
        m_taskList << "清醒状态 - 简单任务 (10分钟)"
                   << "轻度疲劳 - 连续工作 (15分钟)"
                   << "中度疲劳 - 复杂任务 (20分钟)"
                   << "重度疲劳 - 持续专注 (25分钟)";
    } else if (experimentType == "认知负荷评估") {
        m_taskList << "低负荷 - 简单浏览 (5分钟)"
                   << "中等负荷 - 数据对比 (10分钟)"
                   << "高负荷 - 复杂计算 (15分钟)"
                   << "极高负荷 - 多任务处理 (20分钟)";
    } else if (experimentType == "注意力分析") {
        m_taskList << "集中注意 - 单一目标 (8分钟)"
                   << "分散注意 - 多个区域 (10分钟)"
                   << "选择注意 - 干扰环境 (12分钟)"
                   << "持续注意 - 长时监控 (15分钟)";
    } else { // 自定义实验
        m_taskList << "自定义任务 1 (自行设定时间)"
                   << "自定义任务 2 (自行设定时间)"
                   << "自定义任务 3 (自行设定时间)";
    }
}

void DataTable::updateExperimentStatus() {
    if (!m_isExperimentRunning) return;

    // 计算并显示实验用时
    qint64 elapsedSeconds = m_expStartTime.secsTo(QDateTime::currentDateTime());
    int hours = elapsedSeconds / 3600;
    int minutes = (elapsedSeconds % 3600) / 60;
    int seconds = elapsedSeconds % 60;

    m_timeElapsedLabel->setText(QString("用时: %1:%2:%3")
                               .arg(hours, 2, 10, QChar('0'))
                               .arg(minutes, 2, 10, QChar('0'))
                               .arg(seconds, 2, 10, QChar('0')));
}

// ==================== 实时导出功能实现 ====================

void DataTable::startRealTimeExport() {
    if (m_realTimeExportEnabled) {
        return; // 已经启用，避免重复开启
    }

    m_realTimeExportEnabled = true;
    m_exportStartTime = QDateTime::currentDateTime();
    m_exportedRecordCount = 0;
    m_dataBuffer.clear();

    // 初始化导出文件
    if (initializeRealTimeExportFile()) {
        m_realTimeExportTimer->start();
        m_statusLabel->setText(QString("实时导出已启用 - %1").arg(QFileInfo(m_realTimeExportFilePath).fileName()));
    } else {
        m_realTimeExportEnabled = false;
        m_statusLabel->setText("实时导出启动失败");
    }
}

void DataTable::stopRealTimeExport() {
    if (!m_realTimeExportEnabled) {
        return;
    }

    m_realTimeExportEnabled = false;
    m_realTimeExportTimer->stop();

    // 导出剩余缓存数据
    if (!m_dataBuffer.isEmpty()) {
        flushDataBuffer();
    }

    // 关闭文件
    if (m_realTimeExportStream) {
        delete m_realTimeExportStream;
        m_realTimeExportStream = nullptr;
    }

    if (m_realTimeExportFile) {
        m_realTimeExportFile->close();
        delete m_realTimeExportFile;
        m_realTimeExportFile = nullptr;
    }

    m_statusLabel->setText(QString("实时导出已停止 - 共导出 %1 条记录").arg(m_exportedRecordCount));
}

bool DataTable::initializeRealTimeExportFile() {
    // 生成文件名
    m_realTimeExportFilePath = generateExportFileName();

    // 创建文件
    m_realTimeExportFile = new QFile(m_realTimeExportFilePath, this);

    if (!m_realTimeExportFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        // 【修复】使用非阻塞错误提示
        m_statusLabel->setText("❌ 实时导出失败");
        QTimer::singleShot(100, this, [this]() {
            auto* msgBox = new QMessageBox(QMessageBox::Warning,
                                          "实时导出失败",
                                          "无法创建导出文件: " + m_realTimeExportFilePath,
                                          QMessageBox::Ok, this);
            msgBox->setModal(false);
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->show();
        });
        delete m_realTimeExportFile;
        m_realTimeExportFile = nullptr;
        return false;
    }

    // 创建文本流
    m_realTimeExportStream = new QTextStream(m_realTimeExportFile);
    m_realTimeExportStream->setCodec("UTF-8"); // 确保中文正确显示

    // 写入CSV头部 - 使用用户自定义的特征选择
    *m_realTimeExportStream << generateCustomCSVHeader();

    m_realTimeExportStream->flush();
    return true;
}

void DataTable::addDataToBuffer(const ProcessingResult& result) {
    DataRecord record;
    record.timestamp = QDateTime::currentDateTime();
    record.result = result;
    record.experimentType = m_currentExperimentType;
    record.currentTask = (m_currentTaskIndex < m_taskList.size()) ? m_taskList[m_currentTaskIndex] : "无任务";
    record.experimentStatus = m_isRecordingData ? "记录中" : "已暂停";
    record.intention = m_currentIntention;
    record.fatigue = m_currentFatigue;
    record.workload = m_currentWorkload;
    record.notes = m_currentNotes;

    // 存储当前时刻的所有计算特征值
    // 优先使用直接从ProcessingResult和计算变量获取的值，确保数据准确

    // 基础特征（直接从result获取）- 按照特征列表的确切名称
    // 基础特征（按照1.txt中的特征列表完整映射）
    record.calculatedFeatures["时间戳"] = QDateTime::fromMSecsSinceEpoch(result.eyeMetrics.timestamp).toString("hh:mm:ss.zzz");

    // 静态全局帧计数器
    static int globalFrameCounter = 0;
    record.calculatedFeatures["帧号"] = QString::number(++globalFrameCounter);

    record.calculatedFeatures["相机帧率"] = QString::number(result.eyeMetrics.cameraFps, 'f', 1);
    record.calculatedFeatures["瞳孔跟踪帧率"] = QString::number(result.eyeMetrics.frameRate, 'f', 1);
    record.calculatedFeatures["瞳孔中心X坐标"] = QString::number(result.pupilData.center.x, 'f', 2);
    record.calculatedFeatures["瞳孔中心Y坐标"] = QString::number(result.pupilData.center.y, 'f', 2);
    record.calculatedFeatures["瞳孔长轴"] = QString::number(result.pupilData.majorAxis(), 'f', 2);
    record.calculatedFeatures["瞳孔短轴"] = QString::number(result.pupilData.minorAxis(), 'f', 2);
    record.calculatedFeatures["瞳孔宽度"] = QString::number(result.pupilData.width(), 'f', 2);
    record.calculatedFeatures["瞳孔高度"] = QString::number(result.pupilData.height(), 'f', 2);
    record.calculatedFeatures["瞳孔直径"] = QString::number(result.pupilData.diameter(), 'f', 2);
    record.calculatedFeatures["瞳孔未失真直径"] = QString::number(result.pupilData.undistortedDiameter, 'f', 2);
    record.calculatedFeatures["瞳孔物理直径"] = QString::number(result.pupilData.physicalDiameter, 'f', 2);
    record.calculatedFeatures["瞳孔置信度"] = QString::number(result.pupilData.confidence, 'f', 3);
    record.calculatedFeatures["瞳孔轮廓置信度"] = QString::number(result.pupilData.outline_confidence, 'f', 3);
    record.calculatedFeatures["瞳孔周长"] = QString::number(result.pupilData.circumference(), 'f', 2);

    // 瞳孔轴比计算
    double ratio = (result.pupilData.majorAxis() > 1e-6) ?
                   (double)result.pupilData.minorAxis() / (double)result.pupilData.majorAxis() : 0.0;
    record.calculatedFeatures["瞳孔轴比"] = QString::number(ratio, 'f', 3);

    record.calculatedFeatures["眼部开合度"] = QString::number(result.eyeMetrics.eyeOpenness, 'f', 3);
    record.calculatedFeatures["眨眼状态"] = QString::number(result.eyeMetrics.blinkState, 'f', 3);

    // 眼动类型映射 - 与UI显示保持一致
    QString eyeMovementType;
    switch(result.eyeMetrics.eyeMovementType) {
        case 0: eyeMovementType = "0-扫视"; break;
        case 1: eyeMovementType = "1-注视"; break;
        case 3: eyeMovementType = "3-眨眼/无效"; break;
        default: eyeMovementType = "未知"; break;
    }
    record.calculatedFeatures["眼动类型"] = eyeMovementType;

    record.calculatedFeatures["注视稳定性"] = QString::number(result.eyeMetrics.fixationStability, 'f', 3);

    // 计算平均瞳孔直径和相对偏差
    float avgPupilDiameter = m_pupilDiameterHistory.isEmpty() ? result.pupilData.diameter() :
        std::accumulate(m_pupilDiameterHistory.begin(), m_pupilDiameterHistory.end(), 0.0f) / m_pupilDiameterHistory.size();
    record.calculatedFeatures["平均瞳孔直径"] = QString::number(avgPupilDiameter, 'f', 2);

    float relativeDeviation = (avgPupilDiameter > 0) ?
        (result.pupilData.diameter() - avgPupilDiameter) / avgPupilDiameter * 100.0f : 0.0f;
    record.calculatedFeatures["瞳孔相对偏差"] = QString::number(relativeDeviation, 'f', 2);

    // 运动特征（直接从result和计算变量获取）
    // 运动特征（按照1.txt中特征列表映射）
    if (result.gazeValid) {
        record.calculatedFeatures["视线屏幕X(px)"] = QString::number(result.gazeScreenCoords.x, 'f', 2);
        record.calculatedFeatures["视线屏幕Y(px)"] = QString::number(result.gazeScreenCoords.y, 'f', 2);
        record.calculatedFeatures["视线水平速度"] = QString::number(result.gazeVelocity.x, 'f', 2);
        record.calculatedFeatures["视线垂直速度"] = QString::number(result.gazeVelocity.y, 'f', 2);
        record.calculatedFeatures["视线合成速度"] = QString::number(result.gazeSpeed, 'f', 2);
    } else {
        record.calculatedFeatures["视线屏幕X(px)"] = "0.00";
        record.calculatedFeatures["视线屏幕Y(px)"] = "0.00";
        record.calculatedFeatures["视线水平速度"] = "0.00";
        record.calculatedFeatures["视线垂直速度"] = "0.00";
        record.calculatedFeatures["视线合成速度"] = "0.00";
    }

    record.calculatedFeatures["瞳孔水平速度"] = QString::number(result.eyeMetrics.pupilVelocity.x, 'f', 2);
    record.calculatedFeatures["瞳孔垂直速度"] = QString::number(result.eyeMetrics.pupilVelocity.y, 'f', 2);
    record.calculatedFeatures["瞳孔合成速度"] = QString::number(result.eyeMetrics.pupilSpeed, 'f', 2);
    record.calculatedFeatures["瞳孔水平加速度"] = QString::number(result.eyeMetrics.pupilAcceleration.x, 'f', 2);
    record.calculatedFeatures["瞳孔垂直加速度"] = QString::number(result.eyeMetrics.pupilAcceleration.y, 'f', 2);
    record.calculatedFeatures["瞳孔尺寸变化率"] = QString::number(result.eyeMetrics.pupilSizeChange, 'f', 2);

    // 高级特征（从计算变量获取）
    record.calculatedFeatures["扫视幅度"] = QString::number(m_currentSaccadeAmplitude, 'f', 2);
    record.calculatedFeatures["注视密度(1/px)"] = QString::number(m_currentFixationDensity, 'f', 2);
    record.calculatedFeatures["扫视熵"] = QString::number(m_currentSaccadeEntropy, 'f', 2);
    record.calculatedFeatures["主序关系斜率"] = QString::number(m_currentMainSequenceSlope, 'f', 2);
    record.calculatedFeatures["扫视速度峰值"] = QString::number(m_currentSaccadePeakVelocity, 'f', 2);
    record.calculatedFeatures["扫视路径凸包面积(px²)"] = QString::number(m_currentConvexHullArea, 'f', 2);
    record.calculatedFeatures["注视率"] = QString::number(m_currentFixationRate, 'f', 2);

    // 时序特征 - 使用Worker计算的精确状态持续时间
    qint64 stateDuration = 0;
    if (result.eyeMetrics.stateStartTime > 0) {
        stateDuration = result.eyeMetrics.timestamp - result.eyeMetrics.stateStartTime;
    }
    record.calculatedFeatures["当前状态持续时长(ms)"] = QString::number(stateDuration, 'f', 1);

    // 计算眨眼频率 (每分钟)
    qint64 timeSinceLastReset = m_lastBlinkResetTime.msecsTo(QDateTime::currentDateTime());
    float blinkFrequency = (timeSinceLastReset > 0) ? (m_blinkCount * 60000.0f) / timeSinceLastReset : 0.0f;
    record.calculatedFeatures["眨眼频率"] = QString::number(blinkFrequency, 'f', 2);

    // 平均眨眼时长
    float avgBlinkDuration = m_blinkDurations.isEmpty() ? 0.0f :
        std::accumulate(m_blinkDurations.begin(), m_blinkDurations.end(), 0.0f) / m_blinkDurations.size();
    record.calculatedFeatures["眨眼时长"] = QString::number(avgBlinkDuration, 'f', 1);

    // 平均注视时长
    float avgFixationDuration = m_recentFixationDurations.isEmpty() ? 0.0f :
        std::accumulate(m_recentFixationDurations.begin(), m_recentFixationDurations.end(), 0.0f) / m_recentFixationDurations.size();
    record.calculatedFeatures["平均注视时长"] = QString::number(avgFixationDuration, 'f', 1);

    // 计算扫视频率 (每秒)
    float saccadeFrequency = (timeSinceLastReset > 0) ? (m_recentSaccadeDurations.size() * 1000.0f) / timeSinceLastReset : 0.0f;
    record.calculatedFeatures["扫视频率"] = QString::number(saccadeFrequency, 'f', 2);

    // 行为特征（按照1.txt中特征列表映射）
    float multiTaskAbility = 1.0f + (result.eyeMetrics.fixationStability * 9.0f);
    record.calculatedFeatures["多任务能力"] = QString::number(multiTaskAbility, 'f', 2);

    // 用户标签特征
    QStringList intentionLabels = {"浏览", "点击准备", "阅读", "搜索", "选择", "无明确意图"};
    QString intentionLabel = (m_currentIntention >= 0 && m_currentIntention < intentionLabels.size()) ?
                             intentionLabels[m_currentIntention] : "无明确意图";
    record.calculatedFeatures["交互意图"] = intentionLabel;

    record.calculatedFeatures["疲劳等级"] = QString::number(m_currentFatigue);
    record.calculatedFeatures["认知负荷"] = QString::number(m_currentWorkload);

    // 对于其他特征，仍然从UI获取
    for (const FeatureConfig& feature : m_availableFeatures) {
        if (!record.calculatedFeatures.contains(feature.name)) {
            QString featureValue = getFeatureValueFromUI(feature.name);
            record.calculatedFeatures[feature.name] = featureValue;
        }
    }

    m_dataBuffer.append(record);

    // 只在缓存数据过多时才删除老数据（支持长时间实验）
    // 注意：现在支持最多10万条记录，在默认60Hz采样率下可支持约27小时连续记录
    if (m_dataBuffer.size() > MAX_BUFFER_SIZE) {
        // 删除最旧的10%的数据，减少频繁删除操作
        int removeCount = MAX_BUFFER_SIZE / 10;
        for (int i = 0; i < removeCount && !m_dataBuffer.isEmpty(); ++i) {
            m_dataBuffer.removeFirst();
        }
        qDebug() << "【数据缓存管理】缓存达到上限，已删除" << removeCount << "条旧记录，当前缓存数量：" << m_dataBuffer.size();
    }
}

void DataTable::flushDataBuffer() {
    if (!m_realTimeExportStream) {
        return;
    }

    // 写入缓存的数据记录
    if (!m_dataBuffer.isEmpty()) {
        for (const DataRecord& record : m_dataBuffer) {
            writeDataRecordToFile(record);
        }
        m_dataBuffer.clear();
    }

    m_realTimeExportStream->flush(); // 确保数据写入磁盘
}

void DataTable::writeDataRecordToFile(const DataRecord& record) {
    if (!m_realTimeExportStream) return;

    writeCustomDataRecordToFile(record);
    m_exportedRecordCount++;
}

QString DataTable::generateExportFileName() {
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString baseName;

    if (!m_currentExperimentType.isEmpty()) {
        baseName = QString("%1_%2.csv").arg(m_currentExperimentType).arg(timestamp);
    } else {
        baseName = QString("交互意图识别_%1.csv").arg(timestamp);
    }

    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return QDir(documentsPath).absoluteFilePath(baseName);
}

void DataTable::onRealTimeExportTimer() {
    if (m_realTimeExportEnabled && !m_dataBuffer.isEmpty()) {
        flushDataBuffer();

        // 更新状态显示
        m_statusLabel->setText(QString("实时导出中... 已导出 %1 条记录").arg(m_exportedRecordCount));
    }
}

// ==================== 特征选择功能实现 ====================

void DataTable::initializeFeatureList() {
    m_availableFeatures.clear();
    m_selectedFeatures.clear();

    // ==================== 基础特征 (23个) ====================
    m_availableFeatures.append({"时间戳", "基础特征", true, "数据记录的时间戳"});
    m_availableFeatures.append({"帧号", "基础特征", false, "当前处理的帧编号"});
    m_availableFeatures.append({"相机帧率", "基础特征", false, "相机采集帧率"});
    m_availableFeatures.append({"瞳孔跟踪帧率", "基础特征", false, "瞳孔跟踪处理帧率"});
    m_availableFeatures.append({"瞳孔中心X坐标", "基础特征", false, "瞳孔中心X坐标"});
    m_availableFeatures.append({"瞳孔中心Y坐标", "基础特征", false, "瞳孔中心Y坐标"});
    m_availableFeatures.append({"瞳孔长轴", "基础特征", false, "瞳孔椭圆长轴长度"});
    m_availableFeatures.append({"瞳孔短轴", "基础特征", false, "瞳孔椭圆短轴长度"});
    m_availableFeatures.append({"瞳孔宽度", "基础特征", false, "瞳孔边界框宽度"});
    m_availableFeatures.append({"瞳孔高度", "基础特征", false, "瞳孔边界框高度"});
    m_availableFeatures.append({"瞳孔直径", "基础特征", false, "瞳孔等效直径"});
    m_availableFeatures.append({"瞳孔未失真直径", "基础特征", false, "校正失真后的瞳孔直径"});
    m_availableFeatures.append({"瞳孔物理直径", "基础特征", false, "真实物理尺寸的瞳孔直径"});
    m_availableFeatures.append({"瞳孔置信度", "基础特征", false, "瞳孔检测置信度"});
    m_availableFeatures.append({"瞳孔轮廓置信度", "基础特征", false, "瞳孔轮廓检测置信度"});
    m_availableFeatures.append({"瞳孔周长", "基础特征", false, "瞳孔轮廓周长"});
    m_availableFeatures.append({"瞳孔轴比", "基础特征", false, "瞳孔长短轴比例"});
    m_availableFeatures.append({"眼部开合度", "基础特征", false, "眼部开合程度"});
    m_availableFeatures.append({"眨眼状态", "基础特征", true, "当前眨眼状态"});
    m_availableFeatures.append({"眼动类型", "基础特征", true, "眼动运动类型分类"});
    m_availableFeatures.append({"注视稳定性", "基础特征", true, "注视时的稳定程度"});
    m_availableFeatures.append({"平均瞳孔直径", "基础特征", false, "最近100帧瞳孔直径平均值"});
    m_availableFeatures.append({"瞳孔相对偏差", "基础特征", false, "当前瞳孔直径与平均值的相对偏差比例"});

    // ==================== 运动特征 (17个) ====================
    m_availableFeatures.append({"视线屏幕X(px)", "运动特征", true, "视线在屏幕X坐标"});
    m_availableFeatures.append({"视线屏幕Y(px)", "运动特征", true, "视线在屏幕Y坐标"});
    m_availableFeatures.append({"视线水平速度", "运动特征", false, "视线X方向速度"});
    m_availableFeatures.append({"视线垂直速度", "运动特征", false, "视线Y方向速度"});
    m_availableFeatures.append({"视线合成速度", "运动特征", true, "视线移动速度"});
    m_availableFeatures.append({"扫视幅度", "运动特征", false, "扫视运动幅度"});
    m_availableFeatures.append({"扫视路径凸包面积(px²)", "运动特征", true, "一个时间窗口内视觉注意力场的量化面积"});
    m_availableFeatures.append({"注视密度(1/px)", "运动特征", true, "基于质心平均距离的注视空间聚焦程度量化"});
    m_availableFeatures.append({"扫视熵", "运动特征", false, "基于方向分布的扫视模式复杂性和随机性量化"});
    m_availableFeatures.append({"主序关系斜率", "运动特征", false, "扫视幅度与速度峰值线性关系的斜率系数"});
    m_availableFeatures.append({"扫视速度峰值", "运动特征", false, "扫视过程中的最大瞬时速度平均值"});
    m_availableFeatures.append({"瞳孔水平速度", "运动特征", false, "瞳孔X方向速度"});
    m_availableFeatures.append({"瞳孔垂直速度", "运动特征", false, "瞳孔Y方向速度"});
    m_availableFeatures.append({"瞳孔合成速度", "运动特征", false, "瞳孔移动速度"});
    m_availableFeatures.append({"瞳孔水平加速度", "运动特征", false, "瞳孔X方向加速度"});
    m_availableFeatures.append({"瞳孔垂直加速度", "运动特征", false, "瞳孔Y方向加速度"});
    m_availableFeatures.append({"瞳孔尺寸变化率", "运动特征", false, "瞳孔大小变化率"});

    // ==================== 时序特征 (6个) ====================
    m_availableFeatures.append({"当前状态持续时长(ms)", "时序特征", true, "当前眼动状态持续时间"});
    m_availableFeatures.append({"眨眼频率", "时序特征", false, "眨眼频率(/分钟)"});
    m_availableFeatures.append({"眨眼时长", "时序特征", false, "平均眨眼持续时间"});
    m_availableFeatures.append({"平均注视时长", "时序特征", false, "历史注视时长平均值"});
    m_availableFeatures.append({"扫视频率", "时序特征", false, "扫视频率(/秒)"});
    m_availableFeatures.append({"注视率", "时序特征", false, "单位时间内注视次数 (次/秒)"});

    // ==================== 行为特征 (4个) ====================
    m_availableFeatures.append({"多任务能力", "行为特征", false, "多任务处理能力"});
    m_availableFeatures.append({"交互意图", "行为特征", true, "交互意图标签"});
    m_availableFeatures.append({"疲劳等级", "行为特征", true, "疲劳程度等级"});
    m_availableFeatures.append({"认知负荷", "行为特征", true, "认知负荷水平"});

    // 初始化选中状态
    for (const FeatureConfig& feature : m_availableFeatures) {
        m_selectedFeatures[feature.name] = feature.enabled;
    }
}

void DataTable::onFeatureSelectionClicked() {
    showFeatureSelectionDialog();
}

void DataTable::showFeatureSelectionDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("选择导出特征");
    dialog.setMinimumSize(500, 400);

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    // 添加说明文本
    QLabel* instructionLabel = new QLabel("请选择要导出的眼动特征:", &dialog);
    instructionLabel->setStyleSheet("font-weight: bold; margin-bottom: 10px;");
    mainLayout->addWidget(instructionLabel);

    // 创建特征选择区域
    QScrollArea* scrollArea = new QScrollArea(&dialog);
    QWidget* scrollWidget = new QWidget();
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);

    QMap<QString, QList<QCheckBox*>> categoryCheckBoxes;
    QString currentCategory = "";

    for (const FeatureConfig& feature : m_availableFeatures) {
        // 如果是新类别，添加类别标题
        if (feature.category != currentCategory) {
            currentCategory = feature.category;

            QLabel* categoryLabel = new QLabel(feature.category);
            categoryLabel->setStyleSheet("font-weight: bold; color: #2c3e50; margin-top: 10px; margin-bottom: 5px;");
            scrollLayout->addWidget(categoryLabel);

            // 添加该类别的全选/全不选按钮（稍后会连接事件处理程序）
            QHBoxLayout* categoryControlLayout = new QHBoxLayout();
            QPushButton* selectAllBtn = new QPushButton(QString("全选%1").arg(feature.category));
            QPushButton* deselectAllBtn = new QPushButton(QString("全不选%1").arg(feature.category));
            selectAllBtn->setMaximumWidth(120);
            selectAllBtn->setMinimumHeight(28);
            selectAllBtn->setStyleSheet("background-color: #17a2b8; color: white; font-weight: bold; padding: 4px 8px; border-radius: 4px; font-size: 11px;");
            deselectAllBtn->setMaximumWidth(120);
            deselectAllBtn->setMinimumHeight(28);
            deselectAllBtn->setStyleSheet("background-color: #6c757d; color: white; font-weight: bold; padding: 4px 8px; border-radius: 4px; font-size: 11px;");

            // 为按钮设置属性以便稍后识别
            selectAllBtn->setProperty("category", feature.category);
            selectAllBtn->setProperty("action", "selectAll");
            deselectAllBtn->setProperty("category", feature.category);
            deselectAllBtn->setProperty("action", "deselectAll");

            categoryControlLayout->addWidget(selectAllBtn);
            categoryControlLayout->addWidget(deselectAllBtn);
            categoryControlLayout->addStretch();

            scrollLayout->addLayout(categoryControlLayout);
        }

        // 创建特征复选框
        QHBoxLayout* featureLayout = new QHBoxLayout();
        QCheckBox* checkBox = new QCheckBox(feature.name);
        checkBox->setChecked(m_selectedFeatures[feature.name]);

        QLabel* descLabel = new QLabel(feature.description);
        descLabel->setStyleSheet("color: #7f8c8d; font-size: 11px;");
        descLabel->setWordWrap(true);

        featureLayout->addWidget(checkBox);
        featureLayout->addWidget(descLabel, 1);

        scrollLayout->addLayout(featureLayout);

        categoryCheckBoxes[feature.category].append(checkBox);
    }

    scrollWidget->setLayout(scrollLayout);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);
    mainLayout->addWidget(scrollArea);

    // 添加统计信息
    QLabel* statsLabel = new QLabel();
    auto updateStats = [&]() {
        int selectedCount = 0;
        for (auto& checkBoxList : categoryCheckBoxes) {
            for (QCheckBox* cb : checkBoxList) {
                if (cb->isChecked()) selectedCount++;
            }
        }
        statsLabel->setText(QString("已选择 %1 / %2 个特征").arg(selectedCount).arg(m_availableFeatures.size()));
    };
    updateStats();

    // 连接复选框变化信号
    for (auto& checkBoxList : categoryCheckBoxes) {
        for (QCheckBox* cb : checkBoxList) {
            connect(cb, &QCheckBox::toggled, updateStats);
        }
    }

    // 连接类别特定的全选/全不选按钮
    QList<QPushButton*> categoryButtons = scrollWidget->findChildren<QPushButton*>();
    for (QPushButton* btn : categoryButtons) {
        QString category = btn->property("category").toString();
        QString action = btn->property("action").toString();

        if (!category.isEmpty() && !action.isEmpty()) {
            if (action == "selectAll") {
                connect(btn, &QPushButton::clicked, [&categoryCheckBoxes, category, updateStats]() {
                    if (categoryCheckBoxes.contains(category)) {
                        for (QCheckBox* cb : categoryCheckBoxes[category]) {
                            cb->setChecked(true);
                        }
                        updateStats();
                    }
                });
            } else if (action == "deselectAll") {
                connect(btn, &QPushButton::clicked, [&categoryCheckBoxes, category, updateStats]() {
                    if (categoryCheckBoxes.contains(category)) {
                        for (QCheckBox* cb : categoryCheckBoxes[category]) {
                            cb->setChecked(false);
                        }
                        updateStats();
                    }
                });
            }
        }
    }

    mainLayout->addWidget(statsLabel);

    // 添加按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* selectAllButton = new QPushButton("全选");
    QPushButton* deselectAllButton = new QPushButton("全不选");
    QPushButton* okButton = new QPushButton("确定");
    QPushButton* cancelButton = new QPushButton("取消");

    // 全选/全不选功能
    connect(selectAllButton, &QPushButton::clicked, [&]() {
        for (auto& checkBoxList : categoryCheckBoxes) {
            for (QCheckBox* cb : checkBoxList) {
                cb->setChecked(true);
            }
        }
    });

    connect(deselectAllButton, &QPushButton::clicked, [&]() {
        for (auto& checkBoxList : categoryCheckBoxes) {
            for (QCheckBox* cb : checkBoxList) {
                cb->setChecked(false);
            }
        }
    });

    buttonLayout->addWidget(selectAllButton);
    buttonLayout->addWidget(deselectAllButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // 保存用户选择
        for (int i = 0; i < m_availableFeatures.size(); ++i) {
            const QString& featureName = m_availableFeatures[i].name;
            for (auto& checkBoxList : categoryCheckBoxes) {
                for (QCheckBox* cb : checkBoxList) {
                    if (cb->text() == featureName) {
                        m_selectedFeatures[featureName] = cb->isChecked();
                        break;
                    }
                }
            }
        }

        // 更新状态显示
        int selectedCount = 0;
        for (const bool& selected : m_selectedFeatures) {
            if (selected) selectedCount++;
        }

        m_statusLabel->setText(QString("特征选择已更新: %1/%2 个特征").arg(selectedCount).arg(m_availableFeatures.size()));
    }
}

QString DataTable::generateCustomCSVHeader() {
    QString header = "时间戳";

    // 添加实验相关字段
    if (m_isExperimentRunning || !m_currentExperimentType.isEmpty()) {
        if (m_selectedFeatures.value("交互意图", false)) {
            header += ",交互意图";
        }
        if (m_selectedFeatures.value("疲劳等级", false)) {
            header += ",疲劳等级";
        }
        if (m_selectedFeatures.value("认知负荷", false)) {
            header += ",认知负荷";
        }
        if (m_selectedFeatures.value("实验备注", false)) {
            header += ",实验备注";
        }
        header += ",实验类型,当前任务,实验状态";
    }

    // 按照特征列表顺序添加选中的特征
    for (const FeatureConfig& feature : m_availableFeatures) {
        if (m_selectedFeatures.value(feature.name, false) && feature.category != "实验特征") {
            header += "," + feature.name;
        }
    }

    header += "\n";
    return header;
}

void DataTable::writeCustomDataRecordToFile(const DataRecord& record) {
    if (!m_realTimeExportStream) return;

    const PupilData& pupil = record.result.pupilData;
    const EyeMetrics& metrics = record.result.eyeMetrics;

    // 写入时间戳
    *m_realTimeExportStream << record.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");

    // 写入实验相关字段
    if (m_isExperimentRunning || !m_currentExperimentType.isEmpty()) {
        if (m_selectedFeatures.value("交互意图", false)) {
            QStringList intentions = QStringList() << "浏览" << "点击准备" << "阅读" << "搜索" << "选择" << "无明确意图";
            QString intentionText = (record.intention < intentions.size()) ? intentions[record.intention] : "未知";
            *m_realTimeExportStream << "," << intentionText;
        }
        if (m_selectedFeatures.value("疲劳等级", false)) {
            *m_realTimeExportStream << "," << record.fatigue;
        }
        if (m_selectedFeatures.value("认知负荷", false)) {
            *m_realTimeExportStream << "," << record.workload;
        }
        if (m_selectedFeatures.value("实验备注", false)) {
            QString notes = record.notes;
            *m_realTimeExportStream << "," << notes.replace(",", ";");
        }
        *m_realTimeExportStream << "," << record.experimentType
                               << "," << record.currentTask
                               << "," << record.experimentStatus;
    }

    // 按照特征列表顺序写入选中的特征数据
    for (const FeatureConfig& feature : m_availableFeatures) {
        if (!m_selectedFeatures.value(feature.name, false) || feature.category == "实验特征") {
            continue;
        }

        QString value = "0";

        // 基础特征
        if (feature.name == "瞳孔直径") {
            value = QString::number(pupil.diameter(), 'f', 2);
        } else if (feature.name == "瞳孔X坐标") {
            value = QString::number(pupil.center.x, 'f', 2);
        } else if (feature.name == "瞳孔Y坐标") {
            value = QString::number(pupil.center.y, 'f', 2);
        } else if (feature.name == "瞳孔置信度") {
            value = QString::number(pupil.confidence, 'f', 3);
        } else if (feature.name == "轮廓置信度") {
            value = QString::number(pupil.outline_confidence, 'f', 3);
        } else if (feature.name == "瞳孔长轴") {
            value = QString::number(pupil.size.width, 'f', 2);
        } else if (feature.name == "瞳孔短轴") {
            value = QString::number(pupil.size.height, 'f', 2);
        } else if (feature.name == "瞳孔宽度") {
            value = QString::number(pupil.size.width, 'f', 2);
        } else if (feature.name == "瞳孔高度") {
            value = QString::number(pupil.size.height, 'f', 2);
        }
        // 运动特征
        else if (feature.name == "眼部开合度") {
            value = QString::number(metrics.eyeOpenness, 'f', 3);
        } else if (feature.name == "眨眼状态") {
            value = QString::number(metrics.blinkState, 'f', 3);
        } else if (feature.name == "瞳孔速度X") {
            value = QString::number(metrics.pupilVelocity.x, 'f', 2);
        } else if (feature.name == "瞳孔速度Y") {
            value = QString::number(metrics.pupilVelocity.y, 'f', 2);
        } else if (feature.name == "瞳孔速度") {
            value = QString::number(metrics.pupilSpeed, 'f', 2);
        } else if (feature.name == "瞳孔加速度X") {
            value = QString::number(metrics.pupilAcceleration.x, 'f', 2);
        } else if (feature.name == "瞳孔加速度Y") {
            value = QString::number(metrics.pupilAcceleration.y, 'f', 2);
        } else if (feature.name == "瞳孔大小变化") {
            value = QString::number(metrics.pupilSizeChange, 'f', 3);
        }
        // 行为特征
        else if (feature.name == "眼动类型") {
            switch(metrics.eyeMovementType) {
                case 0: value = "0-扫视"; break;
                case 1: value = "1-注视"; break;
                case 3: value = "3-眨眼/无效"; break;
                default: value = "未知"; break;
            }
        } else if (feature.name == "注视稳定性") {
            value = QString::number(metrics.fixationStability, 'f', 3);
        } else if (feature.name == "帧率") {
            value = QString::number(metrics.frameRate, 'f', 1);
        }

        *m_realTimeExportStream << "," << value;
    }

    *m_realTimeExportStream << "\n";
}

QString DataTable::getFeatureValueFromUI(const QString& featureName) {
    // 从UI模型中获取特征值

    // 基础特征
    if (featureName == "瞳孔直径" && m_basicModel->item(4, 0)) {
        return m_basicModel->item(4, 0)->text();
    } else if (featureName == "瞳孔X坐标" && m_basicModel->item(1, 0)) {
        return m_basicModel->item(1, 0)->text();
    } else if (featureName == "瞳孔Y坐标" && m_basicModel->item(2, 0)) {
        return m_basicModel->item(2, 0)->text();
    } else if (featureName == "瞳孔置信度" && m_basicModel->item(6, 0)) {
        return m_basicModel->item(6, 0)->text();
    } else if (featureName == "轮廓置信度" && m_basicModel->item(7, 0)) {
        return m_basicModel->item(7, 0)->text();
    } else if (featureName == "瞳孔长轴" && m_basicModel->item(3, 0)) {
        return m_basicModel->item(3, 0)->text();
    } else if (featureName == "瞳孔短轴" && m_basicModel->item(4, 0)) {
        return m_basicModel->item(4, 0)->text();
    } else if (featureName == "瞳孔宽度" && m_basicModel->item(3, 0)) {
        return m_basicModel->item(3, 0)->text();
    } else if (featureName == "瞳孔高度" && m_basicModel->item(4, 0)) {
        return m_basicModel->item(4, 0)->text();
    } else if (featureName == "平均瞳孔直径" && m_basicModel->item(21, 0)) {
        return m_basicModel->item(21, 0)->text();
    } else if (featureName == "瞳孔相对偏差" && m_basicModel->item(22, 0)) {
        return m_basicModel->item(22, 0)->text();
    }
    // 运动特征
    else if (featureName == "眼部开合度" && m_motionModel->item(0, 0)) {
        return m_motionModel->item(0, 0)->text();
    } else if (featureName == "眨眼状态" && m_motionModel->item(1, 0)) {
        return m_motionModel->item(1, 0)->text();
    } else if (featureName == "瞳孔速度X" && m_motionModel->item(5, 0)) {
        return m_motionModel->item(5, 0)->text();
    } else if (featureName == "瞳孔速度Y" && m_motionModel->item(6, 0)) {
        return m_motionModel->item(6, 0)->text();
    } else if (featureName == "瞳孔速度" && m_motionModel->item(7, 0)) {
        return m_motionModel->item(7, 0)->text();
    } else if (featureName == "瞳孔加速度X" && m_motionModel->item(8, 0)) {
        return m_motionModel->item(8, 0)->text();
    } else if (featureName == "瞳孔加速度Y" && m_motionModel->item(9, 0)) {
        return m_motionModel->item(9, 0)->text();
    } else if (featureName == "瞳孔大小变化" && m_motionModel->item(10, 0)) {
        return m_motionModel->item(10, 0)->text();
    } else if (featureName == "扫视幅度" && m_motionModel->item(11, 0)) {
        return m_motionModel->item(11, 0)->text();
    } else if (featureName == "扫视路径凸包面积(px²)" && m_motionModel->item(12, 0)) {
        return m_motionModel->item(12, 0)->text();
    } else if (featureName == "注视密度(1/px)" && m_motionModel->item(13, 0)) {
        return m_motionModel->item(13, 0)->text();
    } else if (featureName == "扫视熵" && m_motionModel->item(14, 0)) {
        return m_motionModel->item(14, 0)->text();
    } else if (featureName == "主序关系斜率" && m_motionModel->item(15, 0)) {
        return m_motionModel->item(15, 0)->text();
    } else if (featureName == "扫视速度峰值" && m_motionModel->item(16, 0)) {
        return m_motionModel->item(16, 0)->text();
    }
    // 行为特征
    else if (featureName == "眼动类型" && m_basicModel->item(19, 0)) {
        // 眼动类型现在在基础特征表格的第19项
        return m_basicModel->item(19, 0)->text();
    } else if (featureName == "注视稳定性" && m_behaviorModel->item(1, 0)) {
        return m_behaviorModel->item(1, 0)->text();
    } else if (featureName == "帧率" && m_behaviorModel->item(2, 0)) {
        return m_behaviorModel->item(2, 0)->text();
    } else if (featureName == "注视率" && m_behaviorModel->item(0, 0)) {
        return m_behaviorModel->item(0, 0)->text();
    }
    // 时序特征
    else if (featureName == "眨眼时长" && m_temporalModel->item(3, 0)) {
        return m_temporalModel->item(3, 0)->text();
    } else if (featureName == "平均注视时长" && m_temporalModel->item(4, 0)) {
        return m_temporalModel->item(4, 0)->text();
    }

    // 如果没有找到对应的特征值，返回默认值
    return "0";
}

// 新增：直接从数据源获取特征值，避免UI索引错误
QString DataTable::getFeatureValueFromData(const QString& featureName, const ProcessingResult& result) {
    // 注意：这个函数主要用于实时导出，应该使用DataRecord中存储的计算特征值
    // 对于历史数据导出，应该使用重载版本 getFeatureValueFromData(featureName, dataRecord)
    return getFeatureValueFromUI(featureName); // 回退到UI获取，但这不是最佳方案
}

QString DataTable::getFeatureValueFromData(const QString& featureName, const DataRecord& record) {
    // 优先使用存储的计算特征值
    if (record.calculatedFeatures.contains(featureName)) {
        return record.calculatedFeatures[featureName];
    }

    // 使用特征提取函数映射表
    if (m_featureExtractors.contains(featureName)) {
        return m_featureExtractors[featureName](record);
    }

    return "0"; // 未找到对应特征
}

void DataTable::initializeFeatureExtractors() {
    // 基础特征提取器
    m_featureExtractors["时间戳"] = [](const DataRecord& record) {
        return QDateTime::fromMSecsSinceEpoch(record.result.eyeMetrics.timestamp).toString("hh:mm:ss.zzz");
    };

    m_featureExtractors["相机帧率"] = [](const DataRecord& record) {
        return QString::number(record.result.eyeMetrics.cameraFps, 'f', 1);
    };

    m_featureExtractors["瞳孔跟踪帧率"] = [](const DataRecord& record) {
        return QString::number(record.result.eyeMetrics.frameRate, 'f', 1);
    };

    m_featureExtractors["瞳孔中心X"] = [](const DataRecord& record) {
        return QString::number(record.result.pupilData.center.x, 'f', 2);
    };

    m_featureExtractors["瞳孔中心Y"] = [](const DataRecord& record) {
        return QString::number(record.result.pupilData.center.y, 'f', 2);
    };

    m_featureExtractors["瞳孔直径"] = [](const DataRecord& record) {
        return QString::number(record.result.pupilData.diameter(), 'f', 2);
    };

    m_featureExtractors["瞳孔置信度"] = [](const DataRecord& record) {
        return QString::number(record.result.pupilData.confidence, 'f', 3);
    };

    m_featureExtractors["眼部开合度"] = [](const DataRecord& record) {
        return QString::number(record.result.eyeMetrics.eyeOpenness, 'f', 3);
    };

    m_featureExtractors["眨眼状态"] = [](const DataRecord& record) {
        return QString::number(record.result.eyeMetrics.blinkState, 'f', 3);
    };

    m_featureExtractors["注视稳定性"] = [](const DataRecord& record) {
        return QString::number(record.result.eyeMetrics.fixationStability, 'f', 3);
    };

    // 运动特征提取器
    m_featureExtractors["视线屏幕X(px)"] = [](const DataRecord& record) {
        return record.result.gazeValid ? QString::number(record.result.gazeScreenCoords.x, 'f', 2) : "0";
    };

    m_featureExtractors["视线屏幕Y(px)"] = [](const DataRecord& record) {
        return record.result.gazeValid ? QString::number(record.result.gazeScreenCoords.y, 'f', 2) : "0";
    };

    m_featureExtractors["视线速度X"] = [](const DataRecord& record) {
        return record.result.gazeValid ? QString::number(record.result.gazeVelocity.x, 'f', 2) : "0";
    };

    m_featureExtractors["视线速度Y"] = [](const DataRecord& record) {
        return record.result.gazeValid ? QString::number(record.result.gazeVelocity.y, 'f', 2) : "0";
    };

    m_featureExtractors["视线速度"] = [](const DataRecord& record) {
        return record.result.gazeValid ? QString::number(record.result.gazeSpeed, 'f', 2) : "0";
    };

    // 行为特征提取器
    m_featureExtractors["多任务能力"] = [](const DataRecord& record) {
        return QString::number(1 + (record.result.eyeMetrics.fixationStability * 9), 'f', 0);
    };

    m_featureExtractors["交互意图"] = [](const DataRecord& record) {
        QStringList intentions = QStringList() << "浏览" << "点击准备" << "阅读" << "搜索" << "选择" << "无明确意图";
        return intentions[record.intention % intentions.size()];
    };

    m_featureExtractors["疲劳等级"] = [](const DataRecord& record) {
        return QString::number(record.fatigue);
    };

    m_featureExtractors["认知负荷"] = [](const DataRecord& record) {
        return QString::number(record.workload);
    };
}

// ==================== 权威状态机实现 ====================

// dataTable.cpp -> 替换 updateEyeState 函数
void DataTable::updateEyeState(const ProcessingResult& result) {
    QDateTime currentTime = QDateTime::currentDateTime();
    EyeMovementState determinedState; // 我们自己判断出的状态

    // 1. 【修改】优先使用Worker中I-VD算法的分类结果
    if (!result.pupilFound || result.eyeMetrics.blinkState <= 0.1) {
        determinedState = STATE_BLINK;
    } else {
        // 使用Worker的eyeMovementType分类结果（现在基于I-VD算法）
        switch (result.eyeMetrics.eyeMovementType) {
            case 0:  // 扫视
                determinedState = STATE_SACCADE;
                break;
            case 1:  // 注视
                determinedState = STATE_FIXATION;
                break;
            case 2:  // 眨眼
                determinedState = STATE_BLINK;
                break;
            default:
                // 未知状态，保持当前状态
                determinedState = m_currentState;
                break;
        }

        // 【备用方案】如果I-VD算法结果不可信，使用速度阈值作为后备
        if (result.gazeValid) {
            if (result.gazeSpeed > 500) {
                // 极高速度，强制认为是扫视
                determinedState = STATE_SACCADE;
            } else if (result.gazeSpeed < 10) {
                // 极低速度，强制认为是注视
                determinedState = STATE_FIXATION;
            }
        }
    }


    // 2. 状态切换逻辑
    if (determinedState != m_currentState) {
        handleStateTransition(determinedState, result.eyeMetrics.stateStartTime);
    }

    // 3. 轨迹记录逻辑 - 【修复】限制轨迹点数量防止内存过度增长
    if (m_currentState == STATE_SACCADE) {
        // 优先使用视线坐标，如果无效则使用瞳孔坐标
        cv::Point2f trajectoryPoint = result.gazeValid ? result.gazeScreenCoords : result.pupilData.center;


        m_currentSaccadeTrajectory.append(trajectoryPoint);
        m_currentSaccadeTimestamps.append(currentTime.toMSecsSinceEpoch());

        // 防止轨迹数据过多占用内存（最多保留500个点）
        if (m_currentSaccadeTrajectory.size() > 500) {
            m_currentSaccadeTrajectory.removeFirst();
            m_currentSaccadeTimestamps.removeFirst();
        }

        // 扫视轨迹记录（幅度将在扫视结束时计算）
    }
}


// dataTable.cpp -> 替换 handleStateTransition 函数
void DataTable::handleStateTransition(EyeMovementState newState, qint64 workerStateStartTime) {
    EyeMovementState previousState = m_currentState;
    QDateTime currentTime = QDateTime::currentDateTime();
    qint64 stateDuration = m_stateStartTime.msecsTo(currentTime);

    // 【核心决策逻辑】根据上一个状态的类型和持续时间，来决定如何处理
    if (previousState == STATE_SACCADE) {
        // 一个"扫视"状态刚刚结束
        // 【修复】进一步放宽时长要求，只要大于1ms就认为有效
        if (stateDuration >= 1) {
            // qDebug() << "有效扫视结束，时长:" << stateDuration << "ms";
            finalizeSaccadeEvent(stateDuration);
        } else {
            // 持续时间太短或太长，都认为是噪声或错误分类，忽略
            // qDebug() << "伪扫视被过滤，时长:" << stateDuration << "ms";
        }
    }
    else if (previousState == STATE_FIXATION) {
        // 一个"注视"状态刚刚结束
        if (stateDuration >= 100) { // 持续时间超过100ms，这是一次有效的注视
            m_recentFixationDurations.append(stateDuration);
            if (m_recentFixationDurations.size() > 50) {
                m_recentFixationDurations.removeFirst();
            }
            // qDebug() << "有效注视结束，时长:" << stateDuration << "ms";
        } else {
            // 持续时间太短，这不是一次有意义的注视，忽略
            // qDebug() << "注视噪声被过滤，时长:" << stateDuration << "ms";
        }
    }
    else if (previousState == STATE_BLINK) {
        // 一个"眨眼"状态刚刚结束
        if (stateDuration >= 50 && stateDuration <= 1000) { // 有效眨眼时长50-1000ms
            m_blinkDurations.append(stateDuration);
            if (m_blinkDurations.size() > 20) {
                m_blinkDurations.removeFirst();
            }
//            qDebug() << "【有效眨眼结束】时长:" << stateDuration << "ms";
        }
    }

    // 【新增】眨眼计数逻辑 - 从旧状态机移过来
    if (newState == STATE_BLINK && previousState != STATE_BLINK) {
        m_blinkCount++;
//        qDebug() << "【眨眼计数】眨眼次数增加，当前计数:" << m_blinkCount;
    }

    // 更新权威状态到新状态
    m_currentState = newState;

    // 使用Worker提供的状态开始时间，如果有的话
    if (workerStateStartTime > 0) {
        m_stateStartTime = QDateTime::fromMSecsSinceEpoch(workerStateStartTime);
    } else {
        m_stateStartTime = currentTime; // 后备方案：使用当前时间
    }

    // 处理新状态的开始事件
    if (newState == STATE_SACCADE) {
        m_currentSaccadeTrajectory.clear();
        m_currentSaccadeTimestamps.clear();
        // 注意：不重置m_currentSaccadeAmplitude，保持显示最近扫视的平均幅度
    }
}


// dataTable.cpp -> 替换 finalizeSaccadeEvent 函数
void DataTable::finalizeSaccadeEvent(qint64 duration) {
//    qDebug() << "【扫视事件完成调用】轨迹点数:" << m_currentSaccadeTrajectory.size() << " 持续时间:" << duration << "ms";

    // 【修复】降低要求，至少需要2个点即可计算基本指标
    if (m_currentSaccadeTrajectory.size() < 2) {
//        qDebug() << "【扫视数据不足】轨迹点数:" << m_currentSaccadeTrajectory.size() << "，跳过计算。";
        return;
    }

    // 1. 计算扫视幅度
    cv::Point2f startPos = m_currentSaccadeTrajectory.first();
    cv::Point2f endPos = m_currentSaccadeTrajectory.last();
    float amplitude = std::sqrt(std::pow(endPos.x - startPos.x, 2) + std::pow(endPos.y - startPos.y, 2));

    // 2. 计算扫视方向（用于熵计算）
    float angle = calculateSaccadeAngle(startPos, endPos);

    // 3. 计算速度峰值
    float peakVelocity = calculatePeakVelocityFromTrajectory(m_currentSaccadeTrajectory, m_currentSaccadeTimestamps);

    // 【放宽】降低速度阈值，避免丢失有效扫视数据
    if (peakVelocity <= 20.0f) { // 速度低于20px/s才认为无效
        qDebug() << "【扫视计算失败】速度峰值过低(" << peakVelocity << ")，视为无效扫视。";
        return;
    }

    // 4. 创建完整的扫视指标记录
    SaccadeMetrics metrics;
    metrics.amplitude = amplitude;
    metrics.peakVelocity = peakVelocity;
    metrics.duration = duration;
    metrics.timestamp = QDateTime::currentDateTime();

    // 5. 添加到各个历史队列
    m_recentSaccadeAmplitudes.append(amplitude);
    if (m_recentSaccadeAmplitudes.size() > 2) m_recentSaccadeAmplitudes.removeFirst();  // 只保留最近2次

    // 【修复】添加扫视时长到历史记录，用于扫视频率计算
    m_recentSaccadeDurations.append(duration);
    if (m_recentSaccadeDurations.size() > 50) m_recentSaccadeDurations.removeFirst();

    SaccadeDirectionData directionData;
    directionData.angle = angle;
    directionData.timestamp = QDateTime::currentDateTime();
    m_saccadeDirectionHistory.append(directionData);
    // 使用更宽松的清理策略：保留更多历史数据，让指数衰减权重自然处理旧数据
    QDateTime cutoffTime = QDateTime::currentDateTime().addMSecs(-SACCADE_ENTROPY_WINDOW_SIZE * 3); // 保留3倍窗口的数据
    while (!m_saccadeDirectionHistory.isEmpty() && m_saccadeDirectionHistory.first().timestamp < cutoffTime) {
        m_saccadeDirectionHistory.removeFirst();
    }

    m_saccadeMetricsHistory.append(metrics);
    cutoffTime = QDateTime::currentDateTime().addMSecs(-MAIN_SEQUENCE_WINDOW_SIZE * 2); // 保留2倍窗口的数据
    while (!m_saccadeMetricsHistory.isEmpty() && m_saccadeMetricsHistory.first().timestamp < cutoffTime) {
        m_saccadeMetricsHistory.removeFirst();
    }

    m_saccadePeakVelocityHistory.append(peakVelocity);
    if (m_saccadePeakVelocityHistory.size() > PEAK_VELOCITY_HISTORY_SIZE) {
        m_saccadePeakVelocityHistory.removeFirst();
    }

    // 6. 更新当前计算值
    m_currentSaccadeAmplitude = m_recentSaccadeAmplitudes.isEmpty() ? 0.0f :
        std::accumulate(m_recentSaccadeAmplitudes.begin(), m_recentSaccadeAmplitudes.end(), 0.0f) / m_recentSaccadeAmplitudes.size();

//    qDebug() << "【扫视事件完成】幅度:" << amplitude << " 角度:" << angle
//             << " 峰值速度:" << peakVelocity << " 时长:" << duration;
}


// ==================== 新增：导出选择功能 ====================

void DataTable::showExportOptionsDialog() {
    // 根据组合框选择的导出类型进行导出
    QString selectedType = m_exportDataTypeComboBox->currentData().toString();

    if (selectedType == "instantaneous") {
        exportInstantaneousData();
    } else if (selectedType == "realtime") {
        exportRealTimeData();
    } else {
        // 【修复】使用非阻塞错误提示
        m_statusLabel->setText("❌ 错误：请选择有效的导出类型");
        QTimer::singleShot(100, this, [this]() {
            auto* msgBox = new QMessageBox(QMessageBox::Warning,
                                          "错误",
                                          "请选择有效的导出类型",
                                          QMessageBox::Ok, this);
            msgBox->setModal(false);
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->show();
        });
    }
}

void DataTable::exportInstantaneousData() {
    QString fileName = QFileDialog::getSaveFileName(this,
        "导出瞬时特征数据",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/instantaneous_eyetracking_data.csv",
        "CSV files (*.csv);;All files (*.*)");

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // 【修复】使用非阻塞错误提示 - 瞬时导出
        QTimer::singleShot(100, this, [this, fileName]() {
            auto* msgBox = new QMessageBox(QMessageBox::Warning,
                                          "错误",
                                          "无法创建文件: " + fileName,
                                          QMessageBox::Ok, this);
            msgBox->setModal(false);
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->show();
        });
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");

    // 写入CSV头部
    out << "timestamp,experiment_type,current_task,experiment_status,intention,fatigue,workload,notes";
    for (const FeatureConfig& feature : m_availableFeatures) {
        if (m_selectedFeatures.value(feature.name, false)) {
            out << "," << feature.name;
        }
    }
    out << "\n";

    // 写入当前时刻的数据
    if (!m_dataBuffer.isEmpty()) {
        // 优先使用缓存中的最后一条记录
        const DataRecord& lastRecord = m_dataBuffer.last();

        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
            << "," << lastRecord.experimentType
            << "," << lastRecord.currentTask
            << "," << lastRecord.experimentStatus
            << "," << lastRecord.intention
            << "," << lastRecord.fatigue
            << "," << lastRecord.workload
            << "," << lastRecord.notes;

        for (const FeatureConfig& feature : m_availableFeatures) {
            if (m_selectedFeatures.value(feature.name, false)) {
                QString value = getFeatureValueFromData(feature.name, lastRecord);
                out << "," << value;
            }
        }
        out << "\n";
    } else if (m_hasValidProcessingResult) {
        // 如果没有缓存数据，但有最后一次的ProcessingResult，创建临时记录
        DataRecord tempRecord;
        tempRecord.timestamp = QDateTime::currentDateTime();
        tempRecord.result = m_lastProcessingResult;
        tempRecord.experimentType = m_currentExperimentType;
        tempRecord.currentTask = (m_currentTaskIndex < m_taskList.size()) ? m_taskList[m_currentTaskIndex] : "无任务";
        tempRecord.experimentStatus = m_isRecordingData ? "记录中" : "未开始";
        tempRecord.intention = m_currentIntention;
        tempRecord.fatigue = m_currentFatigue;
        tempRecord.workload = m_currentWorkload;
        tempRecord.notes = m_currentNotes;

        // 创建临时记录的特征值（直接从 UI 获取）
        for (const FeatureConfig& feature : m_availableFeatures) {
            if (m_selectedFeatures.value(feature.name, false)) {
                tempRecord.calculatedFeatures[feature.name] = getFeatureValueFromUI(feature.name);
            }
        }

        out << tempRecord.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz")
            << "," << tempRecord.experimentType
            << "," << tempRecord.currentTask
            << "," << tempRecord.experimentStatus
            << "," << tempRecord.intention
            << "," << tempRecord.fatigue
            << "," << tempRecord.workload
            << "," << tempRecord.notes;

        for (const FeatureConfig& feature : m_availableFeatures) {
            if (m_selectedFeatures.value(feature.name, false)) {
                QString value = getFeatureValueFromData(feature.name, tempRecord);
                out << "," << value;
            }
        }
        out << "\n";
    } else {
        // 既没有缓存数据也没有有效的ProcessingResult
        // 【修复】使用非阻塞警告提示
        file.close();
        QTimer::singleShot(100, this, [this]() {
            auto* msgBox = new QMessageBox(QMessageBox::Warning,
                                          "无数据",
                                          "当前没有可用的眼动数据进行瞬时导出。\n\n请确保眼动迟踪系统正在运行。",
                                          QMessageBox::Ok, this);
            msgBox->setModal(false);
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->show();
        });
        return;
    }

    file.close();

    // 【修复】使用非阻塞消息提示
    QTimer::singleShot(100, this, [this, fileName]() {
        auto* msgBox = new QMessageBox(QMessageBox::Information,
                                      "导出完成",
                                      "瞬时数据已成功导出到: " + fileName,
                                      QMessageBox::Ok, this);
        msgBox->setModal(false);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->show();
    });
}

void DataTable::exportRealTimeData() {
    QString fileName = QFileDialog::getSaveFileName(this,
        "导出实时特征数据",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/realtime_eyetracking_data.csv",
        "CSV files (*.csv);;All files (*.*)");

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // 【修复】使用非阻塞错误提示 - 实时导出
        QTimer::singleShot(100, this, [this, fileName]() {
            auto* msgBox = new QMessageBox(QMessageBox::Warning,
                                          "错误",
                                          "无法创建文件: " + fileName,
                                          QMessageBox::Ok, this);
            msgBox->setModal(false);
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->show();
        });
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");

    // 写入CSV头部
    out << "timestamp,experiment_type,current_task,experiment_status,intention,fatigue,workload,notes";
    for (const FeatureConfig& feature : m_availableFeatures) {
        if (m_selectedFeatures.value(feature.name, false)) {
            out << "," << feature.name;
        }
    }
    out << "\n";

    // 写入缓存中的所有数据记录
    for (const DataRecord& record : m_dataBuffer) {
        out << record.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz")
            << "," << record.experimentType
            << "," << record.currentTask
            << "," << record.experimentStatus
            << "," << record.intention
            << "," << record.fatigue
            << "," << record.workload
            << "," << record.notes;

        for (const FeatureConfig& feature : m_availableFeatures) {
            if (m_selectedFeatures.value(feature.name, false)) {
                QString value = getFeatureValueFromData(feature.name, record);
                out << "," << value;
            }
        }
        out << "\n";
    }

    file.close();

    // 【修复】使用非阻塞消息提示
    QTimer::singleShot(100, this, [this, fileName]() {
        auto* msgBox = new QMessageBox(QMessageBox::Information,
                                      "导出完成",
                                      QString("实时数据已成功导出到: %1\n共导出 %2 条记录\n\n提示：系统支持最多10万条记录的缓存，可支持约27小时连续记录").arg(fileName).arg(m_dataBuffer.size()),
                                      QMessageBox::Ok, this);
        msgBox->setModal(false);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->show();
    });
}

// ==================== 窗口管理方法 ====================

void DataTable::positionWindowOnRightSide() {
    // 简化的右侧位置设置
    QDesktopWidget* desktop = QApplication::desktop();
    QRect screenGeometry = desktop->screenGeometry();

    int windowWidth = 450;
    // 重新计算窗口高度：更紧凑的布局，消除空白区域
    int experimentControlHeight = 140;
    int tabWidgetHeight = 180; // 进一步缩小特征表格高度
    int labelInputHeight = 160;
    int exportControlHeight = 70;
    int marginsAndSpacing = 55; // 适度增加边距和间距

    int estimatedContentHeight = experimentControlHeight + tabWidgetHeight + labelInputHeight + exportControlHeight + marginsAndSpacing;
    int windowHeight = qMin(estimatedContentHeight, screenGeometry.height() - 100);
    int xPos = screenGeometry.width() - windowWidth - 10;
    int yPos = 10;

    setGeometry(xPos, yPos, windowWidth, windowHeight);
    setMinimumSize(350, 400); // 设置合理的最小尺寸
    // 移除最大高度限制，允许用户自由调整窗口大小到任意高度

    show();
    raise();
    activateWindow();
}

void DataTable::adjustWindowForScreenChanges() {
    // 简化的屏幕变化调整（空实现）
}


void DataTable::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    // 简化版本：延迟保存窗口几何状态
    QTimer::singleShot(1000, this, &DataTable::saveWindowGeometry);
}

// 简化版本 - 移除复杂的停靠功能

// 移除复杂停靠功能 - 该方法已删除

// 移除停靠位置查询方法 - 该方法已删除

// 移除窗口标题更新方法 - 该方法已删除

// 移除事件过滤器 - 该方法已删除

void DataTable::moveEvent(QMoveEvent* event) {
    QWidget::moveEvent(event);

    // 简化版本：延迟保存窗口几何状态
    QTimer::singleShot(1000, this, &DataTable::saveWindowGeometry);
}

void DataTable::saveWindowState() {
    QSettings settings(m_settingsFile, QSettings::IniFormat);

    // 保存窗口几何信息
    settings.setValue("geometry", saveGeometry());

    // 保存窗口位置和大小
    settings.setValue("x", x());
    settings.setValue("y", y());
    settings.setValue("width", width());
    settings.setValue("height", height());

    settings.sync();
}

void DataTable::loadWindowState() {
    QSettings settings(m_settingsFile, QSettings::IniFormat);

    // 如果没有保存的设置，使用默认位置
    if (!settings.contains("geometry")) {
        positionWindowOnRightSide();
        return;
    }

    // 恢复窗口几何信息
    restoreGeometry(settings.value("geometry").toByteArray());

    // 恢复窗口位置和大小
    int savedX = settings.value("x", -1).toInt();
    int savedY = settings.value("y", -1).toInt();
    int savedWidth = settings.value("width", 450).toInt();
    int savedHeight = settings.value("height", 800).toInt();

    if (savedX >= 0 && savedY >= 0) {
        // 检查保存的位置是否仍然在屏幕范围内
        QDesktopWidget* desktop = QApplication::desktop();
        QRect screenGeometry = desktop->screenGeometry();

        if (savedX < screenGeometry.width() && savedY < screenGeometry.height()) {
            setGeometry(savedX, savedY, savedWidth, savedHeight);
        } else {
            // 如果保存的位置超出屏幕，使用默认位置
            positionWindowOnRightSide();
        }
    }
}

void DataTable::saveWindowGeometry() {
    QSettings settings(m_settingsFile, QSettings::IniFormat);

    // 只保存几何信息，不影响其他设置
    settings.setValue("x", x());
    settings.setValue("y", y());
    settings.setValue("width", width());
    settings.setValue("height", height());
    settings.setValue("geometry", saveGeometry());

    settings.sync();
}

void DataTable::loadWindowGeometry() {
    QSettings settings(m_settingsFile, QSettings::IniFormat);

    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
    }
}

// ==================== 标签事件记录系统实现 ====================

void DataTable::recordLabelChange(const QString& labelType, const QVariant& oldValue,
                                 const QVariant& newValue, const QString& description) {
    // 防止记录相同值的变更
    if (oldValue == newValue) {
        return;
    }

    LabelEvent event;
    event.timestamp = QDateTime::currentDateTime();
    event.labelType = labelType;
    event.oldValue = oldValue;
    event.newValue = newValue;
    event.description = description.isEmpty() ?
        QString("%1从%2变更为%3").arg(labelType).arg(oldValue.toString()).arg(newValue.toString()) :
        description;

    m_labelEvents.append(event);

    // 如果启用实时导出，立即写入标签事件到数据流
    if (m_realTimeExportEnabled && m_realTimeExportStream) {
        QString eventRecord = QString("%1,LABEL_EVENT,%2,%3,%4,%5\n")
            .arg(event.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"))
            .arg(event.labelType)
            .arg(event.oldValue.toString())
            .arg(event.newValue.toString())
            .arg(event.description);

        *m_realTimeExportStream << eventRecord;
        m_realTimeExportStream->flush();
    }

    qDebug() << "标签事件记录:" << event.labelType << oldValue.toString() << "->" << newValue.toString();
}

// =================================================================
// =             新增4个特征的完整实现函数                           =
// =================================================================

void DataTable::updateMeanPupilDiameter(const PupilData& pupil) {
    // 1. 平均瞳孔直径 (基础特征第22项)
    if (pupil.valid() && pupil.diameter() > 0) {
        m_pupilDiameterHistory.append(pupil.diameter());

        // 保持最近100个有效瞳孔记录
        while (m_pupilDiameterHistory.size() > 100) {
            m_pupilDiameterHistory.removeFirst();
        }
    }

    // 计算平均值
    float meanDiameter = 0.0f;
    if (!m_pupilDiameterHistory.isEmpty()) {
        float sum = 0.0f;
        for (float diameter : m_pupilDiameterHistory) {
            sum += diameter;
        }
        meanDiameter = sum / m_pupilDiameterHistory.size();
    }

    // 更新基础特征表格第22项 (索引21)
    tableModel->setItem(21, 0, new QStandardItem(QString::number(meanDiameter, 'f', 2)));

    // 2. 瞳孔相对偏差 (基础特征第23项) - (当前瞳孔直径 - 平均瞳孔直径) / 平均瞳孔直径
    float relativeDeviation = 0.0f;
    if (pupil.valid() && pupil.diameter() > 0 && meanDiameter > 0) {
        relativeDeviation = (pupil.diameter() - meanDiameter) / meanDiameter;
    }

    // 更新基础特征表格第23项 (索引22)
    tableModel->setItem(22, 0, new QStandardItem(QString::number(relativeDeviation, 'f', 2)));

    // 发送数据给graphplot
    emit extendedFeatureData(MEAN_PUPIL_DIAMETER, meanDiameter, QDateTime::currentMSecsSinceEpoch());
    emit extendedFeatureData(PUPIL_RELATIVE_DEVIATION, relativeDeviation, QDateTime::currentMSecsSinceEpoch());
}

void DataTable::updateBlinkDuration(const ProcessingResult& result) {
    // 2. 眨眼时长 (时序特征第8项)
    // 注意：眨眼时长的实际检测和记录现在由 handleStateTransition 统一处理
    // 这里只负责计算和显示平均眨眼时长

    // 计算平均眨眼时长
    float avgBlinkDuration = 0.0f;
    if (!m_blinkDurations.isEmpty()) {
        qint64 sum = 0;
        for (qint64 duration : m_blinkDurations) {
            sum += duration;
        }
        avgBlinkDuration = (float)sum / m_blinkDurations.size();
    } else {
        avgBlinkDuration = 0.0f; // 无数据时为0
    }

    // 更新时序特征表格第2项 (索引2) - 眨眼时长
    m_temporalModel->setItem(2, 0, new QStandardItem(QString::number(avgBlinkDuration, 'f', 1)));

    // 发送数据给graphplot
    emit extendedFeatureData(BLINK_DURATION, avgBlinkDuration, QDateTime::currentMSecsSinceEpoch());
}

void DataTable::updateMeanFixationDuration() {
    // 3. 平均注视时长 (时序特征第9项)
    float meanFixationDuration = 0.0f;

    if (!m_recentFixationDurations.isEmpty()) {
        float sum = 0.0f;
        for (float duration : m_recentFixationDurations) {
            sum += duration;
        }
        meanFixationDuration = sum / m_recentFixationDurations.size();
    } else {
        meanFixationDuration = 0.0f; // 无数据时为0
    }

    // 更新时序特征表格第3项 (索引3) - 平均注视时长
    m_temporalModel->setItem(3, 0, new QStandardItem(QString::number(meanFixationDuration, 'f', 1)));

    // 发送数据给graphplot
    emit extendedFeatureData(MEAN_FIXATION_DURATION, meanFixationDuration, QDateTime::currentMSecsSinceEpoch());
}

void DataTable::updateFixationRate(const ProcessingResult& result) {
    QDateTime currentTime = QDateTime::currentDateTime();
    bool isCurrentlyFixating = (m_currentState == STATE_FIXATION); // 【修复】使用权威状态机判断

    // 检测注视开始（状态从非注视变为注视）
    if (isCurrentlyFixating && !m_wasInFixation) {
        // 新的注视事件开始，添加到滑动窗口
        FixationEvent newEvent;
        newEvent.timestamp = currentTime;
        m_fixationEvents.append(newEvent);
    }

    // 更新上一帧状态
    m_wasInFixation = isCurrentlyFixating;

    // 移除超出滑动窗口的旧注视事件
    QDateTime windowStartTime = currentTime.addMSecs(-FIXATION_RATE_WINDOW_SIZE);
    while (!m_fixationEvents.isEmpty() &&
           m_fixationEvents.first().timestamp < windowStartTime) {
        m_fixationEvents.removeFirst();
    }

    // 计算滑动窗口内的注视率 (次/秒)
    float windowSizeSeconds = FIXATION_RATE_WINDOW_SIZE / 1000.0f; // 转换为秒
    int fixationCount = m_fixationEvents.size(); // 窗口内注视次数

    m_currentFixationRate = fixationCount / windowSizeSeconds;

    // 更新时序特征表格第6项 (索引5) - 注视率现在在时序特征中
    m_temporalModel->setItem(5, 0, new QStandardItem(QString::number(m_currentFixationRate, 'f', 1)));

    // 发送数据给graphplot
    emit extendedFeatureData(FIXATION_RATE, m_currentFixationRate, QDateTime::currentMSecsSinceEpoch());
}

// =================================================================
// =         扫视路径凸包面积计算实现                               =
// =================================================================

void DataTable::updateConvexHullArea(const ProcessingResult& result) {
    QDateTime currentTime = QDateTime::currentDateTime();

    // 只有视线有效时才添加点
    if (!result.gazeValid) {
        return;
    }

    // 添加当前视线坐标点到时间窗口
    cv::Point2f currentGazePoint = result.gazeScreenCoords;

    // 为点添加时间戳信息（使用辅助结构）
    // 【修复】移除静态局部变量和重复结构体定义，使用类成员变量

    // 添加新点
    GazePointWithTime newPoint;
    newPoint.point = currentGazePoint;
    newPoint.timestamp = currentTime;
    m_gazePointsWithTime.append(newPoint);

    // 移除超出时间窗口的点（500ms窗口）
    QDateTime windowStartTime = currentTime.addMSecs(-CONVEX_HULL_WINDOW_SIZE);
    while (!m_gazePointsWithTime.isEmpty() &&
           m_gazePointsWithTime.first().timestamp < windowStartTime) {
        m_gazePointsWithTime.removeFirst();
    }

    // 提取当前窗口内的点
    QList<cv::Point2f> currentWindowPoints;
    for (const auto& pointWithTime : m_gazePointsWithTime) {
        currentWindowPoints.append(pointWithTime.point);
    }

    // 计算凸包面积
    if (currentWindowPoints.size() >= 3) {
        m_currentConvexHullArea = calculateConvexHullArea(currentWindowPoints);
    } else {
        m_currentConvexHullArea = 0.0f; // 少于3个点无法构成面积
    }

    // 发送数据给graphplot
    emit extendedFeatureData(SCANPATH_CONVEX_HULL_AREA, m_currentConvexHullArea,
                            QDateTime::currentMSecsSinceEpoch());
}

float DataTable::calculateConvexHullArea(const QList<cv::Point2f>& points) {
    if (points.size() < 3) {
        return 0.0f;
    }

    // 转换为OpenCV格式
    std::vector<cv::Point2f> cvPoints;
    for (const cv::Point2f& point : points) {
        cvPoints.push_back(point);
    }

    // 使用OpenCV计算凸包
    std::vector<cv::Point2f> hull;
    cv::convexHull(cvPoints, hull, false);

    if (hull.size() < 3) {
        return 0.0f;
    }

    // 计算凸包面积（使用鞋带公式）
    float area = 0.0f;
    int n = hull.size();

    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        area += hull[i].x * hull[j].y;
        area -= hull[j].x * hull[i].y;
    }

    area = std::abs(area) / 2.0f;
    return area;
}

// =================================================================
// =         注视密度计算实现                                       =
// =================================================================

void DataTable::updateFixationDensity(const ProcessingResult& result) {
    QDateTime currentTime = QDateTime::currentDateTime();

    // 调试输出：检查注视检测条件
//    static int fixationDebugCounter = 0;
//    if (++fixationDebugCounter % 60 == 0) { // 每60帧输出一次
//        qDebug() << "注视密度调试 - gazeValid:" << result.gazeValid
//                 << "eyeMovementType:" << result.eyeMetrics.eyeMovementType
//                 << "(1为注视)";
//    }

    // 【修复】使用权威状态而非Worker瞬时分类
    // 只有在权威状态机确认的注视状态时才添加注视点
    if (m_currentState != STATE_FIXATION) {
        // 即使不添加点，也要进行密度计算更新
        m_currentFixationDensity = calculateFixationDensity(m_fixationPointsWindow);
        return;
    }

    // 添加当前注视点到时间窗口（优先使用视线坐标，无效时使用瞳孔坐标）
    cv::Point2f currentFixationPoint = result.gazeValid ? result.gazeScreenCoords : result.pupilData.center;

    // 使用类成员变量存储带时间戳的注视点数据

    // 添加新注视点
    FixationPointWithTime newPoint;
    newPoint.point = currentFixationPoint;
    newPoint.timestamp = currentTime;
    m_fixationPointsWithTime.append(newPoint);

    // 移除超出时间窗口的点（2秒窗口）
    QDateTime windowStartTime = currentTime.addMSecs(-FIXATION_DENSITY_WINDOW_SIZE);
    while (!m_fixationPointsWithTime.isEmpty() &&
           m_fixationPointsWithTime.first().timestamp < windowStartTime) {
        m_fixationPointsWithTime.removeFirst();
    }

    // 提取当前窗口内的注视点
    QList<cv::Point2f> currentWindowFixations;
    for (const auto& pointWithTime : m_fixationPointsWithTime) {
        currentWindowFixations.append(pointWithTime.point);
    }

    // 计算注视密度（使用改进的连续算法）
    float rawDensity = 0.0f;
    if (currentWindowFixations.size() >= 2) {
        rawDensity = calculateFixationDensity(currentWindowFixations);
    }

    // 指数移动平均平滑处理
    if (m_previousFixationDensity >= 0) {
        m_currentFixationDensity = DENSITY_SMOOTHING_ALPHA * rawDensity +
                                  (1.0f - DENSITY_SMOOTHING_ALPHA) * m_previousFixationDensity;
    } else {
        m_currentFixationDensity = rawDensity;
    }

    m_previousFixationDensity = m_currentFixationDensity;

//    if (fixationDebugCounter % 60 == 0) { // 继续使用同一个调试计数器
//        qDebug() << "注视点数量:" << currentWindowFixations.size()
//                 << "原始密度:" << rawDensity
//                 << "平滑密度:" << m_currentFixationDensity;
//    }

    // 发送数据给graphplot
    emit extendedFeatureData(FIXATION_DENSITY, m_currentFixationDensity,
                            QDateTime::currentMSecsSinceEpoch());
}

float DataTable::calculateFixationDensity(const QList<cv::Point2f>& fixationPoints) {
    if (fixationPoints.size() < 2) {
        return 0.0f;
    }

    // 改进的注视密度计算，考虑正常眼动漂移

    // 步骤1：计算注视点的几何中心（质心）
    cv::Point2f centroid(0.0f, 0.0f);
    for (const cv::Point2f& point : fixationPoints) {
        centroid.x += point.x;
        centroid.y += point.y;
    }
    centroid.x /= fixationPoints.size();
    centroid.y /= fixationPoints.size();

    // 步骤2：计算每个注视点到质心的距离
    QList<float> distances;
    for (const cv::Point2f& point : fixationPoints) {
        float dx = point.x - centroid.x;
        float dy = point.y - centroid.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        distances.append(distance);
    }

    // 步骤3：计算RMS距离（均方根距离，更稳定）
    float sumSquaredDistances = 0.0f;
    for (float distance : distances) {
        sumSquaredDistances += distance * distance;
    }
    float rmsDistance = std::sqrt(sumSquaredDistances / distances.size());

    // 步骤4：使用改进的密度计算
    // 正常注视漂移约1-3像素，设置合理的基准距离
    const float baselineDistance = 5.0f; // 基准距离，低于此值认为是高密度注视
    const float epsilon = 1.0f; // 增大epsilon，避免极值

    // 使用sigmoid函数映射，提供更平滑的密度变化
    float normalizedDistance = rmsDistance / baselineDistance;
    float density = 1.0f / (1.0f + normalizedDistance);

    // 根据样本数量进行置信度加权
    float sampleConfidence = std::min(1.0f, (float)fixationPoints.size() / 10.0f); // 10个样本达到满置信度
    density *= sampleConfidence;

    // 添加调试输出
//    static int densityDebugCounter = 0;
//    if (++densityDebugCounter % 60 == 0) {
//        qDebug() << "密度计算 - 点数:" << fixationPoints.size()
//                 << "RMS距离:" << rmsDistance
//                 << "归一化距离:" << normalizedDistance
//                 << "置信度:" << sampleConfidence
//                 << "最终密度:" << density;
//    }

    return density;
}


// =================================================================
// =         扫视熵计算实现                                         =
// =================================================================

void DataTable::updateSaccadeEntropy(const ProcessingResult& result) {
    Q_UNUSED(result); // 不再需要result参数，只从历史数据计算

    // 使用指数衰减权重和置信度加权计算连续扫视熵
//    static int entropyDebugCounter = 0;
//    if (++entropyDebugCounter % 180 == 0) { // 每3秒输出一次调试信息
//        qDebug() << "【扫视熵调试】方向历史数量:" << m_saccadeDirectionHistory.size()
//                 << "最小需要:" << MIN_SACCADES_FOR_ENTROPY
//                 << "当前熵值:" << m_currentSaccadeEntropy;
//    }

    // 计算带权重的扫视熵
    float rawEntropy = calculateContinuousSaccadeEntropy(m_saccadeDirectionHistory);

    // 指数移动平均平滑处理
    if (m_previousSaccadeEntropy >= 0) {
        m_currentSaccadeEntropy = ENTROPY_SMOOTHING_ALPHA * rawEntropy +
                                 (1.0f - ENTROPY_SMOOTHING_ALPHA) * m_previousSaccadeEntropy;
    } else {
        m_currentSaccadeEntropy = rawEntropy;
    }

    m_previousSaccadeEntropy = m_currentSaccadeEntropy;

    // 发送数据给graphplot
    emit extendedFeatureData(SACCADE_ENTROPY, m_currentSaccadeEntropy,
                            QDateTime::currentMSecsSinceEpoch());
}

float DataTable::calculateSaccadeAngle(const cv::Point2f& startPos, const cv::Point2f& endPos) {
    float dx = endPos.x - startPos.x;
    float dy = endPos.y - startPos.y;

    // 计算角度（弧度转度）
    float angle = std::atan2(dy, dx) * 180.0f / M_PI;

    // 确保角度在0-360度范围内
    if (angle < 0) {
        angle += 360.0f;
    }

    return angle;
}

float DataTable::calculateContinuousSaccadeEntropy(const QList<SaccadeDirectionData>& directionData) {
    if (directionData.isEmpty()) {
        return 0.0f;
    }

    QDateTime currentTime = QDateTime::currentDateTime();
    float totalWeight = 0.0f;
    QVector<int> directionCounts(8, 0);
    QVector<float> directionWeights(8, 0.0f);

    // 使用指数衰减权重计算方向分布
    for (const SaccadeDirectionData& data : directionData) {
        // 计算时间权重（指数衰减）
        float ageInSeconds = data.timestamp.msecsTo(currentTime) / 1000.0f;
        float timeWeight = std::exp(-ageInSeconds / ENTROPY_TIME_CONSTANT);

        if (timeWeight < 0.01f) continue; // 忽略权重过小的数据

        // 计算属于哪个方向区间
        float normalizedAngle = data.angle;
        while (normalizedAngle < 0) normalizedAngle += 360.0f;
        while (normalizedAngle >= 360) normalizedAngle -= 360.0f;

        int bin = (int)((normalizedAngle + 22.5f) / 45.0f) % 8;
        directionWeights[bin] += timeWeight;
        totalWeight += timeWeight;
    }

    // 降低权重阈值，避免过度清零
    if (totalWeight < 0.01f) {
        return 0.0f;
    }

    // 计算加权熵 H = -∑(pi × log2(pi))
    float entropy = 0.0f;
    int nonZeroBins = 0;
    for (float weight : directionWeights) {
        if (weight > 0) {
            float p = weight / totalWeight;
            entropy -= p * std::log2(p);
            nonZeroBins++;
        }
    }

    // 修复置信度计算：基于实际扫视数量而非权重总和
    int actualSaccadeCount = 0;
    QDateTime currentTime2 = QDateTime::currentDateTime();
    for (const SaccadeDirectionData& data : directionData) {
        float ageInSeconds = data.timestamp.msecsTo(currentTime2) / 1000.0f;
        if (ageInSeconds <= ENTROPY_TIME_CONSTANT * 3) { // 在有效时间范围内
            actualSaccadeCount++;
        }
    }

    float sampleConfidence = std::min(1.0f, (float)actualSaccadeCount / MIN_SACCADES_FOR_ENTROPY);
    float distributionConfidence = std::min(1.0f, (float)nonZeroBins / 2.0f); // 至少2个方向

    // 使用更温和的置信度加权，避免完全归零
    float confidence = std::max(0.2f, sampleConfidence * distributionConfidence);

    return entropy * confidence; // 置信度加权的熵值
}

float DataTable::calculateSaccadeDirectionEntropy(const QList<SaccadeDirectionData>& directionData) {
    if (directionData.size() < MIN_SACCADES_FOR_ENTROPY) {
        return 0.0f;
    }

    // 将360度分为8个方向区间（每45度一个）
    QVector<int> directionCounts(8, 0);

    for (const SaccadeDirectionData& data : directionData) {
        // 将角度标准化到0-360度范围
        float normalizedAngle = data.angle;
        while (normalizedAngle < 0) normalizedAngle += 360.0f;
        while (normalizedAngle >= 360) normalizedAngle -= 360.0f;

        // 计算属于哪个方向区间
        int bin = (int)((normalizedAngle + 22.5f) / 45.0f) % 8;
        directionCounts[bin]++;
    }

    // 计算熵 H = -∑(pi × log2(pi))
    float entropy = 0.0f;
    int totalSaccades = directionData.size();

    for (int count : directionCounts) {
        if (count > 0) {
            float p = (float)count / totalSaccades;
            entropy -= p * std::log2(p);
        }
    }

    return entropy; // 范围：0-3 bits，越高越随机
}


// =================================================================
// =         主序关系计算实现                                       =
// =================================================================

void DataTable::updateMainSequenceRelationship(const ProcessingResult& result) {
    Q_UNUSED(result); // 不再需要result参数，只从历史数据计算

    // 使用指数衰减权重和置信度加权计算连续主序关系斜率
    float rawSlope = calculateContinuousMainSequenceSlope(m_saccadeMetricsHistory);

    // 指数移动平均平滑处理
    if (m_previousMainSequenceSlope >= 0) {
        m_currentMainSequenceSlope = MAIN_SEQUENCE_SMOOTHING_ALPHA * rawSlope +
                                    (1.0f - MAIN_SEQUENCE_SMOOTHING_ALPHA) * m_previousMainSequenceSlope;
    } else {
        m_currentMainSequenceSlope = rawSlope;
    }

    m_previousMainSequenceSlope = m_currentMainSequenceSlope;

    // 发送数据给graphplot
    emit extendedFeatureData(MAIN_SEQUENCE_SLOPE, m_currentMainSequenceSlope,
                            QDateTime::currentMSecsSinceEpoch());
}

float DataTable::calculateContinuousMainSequenceSlope(const QList<SaccadeMetrics>& metricsData) {
    if (metricsData.isEmpty()) {
        return 0.0f;
    }

    QDateTime currentTime = QDateTime::currentDateTime();
    QList<float> weightedAmplitudes;
    QList<float> weightedVelocities;
    QList<float> weights;
    float totalWeight = 0.0f;

    // 使用指数衰减权重收集有效数据
    for (const SaccadeMetrics& metrics : metricsData) {
        if (metrics.amplitude <= 0 || metrics.peakVelocity <= 0) {
            continue; // 跳过无效数据
        }

        // 计算时间权重（指数衰减）
        float ageInSeconds = metrics.timestamp.msecsTo(currentTime) / 1000.0f;
        float timeWeight = std::exp(-ageInSeconds / MAIN_SEQUENCE_TIME_CONSTANT);

        if (timeWeight < 0.01f) continue; // 忽略权重过小的数据

        weightedAmplitudes.append(metrics.amplitude);
        weightedVelocities.append(metrics.peakVelocity);
        weights.append(timeWeight);
        totalWeight += timeWeight;
    }

    if (weightedAmplitudes.size() < MIN_SACCADES_FOR_REGRESSION || totalWeight < 0.1f) {
        return 0.0f;
    }

    // 计算加权线性回归：PeakVelocity = slope × Amplitude + intercept
    float slope = 0.0f, intercept = 0.0f;
    float correlation = calculateWeightedLinearRegression(weightedAmplitudes, weightedVelocities, weights, slope, intercept);

    // 根据有效样本数和相关性计算置信度
    float effectiveSamples = totalWeight;
    float sampleConfidence = std::min(1.0f, effectiveSamples / MIN_SACCADES_FOR_REGRESSION);
    float correlationConfidence = std::max(0.0f, std::abs(correlation)); // 相关系数的绝对值作为置信度
    float overallConfidence = sampleConfidence * correlationConfidence;

    return slope * overallConfidence; // 置信度加权的斜率
}

float DataTable::calculateMainSequenceSlope(const QList<SaccadeMetrics>& metricsData) {
    if (metricsData.size() < MIN_SACCADES_FOR_REGRESSION) {
        return 0.0f;
    }

    // 提取幅度和速度峰值数据
    QList<float> amplitudes;
    QList<float> peakVelocities;

    for (const SaccadeMetrics& metrics : metricsData) {
        if (metrics.amplitude > 0 && metrics.peakVelocity > 0) { // 只使用有效数据
            amplitudes.append(metrics.amplitude);
            peakVelocities.append(metrics.peakVelocity);
        }
    }

    if (amplitudes.size() < MIN_SACCADES_FOR_REGRESSION) {
        return 0.0f;
    }

    // 计算线性回归：PeakVelocity = slope × Amplitude + intercept
    float slope = 0.0f, intercept = 0.0f;
    float correlation = linearRegression(amplitudes, peakVelocities, slope, intercept);

    // 返回斜率（正常值约为37°/s/°，这里是像素单位）
    return slope;
}

float DataTable::linearRegression(const QList<float>& x, const QList<float>& y,
                                 float& slope, float& intercept) {
    if (x.size() != y.size() || x.size() < 2) {
        slope = 0.0f;
        intercept = 0.0f;
        return 0.0f;
    }

    int n = x.size();
    float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f, sumY2 = 0.0f;

    for (int i = 0; i < n; i++) {
        sumX += x[i];
        sumY += y[i];
        sumXY += x[i] * y[i];
        sumX2 += x[i] * x[i];
        sumY2 += y[i] * y[i];
    }

    float meanX = sumX / n;
    float meanY = sumY / n;

    // 计算斜率和截距
    float denominator = sumX2 - n * meanX * meanX;
    if (std::abs(denominator) < 1e-6) {
        slope = 0.0f;
        intercept = meanY;
        return 0.0f;
    }

    slope = (sumXY - n * meanX * meanY) / denominator;
    intercept = meanY - slope * meanX;

    // 计算相关系数
    float numerator = n * sumXY - sumX * sumY;
    float denomX = n * sumX2 - sumX * sumX;
    float denomY = n * sumY2 - sumY * sumY;

    float correlation = 0.0f;
    if (denomX > 0 && denomY > 0) {
        correlation = numerator / std::sqrt(denomX * denomY);
    }

    return correlation;
}

float DataTable::calculateWeightedLinearRegression(const QList<float>& x, const QList<float>& y,
                                                   const QList<float>& weights, float& slope, float& intercept) {
    if (x.size() != y.size() || x.size() != weights.size() || x.size() < 2) {
        slope = 0.0f;
        intercept = 0.0f;
        return 0.0f;
    }

    int n = x.size();
    float sumW = 0.0f, sumWX = 0.0f, sumWY = 0.0f, sumWXY = 0.0f, sumWX2 = 0.0f, sumWY2 = 0.0f;

    // 计算加权统计量
    for (int i = 0; i < n; i++) {
        float w = weights[i];
        sumW += w;
        sumWX += w * x[i];
        sumWY += w * y[i];
        sumWXY += w * x[i] * y[i];
        sumWX2 += w * x[i] * x[i];
        sumWY2 += w * y[i] * y[i];
    }

    if (sumW < 1e-6) {
        slope = 0.0f;
        intercept = 0.0f;
        return 0.0f;
    }

    float meanWX = sumWX / sumW;
    float meanWY = sumWY / sumW;

    // 计算加权斜率和截距
    float denominator = sumWX2 - sumW * meanWX * meanWX;
    if (std::abs(denominator) < 1e-6) {
        slope = 0.0f;
        intercept = meanWY;
        return 0.0f;
    }

    slope = (sumWXY - sumW * meanWX * meanWY) / denominator;
    intercept = meanWY - slope * meanWX;

    // 计算加权相关系数
    float numerator = sumWXY - (sumWX * sumWY / sumW);
    float denomX = sumWX2 - (sumWX * sumWX / sumW);
    float denomY = sumWY2 - (sumWY * sumWY / sumW);

    float correlation = 0.0f;
    if (denomX > 0 && denomY > 0) {
        correlation = numerator / std::sqrt(denomX * denomY);
    }

    return correlation;
}

// =================================================================
// =         扫视速度峰值计算实现                                   =
// =================================================================

void DataTable::updateSaccadePeakVelocity(const ProcessingResult& result) {
    Q_UNUSED(result); // 不再需要result参数，只从历史数据计算

    // 仅负责从历史数据计算平均扫视速度峰值（数据管理已在状态机中处理）
//    static int peakVelDebugCounter = 0;
//    if (++peakVelDebugCounter % 180 == 0) { // 每3秒输出一次调试信息
//        qDebug() << "【扫视速度峰值调试】历史数量:" << m_saccadePeakVelocityHistory.size()
//                 << "当前峰值:" << m_currentSaccadePeakVelocity;
//    }

    if (!m_saccadePeakVelocityHistory.isEmpty()) {
        float sum = 0.0f;
        for (float velocity : m_saccadePeakVelocityHistory) {
            sum += velocity;
        }
        m_currentSaccadePeakVelocity = sum / m_saccadePeakVelocityHistory.size();
    } else {
        m_currentSaccadePeakVelocity = 0.0f;
    }

    // 发送数据给graphplot
    emit extendedFeatureData(SACCADE_PEAK_VELOCITY, m_currentSaccadePeakVelocity,
                            QDateTime::currentMSecsSinceEpoch());
}

float DataTable::calculatePeakVelocityFromTrajectory(const QList<cv::Point2f>& trajectory,
                                                    const QList<qint64>& timestamps) {
    if (trajectory.size() < 2 || trajectory.size() != timestamps.size()) {
        return 0.0f;
    }

    float maxVelocity = 0.0f;

    // 对于只有2个点的短轨迹，使用简单的平均速度
    if (trajectory.size() == 2) {
        float dt = (timestamps[1] - timestamps[0]) / 1000.0f;
        if (dt > 0) {
            float dx = trajectory[1].x - trajectory[0].x;
            float dy = trajectory[1].y - trajectory[0].y;
            return std::sqrt(dx*dx + dy*dy) / dt;
        }
        return 0.0f;
    }

    // 使用中心差分法计算瞬时速度
    for (int i = 1; i < trajectory.size() - 1; i++) {
        float dt = (timestamps[i+1] - timestamps[i-1]) / 1000.0f; // 转换为秒

        if (dt <= 0) continue; // 避免除零

        float dx = trajectory[i+1].x - trajectory[i-1].x;
        float dy = trajectory[i+1].y - trajectory[i-1].y;

        float velocity = std::sqrt(dx*dx + dy*dy) / dt; // 像素/秒

        if (velocity > maxVelocity) {
            maxVelocity = velocity;
        }
    }

    // 【删除副作用】不在计算函数中修改历史记录
    // 历史记录的管理现在由状态机统一处理

    return maxVelocity;
}

void DataTable::clearDataBuffer() {
    // 【修复】使用非阻塞的确认对话框，避免UI冻结
    auto* msgBox = new QMessageBox(QMessageBox::Question,
                                  "确认清空数据",
                                  QString("确定要清空所有缓存的实时数据吗？\n\n当前共有 %1 条记录。\n\n清空后无法恢复，请确认已导出重要数据。").arg(m_dataBuffer.size()),
                                  QMessageBox::Yes | QMessageBox::No,
                                  this);
    msgBox->setDefaultButton(QMessageBox::No);
    msgBox->setModal(false);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);

    connect(msgBox, &QMessageBox::finished, this, [this](int result) {
        if (result == QMessageBox::Yes) {
            int removedCount = m_dataBuffer.size();
            m_dataBuffer.clear();

            // 【新增】清理所有历史数据和状态，解决状态持久化问题
            clearAllHistoricalData();

            // 更新状态显示
            m_statusLabel->setText(QString("✅ 已清空 %1 条缓存记录").arg(removedCount));

            // 非阻塞成功提示
            QTimer::singleShot(200, this, [this, removedCount]() {
                auto* successMsg = new QMessageBox(QMessageBox::Information,
                                                  "清空完成",
                                                  QString("已成功清空 %1 条缓存记录和所有历史数据。\n\n现在可以开始新的实验记录。").arg(removedCount),
                                                  QMessageBox::Ok, this);
                successMsg->setModal(false);
                successMsg->setAttribute(Qt::WA_DeleteOnClose);
                successMsg->show();
            });

            qDebug() << "【数据缓存管理】用户手动清空数据缓存和历史数据，已删除" << removedCount << "条记录";
        }
    });

    msgBox->show();
}

// 【新增】清理所有历史数据和状态的函数
void DataTable::clearAllHistoricalData() {
    // 清理扫视路径凸包面积的历史数据
    m_gazePointsWithTime.clear();
    m_currentConvexHullArea = 0.0f;

    // 清理注视密度相关数据
    m_fixationPointsWithTime.clear();
    m_fixationPointsWindow.clear();
    m_currentFixationDensity = 0.0f;
    m_previousFixationDensity = -1.0f;

    // 清理扫视熵相关数据
    m_saccadeDirectionHistory.clear();
    m_currentSaccadeEntropy = 0.0f;
    m_previousSaccadeEntropy = -1.0f;

    // 清理主序关系相关数据
    m_saccadeMetricsHistory.clear();
    m_currentMainSequenceSlope = 0.0f;
    m_previousMainSequenceSlope = -1.0f;

    // 清理扫视速度峰值相关数据
    m_saccadePeakVelocityHistory.clear();
    m_currentSaccadePeakVelocity = 0.0f;
    m_currentSaccadeTrajectory.clear();
    m_currentSaccadeTimestamps.clear();

    // 清理扫视幅度相关数据
    m_recentSaccadeAmplitudes.clear();
    m_saccadeStartPosition = cv::Point2f(-1, -1);
    // 注意：不重置m_currentSaccadeAmplitude，保持当前计算出的值

    // 清理其他特征的历史数据
    m_pupilDiameterHistory.clear();
    m_blinkDurations.clear();
    m_recentFixationDurations.clear();
    m_recentSaccadeDurations.clear();
    m_fixationEvents.clear();

    // 重置状态机
    m_currentState = STATE_UNKNOWN;
    m_stateStartTime = QDateTime::currentDateTime();
    m_isInSaccade = false;
    m_wasInFixation = false;

    // 清理视线和瞳孔历史
    m_gazeHistory.clear();
    m_pupilHistory.clear();

    // 重置计数器
    m_blinkCount = 0;
    m_lastBlinkResetTime = QDateTime::currentDateTime();
    m_currentFixationRate = 0.0f;

    qDebug() << "✅ [DataTable] 所有历史数据和状态已清理，准备开始新的实验";
}

// 【新增】DataTable的新方法实现
void DataTable::updateUIForTask(const QString& taskName) {
    TaskUIManager::TaskUIState state = TaskUIManager::getUIStateForTask(taskName);

    // 统一更新所有相关UI组件
    m_paradigmSettingsButton->setEnabled(state.paradigmSettingsEnabled);
    m_paradigmSettingsButton->setToolTip(state.settingsTooltip);
    m_experimentStatusLabel->setText(state.statusText);

    qDebug() << QString("任务UI状态已更新: %1 -> %2").arg(taskName).arg(state.statusText);
}

void DataTable::onParadigmSettingsClicked() {
    // 显示实验参数设置对话框
    ExperimentSettingsDialog dialog(this);
    dialog.setSettings(m_expSettings);

    if (dialog.exec() == QDialog::Accepted) {
        m_expSettings = dialog.getSettings();
        m_targetSelectionCount = m_expSettings.selectionTargetCount;

        // 更新UI显示当前配置
        m_experimentStatusLabel->setText(QString("已配置: %1轮次, 浏览%2秒, 选择%3秒, 目标%4次")
                                        .arg(m_expSettings.roundCount)
                                        .arg(m_expSettings.browseTimeSeconds)
                                        .arg(m_expSettings.selectionTimeSeconds)
                                        .arg(m_expSettings.selectionTargetCount));

        qDebug() << "实验参数已配置:" << m_expSettings.roundCount << "轮次";
    }
}

// updateTrialProgress 函数已删除（范式相关）

QString DataTable::getFeatureValue(const QString& featureName) const {
    // 使用数据驱动而不是UI驱动
    return m_featureCache.getFeatureValue(featureName);
}

// setIntentionFromParadigm 函数已删除（范式相关）

// === 新增：实验参数设置对话框实现 ===
ExperimentSettingsDialog::ExperimentSettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("实验参数设置");
    setModal(true);
    resize(400, 300);

    auto* layout = new QFormLayout(this);

    m_roundCountSpinBox = new QSpinBox();
    m_roundCountSpinBox->setRange(1, 100);
    m_roundCountSpinBox->setValue(m_settings.roundCount);
    layout->addRow("实验轮数:", m_roundCountSpinBox);

    m_browseTimeSpinBox = new QSpinBox();
    m_browseTimeSpinBox->setRange(5, 300);
    m_browseTimeSpinBox->setSuffix(" 秒");
    m_browseTimeSpinBox->setValue(m_settings.browseTimeSeconds);
    layout->addRow("浏览时间:", m_browseTimeSpinBox);

    m_selectionTimeSpinBox = new QSpinBox();
    m_selectionTimeSpinBox->setRange(10, 600);
    m_selectionTimeSpinBox->setSuffix(" 秒");
    m_selectionTimeSpinBox->setValue(m_settings.selectionTimeSeconds);
    layout->addRow("选择时间:", m_selectionTimeSpinBox);

    m_selectionTargetSpinBox = new QSpinBox();
    m_selectionTargetSpinBox->setRange(1, 25);
    m_selectionTargetSpinBox->setValue(m_settings.selectionTargetCount);
    layout->addRow("目标选择次数:", m_selectionTargetSpinBox);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addRow(buttonBox);
}

ExperimentSettingsDialog::ExperimentSettings ExperimentSettingsDialog::getSettings() const {
    ExperimentSettings settings;
    settings.roundCount = m_roundCountSpinBox->value();
    settings.browseTimeSeconds = m_browseTimeSpinBox->value();
    settings.selectionTimeSeconds = m_selectionTimeSpinBox->value();
    settings.selectionTargetCount = m_selectionTargetSpinBox->value();
    return settings;
}

void ExperimentSettingsDialog::setSettings(const ExperimentSettings& settings) {
    m_settings = settings;
    if (m_roundCountSpinBox) m_roundCountSpinBox->setValue(settings.roundCount);
    if (m_browseTimeSpinBox) m_browseTimeSpinBox->setValue(settings.browseTimeSeconds);
    if (m_selectionTimeSpinBox) m_selectionTimeSpinBox->setValue(settings.selectionTimeSeconds);
    if (m_selectionTargetSpinBox) m_selectionTargetSpinBox->setValue(settings.selectionTargetCount);
}

// === 新增：实验流程控制实现 ===
void DataTable::startAutomatedExperiment() {
    if (m_expSettings.roundCount <= 0) {
        // 【修复】使用非阻塞警告提示
        m_experimentStatusLabel->setText("⚠️ 警告：请先配置实验参数！");
        QTimer::singleShot(100, this, [this]() {
            auto* msgBox = new QMessageBox(QMessageBox::Warning,
                                          "警告",
                                          "请先配置实验参数！",
                                          QMessageBox::Ok, this);
            msgBox->setModal(false);
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->show();
        });
        return;
    }

    // 【修复】加载任务列表，确保"当前任务"能正确显示
    loadTasksForExperiment(m_currentExperimentType);

    // 设置常规实验状态（与现有startExperiment保持一致）
    m_isExperimentRunning = true;
    m_isRecordingData = true;
    m_expStartTime = QDateTime::currentDateTime();
    m_currentTaskIndex = 0; // 【修复】设置任务索引

    // 更新UI状态
    m_startExperimentButton->setEnabled(false);
    m_pauseExperimentButton->setEnabled(true);
    m_stopExperimentButton->setEnabled(true);
    m_nextTaskButton->setEnabled(false);
    m_experimentTypeComboBox->setEnabled(false);
    m_paradigmSettingsButton->setEnabled(false); // 实验进行中禁用参数设置

    // 【修复】启动实验计时器，确保"用时"能正确更新
    m_experimentTimer->start(1000);

    // 【修复】更新当前任务标签显示
    if (m_currentTaskIndex >= 0 && m_currentTaskIndex < m_taskList.size()) {
        m_currentTaskLabel->setText(QString("当前任务: %1").arg(m_taskList[m_currentTaskIndex]));
    } else {
        m_currentTaskLabel->setText("当前任务: 交互意图识别实验");
    }

    // 初始化自动实验状态
    m_currentPhase = PHASE_IDLE;
    m_currentRound = 0;
    m_totalRounds = m_expSettings.roundCount;
    m_selectionCount = 0;

    // 开始实验，进入第一轮的浏览阶段
    m_currentRound = 1;
    switchToBrowsePhase();

    qDebug() << "自动实验开始:" << m_totalRounds << "轮次";
}

void DataTable::switchToBrowsePhase() {
    m_currentPhase = PHASE_BROWSE;

    // 停止可能正在运行的定时器
    m_phaseTimer->stop();
    m_selectionMonitorTimer->stop();

    // 设置标签为浏览
    m_intentionComboBox->blockSignals(true);
    int browseIndex = m_intentionComboBox->findText("浏览");
    if (browseIndex >= 0) {
        m_intentionComboBox->setCurrentIndex(browseIndex);
        m_currentIntention = browseIndex; // 同步更新内部值
    }
    m_intentionComboBox->blockSignals(false);

    qDebug() << "标签已设置为浏览，m_currentIntention=" << m_currentIntention;

    // 隐藏交互界面
    emit requestHideInteractionDialog();

    // 更新状态显示
    m_experimentStatusLabel->setText(QString("第%1/%2轮 - 浏览阶段 (%3秒)")
                                    .arg(m_currentRound)
                                    .arg(m_totalRounds)
                                    .arg(m_expSettings.browseTimeSeconds));

    // 启动浏览阶段定时器（定时器已在构造函数中设置为单次触发）
    m_phaseTimer->start(m_expSettings.browseTimeSeconds * 1000);

    qDebug() << QString("进入浏览阶段: 第%1/%2轮, %3秒")
                .arg(m_currentRound).arg(m_totalRounds).arg(m_expSettings.browseTimeSeconds);
}

void DataTable::switchToSelectionPhase() {
    qDebug() << QString("=== 切换到选择阶段：第%1轮 ===").arg(m_currentRound);
    qDebug() << "切换前状态检查 - 当前阶段:" << m_currentPhase << ", 实验运行:" << m_isExperimentRunning;

    m_currentPhase = PHASE_SELECTION;

    // 停止可能正在运行的定时器
    m_phaseTimer->stop();
    m_selectionMonitorTimer->stop();

    qDebug() << "定时器已停止，准备重新启动";

    // 设置标签为选择
    m_intentionComboBox->blockSignals(true);
    int selectionIndex = m_intentionComboBox->findText("选择");
    if (selectionIndex >= 0) {
        m_intentionComboBox->setCurrentIndex(selectionIndex);
        m_currentIntention = selectionIndex; // 同步更新内部值
    }
    m_intentionComboBox->blockSignals(false);

    qDebug() << "标签已设置为选择，m_currentIntention=" << m_currentIntention;

    // 【修复】异步显示交互界面，避免阻塞
    QTimer::singleShot(0, this, [this]() {
        emit requestShowInteractionDialog();
    });

    // 更新状态显示
    m_experimentStatusLabel->setText(QString("第%1/%2轮 - 选择阶段 (0/%3选择)")
                                    .arg(m_currentRound)
                                    .arg(m_totalRounds)
                                    .arg(m_targetSelectionCount));

    // 启动选择阶段定时器（作为超时保护）
    m_phaseTimer->start(m_expSettings.selectionTimeSeconds * 1000);
    qDebug() << "选择阶段主定时器已启动，超时时间:" << m_expSettings.selectionTimeSeconds << "秒";

    // 启动选择监控定时器
    m_selectionMonitorTimer->start();
    qDebug() << "选择监控定时器已启动，检查间隔:" << m_selectionMonitorTimer->interval() << "ms";

    // 重置选择计数和监控状态
    m_selectionCount = 0;
    m_lastSelectionCount = 0;
    m_selectionStartTime = QDateTime::currentDateTime();

    qDebug() << QString("进入选择阶段: 第%1/%2轮, 目标%3次选择")
                .arg(m_currentRound).arg(m_totalRounds).arg(m_targetSelectionCount);
}

void DataTable::onPhaseTimeout() {
    // 【修复】防止重复触发导致状态异常
    if (!m_isExperimentRunning) {
        qDebug() << "【定时器防护】实验已停止，忽略阶段超时事件";
        return;
    }

    // 暂时停止定时器防止重入
    m_phaseTimer->stop();

    switch (m_currentPhase) {
        case PHASE_BROWSE:
            // 浏览时间到，切换到选择阶段
            qDebug() << "【阶段超时】浏览阶段结束，切换到选择阶段";
            switchToSelectionPhase();
            break;

        case PHASE_SELECTION:
            // 选择时间到但未完成，强制进入下一阶段
            qDebug() << "【阶段超时】选择阶段超时，当前选择数:" << m_selectionCount;
            onSelectionCompleted();
            break;

        case PHASE_IDLE:
        case PHASE_COMPLETED:
            qDebug() << "【定时器防护】在空闲/完成状态下收到超时信号，忽略";
            break;

        default:
            qDebug() << "【定时器防护】未知阶段" << m_currentPhase << "，忽略超时事件";
            break;
    }
}

void DataTable::onSelectionCompleted() {
    if (m_currentPhase != PHASE_SELECTION) {
        return;
    }

    qDebug() << QString("=== 第%1轮选择阶段完成 ===").arg(m_currentRound);

    // 停止所有定时器
    m_phaseTimer->stop();
    m_selectionMonitorTimer->stop();

    // 检查是否完成所有轮次
    if (m_currentRound >= m_totalRounds) {
        // 最后一轮完成，结束实验
        finishExperiment();
    } else {
        // 【根本修复】轮次间完整状态重置
        performRoundStateReset();

        // 进入下一轮的浏览阶段
        m_currentRound++;
        switchToBrowsePhase();
    }
}

// 【根本修复】轮次间完整状态重置 - 解决累积性状态泄漏问题
void DataTable::performRoundStateReset() {
    qDebug() << QString("执行第%1轮结束后的状态重置").arg(m_currentRound);

    // 1. 强制断开并重建定时器连接，解决信号槽累积问题
    disconnect(m_phaseTimer, nullptr, this, nullptr);
    disconnect(m_selectionMonitorTimer, nullptr, this, nullptr);

    // 重新连接信号（确保连接状态干净）
    connect(m_phaseTimer, &QTimer::timeout, this, &DataTable::onPhaseTimeout, Qt::UniqueConnection);
    connect(m_selectionMonitorTimer, &QTimer::timeout, this, &DataTable::checkSelectionProgress, Qt::UniqueConnection);

    // 2. 重置所有选择相关状态变量
    m_selectionCount = 0;
    m_lastSelectionCount = 0;
    m_selectionStartTime = QDateTime::currentDateTime();

    // 3. 清理数据缓存中可能的累积（防止内存无限增长）
    if (m_dataBuffer.size() > 50000) { // 如果缓存过大，清理一半旧数据
        int removeCount = m_dataBuffer.size() / 2;
        for (int i = 0; i < removeCount; ++i) {
            m_dataBuffer.removeFirst();
        }
        qDebug() << "清理了" << removeCount << "条旧数据缓存，当前缓存大小:" << m_dataBuffer.size();
    }

    // 4. 重置历史数据数组，防止特征计算状态累积
    m_gazeHistory.clear();
    m_pupilHistory.clear();
    m_gazePointsWithTime.clear();
    m_fixationPointsWithTime.clear();
    m_saccadeDirectionHistory.clear();
    m_saccadeMetricsHistory.clear();
    m_fixationEvents.clear();

    // 5. 重置状态机相关状态
    m_currentState = STATE_UNKNOWN;
    m_stateStartTime = QDateTime::currentDateTime();
    m_rawState = STATE_UNKNOWN;
    m_rawStateStartTime = QDateTime::currentDateTime();

    // 6. 重置计数器和累积变量
    m_blinkCount = 0;
    m_lastBlinkResetTime = QDateTime::currentDateTime();
    // 注意：不重置m_currentSaccadeAmplitude，保持当前计算出的值
    m_currentConvexHullArea = 0.0f;
    m_currentFixationDensity = 0.0f;
    m_currentSaccadeEntropy = 0.0f;
    m_currentMainSequenceSlope = 0.0f;
    m_currentSaccadePeakVelocity = 0.0f;
    m_currentFixationRate = 0.0f;

    // 7. 强制垃圾回收和内存整理
    m_currentSaccadeTrajectory.clear();
    m_currentSaccadeTimestamps.clear();
    m_gazePointsWindow.clear();
    m_fixationPointsWindow.clear();

    // 8. 重置UI相关状态（防止界面状态累积）
    m_intentionComboBox->blockSignals(true);
    m_intentionComboBox->setCurrentIndex(0); // 重置为默认状态
    m_intentionComboBox->blockSignals(false);

    // 9. 确保定时器内部状态完全重置
    if (m_phaseTimer->isActive()) {
        m_phaseTimer->stop();
    }
    if (m_selectionMonitorTimer->isActive()) {
        m_selectionMonitorTimer->stop();
    }

    // 10. 强制内存压缩（对于长时间运行的实验）
    static int resetCount = 0;
    resetCount++;
    if (resetCount % 5 == 0) { // 每5轮强制进行一次深度清理
        qDebug() << "执行深度内存清理（第" << resetCount << "次状态重置）";
        // 强制清理所有历史数据的备份
        m_pupilDiameterHistory.clear();
        m_blinkDurations.clear();
        m_saccadePeakVelocityHistory.clear();
        m_recentFixationDurations.clear();
        m_recentSaccadeDurations.clear();
        m_recentSaccadeAmplitudes.clear();
    }

    qDebug() << "轮次状态重置完成，系统状态已清理";
}

void DataTable::finishExperiment() {
    qDebug() << "自动实验完成:" << m_totalRounds << "轮次";

    // 【修复】先停止所有定时器，然后断开信号，避免竞争条件
    m_phaseTimer->stop();
    m_selectionMonitorTimer->stop();
    m_experimentTimer->stop(); // 确保实验计时器也被停止

    // 断开所有定时器信号连接，确保无重复连接
    disconnect(m_phaseTimer, nullptr, this, nullptr);
    disconnect(m_selectionMonitorTimer, nullptr, this, nullptr);
    disconnect(m_experimentTimer, nullptr, this, nullptr);

    // 隐藏交互界面
    emit requestHideInteractionDialog();

    // 【修复】使用非阻塞状态标签显示完成提示，避免UI冻结
    m_experimentStatusLabel->setText(QString("✅ 实验完成！总轮数: %1，总选择: %2")
                                    .arg(m_totalRounds)
                                    .arg(m_totalRounds * m_targetSelectionCount));

    // 【可选】如果需要弹窗提示，使用定时器延迟非阻塞显示
    QTimer::singleShot(100, this, [this]() {
        auto* msgBox = new QMessageBox(QMessageBox::Information,
                                      "实验完成",
                                      QString("自动实验已完成！\n\n总轮数: %1\n总选择次数: %2")
                                      .arg(m_totalRounds)
                                      .arg(m_totalRounds * m_targetSelectionCount),
                                      QMessageBox::Ok, this);
        msgBox->setModal(false);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->show();
    });

    // 重置自动实验状态
    m_currentPhase = PHASE_IDLE;
    m_currentRound = 0;
    m_totalRounds = 0;
    m_selectionCount = 0;
    m_lastSelectionCount = 0;

    // 恢复标签为无意图状态
    m_intentionComboBox->blockSignals(true);
    int idleIndex = m_intentionComboBox->findText("无明确意图");
    if (idleIndex >= 0) {
        m_intentionComboBox->setCurrentIndex(idleIndex);
        m_currentIntention = idleIndex;
    }
    m_intentionComboBox->blockSignals(false);

    // 直接重置UI状态，不调用stopExperiment()避免重复操作和状态冲突
    m_isExperimentRunning = false;
    m_isRecordingData = false;

    // 更新按钮状态
    m_startExperimentButton->setEnabled(true);
    m_pauseExperimentButton->setEnabled(false);
    m_stopExperimentButton->setEnabled(false);
    m_nextTaskButton->setEnabled(false);
    m_experimentTypeComboBox->setEnabled(true);
    m_paradigmSettingsButton->setEnabled(true);
    m_pauseExperimentButton->setText("暂停");

    // 更新状态显示
    m_experimentStatusLabel->setText("状态: 实验已完成");
    updateUIForTask(m_currentExperimentType);

    // 【修复】重新连接定时器信号，使用UniqueConnection防止重复连接
    connect(m_phaseTimer, &QTimer::timeout, this, &DataTable::onPhaseTimeout, Qt::UniqueConnection);
    connect(m_selectionMonitorTimer, &QTimer::timeout, this, &DataTable::checkSelectionProgress, Qt::UniqueConnection);
    connect(m_experimentTimer, &QTimer::timeout, this, &DataTable::updateExperimentStatus, Qt::UniqueConnection);
}

// === 新增：选择监控实现 ===

void DataTable::checkSelectionProgress() {
    // 【修复】添加更完善的状态检查，防止在错误状态下执行
    if (m_currentPhase != PHASE_SELECTION || !m_isExperimentRunning) {
        // qDebug() << "【选择监控】不在选择阶段或实验已停止，停止监控";
        return;
    }

    // 基于时间进行选择进度显示，但不以次数控制阶段结束
    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsedMs = m_selectionStartTime.msecsTo(now);
    qint64 totalSelectionTimeMs = m_expSettings.selectionTimeSeconds * 1000;

    // 根据时间进度估算选择进度（仅用于显示）
    int estimatedSelections = (int)((double)elapsedMs / totalSelectionTimeMs * m_targetSelectionCount);
    estimatedSelections = qMin(estimatedSelections, m_targetSelectionCount);

    if (estimatedSelections > m_lastSelectionCount) {
        m_lastSelectionCount = estimatedSelections;
        m_selectionCount = estimatedSelections;

        // 更新状态显示
        int remainingSeconds = qMax(0, (int)((totalSelectionTimeMs - elapsedMs) / 1000));
        m_experimentStatusLabel->setText(QString("第%1/%2轮 - 选择阶段 (%3/%4选择, 剩余%5秒)")
                                        .arg(m_currentRound)
                                        .arg(m_totalRounds)
                                        .arg(m_selectionCount)
                                        .arg(m_targetSelectionCount)
                                        .arg(remainingSeconds));

        qDebug() << QString("第%1轮选择进度: %2/%3 (已用时%4秒, 剩余%5秒)")
                    .arg(m_currentRound)
                    .arg(m_selectionCount)
                    .arg(m_targetSelectionCount)
                    .arg(elapsedMs / 1000)
                    .arg(remainingSeconds);
    }

    // 选择阶段现在只通过时间控制，不再通过次数控制结束
    // onPhaseTimeout() 会在时间到达时自动调用 onSelectionCompleted()
}

