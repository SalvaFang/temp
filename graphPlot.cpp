#include "graphPlot.h"
#include "dataTable.h"
#include <QDateTime>
#include <cstdlib>  // for rand()

uint64 GraphPlot::sharedTimestamp = 0;

// Create a graph plot window showing the given plotvalue in real-time
// QCustomPlot library is used for plotting
GraphPlot::GraphPlot(QString plotValue, bool stereoMode, bool legend, QWidget *parent) : QWidget(parent), customPlot(new QCustomPlot(this)), plotValue(plotValue), incrementedTimestamp(0), interaction(false), yinteraction(false), m_currentMinY(0), m_currentMaxY(0), m_yRangeInitialized(false) {

    setWindowTitle(plotValue);

    // Enable window close button and make it a independent window
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
    setAttribute(Qt::WA_DeleteOnClose); // Automatically delete when closed

    // While this works, the scaling of the plot inside the window is wrong, unclear how to fix this
    // TODO any chance to get the opengl qcustomplot scaling to work?
    //customPlot->setOpenGl(true);

    updateDelay = 33; // 30fps

    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(customPlot);

    setLayout(layout);

    customPlot->addGraph();

    if(stereoMode) {
        customPlot->addGraph();

        QPen penSec(Qt::green, 0, Qt::SolidLine);
        customPlot->graph(1)->setPen(penSec);
    }

    customPlot->legend->setVisible(legend);

    QPen pen(Qt::blue, 0, Qt::SolidLine);
    customPlot->graph(0)->setPen(pen);

    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%m:%s");
    customPlot->xAxis->setTicker(timeTicker);
    customPlot->axisRect()->setupFullAxesBox();

    setupPlotAxis();

    // Make left and bottom axes transfer their ranges to right and top axes:
    connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));

    customPlot->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(customPlot, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequest(QPoint)));

    // According to qcustomplot docs, shouldnt be enabled https://www.qcustomplot.com/documentation/performanceimprovement.html
    //setRenderHint(QPainter::Antialiasing);

    customPlot->show();
    timer.start();
}

GraphPlot::~GraphPlot() {
    // customPlot is automatically deleted by Qt's parent-child mechanism
}

void GraphPlot::closeEvent(QCloseEvent *event) {
    // Emit signal to notify MainWindow that this GraphPlot is closing
    emit graphPlotClosed(this);

    // Accept the close event
    event->accept();

    // Call parent implementation
    QWidget::closeEvent(event);
}

QSize GraphPlot::sizeHint() const {
    return QSize(640, 180);
}

// Resets the graph, removes all data and resets timestamp (x-axis)
void GraphPlot::reset() {

    incrementedTimestamp = 0;
    // 【修复】不要重置共享时间戳，这会破坏眼动参数计算的时间连续性
    // sharedTimestamp = 0; // Reset shared timestamp too - 已注释，避免影响其他图表
    customPlot->graph(0)->data()->clear();

    // Clear second graph if it exists (stereo mode)
    if(customPlot->graphCount() > 1) {
        customPlot->graph(1)->data()->clear();
    }

    // 重置Y轴范围管理状态
    m_yRangeInitialized = false;
    m_currentMinY = 0;
    m_currentMaxY = 0;

    // Reset the x-axis range to prevent jumping
    customPlot->xAxis->setRange(0, 15);
    customPlot->replot();
}

// On right click on the plot a context menu is created at the position of the click
void GraphPlot::contextMenuRequest(QPoint pos) {

    QMenu *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    menu->addAction("Clear", this, SLOT(reset()));
    QAction *interactionAct = menu->addAction("Interaction", this, SLOT(enableInteractions()));
    interactionAct->setCheckable(true);
    interactionAct->setChecked(interaction);

    QAction *yinteractionAct = menu->addAction("Scale Y-Axis", this, SLOT(enableYAxisInteraction()));
    yinteractionAct->setCheckable(true);
    yinteractionAct->setChecked(yinteraction);

    menu->addSeparator();
    menu->addAction("Close", this, SLOT(close()));

    menu->popup(customPlot->mapToGlobal(pos));
}

// Enables the interactive plot on all axes
// Enables dragging and zooming on both axis
void GraphPlot::enableInteractions() {
    interaction = !interaction;
    if(interaction) {
        customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes);
    } else {
        customPlot->setInteractions(QFlags<QCP::Interaction>());
    }
}

// Enables the interactive plot only on the Y axis
// Enables dragging and zooming on the A axis
// The X axis still scales and moves automatically
void GraphPlot::enableYAxisInteraction() {
    yinteraction = !yinteraction;
    if(yinteraction) {
        customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
        customPlot->axisRect(0)->setRangeDrag(Qt::Vertical);
    } else {
        customPlot->setInteractions(QFlags<QCP::Interaction>());
        customPlot->axisRect(0)->setRangeDrag(QFlags<Qt::Orientation>());
    }
}

// Sets Y axis label of the graph plot according to the current plot value
void GraphPlot::setupPlotAxis() {

    if(plotValue == DataTable::FRAME_NUMBER) {
        customPlot->yAxis->setLabel("frame number");
    } else if(plotValue == DataTable::CAMERA_FPS) {
        customPlot->yAxis->setLabel("[fps]");
    } else if(plotValue == DataTable::PUPIL_FPS) {
        customPlot->yAxis->setLabel("[fps]");
    } else if(plotValue == DataTable::PUPIL_CENTER_X) {
        customPlot->yAxis->setLabel("pupil center [px]");
    } else if(plotValue == DataTable::PUPIL_CENTER_Y) {
        customPlot->yAxis->setLabel("pupil center [px]");
    } else if(plotValue == DataTable::PUPIL_MAJOR) {
        customPlot->yAxis->setLabel("pupil major axis [px]");
    } else if(plotValue == DataTable::PUPIL_MINOR) {
        customPlot->yAxis->setLabel("pupil minor axis [px]");
    } else if(plotValue == DataTable::PUPIL_WIDTH) {
        customPlot->yAxis->setLabel("pupil width [px]");
    } else if(plotValue == DataTable::PUPIL_HEIGHT) {
        customPlot->yAxis->setLabel("pupil height [px]");
    } else if(plotValue == DataTable::PUPIL_CONFIDENCE) {
        customPlot->yAxis->setLabel("pupil confidence");
        customPlot->yAxis->setRange(-0.2, 1.2);
    } else if(plotValue == DataTable::PUPIL_OUTLINE_CONFIDENCE) {
        customPlot->yAxis->setLabel("pupil outline confidence");
        customPlot->yAxis->setRange(-0.2, 1.2);
    } else if(plotValue == DataTable::PUPIL_CIRCUMFERENCE) {
        customPlot->yAxis->setLabel("pupil circumference [px]");
    } else if(plotValue == DataTable::PUPIL_RATIO) {
        customPlot->yAxis->setLabel("pupil axis ratio");
    } else if(plotValue == DataTable::PUPIL_DIAMETER) {
        customPlot->yAxis->setLabel("pupil diameter [px]");
    } else if(plotValue == DataTable::PUPIL_UNDIST_DIAMETER) {
        customPlot->yAxis->setLabel("pupil undistorted diameter [px]");
    } else if(plotValue == DataTable::PUPIL_PHYSICAL_DIAMETER) {
        customPlot->yAxis->setLabel("pupil physical diameter [mm]");
    }

    // 新增5个特征的Y轴标签设置
    else if(plotValue == DataTable::MEAN_PUPIL_DIAMETER) {
        customPlot->yAxis->setLabel("平均瞳孔直径 [px]");
    } else if(plotValue == DataTable::PUPIL_RELATIVE_DEVIATION) {
        customPlot->yAxis->setLabel("瞳孔相对偏差 [%]");
    } else if(plotValue == DataTable::BLINK_DURATION) {
        customPlot->yAxis->setLabel("眨眼时长 [ms]");
    } else if(plotValue == DataTable::MEAN_FIXATION_DURATION) {
        customPlot->yAxis->setLabel("平均注视时长 [ms]");
    } else if(plotValue == DataTable::FIXATION_RATE) {
        customPlot->yAxis->setLabel("注视率 [次/秒]");
    }

    // 新增扩展特征的Y轴标签设置
    else if(plotValue == DataTable::GAZE_SCREEN_X) {
        customPlot->yAxis->setLabel("视线屏幕X坐标 [px]");
    } else if(plotValue == DataTable::GAZE_SCREEN_Y) {
        customPlot->yAxis->setLabel("视线屏幕Y坐标 [px]");
    } else if(plotValue == DataTable::GAZE_VELOCITY_X) {
        customPlot->yAxis->setLabel("视线速度X [px/s]");
    } else if(plotValue == DataTable::GAZE_VELOCITY_Y) {
        customPlot->yAxis->setLabel("视线速度Y [px/s]");
    } else if(plotValue == DataTable::GAZE_SPEED) {
        customPlot->yAxis->setLabel("视线速度 [px/s]");
    } else if(plotValue == DataTable::CURRENT_STATE_DURATION) {
        customPlot->yAxis->setLabel("当前状态持续时长 [ms]");
    } else if(plotValue == DataTable::SACCADE_AMPLITUDE) {
        customPlot->yAxis->setLabel("扫视幅度 [px]");
    } else if(plotValue == DataTable::SCANPATH_CONVEX_HULL_AREA) {
        customPlot->yAxis->setLabel("扫视路径凸包面积 [px²]");
    } else if(plotValue == DataTable::FIXATION_DENSITY) {
        customPlot->yAxis->setLabel("注视密度 [1/px]");
    } else if(plotValue == DataTable::SACCADE_ENTROPY) {
        customPlot->yAxis->setLabel("扫视熵 [bits]");
        customPlot->yAxis->setRange(0, 3);
    } else if(plotValue == DataTable::MAIN_SEQUENCE_SLOPE) {
        customPlot->yAxis->setLabel("主序关系斜率 [px/s/px]");
    } else if(plotValue == DataTable::SACCADE_PEAK_VELOCITY) {
        customPlot->yAxis->setLabel("扫视速度峰值 [px/s]");
    } else if(plotValue == DataTable::BLINK_FREQUENCY) {
        customPlot->yAxis->setLabel("眨眼频率 [/min]");
    } else if(plotValue == DataTable::INTENTION_LABEL) {
        customPlot->yAxis->setLabel("交互意图类型");
        customPlot->yAxis->setRange(0, 6);
    } else if(plotValue == DataTable::FATIGUE_LEVEL) {
        customPlot->yAxis->setLabel("疲劳等级");
        customPlot->yAxis->setRange(0, 4);
    } else if(plotValue == DataTable::COGNITIVE_WORKLOAD) {
        customPlot->yAxis->setLabel("认知负荷");
        customPlot->yAxis->setRange(0, 4);
    }

    // 瞎编的运动特征Y轴设置已删除（微扫视计数、角速度、空间精度、路径效率、运动平滑度、方向一致性、速度稳定性）

    // ========== 补充完整的时序特征Y轴设置 ==========
    else if(plotValue == "扫视时长(ms)") {
        customPlot->yAxis->setLabel("扫视时长 [ms]");
    // 瞎编特征已删除：停留时间
    } else if(plotValue == "接近时间(ms)") {
        customPlot->yAxis->setLabel("接近时间 [ms]");
    } else if(plotValue == "犹豫时间(ms)") {
        customPlot->yAxis->setLabel("犹豫时间 [ms]");
    } else if(plotValue == "反应时间(ms)") {
        customPlot->yAxis->setLabel("反应时间 [ms]");
    } else if(plotValue == "扫视频率(/s)") {
        customPlot->yAxis->setLabel("扫视频率 [/s]");
    // 瞎编的时序特征Y轴设置已删除（时间一致性、节律稳定性、时序模式、周期性检测、持续性评估、时间窗口分析、序列相关性、时间聚类、频域特征、相位分析）
    } else if(plotValue == "预测延迟(ms)") {
        customPlot->yAxis->setLabel("预测延迟 [ms]");
    }

    // ========== 补充完整的行为特征Y轴设置 ==========
    else if(plotValue == "视觉效率") {
        customPlot->yAxis->setLabel("视觉效率");
        customPlot->yAxis->setRange(0, 1);
    } else if(plotValue == "扫描模式") {
        customPlot->yAxis->setLabel("扫描模式");
        customPlot->yAxis->setRange(0, 1);
    } else if(plotValue == "探索深度") {
        customPlot->yAxis->setLabel("探索深度");
        customPlot->yAxis->setRange(0, 1);
    } else if(plotValue == "重访率(%)") {
        customPlot->yAxis->setLabel("重访率 [%]");
        customPlot->yAxis->setRange(0, 100);
    } else if(plotValue == "区域偏好") {
        customPlot->yAxis->setLabel("区域偏好");
        customPlot->yAxis->setRange(0, 1);
    } else if(plotValue == "搜索效率") {
        customPlot->yAxis->setLabel("搜索效率");
        customPlot->yAxis->setRange(0, 1);
    } else if(plotValue == "目标捕获率") {
        customPlot->yAxis->setLabel("目标捕获率");
        customPlot->yAxis->setRange(0, 1);
    } else if(plotValue == "交互准备度") {
        customPlot->yAxis->setLabel("交互准备度");
        customPlot->yAxis->setRange(0, 1);
    } else if(plotValue == "决策犹豫度") {
        customPlot->yAxis->setLabel("决策犹豫度");
        customPlot->yAxis->setRange(0, 1);
    } else if(plotValue == "任务参与度") {
        customPlot->yAxis->setLabel("任务参与度");
        customPlot->yAxis->setRange(0, 1);
    } else if(plotValue == "专注稳定性") {
        customPlot->yAxis->setLabel("专注稳定性");
        customPlot->yAxis->setRange(0, 1);
    } else if(plotValue == "干扰抗性") {
        customPlot->yAxis->setLabel("干扰抗性");
        customPlot->yAxis->setRange(0, 1);
    // 瞎编行为特征Y轴设置已删除（适应性指标、学习曲线、错误恢复能力、认知策略、用户偏好、整体表现评分）
    } else if(plotValue == "多任务能力") {
        customPlot->yAxis->setLabel("多任务能力");
        customPlot->yAxis->setRange(1, 10);
    } else if(plotValue == "注意力分配") {
        customPlot->yAxis->setLabel("注意力分配");
        customPlot->yAxis->setRange(1, 10);
    } else if(plotValue == "界面熟悉度") {
        customPlot->yAxis->setLabel("界面熟悉度");
        customPlot->yAxis->setRange(1, 10);
    } else if(plotValue == "操作熟练度") {
        customPlot->yAxis->setLabel("操作熟练度");
        customPlot->yAxis->setRange(1, 10);
    }
}

// Slot that is called upon receiving framecounter signals
// Updates the table columns with current camera fps
// This is called for framecounter classes one time per second
void GraphPlot::appendData(const double &fps) {

    // Use shared timestamp system to maintain consistency
    if(sharedTimestamp == 0) {
        sharedTimestamp = QDateTime::currentMSecsSinceEpoch();
    }

    uint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();
    uint64 m_timestamp = currentTimestamp - sharedTimestamp;

    // add data
    if(plotValue == DataTable::CAMERA_FPS || plotValue == DataTable::PUPIL_FPS) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, fps);
    }

    if(timer.elapsed() > updateDelay) {
        timer.restart();

        if(!interaction) {
            if(!yinteraction && plotValue != DataTable::PUPIL_CONFIDENCE && plotValue != DataTable::PUPIL_OUTLINE_CONFIDENCE) {
                // 使用智能扩展缩放，避免Y轴波动
                updateYAxisRangeExpansive(fps);
            }
            // make key axis range scroll with the data (at a constant range size of 15):
            // Keep data always visible by following the latest data point
            double currentTime = m_timestamp/1000.0;

            // Always keep the latest data in view with a rolling window
            if (currentTime > 15.0) {
                // Use a rolling window that follows the data
                customPlot->xAxis->setRange(currentTime - 15.0, currentTime);
            } else {
                // For the first 15 seconds, use a fixed range
                customPlot->xAxis->setRange(0, 15.0);
            }
        }

        // if the first data is older than 4 minutes, remove 2 minutes of worth
        if(customPlot->graph(0)->dataCount() > 0 && (m_timestamp/1000.0) - customPlot->graph()->dataMainKey (0) > 240) {
            customPlot->graph(0)->data()->removeBefore((m_timestamp/1000.0)-120);
        }

        customPlot->replot();
    }
}

// Slot that is called upon receiving framecount signals
// Updates the table columns with current camera framecount
// This is called for framecounter classes one time per second
void GraphPlot::appendData(const int &framecount) {

    // Use shared timestamp system to maintain consistency
    if(sharedTimestamp == 0) {
        sharedTimestamp = QDateTime::currentMSecsSinceEpoch();
    }

    uint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();
    uint64 m_timestamp = currentTimestamp - sharedTimestamp;

    // add data
    if(plotValue == DataTable::FRAME_NUMBER) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, framecount);
    }

    if(timer.elapsed() > updateDelay) {
        timer.restart();

        if(!interaction) {
            if(!yinteraction && plotValue != DataTable::PUPIL_CONFIDENCE && plotValue != DataTable::PUPIL_OUTLINE_CONFIDENCE) {
                // 使用智能扩展缩放，避免Y轴波动
                updateYAxisRangeExpansive(framecount);
            }
            // make key axis range scroll with the data (at a constant range size of 15):
            // Keep data always visible by following the latest data point
            double currentTime = m_timestamp/1000.0;

            // Always keep the latest data in view with a rolling window
            if (currentTime > 15.0) {
                // Use a rolling window that follows the data
                customPlot->xAxis->setRange(currentTime - 15.0, currentTime);
            } else {
                // For the first 15 seconds, use a fixed range
                customPlot->xAxis->setRange(0, 15.0);
            }
        }

        // if the first data is older than 4 minutes, remove 2 minutes of worth
        if(customPlot->graph(0)->dataCount() > 0 && (m_timestamp/1000.0) - customPlot->graph()->dataMainKey (0) > 240) {
            customPlot->graph(0)->data()->removeBefore((m_timestamp/1000.0)-120);
        }

        customPlot->replot();
    }
}

// Slot that is called upon receiving a new pupil detection
// Updates the table columns with current pupil data i.e. all meta information of the pupil
// This is called from the pupil detection process, potentially 120 times per second, however only data is appended in that rate
// Plot replots are executed in rates defined by updateDelay i.e. 30 fps
void GraphPlot::appendData(quint64 timestamp, const PupilData &pupil, const QString &/*filename*/) {

    if(sharedTimestamp==0)
        sharedTimestamp = timestamp;
    uint64 m_timestamp = timestamp - sharedTimestamp;

    // add data and track the value for Y-axis scaling
    double currentValue = 0.0;
    bool dataAdded = false;

    if(plotValue == DataTable::PUPIL_CENTER_X) {
        currentValue = pupil.center.x;
        customPlot->graph(0)->addData(m_timestamp/1000.0, currentValue);
        dataAdded = true;
    } else if(plotValue == DataTable::PUPIL_CENTER_Y) {
        currentValue = pupil.center.y;
        customPlot->graph(0)->addData(m_timestamp/1000.0, currentValue);
        dataAdded = true;
    } else if(plotValue == DataTable::PUPIL_MAJOR) {
        currentValue = pupil.majorAxis();
        customPlot->graph(0)->addData(m_timestamp/1000.0, currentValue);
        dataAdded = true;
    } else if(plotValue == DataTable::PUPIL_MINOR) {
        currentValue = pupil.minorAxis();
        customPlot->graph(0)->addData(m_timestamp/1000.0, currentValue);
        dataAdded = true;
    } else if(plotValue == DataTable::PUPIL_WIDTH) {
        currentValue = pupil.width();
        customPlot->graph(0)->addData(m_timestamp/1000.0, currentValue);
        dataAdded = true;
    } else if(plotValue == DataTable::PUPIL_HEIGHT) {
        currentValue = pupil.height();
        customPlot->graph(0)->addData(m_timestamp/1000.0, currentValue);
        dataAdded = true;
    } else if(plotValue == DataTable::PUPIL_CONFIDENCE) {
        currentValue = pupil.confidence;
        customPlot->graph(0)->addData(m_timestamp/1000.0, currentValue);
        dataAdded = true;
    } else if(plotValue == DataTable::PUPIL_OUTLINE_CONFIDENCE) {
        currentValue = pupil.outline_confidence;
        customPlot->graph(0)->addData(m_timestamp/1000.0, currentValue);
        dataAdded = true;
    } else if(plotValue == DataTable::PUPIL_CIRCUMFERENCE) {
        currentValue = pupil.circumference();
        customPlot->graph(0)->addData(m_timestamp/1000.0, currentValue);
        dataAdded = true;
    } else if(plotValue == DataTable::PUPIL_RATIO) {
        currentValue = (double)pupil.majorAxis() / pupil.minorAxis();
        customPlot->graph(0)->addData(m_timestamp/1000.0, currentValue);
        dataAdded = true;
    } else if(plotValue == DataTable::PUPIL_DIAMETER) {
        currentValue = pupil.diameter();
        customPlot->graph(0)->addData(m_timestamp/1000.0, currentValue);
        dataAdded = true;
    } else if(plotValue == DataTable::PUPIL_UNDIST_DIAMETER) {
        currentValue = pupil.undistortedDiameter;
        customPlot->graph(0)->addData(m_timestamp/1000.0, currentValue);
        dataAdded = true;
    } else if(plotValue == DataTable::PUPIL_PHYSICAL_DIAMETER) {
        currentValue = pupil.physicalDiameter;
        customPlot->graph(0)->addData(m_timestamp/1000.0, currentValue);
        dataAdded = true;
    }

    if(timer.elapsed() > updateDelay) {
        timer.restart();

        if(!interaction) {
            if(!yinteraction && plotValue != DataTable::PUPIL_CONFIDENCE && plotValue != DataTable::PUPIL_OUTLINE_CONFIDENCE) {
                // 使用智能扩展缩放，避免Y轴波动
                if(dataAdded) {
                    updateYAxisRangeExpansive(currentValue);
                }
            }
            // make key axis range scroll with the data (at a constant range size of 15):
            // Keep data always visible by following the latest data point
            double currentTime = m_timestamp/1000.0;

            // Always keep the latest data in view with a rolling window
            if (currentTime > 15.0) {
                // Use a rolling window that follows the data
                customPlot->xAxis->setRange(currentTime - 15.0, currentTime);
            } else {
                // For the first 15 seconds, use a fixed range
                customPlot->xAxis->setRange(0, 15.0);
            }
        }

        //std::cout<<customPlot->graph()->dataMainKey (0)/1000.0<<std::endl;


        // if the first data is older than 4 minutes, remove 2 minutes of worth
        if((m_timestamp/1000.0) - customPlot->graph()->dataMainKey (0) > 240) {
            customPlot->graph(0)->data()->removeBefore((m_timestamp/1000.0)-120);
        }

        customPlot->replot();
    }
}

// Slot that is called upon receiving a new stereo pupil detection
// Updates the table columns with current pupil data i.e. all meta information of both pupil detections
// This is called from the pupil detection process, potentially 120 times per second, however only data is appended in that rate
// Plot replots are executed in rates defined by updateDelay i.e. 30 fps
void GraphPlot::appendData(quint64 timestamp, const PupilData &pupil, const PupilData &pupilSec, const QString &filename) {

    if(sharedTimestamp==0)
        sharedTimestamp = timestamp;
    uint64 m_timestamp = timestamp - sharedTimestamp;

    // add data
    if(plotValue == DataTable::PUPIL_CENTER_X) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, pupil.center.x);
        customPlot->graph(1)->addData(m_timestamp/1000.0, pupilSec.center.x);
    } else if(plotValue == DataTable::PUPIL_CENTER_Y) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, pupil.center.y);
        customPlot->graph(1)->addData(m_timestamp/1000.0, pupilSec.center.y);
    } else if(plotValue == DataTable::PUPIL_MAJOR) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, pupil.majorAxis());
        customPlot->graph(1)->addData(m_timestamp/1000.0, pupilSec.majorAxis());
    } else if(plotValue == DataTable::PUPIL_MINOR) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, pupil.minorAxis());
        customPlot->graph(1)->addData(m_timestamp/1000.0, pupilSec.minorAxis());
    } else if(plotValue == DataTable::PUPIL_WIDTH) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, pupil.width());
        customPlot->graph(1)->addData(m_timestamp/1000.0, pupilSec.width());
    } else if(plotValue == DataTable::PUPIL_HEIGHT) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, pupil.height());
        customPlot->graph(1)->addData(m_timestamp/1000.0, pupilSec.height());
    } else if(plotValue == DataTable::PUPIL_CONFIDENCE) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, pupil.confidence);
        customPlot->graph(1)->addData(m_timestamp/1000.0, pupilSec.confidence);
    } else if(plotValue == DataTable::PUPIL_OUTLINE_CONFIDENCE) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, pupil.outline_confidence);
        customPlot->graph(1)->addData(m_timestamp/1000.0, pupilSec.outline_confidence);
    } else if(plotValue == DataTable::PUPIL_CIRCUMFERENCE) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, pupil.circumference());
        customPlot->graph(1)->addData(m_timestamp/1000.0, pupilSec.circumference());
    } else if(plotValue == DataTable::PUPIL_RATIO) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, (double)pupil.majorAxis() / pupil.minorAxis());
        customPlot->graph(1)->addData(m_timestamp/1000.0, (double)pupilSec.majorAxis() / pupilSec.minorAxis());
    } else if(plotValue == DataTable::PUPIL_DIAMETER) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, pupil.diameter());
        customPlot->graph(1)->addData(m_timestamp/1000.0, pupilSec.diameter());
    } else if(plotValue == DataTable::PUPIL_UNDIST_DIAMETER) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, pupil.undistortedDiameter);
        customPlot->graph(1)->addData(m_timestamp/1000.0, pupilSec.undistortedDiameter);
    } else if(plotValue == DataTable::PUPIL_PHYSICAL_DIAMETER) {
        customPlot->graph(0)->addData(m_timestamp/1000.0, pupil.physicalDiameter);
    }

    if(timer.elapsed() > updateDelay) {
        timer.restart();

        if(!interaction) {
            if(!yinteraction && plotValue != DataTable::PUPIL_CONFIDENCE && plotValue != DataTable::PUPIL_OUTLINE_CONFIDENCE) {
                // 双瞳孔模式：获取两个图表的数据范围，使用智能扩展缩放
                bool found0, found1;
                QCPRange range0 = customPlot->graph(0)->getValueRange(found0);
                QCPRange range1 = customPlot->graph(1)->getValueRange(found1);
                if(found0 && found1) {
                    updateYAxisRangeExpansive(qMax(range0.upper, range1.upper));
                    updateYAxisRangeExpansive(qMin(range0.lower, range1.lower));
                }
            }
            // make key axis range scroll with the data (at a constant range size of 15):
            // Keep data always visible by following the latest data point
            double currentTime = m_timestamp/1000.0;

            // Always keep the latest data in view with a rolling window
            if (currentTime > 15.0) {
                // Use a rolling window that follows the data
                customPlot->xAxis->setRange(currentTime - 15.0, currentTime);
            } else {
                // For the first 15 seconds, use a fixed range
                customPlot->xAxis->setRange(0, 15.0);
            }
        }


        // if the first data is older than 4 minutes, remove 2 minutes of worth
        if((m_timestamp/1000.0) - customPlot->graph(0)->dataMainKey (0) > 240) {
            customPlot->graph(0)->data()->removeBefore((m_timestamp/1000.0)-120);
            customPlot->graph(1)->data()->removeBefore((m_timestamp/1000.0)-120);
        }

        customPlot->replot();
    }
}

// 新增：处理包含眼动指标的完整处理结果
// graphPlot.cpp

// ... 其他 include ...

// graphPlot.cpp

// ... 其他函数保持不变 ...

// ==================== 【【【 完整且正确的函数实现 】】】 ====================
void GraphPlot::appendData(const ProcessingResult &result) {
    // 使用节流阀控制UI更新，防止因数据帧率过高导致界面卡顿
    if(timer.elapsed() < updateDelay) {
        return;
    }

    // 步骤 1: 【修正】使用数据包自带的精确时间戳
    if(sharedTimestamp == 0) {
        sharedTimestamp = result.eyeMetrics.timestamp; // 用第一帧的精确时间戳作为基准
    }
    uint64 m_timestamp = result.eyeMetrics.timestamp - sharedTimestamp;
    double key = m_timestamp / 1000.0; // 转换为秒

    // 步骤 2: 【修正】根据 plotValue 决定要绘制的数据值 (value)
    double value_to_plot = 0.0;
    bool should_plot = false; // 标志位，决定本帧是否要添加数据点

    // --- 【修复】统一的数据提取逻辑：始终绘制数据点，确保时间轴连续性 ---
    // 瞳孔几何参数：Worker已确保无效时为0值，直接使用即可
    if (plotValue == DataTable::PUPIL_CENTER_X) { value_to_plot = result.pupilData.center.x; should_plot = true; }
    else if (plotValue == DataTable::PUPIL_CENTER_Y) { value_to_plot = result.pupilData.center.y; should_plot = true; }
    else if (plotValue == DataTable::PUPIL_MAJOR) { value_to_plot = result.pupilData.majorAxis(); should_plot = true; }
    else if (plotValue == DataTable::PUPIL_MINOR) { value_to_plot = result.pupilData.minorAxis(); should_plot = true; }
    else if (plotValue == DataTable::PUPIL_WIDTH) { value_to_plot = result.pupilData.width(); should_plot = true; }
    else if (plotValue == DataTable::PUPIL_HEIGHT) { value_to_plot = result.pupilData.height(); should_plot = true; }
    else if (plotValue == DataTable::PUPIL_DIAMETER) { value_to_plot = result.pupilData.diameter(); should_plot = true; }
    else if (plotValue == DataTable::PUPIL_UNDIST_DIAMETER) { value_to_plot = result.pupilData.undistortedDiameter; should_plot = true; }
    else if (plotValue == DataTable::PUPIL_PHYSICAL_DIAMETER) { value_to_plot = result.pupilData.physicalDiameter; should_plot = true; }
    else if (plotValue == DataTable::PUPIL_CONFIDENCE) { value_to_plot = result.pupilData.confidence; should_plot = true; }
    else if (plotValue == DataTable::PUPIL_OUTLINE_CONFIDENCE) { value_to_plot = result.pupilData.outline_confidence; should_plot = true; }
    else if (plotValue == DataTable::PUPIL_CIRCUMFERENCE) { value_to_plot = result.pupilData.circumference(); should_plot = true; }
    else if (plotValue == DataTable::PUPIL_RATIO) {
        // 【修复】避免除零错误，无效时绘制0
        if (result.pupilData.majorAxis() > 1e-6) {
            value_to_plot = (double)result.pupilData.minorAxis() / result.pupilData.majorAxis();
        } else {
            value_to_plot = 0.0; // 无效时绘制0而不是跳过
        }
        should_plot = true;
    }

    // 眼动指标：始终绘制，Worker已确保无效时为合理的0值
    else if (plotValue == DataTable::EYE_OPENNESS) { value_to_plot = result.eyeMetrics.eyeOpenness; should_plot = true; }
    else if (plotValue == DataTable::BLINK_STATE) { value_to_plot = result.eyeMetrics.blinkState; should_plot = true; }
    else if (plotValue == DataTable::PUPIL_VELOCITY_X) { value_to_plot = result.eyeMetrics.pupilVelocity.x; should_plot = true; }
    else if (plotValue == DataTable::PUPIL_VELOCITY_Y) { value_to_plot = result.eyeMetrics.pupilVelocity.y; should_plot = true; }
    else if (plotValue == DataTable::PUPIL_SPEED) { value_to_plot = result.eyeMetrics.pupilSpeed; should_plot = true; }
    else if (plotValue == DataTable::PUPIL_ACCELERATION_X) { value_to_plot = result.eyeMetrics.pupilAcceleration.x; should_plot = true; }
    else if (plotValue == DataTable::PUPIL_ACCELERATION_Y) { value_to_plot = result.eyeMetrics.pupilAcceleration.y; should_plot = true; }
    else if (plotValue == DataTable::PUPIL_SIZE_CHANGE) { value_to_plot = result.eyeMetrics.pupilSizeChange; should_plot = true; }
    else if (plotValue == DataTable::EYE_MOVEMENT_TYPE) { value_to_plot = result.eyeMetrics.eyeMovementType; should_plot = true; }
    else if (plotValue == DataTable::FIXATION_STABILITY) { value_to_plot = result.eyeMetrics.fixationStability; should_plot = true; }

    // 注意：扩展特征现在通过DataTable的extendedFeatureData信号传递
    // 这里不再处理扩展特征，避免重复数据

    // 帧率数据总是有效
    if (plotValue == DataTable::CAMERA_FPS || plotValue == DataTable::PUPIL_FPS) {
        value_to_plot = result.eyeMetrics.frameRate;
        should_plot = true;
    }

    // 步骤 3: 如果确定要绘制，则添加数据点
    if (should_plot) {
        customPlot->graph(0)->addData(key, value_to_plot);
    }

    // 步骤 4: 重绘图表和管理坐标轴 (这部分逻辑保持不变)
    timer.restart();
    if(!interaction) {
        if(!yinteraction) {
            if(plotValue == DataTable::PUPIL_CONFIDENCE || plotValue == DataTable::PUPIL_OUTLINE_CONFIDENCE ||
               plotValue == DataTable::EYE_OPENNESS || plotValue == DataTable::FIXATION_STABILITY ||
               plotValue == DataTable::PUPIL_RATIO || plotValue == DataTable::BLINK_STATE) {
                customPlot->yAxis->setRange(-0.1, 1.1);
            } else if (plotValue == DataTable::EYE_MOVEMENT_TYPE) {
                customPlot->yAxis->setRange(-0.5, 4.5);
            } else if (plotValue == DataTable::INTENTION_LABEL) {
                customPlot->yAxis->setRange(-0.5, 6.5);
            } else if (plotValue == DataTable::FATIGUE_LEVEL || plotValue == DataTable::COGNITIVE_WORKLOAD) {
                customPlot->yAxis->setRange(-0.5, 4.5);
            } else {
                // 使用智能扩展缩放，避免Y轴波动
                if(should_plot) {
                    updateYAxisRangeExpansive(value_to_plot);
                }
            }
        }
        if (key > 15.0) {
            customPlot->xAxis->setRange(key - 15.0, key);
        } else {
            customPlot->xAxis->setRange(0, 15.0);
        }
    }

    if(customPlot->graph(0)->dataCount() > 0 && key - customPlot->graph()->dataMainKey(0) > 240) {
        customPlot->graph(0)->data()->removeBefore(key - 120);
    }

    customPlot->replot();
}

// 新增：接收扩展特征数据的槽函数
void GraphPlot::appendExtendedFeatureData(const QString &featureName, double value, qint64 timestamp) {
    // 只处理与当前plotValue匹配的特征
    if (featureName != plotValue) {
        return;
    }

    // 使用节流阀控制更新频率
    if (timer.elapsed() < updateDelay) {
        return;
    }

    // 设置时间戳基准
    if (sharedTimestamp == 0) {
        sharedTimestamp = timestamp;
    }

    uint64 m_timestamp = timestamp - sharedTimestamp;
    double key = m_timestamp / 1000.0; // 转换为秒

    // 添加数据点
    customPlot->graph(0)->addData(key, value);

    // 重绘图表
    timer.restart();
    if (!interaction) {
        if (!yinteraction) {
            // 根据特征类型设置Y轴范围
            if (featureName == "视觉效率" || featureName == "扫描模式" || featureName == "搜索效率" ||
                featureName == "交互准备度") {
                customPlot->yAxis->setRange(-0.1, 1.1);
            } else if (featureName == DataTable::INTENTION_LABEL) {
                customPlot->yAxis->setRange(-0.5, 6.5);
            } else if (featureName == DataTable::FATIGUE_LEVEL || featureName == DataTable::COGNITIVE_WORKLOAD) {
                customPlot->yAxis->setRange(-0.5, 4.5);
            } else if (featureName == "重访率(%)") {
                customPlot->yAxis->setRange(0, 100);
            } else if (featureName == "多任务能力" || featureName == "注意力分配" ||
                      featureName == "界面熟悉度" || featureName == "操作熟练度") {
                customPlot->yAxis->setRange(1, 10);
                // 瞎编特征的Y轴设置已删除（微扫视计数、角速度）
            } else {
                // 使用智能扩展缩放，避免Y轴波动
                updateYAxisRangeExpansive(value);
            }
        }

        // 设置X轴滚动范围
        if (key > 15.0) {
            customPlot->xAxis->setRange(key - 15.0, key);
        } else {
            customPlot->xAxis->setRange(0, 15.0);
        }
    }

    // 清理旧数据
    if (customPlot->graph(0)->dataCount() > 0 && key - customPlot->graph(0)->dataMainKey(0) > 240) {
        customPlot->graph(0)->data()->removeBefore(key - 120);
    }

    customPlot->replot();
}

// 【新增】智能Y轴范围更新函数 - 只扩展不缩小，避免图表波动
void GraphPlot::updateYAxisRangeExpansive(double value) {
    if (!m_yRangeInitialized) {
        // 首次初始化 - 使用当前数据范围作为起点
        bool found;
        QCPRange currentRange = customPlot->graph(0)->getValueRange(found);
        if (found && currentRange.size() > 0) {
            m_currentMinY = currentRange.lower;
            m_currentMaxY = currentRange.upper;
        } else {
            // 如果没有数据，使用合理的默认范围
            m_currentMinY = value - abs(value) * 0.1 - 1;
            m_currentMaxY = value + abs(value) * 0.1 + 1;
        }
        m_yRangeInitialized = true;
        customPlot->yAxis->setRange(m_currentMinY, m_currentMaxY);
        return;
    }

    // 检查是否需要扩展范围
    bool needUpdate = false;
    const double margin = 0.05; // 5% 边距

    if (value < m_currentMinY) {
        // 需要向下扩展
        double expansion = abs(value - m_currentMinY) * (1 + margin);
        m_currentMinY = value - expansion;
        needUpdate = true;
    }

    if (value > m_currentMaxY) {
        // 需要向上扩展
        double expansion = abs(value - m_currentMaxY) * (1 + margin);
        m_currentMaxY = value + expansion;
        needUpdate = true;
    }

    // 只有需要扩展时才更新Y轴
    if (needUpdate) {
        customPlot->yAxis->setRange(m_currentMinY, m_currentMaxY);
    }
}



