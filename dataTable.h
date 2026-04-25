#ifndef PUPILEXT_DATATABLE_H
#define PUPILEXT_DATATABLE_H

#include <QtWidgets/QWidget>
#include <QtWidgets/qtableview.h>
#include <QtWidgets/QTabWidget>        // 新增
#include <QtWidgets/QPushButton>       // 新增
#include <QtWidgets/QCheckBox>         // 新增
#include <QtWidgets/QLabel>            // 新增
#include <QtWidgets/QComboBox>         // 标签输入
#include <QtWidgets/QSlider>           // 标签输入
#include <QtWidgets/QTextEdit>         // 标签输入
#include <QtWidgets/QDialog>           // 导出选择对话框
#include <QtWidgets/QVBoxLayout>       // 布局管理
#include <QtWidgets/QMessageBox>       // 消息提示
#include <QtWidgets/QFormLayout>       // 表单布局
#include <QtWidgets/QDialogButtonBox>  // 对话框按钮
#include <QtWidgets/QSpinBox>          // 数字输入框
#include <QDateTime>
#include <QTimer>
#include <QStandardItemModel>
#include <QMenu>
#include <QElapsedTimer>
#include <QVariant>                     // 标签事件记录需要
#include <QDebug>                       // 调试输出需要
#include <opencv2/opencv.hpp>           // cv::Point2f 需要
#include <functional>                   // std::function 需要
#include <numeric>                      // std::accumulate 需要
#include <algorithm>                    // std::max, std::min 需要
#include "Pupil.h"
#include "worker.h"  // 包含ProcessingResult和EyeMetrics的完整定义

// 前向声明（已包含在worker.h中，这里不再需要）
// struct EyeMetrics;
// struct ProcessingResult;

// 实验参数设置对话框
class ExperimentSettingsDialog : public QDialog {
    Q_OBJECT
public:
    struct ExperimentSettings {
        int roundCount = 5;           // 实验轮数
        int browseTimeSeconds = 30;   // 浏览时间(秒)
        int selectionTimeSeconds = 60; // 选择时间(秒)
        int selectionTargetCount = 10; // 目标选择次数
    };

    explicit ExperimentSettingsDialog(QWidget* parent = nullptr);
    ExperimentSettings getSettings() const;
    void setSettings(const ExperimentSettings& settings);

private:
    QSpinBox* m_roundCountSpinBox;
    QSpinBox* m_browseTimeSpinBox;
    QSpinBox* m_selectionTimeSpinBox;
    QSpinBox* m_selectionTargetSpinBox;
    ExperimentSettings m_settings;
};

// 特征数据缓存类
class FeatureDataCache {
private:
    ProcessingResult m_latestResult;
    QDateTime m_updateTime;
    bool m_dataValid = false;

public:
    void updateFromProcessingResult(const ProcessingResult& result);
    QString getFeatureValue(const QString& featureName) const;
    bool isDataFresh(int maxAgeMs = 100) const;
};

// 任务UI状态管理类
class TaskUIManager {
public:
    enum TaskType {
        // GRID_SELECTION 已删除（范式相关）
        TARGET_SEARCH,
        FATIGUE_MONITOR,
        WORKLOAD_MULTITASK,
        UNKNOWN
    };

    struct TaskUIState {
        bool paradigmSettingsEnabled = false;
        QString settingsTooltip = "";
        QString statusText = "";
        QString taskDescription = "";
        bool startButtonEnabled = false;
    };

    static TaskType getTaskType(const QString& taskName);
    static TaskUIState getUIStateForTask(const QString& taskName);
};

class DataTable : public QWidget {
    Q_OBJECT

public:

    // 窗口独立性方法（公共接口）
    void makeWindowIndependent(); // 强制窗口独立显示


    // 所有常量定义保持不变
    static const QString TIME;
    static const QString FRAME_NUMBER;
    static const QString CAMERA_FPS;
    static const QString PUPIL_FPS;
    static const QString PUPIL_CENTER_X;
    static const QString PUPIL_CENTER_Y;
    static const QString PUPIL_MAJOR;
    static const QString PUPIL_MINOR;
    static const QString PUPIL_WIDTH;
    static const QString PUPIL_HEIGHT;
    static const QString PUPIL_DIAMETER;
    static const QString PUPIL_UNDIST_DIAMETER;
    static const QString PUPIL_PHYSICAL_DIAMETER;
    static const QString PUPIL_CONFIDENCE;
    static const QString PUPIL_OUTLINE_CONFIDENCE;
    static const QString PUPIL_CIRCUMFERENCE;
    static const QString PUPIL_RATIO;
    static const QString EYE_OPENNESS;
    static const QString BLINK_STATE;
    static const QString PUPIL_VELOCITY_X;
    static const QString PUPIL_VELOCITY_Y;
    static const QString PUPIL_SPEED;
    static const QString PUPIL_ACCELERATION_X;
    static const QString PUPIL_ACCELERATION_Y;
    static const QString PUPIL_SIZE_CHANGE;
    static const QString EYE_MOVEMENT_TYPE;
    static const QString FIXATION_STABILITY;

    // 新增扩展特征常量
    static const QString GAZE_SCREEN_X;
    static const QString GAZE_SCREEN_Y;
    static const QString GAZE_VELOCITY_X;
    static const QString GAZE_VELOCITY_Y;
    static const QString GAZE_SPEED;
    static const QString CURRENT_STATE_DURATION;  // 替换 FIXATION_DURATION
    static const QString SACCADE_AMPLITUDE;
    static const QString SCANPATH_CONVEX_HULL_AREA;  // 扫视路径凸包面积
    static const QString FIXATION_DENSITY;           // 注视密度
    static const QString SACCADE_ENTROPY;            // 扫视熵
    static const QString MAIN_SEQUENCE_SLOPE;        // 主序关系斜率
    static const QString SACCADE_PEAK_VELOCITY;      // 扫视速度峰值
    static const QString BLINK_FREQUENCY;
    static const QString ATTENTION_FOCUS;
    static const QString INTENTION_LABEL;
    static const QString FATIGUE_LEVEL;
    static const QString COGNITIVE_WORKLOAD;

    // 新增5个特征常量
    static const QString MEAN_PUPIL_DIAMETER;      // 基础特征 - 平均瞳孔直径
    static const QString PUPIL_RELATIVE_DEVIATION; // 基础特征 - 瞳孔相对偏差
    static const QString BLINK_DURATION;           // 时序特征 - 眨眼时长
    static const QString MEAN_FIXATION_DURATION;   // 时序特征 - 平均注视时长
    static const QString FIXATION_RATE;            // 行为特征 - 注视率

    explicit DataTable(bool stereoMode=false, QWidget *parent=0);
    ~DataTable() override;

    QSize sizeHint() const override;

    // 【新增】范式控制公共方法
    void updateTrialProgress(int current, int total);

private:
    bool stereoMode;
    QTableView *tableView;
    QStandardItemModel *tableModel;
    QElapsedTimer timer;
    int updateDelay;
    QMenu *tableContextMenu;

    // 新增Tab界面组件
    QTabWidget* m_tabWidget;
    QTableView* m_basicTable;
    QTableView* m_motionTable;
    QTableView* m_temporalTable;
    QTableView* m_behaviorTable;

    QStandardItemModel* m_basicModel;
    QStandardItemModel* m_motionModel;
    QStandardItemModel* m_temporalModel;
    QStandardItemModel* m_behaviorModel;

    // 导出控件
    QPushButton* m_exportButton;
    QPushButton* m_featureSelectionButton;
    QLabel* m_statusLabel;

    // 导出数据类型选择
    QComboBox* m_exportDataTypeComboBox;
    QLabel* m_exportDataTypeLabel;

    // 标签输入控件
    QWidget* m_labelInputWidget;
    QComboBox* m_intentionComboBox;
    QSlider* m_fatigueSlider;
    QSlider* m_workloadSlider;
    QLabel* m_fatigueLabel;
    QLabel* m_workloadLabel;
    QPushButton* m_markEventButton;
    QTextEdit* m_eventNotesEdit;

    // 当前标签值
    int m_currentIntention;
    int m_currentFatigue;
    int m_currentWorkload;
    QString m_currentNotes;

    // 实验控制相关
    QWidget* m_experimentControlWidget;
    QPushButton* m_startExperimentButton;
    QPushButton* m_pauseExperimentButton;
    QPushButton* m_stopExperimentButton;
    QPushButton* m_nextTaskButton;
    QLabel* m_experimentStatusLabel;
    QLabel* m_currentTaskLabel;
    QLabel* m_timeElapsedLabel;
    QComboBox* m_experimentTypeComboBox;

    // 新增范式控制组件
    QPushButton* m_paradigmSettingsButton;
    QLabel* m_trialProgressLabel;
    QLabel* m_dataCountLabel;

    // 实验状态
    bool m_isExperimentRunning;
    bool m_isRecordingData;
    QDateTime m_expStartTime;
    QString m_currentExperimentType;
    int m_currentTaskIndex;
    QStringList m_taskList;
    QTimer* m_experimentTimer;

    // 实时导出相关
    bool m_realTimeExportEnabled;
    QTimer* m_realTimeExportTimer;
    QString m_realTimeExportFilePath;
    QFile* m_realTimeExportFile;
    QTextStream* m_realTimeExportStream;
    QDateTime m_exportStartTime;
    int m_exportedRecordCount;

    // 数据缓存队列（用于实时导出）
    struct DataRecord {
        QDateTime timestamp;
        ProcessingResult result;
        QString experimentType;
        QString currentTask;
        QString experimentStatus;
        int intention;
        int fatigue;
        int workload;
        QString notes;

        // 存储计算后的特征值（避免实时导出时特征值为0的问题）
        QMap<QString, QString> calculatedFeatures;
    };
    QList<DataRecord> m_dataBuffer;
    static const int MAX_BUFFER_SIZE = 100000; // 最大缓存10万条记录（支持长时间实验）

    // 保存最后一次的ProcessingResult（用于瞬时导出）
    ProcessingResult m_lastProcessingResult;
    bool m_hasValidProcessingResult;

    // 特征选择相关
    struct FeatureConfig {
        QString name;
        QString category;
        bool enabled;
        QString description;
    };
    QList<FeatureConfig> m_availableFeatures;
    QMap<QString, bool> m_selectedFeatures; // 特征名 -> 是否选中

    // 窗口状态记忆相关
    QString m_settingsFile;
    void saveWindowState();
    void loadWindowState();
    void saveWindowGeometry();
    void loadWindowGeometry();

    // 真实特征计算所需的历史数据
    struct HistoricalData {
        QDateTime timestamp;
        cv::Point2f gazeCoords;
        cv::Point2f pupilCenter;
        float pupilDiameter;
        bool isValid;
    };
    QList<HistoricalData> m_gazeHistory;      // 视线历史数据
    QList<HistoricalData> m_pupilHistory;     // 瞳孔历史数据
    static const int MAX_HISTORY_SIZE = 1000; // 保持最近1000帧数据

    // 权威状态机
    enum EyeMovementState {
        STATE_FIXATION,
        STATE_SACCADE,
        STATE_BLINK,
        STATE_UNKNOWN
    };
    EyeMovementState m_currentState;
    QDateTime m_stateStartTime;

    // 状态稳定性管理
    EyeMovementState m_rawState;        // Worker原始分类
    QDateTime m_rawStateStartTime;      // 原始状态开始时间
    static const int MIN_STATE_DURATION = 10;  // 最小状态持续时间(毫秒)
    cv::Point2f m_fixationCenter;
    QList<float> m_recentFixationDurations;   // 最近的注视时长记录
    QList<float> m_recentSaccadeDurations;    // 最近的扫视时长记录
    int m_blinkCount;                         // 眨眼计数
    QDateTime m_lastBlinkResetTime;           // 上次重置眨眼计数的时间

    // 扫视幅度相关状态
    cv::Point2f m_saccadeStartPosition;       // 扫视开始位置
    QList<float> m_recentSaccadeAmplitudes;   // 最近的扫视幅度记录
    float m_currentSaccadeAmplitude;          // 当前扫视幅度

    // 扫视路径凸包面积相关状态
    struct GazePointWithTime {
        cv::Point2f point;
        QDateTime timestamp;
    };
    QList<GazePointWithTime> m_gazePointsWithTime;  // 【修复】改为成员变量，避免静态局部变量问题
    QList<cv::Point2f> m_gazePointsWindow;    // 时间窗口内的视线坐标点
    static const int CONVEX_HULL_WINDOW_SIZE = 500;  // 时间窗口大小(毫秒)
    QDateTime m_lastConvexHullUpdateTime;     // 上次更新时间
    float m_currentConvexHullArea;            // 当前凸包面积

    // 注视密度相关状态
    struct FixationPointWithTime {
        cv::Point2f point;
        QDateTime timestamp;
    };
    QList<FixationPointWithTime> m_fixationPointsWithTime; // 带时间戳的注视点数据
    QList<cv::Point2f> m_fixationPointsWindow; // 时间窗口内的注视点坐标
    static const int FIXATION_DENSITY_WINDOW_SIZE = 500;  // 注视密度时间窗口(3秒)
    float m_currentFixationDensity;           // 当前注视密度
    float m_previousFixationDensity;          // 上一次注视密度（用于平滑）
    static constexpr float DENSITY_SMOOTHING_ALPHA = 0.4f; // 注视密度平滑系数

    // 扫视熵相关状态
    struct SaccadeDirectionData {
        float angle;                          // 扫视角度(度)
        QDateTime timestamp;                  // 扫视时间戳
    };
    QList<SaccadeDirectionData> m_saccadeDirectionHistory; // 扫视方向历史数据
    static const int SACCADE_ENTROPY_WINDOW_SIZE = 5000;  // 扫视熵时间窗口(5秒)
    static const int MIN_SACCADES_FOR_ENTROPY = 2;         // 计算熵所需最少扫视次数（降低为2次）
    float m_currentSaccadeEntropy;            // 当前扫视熵
    float m_previousSaccadeEntropy;           // 上一次扫视熵（用于平滑）
    static constexpr float ENTROPY_TIME_CONSTANT = 5.0f;   // 扫视熵时间常数（秒）
    static constexpr float ENTROPY_SMOOTHING_ALPHA = 0.3f; // 扫视熵平滑系数

    // 主序关系相关状态
    struct SaccadeMetrics {
        float amplitude;                      // 扫视幅度(像素)
        float peakVelocity;                  // 速度峰值(像素/秒)
        float duration;                      // 扫视持续时间(毫秒)
        QDateTime timestamp;                 // 扫视时间戳
    };
    QList<SaccadeMetrics> m_saccadeMetricsHistory; // 扫视指标历史数据
    static const int MAIN_SEQUENCE_WINDOW_SIZE = 10000;    // 主序关系时间窗口(10秒)
    static const int MIN_SACCADES_FOR_REGRESSION = 3;      // 线性回归所需最少扫视次数（降低为3次）
    float m_currentMainSequenceSlope;         // 当前主序关系斜率
    float m_previousMainSequenceSlope;        // 上一次主序关系斜率（用于平滑）
    static constexpr float MAIN_SEQUENCE_TIME_CONSTANT = 5.0f;   // 主序关系时间常数（秒）
    static constexpr float MAIN_SEQUENCE_SMOOTHING_ALPHA = 0.2f; // 主序关系平滑系数

    // 扫视速度峰值相关状态
    struct SaccadeTrajectory {
        QList<cv::Point2f> points;           // 扫视轨迹点
        QList<qint64> timestamps;            // 对应时间戳
        float peakVelocity;                  // 该次扫视的速度峰值
    };
    QList<float> m_saccadePeakVelocityHistory; // 扫视速度峰值历史
    static const int PEAK_VELOCITY_HISTORY_SIZE = 30;      // 保存最近50次扫视速度峰值
    float m_currentSaccadePeakVelocity;       // 当前平均扫视速度峰值

    // 扫视检测状态
    bool m_isInSaccade;                       // 是否正在扫视状态
    QDateTime m_saccadeStartTime;             // 当前扫视开始时间

    QList<cv::Point2f> m_currentSaccadeTrajectory; // 当前扫视轨迹
    QList<qint64> m_currentSaccadeTimestamps; // 当前扫视时间戳

    // 新增4个特征的历史数据结构
    QList<float> m_pupilDiameterHistory;      // 瞳孔直径历史 (用于平均瞳孔直径)
    QList<qint64> m_blinkDurations;           // 眨眼时长历史 (由 handleStateTransition 统一管理)

    // 注视率相关 (滑动窗口计数法)
    struct FixationEvent {
        QDateTime timestamp;                  // 注视开始时间戳
    };
    QList<FixationEvent> m_fixationEvents;    // 滑动窗口内的注视事件
    static const int FIXATION_RATE_WINDOW_SIZE = 5000;  // 滑动窗口大小(5秒)
    bool m_wasInFixation;                     // 上一帧是否在注视状态
    float m_currentFixationRate;              // 当前注视率

    // 标签事件记录系统
    struct LabelEvent {
        QDateTime timestamp;
        QString labelType;       // "intention", "fatigue", "workload", "task_switch"
        QVariant oldValue;
        QVariant newValue;
        QString description;
    };
    QList<LabelEvent> m_labelEvents;         // 所有标签变更事件
    int m_lastRecordedIntention;             // 上次记录的意图值
    int m_lastRecordedFatigue;               // 上次记录的疲劳值
    int m_lastRecordedWorkload;              // 上次记录的负荷值


    // 辅助函数
    void setPupilData(const PupilData &pupil, int column=0);
    void setEyeMetricsData(const EyeMetrics &metrics, int column=0);
    void clearPupilData(int column=0);
    void clearAllData();
    void clearAllHistoricalData(); // 【新增】清理所有历史数据和状态

    // 新增私有方法
    void setupTabInterface();
    void updateFeatureTabs(const ProcessingResult &result);
    void setupBasicTab();
    void setupMotionTab();
    void setupTemporalTab();
    void setupBehaviorTab();
    void setupTableView(QTableView* table, QStandardItemModel* model, const QString& tabName);
    void setupLabelInputWidget();
    void setupExperimentControlWidget();
    void loadTasksForExperiment(const QString& experimentType);
    void updateExperimentTimer();

    // 特征更新方法
    void updateBasicFeatures(const ProcessingResult &result);
    void updateMotionFeatures(const ProcessingResult &result);
    void updateTemporalFeatures(const ProcessingResult &result);
    void updateBehaviorFeatures(const ProcessingResult &result);

    // 标签事件记录函数
    void recordLabelChange(const QString& labelType, const QVariant& oldValue, const QVariant& newValue, const QString& description = "");

    // 导出辅助方法
    void exportToCSV(QTextStream& out);
    void exportToJSON(QTextStream& out);
    void exportModelToCSV(QTextStream& out, QStandardItemModel* model,
                         const QString& category, const QString& timestamp);
    void exportModelToJSON(QTextStream& out, QStandardItemModel* model,
                          const QString& category, bool isFirst);

    // 实时导出辅助方法
    void startRealTimeExport();
    void stopRealTimeExport();
    bool initializeRealTimeExportFile();
    void flushDataBuffer();
    void addDataToBuffer(const ProcessingResult& result);
    void writeDataRecordToFile(const DataRecord& record);
    QString generateExportFileName();
    void exportBufferedData();

    // 特征选择辅助方法
    void initializeFeatureList();
    void showFeatureSelectionDialog();
    QString generateCustomCSVHeader();
    void writeCustomDataRecordToFile(const DataRecord& record);
    void exportOriginalCSV(QTextStream& out);
    QString getFeatureValueFromUI(const QString& featureName);
    QString getFeatureValueFromData(const QString& featureName, const ProcessingResult& result); // 新增：直接从数据获取
    QString getFeatureValueFromData(const QString& featureName, const DataRecord& record); // 新增：从DataRecord获取存储的特征值

    // 特征值提取函数映射表
    typedef std::function<QString(const DataRecord&)> FeatureExtractor;
    void initializeFeatureExtractors();
    QMap<QString, FeatureExtractor> m_featureExtractors;

    // 新增4个特征的计算方法
    void updateMeanPupilDiameter(const PupilData& pupil);
    void updateBlinkDuration(const ProcessingResult& result);
    void updateMeanFixationDuration();
    void updateFixationRate(const ProcessingResult& result);

    // 扫视路径凸包面积计算方法
    void updateConvexHullArea(const ProcessingResult& result);
    float calculateConvexHullArea(const QList<cv::Point2f>& points);

    // 注视密度计算方法
    void updateFixationDensity(const ProcessingResult& result);
    float calculateFixationDensity(const QList<cv::Point2f>& fixationPoints);

    // 扫视熵计算方法
    void updateSaccadeEntropy(const ProcessingResult& result);
    float calculateSaccadeDirectionEntropy(const QList<SaccadeDirectionData>& directionData);
    float calculateContinuousSaccadeEntropy(const QList<SaccadeDirectionData>& directionData); // 新增连续扫视熵计算
    float calculateSaccadeAngle(const cv::Point2f& startPos, const cv::Point2f& endPos);

    // 主序关系计算方法
    void updateMainSequenceRelationship(const ProcessingResult& result);
    float calculateMainSequenceSlope(const QList<SaccadeMetrics>& metricsData);
    float calculateContinuousMainSequenceSlope(const QList<SaccadeMetrics>& metricsData); // 新增连续主序关系计算
    float linearRegression(const QList<float>& x, const QList<float>& y, float& slope, float& intercept);
    float calculateWeightedLinearRegression(const QList<float>& x, const QList<float>& y,
                                          const QList<float>& weights, float& slope, float& intercept); // 新增加权线性回归

    // 扫视速度峰值计算方法
    void updateSaccadePeakVelocity(const ProcessingResult& result);
    float calculatePeakVelocityFromTrajectory(const QList<cv::Point2f>& trajectory,
                                             const QList<qint64>& timestamps);

    // 状态管理方法
    void updateEyeState(const ProcessingResult& result);  // 权威状态机
    void handleStateTransition(EyeMovementState newState, qint64 workerStateStartTime = 0); // 状态切换处理
    void finalizeSaccadeEvent(qint64 duration);           // 扫视事件完成处理

    // 窗口管理方法
    void positionWindowOnRightSide();
    void adjustWindowForScreenChanges();
    void ensureWindowIndependence(); // 确保窗口完全独立

protected:
    void resizeEvent(QResizeEvent* event) override;
    void moveEvent(QMoveEvent* event) override;

public slots:
    // 【核心】唯一的、主数据更新槽
    void onProcessingResult(const ProcessingResult &result);
    // 【保留】用于更新独立的全局帧号
    void onCameraFramecount(int framecount);

    // 【废弃】这些槽函数不再被使用
    void onPupilData(quint64 timestamp, const PupilData &pupil, const QString &filename);
    void onStereoPupilData(quint64 timestamp, const PupilData &pupil, const PupilData &pupilSec, const QString &filename);
    void onCameraFPS(double fps);
    void onProcessingFPS(double fps);

    // UI交互槽
    void customMenuRequested(QPoint pos);
    void onContextMenuClick(QAction* action);

    // 新增导出槽函数
    void exportData();

    // 标签输入槽函数
    void onIntentionChanged(int index);
    void onFatigueChanged(int value);
    void onWorkloadChanged(int value);
    void onMarkEvent();
    void onEventNotesChanged();

    // 实验控制槽函数
    void startExperiment();
    void pauseExperiment();
    void stopExperiment();
    void nextTask();
    void onExperimentTypeChanged(const QString& type);
    void updateExperimentStatus();

    // 实时导出槽函数
    void onRealTimeExportTimer();

    // 特征选择槽函数
    void onFeatureSelectionClicked();

    // 新增：导出选择功能
    void showExportOptionsDialog();
    void exportInstantaneousData();    // 瞬时导出
    void exportRealTimeData();         // 实时数据导出
    void clearDataBuffer();            // 清空数据缓存

    // 实验流程槽函数
    void checkSelectionProgress(); // 定时器检查选择进度

    // 范式相关槽函数已删除

private:
    // 范式相关新增成员
    FeatureDataCache m_featureCache;

    // 实验流程状态机
    enum ExperimentPhase {
        PHASE_IDLE,           // 空闲状态
        PHASE_BROWSE,         // 浏览阶段
        PHASE_SELECTION,      // 选择阶段
        PHASE_COMPLETED       // 实验完成
    };

    ExperimentPhase m_currentPhase;
    int m_currentRound;
    int m_totalRounds;
    int m_selectionCount;
    int m_targetSelectionCount;
    QTimer* m_phaseTimer;
    QTimer* m_selectionMonitorTimer; // 监控选择进度的定时器
    ExperimentSettingsDialog::ExperimentSettings m_expSettings;

    // 选择监控状态
    QDateTime m_selectionStartTime;
    int m_lastSelectionCount;

    // 统一UI状态管理方法
    void updateUIForTask(const QString& taskName);

    // 实验流程控制方法
    void onParadigmSettingsClicked();
    void startAutomatedExperiment();
    void switchToBrowsePhase();
    void switchToSelectionPhase();
    void onPhaseTimeout();
    void onSelectionCompleted();
    void finishExperiment();
    void performRoundStateReset();      // 轮次间完整状态重置
    QString getFeatureValue(const QString& featureName) const;

signals:
    void createGraphPlot(const QString &value);
    void extendedFeatureData(const QString &featureName, double value, qint64 timestamp);

    // 实验流程控制信号
    void requestShowInteractionDialog();    // 请求显示交互对话框
    void requestHideInteractionDialog();    // 请求隐藏交互对话框
    void requestSelectionCount();           // 请求当前选择计数
};

#endif //PUPILEXT_DATATABLE_H
