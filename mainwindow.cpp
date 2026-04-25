#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QGuiApplication>
#include <QScreen>
#include <cstring>  // For memcpy

// 包含在.h中前向声明的头文件
#include "showcal.h"
#include "worker.h"
#include "eyetrackingdialog.h"
#include "showpoint.h"
#include "PupilDetectionMethod.h"
#include "PupilTracker.h"
#include "PuReST.h"
#include "PupilTrackingMethod.h"
#include "FrameGrabber.h"
#include "caliberror.h"
#include "eyetrackingcalibrator.h"
#include "directshowopencvcamera.h"
#include "aroverlay.h"
#include "Pupil.h"
#include "dataTable.h"
#include "graphPlot.h"
#include <cstdlib>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <QMessageBox>
#include <QTime>
#include <QDateTime>
#include <QList>
#include <QtCore/qmath.h>
#include "showpoint.h"
#include <QApplication>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QDebug>
#include "ElSe.h"
#include "PuRe.h"
#include "utils.h"
#include "eyeimageprocessor.h"
#include "PuReST.h"
#include "qthread.h"
#include <qdebug.h>
#include <chrono>
#include<calibDlg.h>
#include<windows.h>
#include<testresultdlg.h>
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/calib3d.hpp>
#include <iostream>
#include <QMenuBar>
#include <QMenu>
#include <QSet>
#include<math.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>

#include <random>
#include <ctime>
#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include"calibpointdlg.h"
#include <QDesktopWidget>

#include "directshowopencvcamera.h"
#include "opencvimagecallback.h"

#include <QSplitter>
#include <QFormLayout>
#include <QGroupBox>
#include <QStatusBar>
#include <QDockWidget>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QTabWidget>
#include <QPainter>
#include <QHBoxLayout>
#include <QVBoxLayout>


using namespace tbb;
using namespace std;
using namespace cv;
using namespace aruco;




MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
  , ui(new Ui::MainWindow)
    , workerThread(nullptr)
    , worker(nullptr)
    , m_currentPoint(0)
    , m_recording(true)
    , m_gazeCount(0)
    , m_maxGazePerPoint(10)
    , m_calibError(nullptr)
    , m_arOverlayEnabled(false)           // AR覆盖层是否启用
    , m_arOverlayWidget(nullptr)          // AR覆盖层窗口指针
    , m_lastHeadPosition(0, 0, 0)         // 上次头部位置
    , m_lastHeadRotation(0, 0, 0)         // 上次头部旋转
    , m_arOpacity(0.6)                    // AR透明度
    , m_arUpdateTimer(nullptr)            // AR更新定时器指针
    , m_lastAttractedGaze(0, 0)           // 上次吸附注视点位置
    , m_useAttractedGaze(false)           // 是否使用吸附注视功能
    , m_attractionActive(false)           // 吸附功能是否激活
    , m_rawGazeBeforeCorrection(0, 0)     // 校正前原始坐标初始化
    , m_gazeCorrectionEnabled(false)      // 视线校正是否启用
    , m_correctionSampleCount(0)          // 校正样本数量
    , m_averageOffset(0, 0)               // 平均偏差初始化
    , m_systematicErrorOffset(0, 0)       // 系统性误差偏移初始化
    , m_systematicErrorCorrectionEnabled(false)  // 系统性误差校正初始化
    , m_cumulativeErrorOffset(0, 0)      // 累积误差偏移初始化
    , m_cumulativeErrorCorrectionEnabled(false)  // 累积误差校正初始化
    , m_finalErrorOffset(0, 0)            // 最终误差偏置初始化
    , m_useFixedErrorOffset(false)        // 初始不使用固定偏置
    , m_firstCalibErrorCycleCompleted(false) // 第一轮未完成
    , m_freeGazeMode(false)               // T模式初始化为关闭
    , m_currentFreeGaze(0, 0)             // 当前自由注视点初始化
    , m_gazeTrailTimer(nullptr)           // 轨迹更新定时器初始化
    , m_lastGazeTime(0)                   // 上次注视时间初始化
    , m_dataTable(nullptr)                // DataTable初始化
    , m_dataSplitter(nullptr)             // 数据分析Split窗口初始化
    , m_dataTabWidget(nullptr)            // GraphPlot Tab容器初始化
    , m_dataDock(nullptr)                 // 数据分析停靠窗口初始化
    , m_graphTabDock(nullptr)             // GraphPlot Tab Dock初始化
    , m_sceneDock(nullptr)                // 场景图像窗口初始化
    , m_eyeDock(nullptr)                  // 眼睛图像窗口初始化
    , m_controlsDock(nullptr)             // 控件窗口初始化
    , m_cameraDock(nullptr)               // 摄像头控制窗口初始化
    , m_viewMenu(nullptr)                 // 视图菜单初始化
    , m_frameCount(0)                     // Frame计数器初始化
    , m_fpsFrameCount(0)                  // FPS计数器初始化
    , m_processingFrameCount(0)           // Processing FPS计数器初始化
    , m_selectedEyeCameraIndex(-1)        // 眼球摄像头索引初始化
    , m_selectedSceneCameraIndex(-1)      // 场景摄像头索引初始化
    , m_camerasInitialized(false)         // 摄像头初始化状态
    , m_advancedCameraTree(nullptr)       // 专业相机控制树形控件
    , m_advancedCameraGroupBox(nullptr)   // 专业相机控制分组框
    , m_exposureButton(nullptr)           // 曝光控制按钮
    , m_exposureForm(nullptr)             // 曝光控制表单
    , m_cameraAdapter(nullptr)            // 相机设置适配器
    , m_previewCamera(nullptr)             // DirectShow预览摄像头初始化
    , m_previewTimer(nullptr)             // 预览定时器初始化
    , m_cameraGroupBox(nullptr)           // 摄像头控件组初始化
    , m_eyeCameraComboBox(nullptr)        // 眼球摄像头下拉框初始化
    , m_sceneCameraComboBox(nullptr)      // 场景摄像头下拉框初始化
    , m_eyeCameraResComboBox(nullptr)     // 眼球摄像头分辨率下拉框初始化
    , m_eyeCameraFpsComboBox(nullptr)     // 眼球摄像头帧率下拉框初始化
    , m_sceneCameraResComboBox(nullptr)   // 场景摄像头分辨率下拉框初始化
    , m_sceneCameraFpsComboBox(nullptr)   // 场景摄像头帧率下拉框初始化
    , m_eyeCameraPreviewBtn(nullptr)      // 眼球摄像头预览按钮初始化
    , m_sceneCameraPreviewBtn(nullptr)    // 场景摄像头预览按钮初始化
    , m_refreshCamerasBtn(nullptr)        // 刷新设备按钮初始化
    , m_applyCameraSettingsBtn(nullptr)   // 应用设置按钮初始化
    , m_cameraStatusLabel(nullptr)        // 摄像头状态标签初始化
    , m_imageSizeSlider(nullptr)          // 图像尺寸滑块初始化
    , m_imageSizeLabel(nullptr)           // 图像尺寸标签初始化
    , m_currentImageWidth(400)            // 默认图像宽度400
    , m_currentImageHeight(300)           // 默认图像高度300
    , m_currentCropOffset(0, 0)           // 截图偏移量初始化
    , m_dualGlintMapping()                // 双亮斑映射结构初始化
    , pupilCenter(-1, -1)                // 瞳孔中心初始化
    , m_is3DZeroCalibrated(false)
    , m_zeroAzimuthOffset(0.0f)
    , m_zeroElevationOffset(0.0f)
    , m_fake3D_zeroPitchOffset(0.0f)
    , m_fake3D_zeroYawOffset(0.0f)
    , m_fake3D_isCalibrated(false)
    , m_fake3D_multiPointCalibrated(false)
    , m_3DRegionalCorrectionEnabled(false) // 3D区域校正开关
    , m_is3DCalibrated(false) // 3D标定状态初始化
    , glintsDetected(false)              // 亮斑检测状态初始化
    , glint1_x(-1), glint1_y(-1)         // 亮斑1坐标初始化
    , glint2_x(-1), glint2_y(-1)         // 亮斑2坐标初始化
    , m_gazeOverlay(nullptr) // 初始化为空指针
    , showPoint(nullptr)    // ShowPoint指针初始化
    , m_grabber(nullptr)    // FrameGrabber指针初始化
    , m_grabber1(nullptr)   // 第二个FrameGrabber指针初始化
    , calWin(nullptr)       // ShowCal指针初始化
    , pupilTrackingMethod(nullptr)    // 瞳孔跟踪方法初始化
    , pupilDetectionMethod(nullptr)   // 瞳孔检测方法初始化
    , glintTrackingMethod(nullptr)    // 亮斑跟踪方法初始化
    , m_dialog(nullptr)     // EyeTrackingDialog指针初始化
    , m_calibrator(nullptr) // EyeTrackingCalibrator指针初始化
    // 实验范式相关初始化已清空
    // 保留视线数据存储（被其他功能使用）
    // Shared memory initialization
#ifdef _WIN32
    , hImageMapping(nullptr)
    , hGazeMapping(nullptr)
    , hResultsMapping(nullptr)
#else
    , shm_image_fd(-1)
    , shm_gaze_fd(-1)
    , shm_results_fd(-1)
#endif
    , shm_image_ptr(nullptr)
    , shm_gaze_ptr(nullptr)
    , shm_results_ptr(nullptr)
    , m_sharedMemoryEnabled(false)
    , m_aiResultsTimer(nullptr)
    , m_aiStatusDock(nullptr)
    , m_voiceDisplayEdit(nullptr)
    , m_aiResponseDisplayEdit(nullptr)
    , m_detectionInfoLabel(nullptr)
    , m_targetDetectionButton(nullptr)
    , m_modeStatusLabel(nullptr)
    , m_modeAButton(nullptr)
    , m_modeSButton(nullptr)
    , m_modeDButton(nullptr)
    , m_modeFButton(nullptr)
    , m_aiResultsConnected(false)
#ifdef _WIN32
    , hModeControlMapping(nullptr)
    , shm_mode_control_ptr(nullptr)
#else
    , shm_mode_control_fd(-1)
    , shm_mode_control_ptr(nullptr)
#endif
    , m_modeControlConnected(false)
{
    // ✅ 获取主屏幕信息
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    QRect screenGeometry = primaryScreen->availableGeometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();
    qreal devicePixelRatio = primaryScreen->devicePixelRatio();  // 获取 DPI 缩放比例
    // 获取可用桌面大小
//    QRect deskRect = desktopWidget->availableGeometry();
    // 获取设备屏幕大小

    num = 0;
    gvar = 1;
    gmark = 0;
    unknowns = 4;
    plType = POLY_1_X_Y_XY;

    monoRightCalibrated = false;
    CALIBRATIONPOINTS9 = 1;

    // 初始化双亮斑映射系统
    m_dualGlint_pupilCenters.clear();
    m_dualGlint_glint1Positions.clear();
    m_dualGlint_glint2Positions.clear();
    m_dualGlint_screenTargets.clear();
    m_dualGlint_hasGlint1.clear();
    m_dualGlint_hasGlint2.clear();
    m_useDualGlintMapping = true;  // 默认启用双亮斑映射

//    qDebug() << "[DualGlint] 双亮斑映射系统初始化完成，状态:" << (m_useDualGlintMapping ? "启用" : "禁用");

    // ========== 初始化 Worker 线程系统 ==========
    setupWorkerThread();

    // 初始化角膜亮斑检测器，针对15-40像素直径的亮斑优化
    // 直径15像素 ≈ 面积177，直径40像素 ≈ 面积1256
    // 使用180-1300的面积范围，圆形度0.6，阈值比0.8
//    initGlintDetector(180, 1300, 0.6f, 0.8f);

    // 延迟初始化AR功能，避免在构造函数中抛出异常
//    QTimer::singleShot(1000, this, [this]() {
//        try {
//            initializeVirtualScreen();
//            createAROverlay();
//            // 不自动启用，让用户按A键手动启用
//            qDebug() << "AR功能初始化完成，按A键启用";
//        } catch (const std::exception& e) {
//            qDebug() << "AR延迟初始化异常:" << e.what();
//        } catch (...) {
//            qDebug() << "AR延迟初始化发生未知异常";
//        }
//    });

    m_isDestroying= false;
    flag_calibration_status = 1;

    // 安全初始化3D注视计算器，不影响现有功能
    try {
        m_gaze3DCalculator = std::unique_ptr<Gaze3DCalculator>(new Gaze3DCalculator());
        m_currentEyeSphereCenter = cv::Point2f(320, 240); // 默认屏幕中心
        m_eyeCenterHistory.clear(); // 初始化眼球中心历史
        m_eyeCenterHistory.reserve(MAX_EYE_CENTER_HISTORY); // 预分配内存

        // 【关键修复】基于实际角度数据初始化3D屏幕映射参数
        m_maxAzimuthDeg = 120.0f;    // ±120度水平范围（容纳±94度）
        m_maxElevationDeg = 100.0f;  // ±100度垂直范围（容纳±73度）

        // 初始化3D标定系统
        m_3DCalibrated = false;
        m_referenceGazeDirection = cv::Point3f(0, 0, -1);  // 默认向前
        m_referenceSphereCenter = cv::Point3f(0, 0, 0);
        m_calibrationDirections.clear();
        m_calibrationScreenPoints.clear();

//        qDebug() << "3D注视计算器初始化成功";
//        qDebug() << QString("3D映射范围: 水平±%1°, 垂直±%2°").arg(m_maxAzimuthDeg).arg(m_maxElevationDeg);
//        qDebug() << "3D标定状态: 未标定，请注视屏幕中心并按空格键进行参考点标定";
    } catch (...) {
//        qDebug() << "3D注视计算器初始化失败，使用2D模式";
        m_gaze3DCalculator.reset();
    }

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenRect = screen->availableGeometry();
    int w = screenRect.width();
    int h = screenRect.height();
    m_screenWidth = w;
    m_screenHeight = h;






    // 初始化标定图像（白色背景）
//    m_calibrationImage = QImage(m_screenWidth, m_screenHeight, QImage::Format_RGB32);
//    m_calibrationImage.fill(Qt::white);



//    qDebug() << "width:" << m_screenWidth << "----" << "height:" << m_screenHeight << endl;

    //this->setFixedSize(width, height);0

    // 保持原始逻辑，不做特殊处理

    ui->setupUi(this);


    // 【关键】不在构造函数中处理显示，交给main.cpp控制

    // 最后一步：恢复窗口状态 - 延迟到所有初始化完成后
    QTimer::singleShot(0, this, [this]() {
        QSettings settings("EyeTracker", "MainWindow");
//        qDebug() << "==========================================";
//        qDebug() << "【恢复】设置文件位置:" << settings.fileName();

        QByteArray geometry = settings.value("geometry").toByteArray();
        QByteArray windowState = settings.value("windowState").toByteArray();

//        qDebug() << "【恢复】几何数据大小:" << geometry.size() << "字节";
//        qDebug() << "【恢复】窗口状态数据大小:" << windowState.size() << "字节";
//        qDebug() << "【恢复前】窗口位置:" << pos() << "尺寸:" << size();

        if (!geometry.isEmpty()) {
            bool success = restoreGeometry(geometry);
//            qDebug() << "【恢复后】窗口位置:" << pos() << "尺寸:" << size();
//            qDebug() << "【恢复】几何恢复成功:" << success;
        } else {
//            qDebug() << "【恢复】没有几何数据，使用默认";
            // ✅ 基于屏幕比例设置
            int minW = static_cast<int>(m_screenWidth * 0.5);  // 最小宽度为屏幕 50%
            int minH = static_cast<int>(m_screenHeight * 0.5);
            this->setMinimumSize(minW, minH);

            // 初始窗口大小为屏幕的 70%
            this->resize(static_cast<int>(m_screenWidth * 0.7),
                         static_cast<int>(m_screenHeight * 0.7));
        }

        if (!windowState.isEmpty()) {
            bool success = restoreState(windowState);
//            qDebug() << "【恢复】窗口状态恢复成功:" << success;

            // 【修复】强制显示数据分析窗口，避免被隐藏
            if (m_dataDock) {
                m_dataDock->show();
                m_dataDock->raise();
//                qDebug() << "【修复】强制显示数据分析窗口";

                // 【修复】刷新数据表格显示，修复行高问题
                if (m_dataTable) {
                    m_dataTable->updateGeometry();
                    m_dataTable->repaint();
//                    qDebug() << "【修复】刷新数据表格显示";
                }
            }

            // 【新增】延迟恢复dock尺寸，确保布局稳定后再应用
            QTimer::singleShot(100, this, [this]() {
                // 恢复保存的dock尺寸
                QSettings dockSettings("EyeTracker", "MainWindow");

                // 恢复各个dock的宽度
                if (m_dataDock && dockSettings.contains("dataDockWidth")) {
                    int width = dockSettings.value("dataDockWidth").toInt();
                    int height = dockSettings.value("dataDockHeight", 600).toInt();  // 添加高度恢复
                    int currentScreenWidth = QGuiApplication::primaryScreen()->availableGeometry().width();

                    if (m_dataDock && dockSettings.contains("dataDockWidth")) {
                        int width = dockSettings.value("dataDockWidth").toInt();
                        int height = dockSettings.value("dataDockHeight", 600).toInt();

                        // 限制最大宽度为屏幕宽度的 50%，防止越界
                        if (width > 100 && width < currentScreenWidth * 0.5) {
                            resizeDocks({m_dataDock}, {width}, Qt::Horizontal);
                        }
                        // 高度同理
                        if (height > 200 && height < QGuiApplication::primaryScreen()->availableGeometry().height() * 0.8) {
                            resizeDocks({m_dataDock}, {height}, Qt::Vertical);
                        }
                    }
                }

                if (m_controlsDock && dockSettings.contains("controlsDockWidth")) {
                    int width = dockSettings.value("controlsDockWidth").toInt();
                    int height = dockSettings.value("controlsDockHeight", 400).toInt();  // 添加高度恢复
                    if (width > 100) {
                        resizeDocks({m_controlsDock}, {width}, Qt::Horizontal);
                        // 【新增】同时恢复高度，避免累积变化
                        if (height > 200) {  // 确保高度合理
                            resizeDocks({m_controlsDock}, {height}, Qt::Vertical);
                        }
//                        qDebug() << "【恢复】控件窗口宽度:" << width;
                    }
                }

                if (m_cameraDock && dockSettings.contains("cameraDockWidth")) {
                    int width = dockSettings.value("cameraDockWidth").toInt();
                    int height = dockSettings.value("cameraDockHeight", 300).toInt();  // 添加高度恢复
                    if (width > 100) {
                        resizeDocks({m_cameraDock}, {width}, Qt::Horizontal);
                        // 【新增】同时恢复高度，避免累积变化
                        if (height > 200) {  // 确保高度合理
                            resizeDocks({m_cameraDock}, {height}, Qt::Vertical);
                        }
//                        qDebug() << "【恢复】摄像头控制窗口宽度:" << width;
                    }
                }

                if (m_aiStatusDock && dockSettings.contains("aiStatusDockWidth")) {
                    int width = dockSettings.value("aiStatusDockWidth").toInt();
                    int height = dockSettings.value("aiStatusDockHeight", 400).toInt();
                    if (width > 100) {
                        resizeDocks({m_aiStatusDock}, {width}, Qt::Horizontal);
                        if (height > 200) {
                            resizeDocks({m_aiStatusDock}, {height}, Qt::Vertical);
                        }
//                        qDebug() << "【恢复】AI状态窗口宽度:" << width;
                    }
                }

                if (m_graphTabDock && dockSettings.contains("graphTabDockWidth")) {
                    int width = dockSettings.value("graphTabDockWidth").toInt();
                    int height = dockSettings.value("graphTabDockHeight", 400).toInt();  // 添加高度恢复
                    if (width > 100) {
                        resizeDocks({m_graphTabDock}, {width}, Qt::Horizontal);
                        // 【新增】同时恢复高度，避免累积变化
                        if (height > 200) {  // 确保高度合理
                            resizeDocks({m_graphTabDock}, {height}, Qt::Vertical);
                        }
//                        qDebug() << "【恢复】图表分析窗口宽度:" << width;
                    }
                }

//                qDebug() << "【恢复】所有dock尺寸恢复完成";

                // 【修复】延迟刷新数据表格和所有dock，修复字体和行高问题
                QTimer::singleShot(50, this, [this]() {
                    // 修复数据表格的字体和行高问题
                    if (m_dataTable) {
                        // 重置表格的字体，防止DPI缩放影响
                        QFont tableFont("Arial", 10); // 与全局字体保持一致
                        m_dataTable->setFont(tableFont);

                        m_dataTable->adjustSize();
                        m_dataTable->updateGeometry();
                        m_dataTable->update();
//                        qDebug() << "【修复】最终刷新数据表格布局和字体";
                    }

                    // 字体已在各dock创建时统一设置，无需重复设置

//                    qDebug() << "【修复】所有dock字体和布局已重置";

                    // 【修复完成】所有初始化操作已完成，窗口准备就绪
//                    qDebug() << "【优化】窗口初始化完成，统一显示";
                });
            });
        } else {
            // 没有保存的窗口状态时，直接显示
            QTimer::singleShot(50, this, [this]() {
                this->show();
                this->raise();
                this->activateWindow();
            });
        }
//        qDebug() << "==========================================";
    });

    // ==================== 【新增】专业商业级界面主题 ====================
    // 设置现代化专业主题样式
    this->setStyleSheet(
        "/* 主窗口样式 */"
        "QMainWindow {"
        "    background-color: #F5F5F5;"
        "    color: #333333;"
        "}"

        "/* 工具栏样式 */"
        "QToolBar {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #FFFFFF, stop:1 #F0F0F0);"
        "    border: 1px solid #CCCCCC;"
        "    spacing: 3px;"
        "    padding: 2px;"
        "}"

        "/* 菜单栏样式 */"
        "QMenuBar {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #FFFFFF, stop:1 #F8F8F8);"
        "    border-bottom: 1px solid #CCCCCC;"
        "    padding: 2px;"
        "}"
        "QMenuBar::item {"
        "    background: transparent;"
        "    padding: 4px 8px;"
        "    margin: 2px;"
        "    border-radius: 4px;"
        "}"
        "QMenuBar::item:selected {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #E3F2FD, stop:1 #BBDEFB);"
        "    color: #1976D2;"
        "}"

        "/* 按钮通用样式 */"
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #FFFFFF, stop:1 #F0F0F0);"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 6px;"
        "    padding: 6px 12px;"
        "    font-weight: bold;"
        "    min-width: 60px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #E3F2FD, stop:1 #BBDEFB);"
        "    border-color: #2196F3;"
        "    color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #BBDEFB, stop:1 #90CAF9);"
        "    border-color: #1976D2;"
        "}"

        "/* 标签页样式 */"
        "QTabWidget::pane {"
        "    border: 1px solid #CCCCCC;"
        "    background-color: #FFFFFF;"
        "    border-radius: 6px;"
        "}"
        "QTabBar::tab {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #F8F8F8, stop:1 #E0E0E0);"
        "    border: 1px solid #CCCCCC;"
        "    padding: 8px 16px;"
        "    margin-right: 2px;"
        "    border-radius: 6px 6px 0px 0px;"
        "}"
        "QTabBar::tab:selected {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #4CAF50, stop:1 #388E3C);"
        "    color: white;"
        "    font-weight: bold;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #E8F5E8, stop:1 #C8E6C9);"
        "}"

        "/* 分组框样式 */"
        "QGroupBox {"
        "    font-family: 'Microsoft YaHei';"
        "    font-weight: bold;"
        "    font-size: 10px;"
        "    border: 2px solid #CCCCCC;"
        "    border-radius: 8px;"
        "    margin-top: 20px;"
        "    padding-top: 15px;"
        "    background-color: #FAFAFA;"
        "    color: #333333;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 15px;"
        "    top: -12px;"
        "    padding: 4px 12px 4px 12px;"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border-radius: 8px;"
        "    font-size: 10px;"
        "    font-weight: bold;"
        "    font-family: 'Microsoft YaHei';"
        "    min-width: 100px;"
        "    border: 1px solid #388E3C;"
        "}"

        "/* 下拉框样式 */"
        "QComboBox {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    background-color: #FFFFFF;"
        "    min-width: 80px;"
        "}"
        "QComboBox:hover {"
        "    border-color: #2196F3;"
        "    background-color: #F3F9FF;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #F0F0F0, stop:1 #E0E0E0);"
        "    border-radius: 3px;"
        "}"

        "/* 状态栏样式 */"
        "QStatusBar {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #F8F8F8, stop:1 #E8E8E8);"
        "    border-top: 1px solid #CCCCCC;"
        "    color: #555555;"
        "}"

        "/* 右键菜单样式 */"
        "QMenu {"
        "    background-color: #FFFFFF;"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 4px;"
        "    font-family: 'Microsoft YaHei';"
        "    font-size: 11px;"
        "    color: #333333;"
        "}"
        "QMenu::item {"
        "    background-color: transparent;"
        "    padding: 6px 20px 6px 30px;"
        "    color: #333333;"
        "    font-size: 11px;"
        "    min-height: 18px;"
        "}"
        "QMenu::item:selected {"
        "    background-color: #E3F2FD;"
        "    color: #1976D2;"
        "}"
        "QMenu::item:checked {"
        "    background-color: #E8F5E8;"
        "    color: #2E7D32;"
        "}"
        "QMenu::indicator {"
        "    width: 16px;"
        "    height: 16px;"
        "    left: 6px;"
        "}"
        "QMenu::indicator:checked {"
        "    image: none;"
        "    background-color: #4CAF50;"
        "    border-radius: 8px;"
        "}"
        "QMenu::separator {"
        "    height: 1px;"
        "    background-color: #E0E0E0;"
        "    margin: 4px 8px;"
        "}"
    );

    // 设置窗口图标和标题
    this->setWindowTitle("眼动追踪系统 - 专业版 v2.0");
    QFont mainTitleFont("Microsoft YaHei", 12, QFont::Bold);
    this->setFont(mainTitleFont);
//    qDebug() << "[INFO] 专业商业级界面主题加载完成";

    // ==================== 【新增】专业控件图标已在setupUIButtonIcons中设置 ====================

    // ==================== 【新增】初始化摄像头管理系统 ====================
    // 初始化摄像头相关变量
    m_availableCameras.clear();
    // m_previewCamera 使用 unique_ptr 自动初始化为 nullptr
    m_previewTimer = new QTimer(this);
    connect(m_previewTimer, &QTimer::timeout, this, &MainWindow::updateCameraPreview);

    // 摄像头选择控件将在创建控件窗口时添加
//    qDebug() << "摄像头管理系统初始化完成";
    // ================================================================

    // ==================== 应用专业化样式表 ====================
    QString styleSheetPath = QApplication::applicationDirPath() + "/professional_style.qss";
    QFile styleFile(styleSheetPath);
    if (!styleFile.exists()) {
        // 如果外部文件不存在，使用内嵌样式
        loadProfessionalStyle();
    } else {
        styleFile.open(QFile::ReadOnly);
        QString styleSheet = styleFile.readAll();
        this->setStyleSheet(styleSheet);
        styleFile.close();
    }

    // 创建菜单栏并添加菜单
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // 设置窗口标题和图标（在菜单栏创建后）
    this->setWindowTitle("专业眼动追踪分析系统 - EyeTracking Pro v2.0");
    QFont secondaryTitleFont("Microsoft YaHei", 12, QFont::Bold);
    this->setFont(secondaryTitleFont);

    // 【终极解决方案】尝试多种图标格式和方法
//    qDebug() << "=== 开始设置窗口图标 ===";

    // 方法1: 使用Windows原生ICO格式
    QString icoPath = ":/icons/eye.ico";
    QIcon icoIcon(icoPath);
    if (!icoIcon.isNull()) {
//        qDebug() << "✓ ICO图标加载成功:" << icoPath;
        this->setWindowIcon(icoIcon);
        QApplication::setWindowIcon(icoIcon);

        // 验证图标是否真的设置成功
        QIcon currentIcon = this->windowIcon();
//        qDebug() << "ICO图标设置后验证 - 图标为空:" << currentIcon.isNull();
        if (!currentIcon.isNull()) {
//            qDebug() << "ICO图标可用尺寸:" << currentIcon.availableSizes();
        }
    } else {
//        qDebug() << "✗ ICO图标加载失败";
    }

    // 方法2: 使用PNG格式作为备选
    QString pngPath = ":/icons/eye.png";
    QIcon pngIcon(pngPath);
    if (!pngIcon.isNull()) {
//        qDebug() << "✓ PNG图标加载成功:" << pngPath;
        if (icoIcon.isNull()) { // 只有ICO失败时才用PNG
            this->setWindowIcon(pngIcon);
            QApplication::setWindowIcon(pngIcon);
//            qDebug() << "使用PNG图标作为备选方案";
        }
    } else {
//        qDebug() << "✗ PNG图标加载失败";
    }

    // 方法3: SVG方案（如果以上都失败）
    QString svgPath = ":/icons/eye.png";
    QIcon svgIcon(svgPath);
    if (!svgIcon.isNull() && icoIcon.isNull() && pngIcon.isNull()) {
//        qDebug() << "✓ SVG图标加载成功，作为最后备选:" << svgPath;
        QIcon finalSvgIcon;
        finalSvgIcon.addPixmap(svgIcon.pixmap(16, 16));
        finalSvgIcon.addPixmap(svgIcon.pixmap(32, 32));
        this->setWindowIcon(finalSvgIcon);
        QApplication::setWindowIcon(finalSvgIcon);
    }

    // 【关键调试】检查窗口属性和系统信息
//    qDebug() << "=== 窗口图标调试信息 ===";
//    qDebug() << "当前窗口句柄:" << (void*)this->winId();
//    qDebug() << "窗口标志:" << this->windowFlags();
//    qDebug() << "窗口状态:" << this->windowState();
//    qDebug() << "是否可见:" << this->isVisible();

    // 最终验证
    QIcon finalIcon = this->windowIcon();
//    qDebug() << "最终设置的图标是否为空:" << finalIcon.isNull();
    if (!finalIcon.isNull()) {
//        qDebug() << "最终图标可用尺寸:" << finalIcon.availableSizes().size();
    }
//    qDebug() << "=== 窗口图标设置完成 ===";
    // 创建专业化菜单
    QMenu *fileMenu = menuBar->addMenu(tr("文件"));
    QMenu *calibrationMenu = menuBar->addMenu(tr("校准"));
    QMenu *testMenu = menuBar->addMenu(tr("测试"));
    m_viewMenu = menuBar->addMenu(tr("视图"));
    QMenu *calibrationModeMenu = menuBar->addMenu(tr("校准模式"));

    // 为菜单设置图标（使用本地文件）
    /*QString appDir = QApplication::applicationDirPath();

    QString fileIconPath = ":/icons/iconSet/settings.png";
    if (QFile::exists(fileIconPath)) {
        fileMenu->setIcon(QIcon(fileIconPath));
    }

    QString calibrateIconPath = ":/icons/iconSet/misc.png";
    if (QFile::exists(calibrateIconPath)) {
        calibrationMenu->setIcon(QIcon(calibrateIconPath));
    }

    QString viewIconPath = ":/icons/iconSet/resolution.png";
    if (QFile::exists(viewIconPath)) {
        m_viewMenu->setIcon(QIcon(viewIconPath));
    }*/

    // 文件菜单动作（保持按钮名称和功能）
    QAction *openCamAction = new QAction(ui->Markcalib->text(), this);
    connect(openCamAction, &QAction::triggered, this, &MainWindow::on_Markcalib_clicked);
    fileMenu->addAction(openCamAction);

    QAction *collectCalibImgAction = new QAction(ui->calibImgCollectBtn->text(), this);
    connect(collectCalibImgAction, &QAction::triggered, this, &MainWindow::on_calibImgCollectBtn_clicked);
    fileMenu->addAction(collectCalibImgAction);

    QAction *reloadAction = new QAction(ui->ReloadButton->text(), this);
    connect(reloadAction, &QAction::triggered, this, &MainWindow::clickReloadButton);
    fileMenu->addAction(reloadAction);



    QAction *cameraAction = new QAction(ui->openButton->text(), this);
    connect(cameraAction, &QAction::triggered, this, &MainWindow::on_openButton_clicked);
    fileMenu->addAction(cameraAction);

    QAction *exitAction = new QAction(tr("退出"), this);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAction);

    // 校准菜单动作（保持按钮名称和功能）
    QAction *setOriginalAction = new QAction(ui->setOrignalBtn->text(), this);
    connect(setOriginalAction, &QAction::triggered, this, &MainWindow::on_setOrignalBtn_clicked);
    calibrationMenu->addAction(setOriginalAction);

    QAction *calibrateAction = new QAction(ui->pushButton->text(), this);
    connect(calibrateAction, &QAction::triggered, this, &MainWindow::clickCalibButton);
    calibrationMenu->addAction(calibrateAction);

    QAction *calibPointsAction = new QAction(ui->calibPointBtn->text(), this);
    connect(calibPointsAction, &QAction::triggered, this, &MainWindow::on_calibPointBtn_clicked);
    calibrationMenu->addAction(calibPointsAction);

    QAction *pointCalibAction = new QAction(ui->pointcalibButton_2->text(), this);
    connect(pointCalibAction, &QAction::triggered, this, &MainWindow::on_pointcalibButton_2_clicked);
    calibrationMenu->addAction(pointCalibAction);

    QAction *markerAction = new QAction(ui->MarkerBtn->text(), this);
    connect(markerAction, &QAction::triggered, this, &MainWindow::on_MarkerBtn_clicked);
    calibrationMenu->addAction(markerAction);

    // 测试菜单动作（保持按钮名称和功能）
    QAction *testAction = new QAction(ui->TestButton->text(), this);
    connect(testAction, &QAction::triggered, this, &MainWindow::on_TestButton_clicked);
    testMenu->addAction(testAction);

    QAction *testResultAction = new QAction(ui->TestResultButton->text(), this);
    connect(testResultAction, &QAction::triggered, this, &MainWindow::on_TestResultButton_clicked);
    testMenu->addAction(testResultAction);


    QAction *interact01Action = new QAction(ui->interact01->text(), this);
    connect(interact01Action, &QAction::triggered, this, &MainWindow::on_interact01_clicked);
    testMenu->addAction(interact01Action);




    // 视图菜单动作
    m_viewMenu->addAction(tr("误差测试窗口"), this, &MainWindow::showCalibrationWindow);

    // 添加图像窗口控制选项
    m_viewMenu->addSeparator();
    QAction *toggleSceneAction = m_viewMenu->addAction(tr("显示/隐藏场景图像"));
    connect(toggleSceneAction, &QAction::triggered, this, &MainWindow::onToggleSceneWindow);

    QAction *toggleEyeAction = m_viewMenu->addAction(tr("显示/隐藏眼睛图像"));
    connect(toggleEyeAction, &QAction::triggered, this, &MainWindow::onToggleEyeWindow);

    QAction *toggleControlsAction = m_viewMenu->addAction(tr("显示/隐藏控件面板"));
    connect(toggleControlsAction, &QAction::triggered, this, &MainWindow::onToggleControlsWindow);

    QAction *toggleCameraAction = m_viewMenu->addAction(tr("显示/隐藏摄像头控制"));
    connect(toggleCameraAction, &QAction::triggered, this, [this]() {
        if (m_cameraDock) {
            if (m_cameraDock->isVisible()) {
                m_cameraDock->hide();
//                qDebug() << "摄像头控制窗口已隐藏";
            } else {
                m_cameraDock->show();
                m_cameraDock->raise();
                m_cameraDock->setVisible(true);
//                qDebug() << "摄像头控制窗口已显示";
            }
        } else {
//            qDebug() << "摄像头控制窗口不存在，尝试创建";
            createCameraDockWindow();
        }
    });

    QAction *toggleAIStatusAction = m_viewMenu->addAction(tr("显示/隐藏AI分析状态"));
    connect(toggleAIStatusAction, &QAction::triggered, this, [this]() {
        if (m_aiStatusDock) {
            if (m_aiStatusDock->isVisible()) {
                m_aiStatusDock->hide();
                qDebug() << "AI状态窗口已隐藏";
            } else {
                m_aiStatusDock->show();
                m_aiStatusDock->raise();
                m_aiStatusDock->setVisible(true);
                qDebug() << "AI状态窗口已显示";
            }
        } else {
            qDebug() << "AI状态窗口不存在，尝试创建";
            createAIStatusDockWindow();
        }
    });

    // 添加数据窗口控制选项
    m_viewMenu->addSeparator();
    QAction *toggleDataTableAction = m_viewMenu->addAction(tr("显示/隐藏数据表格"));
    connect(toggleDataTableAction, &QAction::triggered, this, &MainWindow::onToggleDataTable);

    // 停靠相关菜单项已删除

    // 停靠功能的connect()实现已删除

    // 第二个停靠功能的connect()实现已删除

    // 第三个停靠功能的connect()实现已删除

    // 锁定数据分析窗口尺寸功能已删除

    // 解锁数据分析窗口尺寸功能已删除

    // 恢复数据分析窗口到dock模式功能已删除

    m_viewMenu->addSeparator();

    // 【新增】恢复默认布局
    QAction *restoreDefaultLayoutAction = m_viewMenu->addAction(tr("恢复默认窗口布局"));
    connect(restoreDefaultLayoutAction, &QAction::triggered, this, [this]() {
        // 恢复所有dock窗口的默认位置
        if (m_controlsDock) {
            removeDockWidget(m_controlsDock);
            addDockWidget(Qt::LeftDockWidgetArea, m_controlsDock);
        }
        if (m_cameraDock) {
            removeDockWidget(m_cameraDock);
            addDockWidget(Qt::LeftDockWidgetArea, m_cameraDock);
        }
        if (m_aiStatusDock) {
            removeDockWidget(m_aiStatusDock);
            addDockWidget(Qt::RightDockWidgetArea, m_aiStatusDock);
        }
        if (m_dataDock) {
            removeDockWidget(m_dataDock);
            addDockWidget(Qt::RightDockWidgetArea, m_dataDock);
        }
        if (m_graphTabDock) {
            removeDockWidget(m_graphTabDock);
            addDockWidget(Qt::RightDockWidgetArea, m_graphTabDock);
        }

        // 重新应用tabify设置
        if (m_dataDock && m_graphTabDock) {
            tabifyDockWidget(m_dataDock, m_graphTabDock);
            m_graphTabDock->raise(); // 激活图表分析tab以便记住大小
        }

//        qDebug() << "已恢复默认窗口布局";
    });

    QAction *showAllGraphsAction = m_viewMenu->addAction(tr("显示所有图表"));
    connect(showAllGraphsAction, &QAction::triggered, this, &MainWindow::onShowAllGraphPlots);

    QAction *hideAllGraphsAction = m_viewMenu->addAction(tr("隐藏所有图表"));
    connect(hideAllGraphsAction, &QAction::triggered, this, &MainWindow::onHideAllGraphPlots);

    m_viewMenu->addSeparator();
    QAction *restoreAllWindowsAction = m_viewMenu->addAction(tr("恢复所有窗口"));
    restoreAllWindowsAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(restoreAllWindowsAction, &QAction::triggered, this, &MainWindow::onRestoreAllWindows);

    QAction *saveSessionAction = m_viewMenu->addAction(tr("保存当前会话"));
    saveSessionAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(saveSessionAction, &QAction::triggered, this, [this]() {
        // 强制保存所有窗口的当前状态
//        qDebug() << "用户手动保存会话 - 强制保存所有窗口状态";

        // 使用exe上一层目录的统一文件路径
        QString exeDir = QCoreApplication::applicationDirPath();
        QDir dir(exeDir);
        dir.cdUp(); // 回到上一层目录
        QString settingsPath = dir.absolutePath() + "/EyeTracker_WindowConfig.ini";
        QDir().mkpath(QFileInfo(settingsPath).absolutePath());

        // 保存会话和窗口状态
        saveSessionState();
        saveWindowPositions();

        // 验证文件是否保存成功
        QFileInfo fileInfo(settingsPath);
        if (fileInfo.exists() && fileInfo.size() > 0) {
//            qDebug() << "会话状态保存完成，文件大小:" << fileInfo.size() << "字节";
            QMessageBox::information(this, tr("保存成功"),
                tr("当前会话状态已保存\n设置文件: %1\n文件大小: %2 字节").arg(settingsPath).arg(fileInfo.size()));
        } else {
            QMessageBox::warning(this, tr("保存失败"), tr("无法保存会话状态到文件:\n%1").arg(settingsPath));
        }
    });

    QAction *restoreSessionAction = m_viewMenu->addAction(tr("恢复上次会话"));
    restoreSessionAction->setShortcut(QKeySequence("Ctrl+Shift+R"));
    connect(restoreSessionAction, &QAction::triggered, this, [this]() {
        restoreSessionState();
        restoreWindowPositions();
        QMessageBox::information(this, tr("恢复成功"), tr("上次会话状态已恢复"));
    });

    m_viewMenu->addSeparator();
    QAction *debugWindowAction = m_viewMenu->addAction(tr("调试窗口状态"));
    debugWindowAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(debugWindowAction, &QAction::triggered, this, &MainWindow::debugWindowState);

    // 校准模式菜单动作（保持单选按钮名称和功能）
    // 在构造函数中初始化 yanshiAction, jingqueAction 和 gaze3DAction
    yanshiAction = new QAction(ui->yanshi_radioButton->text(), this);
    yanshiAction->setCheckable(true);
    jingqueAction = new QAction(ui->jingque_radioButton->text(), this);
    jingqueAction->setCheckable(true);
    gaze3DAction = new QAction("3D注视模式", this);
    gaze3DAction->setCheckable(true);

    QActionGroup *calibModeGroup = new QActionGroup(this);
    calibModeGroup->addAction(yanshiAction);
    calibModeGroup->addAction(jingqueAction);
    calibModeGroup->addAction(gaze3DAction);
    calibModeGroup->setExclusive(true);

    calibrationModeMenu->addAction(yanshiAction);
    calibrationModeMenu->addAction(jingqueAction);
    calibrationModeMenu->addAction(gaze3DAction);

    yanshiAction->setChecked(true); // 默认选中“演示”
    connect(calibModeGroup, &QActionGroup::triggered, this, &MainWindow::slots_calibration_status);

    // 隐藏原始按钮和单选按钮
    ui->pushButton->hide();
    ui->ReloadButton->hide();
    ui->calibPointBtn->hide();
    ui->setOrignalBtn->hide();
    ui->calibImgCollectBtn->hide();
    ui->openButton->hide();

    ui->TestButton->hide();
    ui->TestResultButton->hide();
    ui->Markcalib->hide();
    ui->pointcalibButton_2->hide();
    ui->MarkerBtn->hide();
    ui->yanshi_radioButton->hide();
    ui->jingque_radioButton->hide();

    // 为可见的UI按钮设置专业图标
    setupUIButtonIcons();

    // 设置中央小部件（相机视图）
    // 设置中央小部件（相机视图）

    setCentralWidget(new QWidget(this));


    ui->label_scene->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->label_eye->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 设置最小尺寸，防止缩得太小看不清，但允许变大
    // ✅ 使用缩放后的尺寸
    ui->label_scene->setMinimumSize(scaleValue(320), scaleValue(240));
    ui->label_eye->setMinimumSize(scaleValue(160), scaleValue(120));

    // ✅ 确保可以扩展
    ui->label_scene->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->label_eye->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // ✅ 保持内容缩放
    ui->label_scene->setScaledContents(true);
    ui->label_eye->setScaledContents(true);

    // 保持内容缩放（这行保留，非常重要）
    ui->label_scene->setScaledContents(true);
    ui->label_eye->setScaledContents(true);

    // 重点在这里：设置 QLabel 自动缩放其内容以适应自身大小
    ui->label_scene->setScaledContents(true); // 即使场景摄像头是640x480，设置它也没有坏处
    ui->label_eye->setScaledContents(true);   // 确保1920x1080的图像能完整显示在640x480的区域内
    ui->label_scene->setStyleSheet("border: 1px dashed black;");
    ui->label_eye->setStyleSheet("border: 1px dashed black;");
    ui->label_scene->setText(""); // 图像为空时清除文本
    ui->label_eye->setText(""); // 图像为空时清除文本


    // 创建图像Tab窗口
    QTabWidget *imageTabWidget = new QTabWidget(this);

    // 为图像标签页添加专业图标
    QIcon sceneIcon(":/icons/iconSet/cameraList.png");
    QIcon eyeIcon(":/icons/iconSet/eye.png");

    if (!sceneIcon.isNull()) {
        imageTabWidget->addTab(ui->label_scene, sceneIcon, tr("场景图像"));
    } else {
        imageTabWidget->addTab(ui->label_scene, tr("场景图像"));
    }

    if (!eyeIcon.isNull()) {
        imageTabWidget->addTab(ui->label_eye, eyeIcon, tr("👁️ 眼球图像"));
    } else {
        imageTabWidget->addTab(ui->label_eye, tr("👁️ 眼球图像"));
    }

    // 设置专业标签页样式
    imageTabWidget->setStyleSheet(
        "QTabWidget::pane {"
        "    border: 2px solid #E0E0E0;"
        "    background-color: #FFFFFF;"
        "    border-radius: 8px;"
        "}"
        "QTabBar::tab {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #F8F8F8, stop:1 #E8E8E8);"
        "    border: 1px solid #CCCCCC;"
        "    padding: 10px 16px;"
        "    margin-right: 2px;"
        "    border-radius: 8px 8px 0px 0px;"
        "    font-weight: bold;"
        "    min-width: 100px;"
        "}"
        "QTabBar::tab:selected {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #4A90E2, stop:1 #357ABD);"
        "    color: white;"
        "    border-bottom: 2px solid #357ABD;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #E3F2FD, stop:1 #BBDEFB);"
        "    color: #1976D2;"
        "}"
    );

    // 创建专业级彩色图像显示窗口
    QString imageIconPath = ":/icons/iconSet/resolution.png";
    QIcon imageIcon(imageIconPath);

    // 创建现代化自定义标题栏
    QWidget *imageTitleWidget = new QWidget();
    imageTitleWidget->setStyleSheet(
        "QWidget {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #4A90E2, stop:1 #357ABD);"
        "    border-radius: 6px;"
        "    margin: 2px;"
        "}"
    );

    QHBoxLayout *imageTitleLayout = new QHBoxLayout(imageTitleWidget);
    imageTitleLayout->setContentsMargins(8, 4, 8, 4);
    imageTitleLayout->setSpacing(8);

    QLabel *imageIconLabel = new QLabel();
    if (!imageIcon.isNull()) {
        // 创建带边框效果的图标
        QPixmap originalPixmap = imageIcon.pixmap(20, 20);
        QPixmap enhancedPixmap(24, 24);
        enhancedPixmap.fill(Qt::transparent);

        QPainter painter(&enhancedPixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        // 绘制圆形背景
        painter.setBrush(QBrush(QColor(255, 255, 255, 200)));
        painter.setPen(QPen(QColor(255, 255, 255, 100), 1));
        painter.drawEllipse(2, 2, 20, 20);

        // 绘制图标
        painter.drawPixmap(2, 2, originalPixmap);

        imageIconLabel->setPixmap(enhancedPixmap);
//        qDebug() << "[DEBUG] 专业级图像显示窗口图标设置成功";
    } else {
        // 如果图标加载失败，使用Unicode图标
        imageIconLabel->setText("🖼️");
        imageIconLabel->setStyleSheet("font-size: 18px;");
//        qDebug() << "[WARNING] 图像显示窗口图标加载失败，使用Unicode替代!";
    }

    QLabel *imageTitleLabel = new QLabel(tr("📷 图像显示"));
    imageTitleLabel->setStyleSheet(
        "QLabel {"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "    color: white;"
        "    text-shadow: 1px 1px 2px rgba(0,0,0,0.5);"
        "}"
    );

    // 添加状态指示灯
    QLabel *statusIndicator = new QLabel("●");
    statusIndicator->setStyleSheet(
        "QLabel {"
        "    color: #4CAF50;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
    );
    statusIndicator->setToolTip("图像显示模块状态：正常");

    // 添加浮动按钮 - 圆形设计
    QPushButton *floatButton = new QPushButton("↗");
    floatButton->setMinimumSize(24, 24);
    floatButton->setStyleSheet(
        "QPushButton {"
        "    background: qradial-gradient(circle, rgba(255,255,255,0.9) 0%, rgba(240,240,240,0.8) 100%);"
        "    border: 2px solid rgba(74,144,226,0.6);"
        "    border-radius: 12px;"
        "    color: #333;"
        "    font-weight: bold;"
        "    font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "    background: qradial-gradient(circle, rgba(255,255,255,1.0) 0%, rgba(230,240,255,0.9) 100%);"
        "    border: 2px solid #4A90E2;"
        "    transform: scale(1.1);"
        "}"
        "QPushButton:pressed {"
        "    background: qradial-gradient(circle, rgba(200,220,255,0.9) 0%, rgba(180,200,240,0.8) 100%);"
        "    border: 2px solid #357ABD;"
        "}"
    );
    floatButton->setToolTip("浮动窗口");

    // 添加关闭按钮 - 圆形设计
    QPushButton *closeButton = new QPushButton("✕");
    closeButton->setFixedSize(24, 24);
    closeButton->setStyleSheet(
        "QPushButton {"
        "    background: qradial-gradient(circle, rgba(255,107,107,0.9) 0%, rgba(239,83,80,0.8) 100%);"
        "    border: 2px solid rgba(244,67,54,0.6);"
        "    border-radius: 12px;"
        "    color: white;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background: qradial-gradient(circle, rgba(255,87,87,1.0) 0%, rgba(255,120,120,0.9) 100%);"
        "    border: 2px solid #F44336;"
        "    transform: scale(1.1);"
        "}"
        "QPushButton:pressed {"
        "    background: qradial-gradient(circle, rgba(211,47,47,0.9) 0%, rgba(183,28,28,0.8) 100%);"
        "    border: 2px solid #D32F2F;"
        "}"
    );
    closeButton->setToolTip("关闭图像显示窗口");

    imageTitleLayout->addWidget(imageIconLabel);
    imageTitleLayout->addWidget(imageTitleLabel);
    imageTitleLayout->addStretch();
    imageTitleLayout->addWidget(statusIndicator);
    imageTitleLayout->addWidget(floatButton);
    imageTitleLayout->addWidget(closeButton);

    QDockWidget *imageDock = new QDockWidget(tr("🖼️ 图像显示"), this);

    // 字体已在CSS中设置

    // 连接图像显示窗口按钮功能
    connect(floatButton, &QPushButton::clicked, [imageDock]() {
        imageDock->setFloating(!imageDock->isFloating());
    });
    connect(closeButton, &QPushButton::clicked, [imageDock]() {
        imageDock->hide();
    });
    imageDock->setWidget(imageTabWidget);
    imageDock->setObjectName("ImageDock");
    // 暂时禁用自定义标题栏以修复停靠灵敏度
    // imageDock->setTitleBarWidget(imageTitleWidget);
    imageDock->setWindowIcon(imageIcon);

    // 样式由全局CSS统一管理

//    qDebug() << "[DEBUG] 专业级图像显示窗口创建完成";
    addDockWidget(Qt::TopDockWidgetArea, imageDock);

    // 保存dock引用以便后续使用
    m_sceneDock = imageDock;  // 复用现有变量
    m_eyeDock = imageDock;
    m_dockWidgets.append(m_sceneDock); // 添加到状态管理列表
    // 设置空的中央小部件，支持上下左右停靠
    setCentralWidget(new QWidget(this));

    // 创建数据分析Split窗口
    m_dataSplitter = new QSplitter(Qt::Horizontal, this);
    m_dataSplitter->setChildrenCollapsible(true); // 允许更灵活的尺寸调整
    m_dataSplitter->setHandleWidth(8); // 设置分割条宽度

    // 创建专业级彩色数据分析窗口
    QString dataIconPath = ":/icons/iconSet/histogram.png";
    QIcon dataIcon(dataIconPath);

    // 创建现代化数据分析标题栏
    QWidget *dataTitleWidget = new QWidget();
    dataTitleWidget->setStyleSheet(
        "QWidget {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #FF6B6B, stop:1 #E55353);"
        "    border-radius: 6px;"
        "    margin: 2px;"
        "}"
    );

    QHBoxLayout *dataTitleLayout = new QHBoxLayout(dataTitleWidget);
    dataTitleLayout->setContentsMargins(8, 4, 8, 4);
    dataTitleLayout->setSpacing(8);

    QLabel *dataIconLabel = new QLabel();
    if (!dataIcon.isNull()) {
        // 创建增强的数据分析图标
        QPixmap originalPixmap = dataIcon.pixmap(20, 20);
        QPixmap enhancedPixmap(24, 24);
        enhancedPixmap.fill(Qt::transparent);

        QPainter painter(&enhancedPixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        // 绘制圆形背景
        painter.setBrush(QBrush(QColor(255, 255, 255, 200)));
        painter.setPen(QPen(QColor(255, 255, 255, 100), 1));
        painter.drawEllipse(2, 2, 20, 20);

        // 绘制图标
        painter.drawPixmap(2, 2, originalPixmap);

        dataIconLabel->setPixmap(enhancedPixmap);
//        qDebug() << "[DEBUG] 专业级数据分析窗口图标设置成功";
    } else {
        // 如果图标加载失败，使用Unicode图标
        dataIconLabel->setText("📊");
        dataIconLabel->setStyleSheet("font-size: 18px;");
//        qDebug() << "[WARNING] 数据分析窗口图标加载失败，使用Unicode替代!";
    }

    QLabel *dataTitleLabel = new QLabel(tr("📈 数据分析"));
    dataTitleLabel->setStyleSheet(
        "QLabel {"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "    color: white;"
        "    text-shadow: 1px 1px 2px rgba(0,0,0,0.5);"
        "}"
    );

    // 添加数据分析状态指示器
    QLabel *dataStatusIndicator = new QLabel("●");
    dataStatusIndicator->setStyleSheet(
        "QLabel {"
        "    color: #FFC107;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
    );
    dataStatusIndicator->setToolTip("数据分析模块状态：待机中");

    // 添加实时数据计数器
    QLabel *dataCounter = new QLabel("0");
    dataCounter->setStyleSheet(
        "QLabel {"
        "    background-color: rgba(255,255,255,0.3);"
        "    color: white;"
        "    font-weight: bold;"
        "    font-size: 10px;"
        "    border-radius: 8px;"
        "    padding: 2px 6px;"
        "    min-width: 20px;"
        "}"
    );
    dataCounter->setToolTip("当前数据集数量");

    // 添加数据分析窗口的浮动和关闭按钮 - 圆形设计
    QPushButton *dataFloatButton = new QPushButton("↗");
    dataFloatButton->setFixedSize(24, 24);
    dataFloatButton->setStyleSheet(
        "QPushButton {"
        "    background: qradial-gradient(circle, rgba(255,255,255,0.9) 0%, rgba(240,240,240,0.8) 100%);"
        "    border: 2px solid rgba(255,107,107,0.6);"
        "    border-radius: 12px;"
        "    color: #333;"
        "    font-weight: bold;"
        "    font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "    background: qradial-gradient(circle, rgba(255,255,255,1.0) 0%, rgba(255,240,240,0.9) 100%);"
        "    border: 2px solid #FF6B6B;"
        "    transform: scale(1.1);"
        "}"
        "QPushButton:pressed {"
        "    background: qradial-gradient(circle, rgba(255,220,220,0.9) 0%, rgba(240,200,200,0.8) 100%);"
        "    border: 2px solid #E55353;"
        "}"
    );
    dataFloatButton->setToolTip("浮动窗口");

    QPushButton *dataCloseButton = new QPushButton("✕");
    dataCloseButton->setFixedSize(24, 24);
    dataCloseButton->setStyleSheet(
        "QPushButton {"
        "    background: qradial-gradient(circle, rgba(255,107,107,0.9) 0%, rgba(239,83,80,0.8) 100%);"
        "    border: 2px solid rgba(244,67,54,0.6);"
        "    border-radius: 12px;"
        "    color: white;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background: qradial-gradient(circle, rgba(255,87,87,1.0) 0%, rgba(255,120,120,0.9) 100%);"
        "    border: 2px solid #F44336;"
        "    transform: scale(1.1);"
        "}"
        "QPushButton:pressed {"
        "    background: qradial-gradient(circle, rgba(211,47,47,0.9) 0%, rgba(183,28,28,0.8) 100%);"
        "    border: 2px solid #D32F2F;"
        "}"
    );
    dataCloseButton->setToolTip("关闭数据分析窗口");

    dataTitleLayout->addWidget(dataIconLabel);
    dataTitleLayout->addWidget(dataTitleLabel);
    dataTitleLayout->addStretch();
    dataTitleLayout->addWidget(dataCounter);
    dataTitleLayout->addWidget(dataStatusIndicator);
    dataTitleLayout->addWidget(dataFloatButton);
    dataTitleLayout->addWidget(dataCloseButton);

    m_dataDock = new QDockWidget(tr("📈 数据分析"), this);

    // 字体已在CSS中设置

    // 移除手动位置保存连接，统一使用Qt原生状态管理

    // 连接数据分析窗口按钮功能
    connect(dataFloatButton, &QPushButton::clicked, [this]() {
        m_dataDock->setFloating(!m_dataDock->isFloating());
    });
    connect(dataCloseButton, &QPushButton::clicked, [this]() {
        m_dataDock->hide();
    });
    m_dataDock->setWidget(m_dataSplitter);
    m_dataDock->setObjectName("DataAnalysisDock");
    m_dataDock->setAttribute(Qt::WA_DeleteOnClose, false);
    // 暂时禁用自定义标题栏以修复停靠灵敏度
    // m_dataDock->setTitleBarWidget(dataTitleWidget);
    m_dataDock->setWindowIcon(dataIcon);

    // 样式由全局CSS统一管理

//    qDebug() << "[DEBUG] 专业级数据分析窗口创建完成";
    addDockWidget(Qt::RightDockWidgetArea, m_dataDock);
    m_dockWidgets.append(m_dataDock); // 添加到状态管理列表

    // 创建GraphPlot Tab容器
    m_dataTabWidget = new QTabWidget(this);
    m_dataTabWidget->setTabsClosable(true);

    m_graphTabDock = new QDockWidget(tr("📊 图表分析"), this);
    // 样式由全局CSS统一管理

    // 字体已在CSS中设置
    m_graphTabDock->setWidget(m_dataTabWidget);
    m_graphTabDock->setObjectName("GraphTabDock");
    m_graphTabDock->setAttribute(Qt::WA_DeleteOnClose, false);

    // 移除手动位置保存连接，统一使用Qt原生状态管理
    // 调试：检查图表分析窗口图标
    QString graphIconPath = ":/icons/iconSet/histogram.png";
//    qDebug() << "[DEBUG] 设置图表分析窗口图标，路径:" << graphIconPath;

    QIcon graphIcon(graphIconPath);
    if (!graphIcon.isNull()) {
//        qDebug() << "[DEBUG] 图表分析窗口图标加载成功";
        // 采用按钮图标的成功模式：预生成多尺寸像素图
        QIcon finalGraphIcon;
        finalGraphIcon.addPixmap(graphIcon.pixmap(16, 16));
        finalGraphIcon.addPixmap(graphIcon.pixmap(24, 24));
        finalGraphIcon.addPixmap(graphIcon.pixmap(32, 32));
        graphIcon = finalGraphIcon;
//        qDebug() << "[DEBUG] 图表分析窗口图标已预生成多尺寸";
    } else {
//        qDebug() << "[WARNING] 图表分析窗口图标加载失败!";
    }

    m_graphTabDock->setWindowIcon(graphIcon);
//    qDebug() << "[DEBUG] 图表分析窗口图标设置完成";
    addDockWidget(Qt::BottomDockWidgetArea, m_graphTabDock);
    m_dockWidgets.append(m_graphTabDock); // 添加到状态管理列表

    // 连接Tab关闭信号
    connect(m_dataTabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget* widget = m_dataTabWidget->widget(index);
        GraphPlot* graphPlot = qobject_cast<GraphPlot*>(widget);
        if (graphPlot) {
            onCloseGraphPlot(graphPlot);
        }
    });

    // 创建专业级彩色控件窗口
    QString controlsIconPath = ":/icons/iconSet/settings.png";
    QIcon controlsIcon(controlsIconPath);

    // 创建现代化控件标题栏
    QWidget *controlsTitleWidget = new QWidget();
    controlsTitleWidget->setStyleSheet(
        "QWidget {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                stop:0 #9C27B0, stop:1 #7B1FA2);"
        "    border-radius: 6px;"
        "    margin: 2px;"
        "}"
    );

    QHBoxLayout *controlsTitleLayout = new QHBoxLayout(controlsTitleWidget);
    controlsTitleLayout->setContentsMargins(8, 4, 8, 4);
    controlsTitleLayout->setSpacing(8);

    QLabel *controlsIconLabel = new QLabel();
    if (!controlsIcon.isNull()) {
        // 创建增强的控件图标
        QPixmap originalPixmap = controlsIcon.pixmap(20, 20);
        QPixmap enhancedPixmap(24, 24);
        enhancedPixmap.fill(Qt::transparent);

        QPainter painter(&enhancedPixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        // 绘制圆形背景
        painter.setBrush(QBrush(QColor(255, 255, 255, 200)));
        painter.setPen(QPen(QColor(255, 255, 255, 100), 1));
        painter.drawEllipse(2, 2, 20, 20);

        // 绘制图标
        painter.drawPixmap(2, 2, originalPixmap);

        controlsIconLabel->setPixmap(enhancedPixmap);
//        qDebug() << "[SUCCESS] 专业级控件窗口图标设置成功";
    } else {
        // 如果图标加载失败，使用Unicode图标
        controlsIconLabel->setText("⚙️");
        controlsIconLabel->setStyleSheet("font-size: 18px;");
//        qDebug() << "[WARNING] 控件窗口图标加载失败，使用Unicode替代!";
    }

    QLabel *controlsTitleLabel = new QLabel(tr("⚙️ 控件"));
    controlsTitleLabel->setStyleSheet(
        "QLabel {"
        "    font-weight: bold;"
        "    font-size: 20px;"
        "    color: white;"
        "    text-shadow: 1px 1px 2px rgba(0,0,0,0.5);"
        "}"
    );

    // 添加控件状态指示器
    QLabel *controlsStatusIndicator = new QLabel("●");
    controlsStatusIndicator->setStyleSheet(
        "QLabel {"
        "    color: #4CAF50;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
    );
    controlsStatusIndicator->setToolTip("控件模块状态：正常");

    // 添加控件窗口的浮动和关闭按钮 - 圆形设计
    QPushButton *controlsFloatButton = new QPushButton("↗");
    controlsFloatButton->setFixedSize(24, 24);
    controlsFloatButton->setStyleSheet(
        "QPushButton {"
        "    background: qradial-gradient(circle, rgba(255,255,255,0.9) 0%, rgba(240,240,240,0.8) 100%);"
        "    border: 2px solid rgba(156,39,176,0.6);"
        "    border-radius: 12px;"
        "    color: #333;"
        "    font-weight: bold;"
        "    font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "    background: qradial-gradient(circle, rgba(255,255,255,1.0) 0%, rgba(245,240,255,0.9) 100%);"
        "    border: 2px solid #9C27B0;"
        "    transform: scale(1.1);"
        "}"
        "QPushButton:pressed {"
        "    background: qradial-gradient(circle, rgba(230,220,255,0.9) 0%, rgba(210,200,240,0.8) 100%);"
        "    border: 2px solid #7B1FA2;"
        "}"
    );
    controlsFloatButton->setToolTip("浮动窗口");

    QPushButton *controlsCloseButton = new QPushButton("✕");
    controlsCloseButton->setFixedSize(24, 24);
    controlsCloseButton->setStyleSheet(
        "QPushButton {"
        "    background: qradial-gradient(circle, rgba(255,107,107,0.9) 0%, rgba(239,83,80,0.8) 100%);"
        "    border: 2px solid rgba(244,67,54,0.6);"
        "    border-radius: 12px;"
        "    color: white;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background: qradial-gradient(circle, rgba(255,87,87,1.0) 0%, rgba(255,120,120,0.9) 100%);"
        "    border: 2px solid #F44336;"
        "    transform: scale(1.1);"
        "}"
        "QPushButton:pressed {"
        "    background: qradial-gradient(circle, rgba(211,47,47,0.9) 0%, rgba(183,28,28,0.8) 100%);"
        "    border: 2px solid #D32F2F;"
        "}"
    );
    controlsCloseButton->setToolTip("关闭控件窗口");

    controlsTitleLayout->addWidget(controlsIconLabel);
    controlsTitleLayout->addWidget(controlsTitleLabel);
    controlsTitleLayout->addStretch();
    controlsTitleLayout->addWidget(controlsStatusIndicator);
    controlsTitleLayout->addWidget(controlsFloatButton);
    controlsTitleLayout->addWidget(controlsCloseButton);

    m_controlsDock = new QDockWidget(tr("⚙️ 控件"), this);

    // 字体已在CSS中设置
    m_controlsDock->setObjectName("ControlsDock");
    m_controlsDock->setAttribute(Qt::WA_DeleteOnClose, false);

    // 移除手动位置保存连接，统一使用Qt原生状态管理

    // 连接控件窗口按钮功能
    connect(controlsFloatButton, &QPushButton::clicked, [this]() {
        m_controlsDock->setFloating(!m_controlsDock->isFloating());
    });
    connect(controlsCloseButton, &QPushButton::clicked, [this]() {
        m_controlsDock->hide();
    });

    // 暂时禁用自定义标题栏以修复停靠灵敏度
    // m_controlsDock->setTitleBarWidget(controlsTitleWidget);
    m_controlsDock->setWindowIcon(controlsIcon);

    // 样式由全局CSS统一管理

//    qDebug() << "[SUCCESS] 专业级控件窗口创建完成";

    // 创建控件窗口内容（参考temp03的完整实现）
    QWidget *controlsWidget = new QWidget();
    QFormLayout *controlsLayout = new QFormLayout(controlsWidget);
    controlsLayout->setSpacing(8);
    controlsLayout->setContentsMargins(10, 10, 10, 10);
    controlsLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // 原有控件窗口保持纯净，所有相机相关功能已移至独立的摄像头控制窗口

    // 重新父化并布局控制元素（从temp03恢复）
    ui->label_num->setParent(controlsWidget);
    controlsLayout->addRow(tr("数量"), ui->label_num);
//    ui->label_3->setParent(controlsWidget);
//    controlsLayout->addRow(tr("测试点坐标"), ui->label_3);
//    ui->label_4->setParent(controlsWidget);
//    controlsLayout->addRow(tr("X:"), ui->label_4);
//    ui->xZuobiao->setParent(controlsWidget);
    ui->xZuobiao->setParent(controlsWidget);
    ui->xZuobiao->setMaximumHeight(25);  // 缩小编辑框高度
    ui->xZuobiao->clear();  // 清除覆盖文字
    ui->xZuobiao->setStyleSheet(
        "QTextEdit {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 2px 4px;"
        "    background-color: #FFFFFF;"
        "    font-size: 12px;"
        "    max-height: 25px;"
        "}"
    );
    controlsLayout->addRow(tr("X坐标"), ui->xZuobiao);
//    ui->label_5->setParent(controlsWidget);
//    controlsLayout->addRow(tr("Y:"), ui->label_5);
    ui->yZuobiao->setParent(controlsWidget);
    ui->yZuobiao->setMaximumHeight(25);  // 缩小编辑框高度
    ui->yZuobiao->clear();  // 清除覆盖文字
    ui->yZuobiao->setStyleSheet(
        "QTextEdit {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 2px 4px;"
        "    background-color: #FFFFFF;"
        "    font-size: 12px;"
        "    max-height: 25px;"
        "}"
    );
    controlsLayout->addRow(tr("Y坐标"), ui->yZuobiao);
    ui->label->setParent(controlsWidget);
//    controlsLayout->addRow(tr("Pitch(单位:°)"), ui->label);
    ui->pitch_qs->setParent(controlsWidget);
    ui->pitch_qs->setMaximumHeight(25);  // 缩小编辑框高度
    ui->pitch_qs->clear();  // 清除覆盖文字
    ui->pitch_qs->setStyleSheet(
        "QTextEdit {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 2px 4px;"
        "    background-color: #FFFFFF;"
        "    font-size: 12px;"
        "    max-height: 25px;"
        "}"
    );
    controlsLayout->addRow(tr("俯仰"), ui->pitch_qs);
    ui->label_2->setParent(controlsWidget);
//    controlsLayout->addRow(tr("Yaw(单位:°)"), ui->label_2);
    ui->yaw_qs->setParent(controlsWidget);
    ui->yaw_qs->setMaximumHeight(25);  // 缩小编辑框高度
    ui->yaw_qs->clear();  // 清除覆盖文字
    ui->yaw_qs->setStyleSheet(
        "QTextEdit {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 2px 4px;"
        "    background-color: #FFFFFF;"
        "    font-size: 12px;"
        "    max-height: 25px;"
        "}"
    );
    controlsLayout->addRow(tr("偏航"), ui->yaw_qs);
    ui->comboBox->setParent(controlsWidget);
    controlsLayout->addRow(tr("标定点数"), ui->comboBox);
    ui->comboBox->setStyleSheet(
        "QComboBox {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    background-color: #FFFFFF;"
        "    min-width: 80px;"
        "}"
        "QComboBox:hover {"
        "    border-color: #2196F3;"
        "    background-color: #F3F9FF;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #FFFFFF;"
        "    border: 1px solid #E0E0E0;"
        "    selection-background-color: #E3F2FD;"
        "    selection-color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "    padding: 6px 8px;"
        "    border: none;"
        "    background-color: #FFFFFF;"
        "    color: #212121;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "    background-color: #E3F2FD;"
        "    color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "    background-color: #BBDEFB;"
        "    color: #1976D2;"
        "}"
    );
    ui->comboBox_2->setParent(controlsWidget);
    controlsLayout->addRow(tr("拟合方法"), ui->comboBox_2);
    ui->comboBox_2->setStyleSheet(
        "QComboBox {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    background-color: #FFFFFF;"
        "    min-width: 80px;"
        "}"
        "QComboBox:hover {"
        "    border-color: #2196F3;"
        "    background-color: #F3F9FF;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #FFFFFF;"
        "    border: 1px solid #E0E0E0;"
        "    selection-background-color: #E3F2FD;"
        "    selection-color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "    padding: 6px 8px;"
        "    border: none;"
        "    background-color: #FFFFFF;"
        "    color: #212121;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "    background-color: #E3F2FD;"
        "    color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "    background-color: #BBDEFB;"
        "    color: #1976D2;"
        "}"
    );

    // ==================== 【新增】图像传输尺寸控制滑块 ====================
    // 创建图像尺寸控制滑块
    m_imageSizeSlider = new QSlider(Qt::Horizontal, controlsWidget);
    m_imageSizeSlider->setRange(0, 100); // 0-100的百分比值
    m_imageSizeSlider->setValue(0); // 默认值对应400x300
    m_imageSizeSlider->setTickPosition(QSlider::TicksBelow);
    m_imageSizeSlider->setTickInterval(25);
    m_imageSizeSlider->setMinimumHeight(30);

    // 创建图像尺寸显示标签
    m_imageSizeLabel = new QLabel("400x300", controlsWidget);
    m_imageSizeLabel->setMinimumWidth(80);
    m_imageSizeLabel->setStyleSheet(
        "QLabel {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 2px 6px;"
        "    background-color: #F5F5F5;"
        "    font-size: 12px;"
        "}"
    );

    // 创建水平布局来组合滑块和标签
    QHBoxLayout* imageSizeLayout = new QHBoxLayout();
    imageSizeLayout->addWidget(m_imageSizeSlider);
    imageSizeLayout->addWidget(m_imageSizeLabel);
    imageSizeLayout->setSpacing(8);
    imageSizeLayout->setContentsMargins(0, 0, 0, 0);

    QWidget* imageSizeWidget = new QWidget(controlsWidget);
    imageSizeWidget->setLayout(imageSizeLayout);

    controlsLayout->addRow(tr("图像大小"), imageSizeWidget);

    // 初始化图像尺寸变量
    // ✅ 优化后（响应式）

    qreal dpr = QGuiApplication::primaryScreen()->devicePixelRatio();

    // 基于屏幕比例的动态尺寸
    m_currentImageWidth = static_cast<int>(screenWidth * 0.15 / dpr);
    m_currentImageHeight = static_cast<int>(screenHeight * 0.1 / dpr);
    m_imageSizeSlider->setValue(m_currentImageWidth); // 同步滑块

    // 连接滑块信号
    connect(m_imageSizeSlider, &QSlider::valueChanged, this, &MainWindow::onImageSizeSliderChanged);
    // ================================================================

    // 使用滚动区域包装控件内容
    QScrollArea *controlsScrollArea = new QScrollArea();
    controlsScrollArea->setWidgetResizable(true);
    controlsScrollArea->setFrameShape(QFrame::NoFrame);
    controlsScrollArea->setWidget(controlsWidget);

    m_controlsDock->setWidget(controlsScrollArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_controlsDock);
    m_dockWidgets.append(m_controlsDock);

    // 设置控件dock允许停靠到所有区域，支持灵活布局
    m_controlsDock->setAllowedAreas(Qt::AllDockWidgetAreas);

    // 【恢复】启用停靠窗口嵌套，支持完全自由布局
    setDockNestingEnabled(true);

    // 设置dock widgets的tabified布局，避免相互遮挡
    setupTabifiedDockWidgets();


    // 初始化误差测试窗口

    m_calibError = nullptr;  // 将在showCalibrationWindow()中初始化


    m_dialog = new EyeTrackingDialog(); // 实例化在这里

    // 连接EyeTrackingDialog的attractedGazeChanged信号到MainWindow的receiveAttractedGaze槽
    connect(m_dialog, &EyeTrackingDialog::attractedGazeChanged, this, &MainWindow::receiveAttractedGaze);

    // 连接误差校正信号
    connect(m_dialog, &EyeTrackingDialog::errorCorrectionTriggered, this, &MainWindow::onErrorCorrectionTriggered);

    // 定时器和闪动设置
    m_flashTimer = new QTimer(this);
    connect(m_flashTimer, &QTimer::timeout, this, &MainWindow::updateFlashColor);
    m_flashTimer->start(300);

    z_length = 500;
    l_perPix = 0.12;

    setFocusPolicy(Qt::StrongFocus);

    // 初始化T模式定时器
    m_gazeTrailTimer = new QTimer(this);
    m_gazeTrailTimer->setSingleShot(false);
    m_gazeTrailTimer->setInterval(33); // 30 FPS轨迹更新
    connect(m_gazeTrailTimer, &QTimer::timeout, this, &MainWindow::updateGazeTrail);

    // 初始化检测方法
    pupilDetectionMethod = new PuRe;
    pupilTrackingMethod = new PuReST();

    // 🚀 初始化亮斑跟踪器（简化模式）
    glintTrackingMethod = new PuReST();  // 复用相同的跟踪算法
    // 简化状态管理，不需要复杂的跟踪状态变量

    dict = getPredefinedDictionary(DICT_4X4_250);
    detectorParameters = new DetectorParameters();
    detectorParameters->markerBorderBits = 2;
    detectorParameters->minMarkerPerimeterRate = 0.05;

    // Initialize shared memory for communication with main05.py
    initializeSharedMemory();

    // Initialize AI results shared memory and UI
    setupAIResultsUI();
    connectToAIResultsSharedMemory();

    // Initialize mode control shared memory
    initializeModeControlSharedMemory();

    dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
    parameters = cv::aruco::DetectorParameters::create();
//    theTimer.start(10);



    // 初始化相机参数
    m_cameraMatrix = (cv::Mat_<double>(3, 3) <<
        464, 0, 326,    // fx, 0, cx
        0, 462, 225.7,    // 0, fy, cy
        0, 0, 1         // 0, 0, 1
    );
    m_distCoeffs = (cv::Mat_<double>(5, 1) << 0, 0, 0, 0, 0);


    // 连接信号和槽
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateImage);
    // 启动定时器
    timer->start(4);

//    initializeVirtualScreen();
    // 初始化AR悬浮窗口
//       createAROverlay();
    calibrationPointLabel = nullptr;
    instructionLabel = nullptr;
    flag_calibration_status = 1;


    // QT相机初始化
//    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();

//    if (cameras.isEmpty()) {
//            qDebug() << "No cameras found.";
//        } else {
//            for (int i = 0; i < cameras.size(); ++i) {
//                const QCameraInfo &cameraInfo = cameras[i];
//                qDebug() << "  Camera" << i << ":";
//                qDebug() << "    Description:" << cameraInfo.description();
//                qDebug() << "    Device Name:" << cameraInfo.deviceName(); // 例如 "/dev/video0", "/dev/video1" (Linux) 或设备路径 (Windows)

//            }
//        }



//    if (cameras.size() > 1) {
//        m_grabber = new FrameGrabber("Camera 1", CV_8UC3);

//        camera0 = new QCamera(cameras[0]);
//        QCameraViewfinderSettings viewfinderSettings;
//        viewfinderSettings.setResolution(640, 480);
//        camera0->setViewfinderSettings(viewfinderSettings);
//        camera0->setViewfinder(m_grabber);

//        if (cameras.size() > 1) {
//            m_grabber1 = new FrameGrabber("Camera 0", CV_8UC3);
//            camera1 = new QCamera(cameras[1]);
//            QCameraViewfinderSettings viewfinderSettings1;
//            viewfinderSettings1.setResolution(640, 480);
//            camera1->setViewfinderSettings(viewfinderSettings1);
//            camera1->setViewfinder(m_grabber1);
//        } else {
//            QMessageBox::information(this, "info", "Only one camera available");
//        }

//        connect(m_grabber, &FrameGrabber::newFrame, this, &MainWindow::handleCamera1Image);
//        connect(m_grabber1, &FrameGrabber::newFrame, this, &MainWindow::handleCamera2Image);

//        camera0->start();
//        QThread::msleep(400);
//        if (camera1 != camera0) {
//            camera1->start();
//        }
//    } else {
//        QMessageBox::information(this, "info", "设备连接异常");
//    }

//    if (camera0->status() != QCamera::ActiveStatus ||camera1->status() != QCamera::ActiveStatus) {
//        qDebug() << "Camera 0 failed to start:" << camera0->errorString();
//        QMessageBox::critical(this, "Camera Error", "Camera 0 failed to start: " + camera0->errorString());
//        return; // 或其他错误处理
//    }

    // ==================== 【修改】使用可配置的摄像头初始化 ====================
    // 初始化COM库
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM" << std::endl;
//        qDebug() << "COM初始化失败，但继续运行";
        // 不再直接退出，而是继续运行，允许用户手动配置摄像头
    }

    // 【重要】移除硬编码的摄像头初始化，改为用户可选择的方式
    // 摄像头将在用户点击"应用设置"按钮时初始化
    // 这样用户可以先选择合适的摄像头设备，然后再初始化

//    qDebug() << "摄像头系统准备就绪，等待用户选择设备并点击'应用设置'按钮";
    // ================================================================

       // Add DataTable and GraphPlot menu items
       QMenu* dataMenu = menuBar->addMenu("数据分析");
       QAction* dataTableAction = dataMenu->addAction("打开数据表");
       connect(dataTableAction, &QAction::triggered, this, &MainWindow::onCreateDataTable);

       // 添加创建独立窗口GraphPlot的选项
       dataMenu->addSeparator();
       QAction* createWindowGraphAction = dataMenu->addAction(tr("创建独立图表窗口"));
       connect(createWindowGraphAction, &QAction::triggered, this, [this]() {
           // 这里可以弹出对话框让用户选择图表类型，现在先用默认值
           onCreateGraphPlotAsWindow("示例图表");
       });

//       clickReloadButton();

       // 确保摄像头dock在启动时就存在，这样才能被状态管理系统处理
       if (!m_cameraDock) {
           createCameraDockWindow();
//           qDebug() << "在启动时创建摄像头dock以支持状态保存/恢复";
       }

       // 确保AI状态dock在启动时就存在，这样才能被状态管理系统处理
       if (!m_aiStatusDock) {
           createAIStatusDockWindow();
//           qDebug() << "在启动时创建AI状态dock以支持状态保存/恢复";
       }

       // 【删除】窗口状态恢复已在构造函数中处理，这里不需要重复调用

       // 【修复】延迟检查并显示dock窗口，避免闪现
       QTimer::singleShot(100, this, [this]() {
           // 只有在状态恢复后仍然需要显示的窗口才显示
           if (m_controlsDock && !m_controlsDock->isVisible()) {
               m_controlsDock->setVisible(true);
//               qDebug() << "延迟显示控件窗口（避免闪现）";
           }
           if (m_cameraDock && !m_cameraDock->isVisible()) {
               m_cameraDock->setVisible(true);
//               qDebug() << "延迟显示摄像头窗口（避免闪现）";
           }
           if (m_aiStatusDock && !m_aiStatusDock->isVisible()) {
               m_aiStatusDock->setVisible(true);
//               qDebug() << "延迟显示AI状态窗口（避免闪现）";
           }
       });

       // Initialize timing for FPS calculation


       // 初始化瞳孔-角膜标定器，使用双特征融合模型
       m_calibrator = std::make_unique<EyeTrackingCalibrator>(EyeTrackingCalibrator::PUPIL_CORNEAL_DUAL);
//       qDebug() << "瞳孔-角膜标定器初始化完成";

       m_gazeOverlay = new GazeOverlayWidget(nullptr);


       initializeGazeKalmanFilter(); // <-- 添加这一行

       m_frameTimer.start(); // <-- 启动用于计算dt的计时器
        m_globalErrorOffset = cv::Point2f(0.0f, 0.0f);
        m_lastFPSTime = std::chrono::steady_clock::now();
        m_lastProcessingFPSTime = std::chrono::steady_clock::now();
       // 尝试加载最新的标定文件
       loadLatestCalibration();

       // 【修复】延迟创建DataTable，但要在状态恢复完成后



       QTimer::singleShot(0, this, [this]() {


               // 2. 在所有布局都完成后，再将主窗口显示出来
               this->show();
                restoreSessionState();
               // 3. (可选但推荐) 确保关键窗口被激活
               this->raise();
               this->activateWindow();

           });

    // 【新增】设置自动保存定时器，定期保存窗口状态
    QTimer* autoSaveTimer = new QTimer(this);
    connect(autoSaveTimer, &QTimer::timeout, this, &MainWindow::saveWindowPositions);
    autoSaveTimer->start(30000); // 每30秒自动保存一次窗口状态

    // 【新增】连接dock窗口的状态变化信号，实时保存状态
    auto connectDockSignals = [this](QDockWidget* dock) {
        if (dock) {
            connect(dock, &QDockWidget::dockLocationChanged, this, [this]() {
                // 延迟保存，避免频繁写入
                QTimer::singleShot(1000, this, &MainWindow::saveWindowPositions);
            });
            connect(dock, &QDockWidget::topLevelChanged, this, [this]() {
                QTimer::singleShot(1000, this, &MainWindow::saveWindowPositions);
            });
            connect(dock, &QDockWidget::visibilityChanged, this, [this]() {
                QTimer::singleShot(1000, this, &MainWindow::saveWindowPositions);
            });
        }
    };

    // 为所有dock窗口连接信号
    connectDockSignals(m_dataDock);
    connectDockSignals(m_controlsDock);
    connectDockSignals(m_cameraDock);
    connectDockSignals(m_aiStatusDock);  // 添加AI状态窗口的信号连接
    connectDockSignals(m_graphTabDock);
    connectDockSignals(m_sceneDock);
    connectDockSignals(m_eyeDock);


}



MainWindow::~MainWindow()
{
    m_isDestroying = true;  // ✅ 关键：设置析构标志
    qDebug() << "=== 开始析构 MainWindow ===" << (void*)this;

    // ========================================================================
    // 1️⃣ 断开所有信号槽连接（防止析构期间触发新对象创建）
    // ========================================================================
    qDebug() << "[Checkpoint 1] 断开信号槽连接...";
    disconnect();

    // ========================================================================
    // 2️⃣ 停止定时器
    // ========================================================================
    qDebug() << "[Checkpoint 2] 停止定时器...";
    if (timer) {
        timer->stop();
        qDebug() << "定时器已停止";
    }

    // ========================================================================
    // 3️⃣ 停止相机
    // ========================================================================
    qDebug() << "[Checkpoint 3] 停止 Camera1...";
    if (camera1) {
        qDebug() << "  camera1 地址:" << (void*)camera1.get();
        camera1->Stop();
        qDebug() << "  Camera1 已停止";
    } else {
        qDebug() << "⚠️ camera1 为空指针，跳过";
    }

    qDebug() << "[Checkpoint 4] 停止 Camera2...";
    if (camera2) {
        qDebug() << "  camera2 地址:" << (void*)camera2.get();
        camera2->Stop();
        qDebug() << "  Camera2 已停止";
    } else {
        qDebug() << "⚠️ camera2 为空指针，跳过";
    }

    // ========================================================================
    // 4️⃣ 清理子进程（QPointer 自动保护）
    // ========================================================================
    qDebug() << "[Checkpoint 5] 清理子进程...";
    if (process) {
        qDebug() << "  process 地址:" << (void*)process.data() << "(QPointer 有效)";
        if (process->state() != QProcess::NotRunning) {
            process->terminate();
            if (!process->waitForFinished(3000)) {
                process->kill();
                process->waitForFinished(1000);
            }
            qDebug() << "子进程已清理，退出码:" << process->exitCode();
        }
    } else {
        qDebug() << "⚠️ process 为 QPointer 空指针（对象已销毁），跳过清理";
    }

    // ========================================================================
    // 5️⃣ 线程清理
    // ========================================================================
    qDebug() << "[Checkpoint 6] 清理线程...";
    if (workerThread && workerThread->isRunning()) {
        workerThread->quit();
        if (!workerThread->wait(3000)) {
            workerThread->terminate();
            workerThread->wait(1000);
        }
        qDebug() << "worker QThread 已停止";
    }

    if (worker) {
        worker->stop();
        qDebug() << "Worker 内部线程已停止";
    }

    // ========================================================================
    // 6️⃣ 关闭子窗口
    // ========================================================================
    qDebug() << "[Checkpoint 7] 关闭子窗口...";
    auto safeCloseDelete = [](QWidget* widget, const char* name) {
        if (widget) {
            widget->close();
            widget->deleteLater();
            qDebug() << "  关闭" << name << "地址:" << (void*)widget;
        }
    };

    safeCloseDelete(static_cast<QWidget*>(m_dialog), "m_dialog");
    safeCloseDelete(static_cast<QWidget*>(m_gazeOverlay), "m_gazeOverlay");
    safeCloseDelete(static_cast<QWidget*>(m_calibError), "m_calibError");

    m_dialog = nullptr;
    m_gazeOverlay = nullptr;
    m_calibError = nullptr;

    // ========================================================================
    // 7️⃣ 共享内存清理
    // ========================================================================
    qDebug() << "[Checkpoint 8] 清理共享内存...";
    disconnectAIResultsSharedMemory();

    if (m_modeControlConnected) {
#ifdef _WIN32
        if (shm_mode_control_ptr) {
            UnmapViewOfFile(shm_mode_control_ptr);
            shm_mode_control_ptr = nullptr;
        }
        if (hModeControlMapping) {
            CloseHandle(hModeControlMapping);
            hModeControlMapping = nullptr;
        }
#else
        if (shm_mode_control_ptr && shm_mode_control_ptr != MAP_FAILED) {
            munmap(shm_mode_control_ptr, 256);
            shm_mode_control_ptr = nullptr;
        }
        if (shm_mode_control_fd != -1) {
            close(shm_mode_control_fd);
            shm_mode_control_fd = -1;
        }
#endif
        m_modeControlConnected = false;
        qDebug() << "模式控制共享内存已清理";
    }

    cleanupSharedMemory();

    // ========================================================================
    // 8️⃣ 保存状态
    // ========================================================================
    qDebug() << "[Checkpoint 9] 保存状态...";
    saveSessionState();
    saveWindowPositions();

    // ========================================================================
    // 9️⃣ 清理标定器
    // ========================================================================
    qDebug() << "[Checkpoint 10] 清理标定器...";
    if (m_calibrator) {
        m_calibrator.reset();
        qDebug() << "标定器已释放";
    }

    // ========================================================================
    // 🔟 清理 DataTable 和 GraphPlots
    // ========================================================================
    qDebug() << "[Checkpoint 11] 清理 DataTable...";
    if (m_dataTable) {
        QDockWidget* dataTableDock = qobject_cast<QDockWidget*>(m_dataTable->parent());
        if (dataTableDock) {
            dataTableDock->close();
            dataTableDock->deleteLater();
        } else {
            m_dataTable->close();
            m_dataTable->deleteLater();
        }
        m_dataTable = nullptr;
    }

    qDebug() << "[Checkpoint 12] 清理 GraphPlots...";
    for (GraphPlot* plot : m_graphPlots) {
        if (plot) {
            QDockWidget* plotDock = qobject_cast<QDockWidget*>(plot->parent());
            if (plotDock) {
                plotDock->close();
            } else {
                plot->close();
                plot->deleteLater();
            }
        }
    }
    m_graphPlots.clear();

    destroyAllWindows();

    // ========================================================================
    // 1️⃣1️⃣ 删除业务对象
    // ========================================================================
    qDebug() << "[Checkpoint 13] 删除业务对象...";
    if (pupilDetectionMethod) { delete pupilDetectionMethod; pupilDetectionMethod = nullptr; }
    if (pupilTrackingMethod) { delete pupilTrackingMethod; pupilTrackingMethod = nullptr; }
    if (glintTrackingMethod) { delete glintTrackingMethod; glintTrackingMethod = nullptr; }

    // ========================================================================
    // 1️⃣2️⃣ 最后删除 ui
    // ========================================================================
    qDebug() << "[Checkpoint 14] 删除 UI...";
    if (ui) {
        delete ui;
        ui = nullptr;
        qDebug() << "UI 已删除";
    }

    // ========================================================================
    // 1️⃣3️⃣ 清理智能指针
    // ========================================================================
    qDebug() << "[Checkpoint 15] 清理智能指针...";
    if (camera1) { camera1.reset(); qDebug() << "camera1 已释放"; }
    if (camera2) { camera2.reset(); qDebug() << "camera2 已释放"; }

    if (ui) { delete ui; ui = nullptr; }

        // ✅ 关键：清空事件队列，丢弃所有未处理的延迟信号
        qDebug() << "[Final] 清空事件队列，防止延迟信号...";
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

        qDebug() << "=== MainWindow 析构完成 ===" << (void*)this;
    qDebug() << "=== MainWindow 析构完成 ===" << (void*)this;
}

// 在 MainWindow.cpp 文件末尾添加这两个函数的实现：

//cv::Point2f MainWindow::fuseGazeWithHeadPose(const cv::Point2f& rawGaze,
//                                           const cv::Mat& rotationMatrix,
//                                           const cv::Mat& translationVector)
//{
//    // 确保数据类型
//    cv::Mat rot_mat, trans_vec;
//    rotationMatrix.convertTo(rot_mat, CV_32F);
//    translationVector.convertTo(trans_vec, CV_32F);

//    // 屏幕参数
//    float screenWidth = static_cast<float>(m_screenWidth);
//    float screenHeight = static_cast<float>(m_screenHeight);
//    float screenPhysicalWidth = 0.321f;
//    float screenPhysicalHeight = 0.19f;
//    float screenDistance = 0.21f;

//    // 像素到物理坐标转换
//    float pixelToMeterX = screenPhysicalWidth / screenWidth;
//    float pixelToMeterY = screenPhysicalHeight / screenHeight;

//    // 将屏幕坐标转换为以屏幕中心为原点的物理坐标
//    // 注意：统一坐标系，Y轴向上为正
//    float gazePhysicalX = (rawGaze.x - screenWidth/2.0f) * pixelToMeterX;
//    float gazePhysicalY = -(rawGaze.y - screenHeight/2.0f) * pixelToMeterY; // Y轴取负

//    // 屏幕上注视点的3D坐标（相对于相机坐标系）
//    cv::Mat gazePointScreen = (cv::Mat_<float>(3, 1) <<
//        gazePhysicalX, gazePhysicalY, screenDistance);

//    // 计算眼部在世界坐标系中的位置
//    cv::Mat eyePositionHead = (cv::Mat_<float>(3, 1) <<
//        m_eyeOffset.x, m_eyeOffset.y, m_eyeOffset.z);
//    cv::Mat eyePositionWorld = rot_mat * eyePositionHead + trans_vec;

//    // 计算从眼部到屏幕注视点的方向向量
//    cv::Mat gazeDirectionWorld = gazePointScreen - eyePositionWorld;

//    // 归一化
//    double norm = cv::norm(gazeDirectionWorld);
//    if (norm > 1e-6) {
//        gazeDirectionWorld = gazeDirectionWorld / norm;
//    }

//    // 计算与屏幕平面的交点
//    float t = screenDistance;
//    if (abs(gazeDirectionWorld.at<float>(2)) > 1e-6f) {
//        t = (screenDistance - eyePositionWorld.at<float>(2)) /
//            gazeDirectionWorld.at<float>(2);
//    }

//    float intersectionX = eyePositionWorld.at<float>(0) + t * gazeDirectionWorld.at<float>(0);
//    float intersectionY = eyePositionWorld.at<float>(1) + t * gazeDirectionWorld.at<float>(1);

//    // 转换回屏幕像素坐标
//    float compensatedX = intersectionX / pixelToMeterX + screenWidth/2.0f;
//    float compensatedY = -intersectionY / pixelToMeterY + screenHeight/2.0f; // Y轴转换回屏幕坐标

//    // 边界限制
//    compensatedX = max(0.0f, min(compensatedX, screenWidth - 1.0f));
//    compensatedY = max(0.0f, min(compensatedY, screenHeight - 1.0f));

//    return cv::Point2f(compensatedX, compensatedY);
//}


// ===================================================================
// 修改后的头部位姿补偿函数 - 基于PDF文档的正确方法
// ===================================================================
// 重写：眼动跟踪与头部位姿融合函数（修正单位和坐标系问题，优化性能）
cv::Point2f MainWindow::fuseGazeWithHeadPose(
    const cv::Point2f& rawGaze,        // 原始视线坐标（像素）
    const cv::Vec3d& eulerAngles,      // 头部欧拉角（已计算）
    const cv::Mat& translationVector)  // 头部平移向量（米）
{
    // 输入有效性检查
        if (translationVector.empty()) {
//            qDebug() << "Warning: Empty translation vector";
            return rawGaze;
        }

        if (translationVector.rows != 3 || translationVector.cols != 1) {
//            qDebug() << "Error: Invalid translation vector dimensions";
            return rawGaze;
        }

        try {
            // 屏幕参数（统一使用）
            const double screenPixelWidth = m_screenWidth;   // 屏幕分辨率
            const double screenPixelHeight = m_screenHeight;
            const double screenPhysicalWidth = 0.31;   // 屏幕物理尺寸（米）
            const double screenPhysicalHeight = 0.21;

            // 简化的姿态校准
            static bool isCalibrated = false;
            static cv::Vec3d refAngles(0, 0, 0);

            cv::Vec3d relativeAngles = eulerAngles;
            if (!isCalibrated) {
                refAngles = eulerAngles;
                isCalibrated = true;
//                qDebug() << "Head pose calibrated";
            } else {
                relativeAngles = eulerAngles - refAngles;
            }

            double pitch = relativeAngles[1];  // 俯仰角（绕Y轴）
            double yaw = relativeAngles[2];    // 偏航角（绕Z轴）

            // 距离检查
            double distance = cv::norm(translationVector);
            if (distance < 0.2 || distance > 1.5) {
                return rawGaze;  // 距离异常，不补偿
            }

            // 核心几何补偿计算
            // 1. 屏幕坐标归一化到[-1,1]
            double centerX = screenPixelWidth / 2.0;
            double centerY = screenPixelHeight / 2.0;
            double normX = (rawGaze.x - centerX) / centerX;
            double normY = (rawGaze.y - centerY) / centerY;

            // 2. 头部运动的物理影响计算
            // 距离d，角度θ，屏幕上的物理偏移 = d * tan(θ)
            double physicalOffsetX = distance * tan(yaw);      // 水平偏移（米）
            double physicalOffsetY = distance * tan(pitch);    // 垂直偏移（米）

            // 3. 物理偏移转换为归一化坐标偏移
            double normOffsetX = (physicalOffsetX / screenPhysicalWidth) * 2.0;  // 转换到[-1,1]
            double normOffsetY = (physicalOffsetY / screenPhysicalHeight) * 2.0;

            // 4. 应用补偿（头部右转，视线左移，所以是减法）
            const double compensationFactor = 0.7;  // 补偿强度
            double compensatedNormX = normX - normOffsetX * compensationFactor;
            double compensatedNormY = normY - normOffsetY * compensationFactor;

            // 5. 限制补偿范围
            compensatedNormX = max(-1.0, min(1.0, compensatedNormX));
            compensatedNormY = max(-1.0, min(1.0, compensatedNormY));

            // 6. 转换回屏幕像素坐标
            double finalX = compensatedNormX * centerX + centerX;
            double finalY = compensatedNormY * centerY + centerY;

            // 7. 边界限制
            finalX = max(0.0, min(screenPixelWidth - 1.0, finalX));
            finalY = max(0.0, min(screenPixelHeight - 1.0, finalY));

            // 调试输出（降低频率）
            static int counter = 0;
            if (++counter % 30 == 0) {
//                qDebug() << "=== Head Compensation Debug ===";
//                qDebug() << "  Input:" << rawGaze.x << "," << rawGaze.y;
//                qDebug() << "  Angles (deg): Pitch=" << pitch * 180.0/CV_PI
//                         << " Yaw=" << yaw * 180.0/CV_PI;
//                qDebug() << "  Distance:" << distance << "m";
//                qDebug() << "  Physical offset (mm):" << physicalOffsetX*1000 << "," << physicalOffsetY*1000;
//                qDebug() << "  Output:" << finalX << "," << finalY;
//                qDebug() << "  Compensation (pixels):" << (finalX - rawGaze.x) << "," << (finalY - rawGaze.y);
            }

            return cv::Point2f(static_cast<float>(finalX), static_cast<float>(finalY));

        } catch (const std::exception& e) {
//            qDebug() << "Exception in fuseGazeWithHeadPose:" << e.what();
            return rawGaze;
        }

}








// 简单的时间滤波器（用于平滑视线数据）
cv::Point2f MainWindow::temporalFilter(const cv::Point2f& currentGaze)
{
    static cv::Point2f previousGaze(-1, -1);
    static bool isInitialized = false;

    if (!isInitialized) {
        previousGaze = currentGaze;
        isInitialized = true;
        return currentGaze;
    }

    // 简单的指数滑动平均
    const float alpha = 0.7f;  // 平滑系数（越小越平滑）

    cv::Point2f filteredGaze;
    filteredGaze.x = alpha * currentGaze.x + (1.0f - alpha) * previousGaze.x;
    filteredGaze.y = alpha * currentGaze.y + (1.0f - alpha) * previousGaze.y;

    previousGaze = filteredGaze;
    return filteredGaze;
}



void MainWindow::on_openButton_clicked()
{

    if (camera1) camera1->Stop();
    if (camera2) camera2->Stop();
    // 启动摄像头
    camera1->Start();
    camera2->Start();

}

QImage MainWindow::cvMatToQImage(const cv::Mat& mat) {
    // 将OpenCV的RGB图像转换为QImage
    if (mat.type() == CV_8UC3) {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
    }
    return QImage(); // 返回空图像如果格式不匹配
}

void MainWindow::updateCamera1Label() {
    cv::Mat frame;
    // 获取第一个摄像头最新帧并更新显示
    if (camera1->opencv_callback_->GetLatestFrame(frame)) {
        if (!frame.empty()) {
            eyeImage=frame.clone();

//            QImage img = cvMatToQImage(frame);
//            ui->label_eye->setPixmap(QPixmap::fromImage(img));
        }
    }
}

void MainWindow::updateCamera2Label() {
    cv::Mat frame;
    // 获取第二个摄像头最新帧并更新显示
    if (camera2->opencv_callback_->GetLatestFrame(frame)) {
        if (!frame.empty()) {
            cv::flip(frame, frame, 0); // 修复翻转
            sceneImage=frame.clone();
//            QImage img = cvMatToQImage(frame);
//            ui->label_scene->setPixmap(QPixmap::fromImage(img));
        }
    }
}



std::vector<cv::Point2f> MainWindow::generateCalibrationPoints(int numberOfPoints) {
    std::vector<cv::Point2f> currentPoints;
    // 获取屏幕尺寸
   QRect screenRect = QApplication::desktop()->screenGeometry();
   int w = screenRect.width();
   int h = screenRect.height();

    switch (numberOfPoints) {
    case 3:
        currentPoints = {
            {w / 6.0f, h / 6.0f},             // 左上
            {w * 5.0f / 6.0f, h / 6.0f},       // 右上
            {w / 2.0f, h * 5.0f / 6.0f}        // 中下
        };
        break;
    case 5:
        currentPoints = {
            {w / 2.0f, h / 2.0f},              // 屏幕中心
            {w / 6.0f, h / 6.0f},              // 左上
            {w * 5.0f / 6.0f, h / 6.0f},       // 右上
            {w * 5.0f / 6.0f, h * 5.0f / 6.0f},// 右下
            {w / 6.0f, h * 5.0f / 6.0f}        // 左下
        };
        break;
    case 9:
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                currentPoints.push_back({
                    float(w * (j * 0.5 + 0.05) / 1.1),
                    float(h * (i * 0.5 + 0.05) / 1.1)
                });
        break;
    case 10: {
        int cx = w / 2;
        int cy = h / 2;
        float s = qMin(w, h) * 0.4f / 10.0f;
        std::vector<QPoint> base = {
            {0, -10}, {0, -8}, {0, -4}, {0, 4}, {0, 8}, {0, 10},
            {-10, 0}, {-8, 0}, {-4, 0}, {4, 0}, {8, 0}, {10, 0}
        };
        for (const auto& pt : base)
            currentPoints.push_back(cv::Point2f(cx + pt.x() * s, cy + pt.y() * s));
        break;
    }
    case 12:
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 4; ++j)
            {
                currentPoints.push_back({
                                            w * (j * 2 + 1) / 8.0f,
                                            h * (i * 2 + 1) / 6.0f});

                m_calibrationPoints.append(cv::Point2f(
                                           w * (j * 2 + 1) / 8.0f,
                                           h * (i * 2 + 1) / 6.0f));
            }


        break;
    case 16:
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                currentPoints.push_back({
                    float(w * (j * 2 + 1) / 8.0f),
                    float(h * (i * 2 + 1) / 8.0f)
                });
        break;
    default:
//        qDebug() << "未支持的标定点数量: " << numberOfPoints;
        break;
    }


    return currentPoints;
}



QVector<QPointF> toQPointFVector(const QVector<cv::Point2f>& input) {
    QVector<QPointF> output;
    output.reserve(input.size());
    for (const auto& pt : input) {
        output.append(QPointF(pt.x, pt.y));
    }
    return output;
}

QVector<QVector<QPointF>> toNestedQPointFVector(const QVector<QVector<cv::Point2f>>& input) {
    QVector<QVector<QPointF>> output;
    output.reserve(input.size());
    for (const auto& inner : input) {
        QVector<QPointF> temp;
        temp.reserve(inner.size());
        for (const auto& pt : inner) {
            temp.append(QPointF(pt.x, pt.y));
        }
        output.append(temp);
    }
    return output;
}

QVector<QVector<float>> convertQrealToFloat(const QVector<QVector<qreal>>& input) {
    QVector<QVector<float>> output;
    output.reserve(input.size());
    for (const auto& inner : input) {
        QVector<float> temp;
        temp.reserve(inner.size());
        for (qreal value : inner) {
            temp.append(static_cast<float>(value));
        }
        output.append(temp);
    }
    return output;
}


void MainWindow::updateFlashColor()
{
    switch (m_flashState % 2) {
        case 0: m_flashColor = Qt::white; break;
        case 1: m_flashColor = Qt::black; break;
    }
    m_flashState++;

    if (m_calibError && m_recording && !m_calibrationPoints.isEmpty()) {
        // 确保数组大小一致，避免索引越界
        if (m_currentPoint < m_calibrationPoints.size()) {
            m_calibError->setCalibrationData(
                toQPointFVector(m_calibrationPoints),
                toNestedQPointFVector(m_gazePoints),
                convertQrealToFloat(m_errors),  // 这里转换
                m_flashColor,
                m_currentPoint,
                m_recording
            );
        }
    }
}



double MainWindow::calculateError(const cv::Point2f &gt, const cv::Point2f &gaze)
{
    return cv::norm(gt - gaze);
}

// 考虑吸附的误差计算函数
double MainWindow::calculateErrorWithAttraction(const cv::Point2f &gt, const cv::Point2f &gaze)
{
    cv::Point2f finalGaze = gaze;

    // 如果吸附功能激活且有有效的吸附坐标，使用吸附后的坐标
    if (m_attractionActive && m_useAttractedGaze) {
        finalGaze = m_lastAttractedGaze;
    }

    return cv::norm(gt - finalGaze);
}

void MainWindow::calculateErrors()
{
    double total_error = 0;
    int total_points = 0;
    for (int i = 0; i < m_errors.size(); ++i) {
        double point_error_sum = 0;
        for (double err : m_errors[i]) {
            point_error_sum += err;
            total_error += err;
            total_points++;
        }
        double avg_point_error = point_error_sum / m_errors[i].size();
//        qDebug() << "Point" << i + 1 << "Average Error:" << avg_point_error;
    }
    if (total_points > 0) {
//        qDebug() << "Overall Average Error:" << total_error / total_points;
    }
}

void MainWindow::slots_calibration_status(QAction *action) {
    if (action == yanshiAction) {
        this->on_setOrignalBtn_clicked();
        // 【修复】切换到演示模式时，重置3D相关状态
        if (flag_calibration_status == 2) {
            reset3DStates();
        }
        flag_calibration_status = 1;
//        qDebug() << "演示模式激活: " << flag_calibration_status;
    } else if (action == jingqueAction) {
        this->on_setOrignalBtn_clicked();
        // 【修复】切换到精确模式时，重置3D相关状态
        if (flag_calibration_status == 2) {
            reset3DStates();
        }
        flag_calibration_status = 0;
//        qDebug() << "精确模式激活: " << flag_calibration_status;
    } else if (action == gaze3DAction) {
        this->on_setOrignalBtn_clicked();
        flag_calibration_status = 2;
//        qDebug() << "🎆 真3D眼动跟踪模式激活: " << flag_calibration_status;

        // 【修改】不强制启用真3D，保持假3D模式
        if (worker) {
            double focalLength = 640.0; // 使用默认焦距
            // worker->enableReal3DMode(true, focalLength);  // 注释掉强制真3D
            qDebug() << QString("📊 进入3D模式，当前状态: %1").arg(worker->isReal3DModeEnabled() ? "真3D" : "假3D");

//            qDebug() << "✅ 已启用Worker真3D系统，焦距:" << focalLength;
//            qDebug() << "📊 真3D系统状态检查:";
//            qDebug() << "   - 真3D模式启用:" << worker->isReal3DModeEnabled();
//            qDebug() << "   - 3D模型已构建:" << worker->isReal3DModelBuilt();
//            qDebug() << "   - 构建进度:" << worker->get3DModelProgress();

//            QMessageBox::information(this, "🔭 真3D模式激活",
//                "✨ 真3D眼球跟踪模式已激活\n"
//                "🎯 焦距: 640.0 px\n"
//                "👁️ 请注视不同方向，系统将自动建模\n"
//                "📊 模型构建完成后即可进行高精度3D追踪\n\n"
//                "💡 快捷键:\n"
//                "   R - 重置3D模型\n"
//                "   P - 增加样本计数（调试用）");
        } else {
//            qDebug() << "⚠️ Worker未初始化，无法启用真3D模式";
            QMessageBox::warning(this, "启动失败", "Worker未初始化，请重启程序");
        }
    } else {
//        qDebug() << "未知校准模式被触发";
    }
}





void MainWindow::paintEvent(QPaintEvent *e)
{
    QMainWindow::paintEvent(e);

    // 如果T模式开启，绘制自由注视轨迹
//    if (m_freeGazeMode) {
//        QPainter painter(this);
//        painter.setRenderHint(QPainter::Antialiasing);
//        drawFreeGazeTrail(painter);
//    }
}

void MainWindow::on_Markcalib_clicked()
{
    if(rightPupil.size()<4)
        return;
    if(CALIBRATIONPOINTS9==12)
    {//jingque calibration with 12points
        bool tmp = calibrate();
        if(tmp)
        {
            QMessageBox::information(this,"info","jingque calibrated sucess");
            rightPupil = PointVector();  // 创建新对象，旧数据被释放

        }
    }
    else
    {//calibme method mark
        destroyWindow("mark");
        bool tmp = calibrate();
        if(tmp)
        {
            QMessageBox::information(this,"info","Mark calibrated sucess");
            gmark = 0;
        }
        else
            QMessageBox::information(this,"info","Mark calibrated Fail");
    }

}




void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_D:
            this->on_calibImgCollectBtn_clicked();
            break;
        case Qt::Key_Escape:
            break;
        case Qt::Key_T:
            // T键：绘制屏幕视线坐标（实时显示当前注视点位置）
            qDebug() << "[T键] 开启视线坐标绘制模式";
            setFreeGazeMode(!m_freeGazeMode);
            break;

        case Qt::Key_Space:

            break;
        case Qt::Key_C:
            gmark = !gmark;

            if (gmark) {
                 on_MarkerBtn_clicked();  // 显示标记，开始采集数据
            } else {
                on_Markcalib_clicked();   // 执行CalibMe标定算法
                if(labelm) labelm->close();
            }
            break;

        case Qt::Key_B:
            m_imageProcessingPaused = !m_imageProcessingPaused;

            if (m_imageProcessingPaused) {
                // 冻结时保存当前帧
                if (!eyeImage.empty() && !sceneImage.empty()) {
                    m_frozenEyeImage = eyeImage.clone();
                    m_frozenSceneImage = sceneImage.clone();
                    m_hasFrozenImages = true;
//                    qDebug() << "✅ 图像已冻结";
                }
            } else {
//                qDebug() << "▶️ 图像恢复实时模式";
            }
            updateStatusIndicators();
            break;



        case Qt::Key_R:
            // R键重置逻辑：根据当前模式选择重置目标
            if (flag_calibration_status == 2 && worker && worker->isReal3DModeEnabled()) {
                // 【新增】真3D模式：重置Worker的3D模型
                worker->resetReal3DModel();
//                qDebug() << "🔄 R键重置真3D眼球模型";
                // QMessageBox::information(this, "3D模型重置",
                //     "✅ 真3D眼球模型已重置\n"
                //     "👁️ 请重新注视不同方向进行建模");
            } else if (flag_calibration_status == 2) {
                // 【新增】假3D模式：重置假3D的历史数据
                m_rayHistory.clear();
                m_storedIntersections.clear();
                m_eyeCenterHistory.clear();
                m_currentEyeSphereCenter = cv::Point2f(320, 240);
//                qDebug() << "🔄 R键重置假3D历史数据";
                QMessageBox::information(this, "假3D数据重置",
                    "✅ 假3D历史数据已重置\n"
                    "👁️ 眼球中心和射线历史已清除");
            } else if (!m_recording && !isCollecting) {
                // 在非标定状态下，使用手动重置函数
                manualResetCumulativeErrorCorrection();
//                qDebug() << "🔄 R键手动重置所有误差校正";
            } else {
                // 在标定过程中，只重置普通视线校正
                resetGazeCorrection();
            }
            break;



        case Qt::Key_P:
            // P键增加3D模型样本计数（调试功能，加快3D模型构建）
            if (flag_calibration_status == 2 && worker && worker->isReal3DModeEnabled()) {
                worker->add3DModelSamples(10);
//                qDebug() << "📈 P键增加3D模型样本计数，当前进度:" << worker->get3DModelProgress();
            }
            break;

        case Qt::Key_Q:
            // Q键手动触发标定质量评估
            if (!m_calibrationPoints.isEmpty() && !m_gazePoints.isEmpty()) {
                CalibrationQuality quality = evaluateCalibrationQuality();
                showCalibrationResult(quality);
//                qDebug() << "📊 手动触发标定质量评估完成";
            } else {
//                qDebug() << "❌ 无标定数据，无法评估质量";
            }
            break;

    case Qt::Key_E:
            // E键：保留用于其他功能扩展
//            qDebug() << "[E键] 暂未定义功能";
        break;

        case Qt::Key_G:
            // G键：专用于假3D单点零点校正，独立于多点标定
            if (flag_calibration_status == 2 && worker && !worker->isReal3DModeEnabled()) {
                performFake3DCalibration();
                qDebug() << "[G键校正] 执行单点零点偏移校正";
            }
        break;

        case Qt::Key_F:
            // F键切换真3D和假3D模式（仅在3D模式下生效）
            if (flag_calibration_status == 2) {
                if (worker && worker->isReal3DModeEnabled()) {
                    // 当前是真3D，切换到假3D
                    worker->enableReal3DMode(false);
                    qDebug() << "🔄 F键切换到假3D模式";

                } else {
                    // 当前是假3D，切换到真3D
                    double focalLength = 640.0;
                    worker->enableReal3DMode(true, focalLength);
                    qDebug() << "🔄 F键切换到真3D模式";

                }
            }
        break;

//        case Qt::Key_A:
//                    // 切换AR虚拟透明屏幕开关
//                    try {
//                        enableAROverlay(!m_arOverlayEnabled);
//                        calibrateHeadPose();
//                        qDebug() << "AR覆盖层状态切换为:" << (m_arOverlayEnabled ? "开启" : "关闭");

//                    } catch (const std::exception& e) {
//                        qDebug() << "AR切换异常:" << e.what();
//                    }
//                    break;
//    case Qt::Key_Q:
//          // ESC键安全关闭AR覆盖层
//          try {
//              if (m_arOverlayEnabled) {
//                  enableAROverlay(false);
//                  qDebug() << "ESC键关闭AR覆盖层";
//              }
//              if (m_calibError) {
//                     m_calibError->close();
//                     m_calibError->deleteLater();
//                     m_calibError = nullptr;
//                 }
//              // 安全删除AR组件
//              if (m_arUpdateTimer) {
//                  m_arUpdateTimer->stop();
//                  m_arUpdateTimer->deleteLater();
//                  m_arUpdateTimer = nullptr;
//              }

//          } catch (const std::exception& e) {
//              qDebug() << "ESC键处理异常:" << e.what();
//          }
//          break;
//        case Qt::Key_H:
//            calibrateHeadPose();
//            break;

        default:
            break;
    }

    QWidget::keyPressEvent(event); // 保持默认事件传递
}

cv::Point2f MainWindow::mapToScreenUsingPolynomial(const cv::Point2f& pupilPoint) {
    // 检查多项式系数是否已初始化
    if (coeffsX.empty() || coeffsY.empty()) {
//        qDebug() << "❌ 多项式系数未初始化，无法进行坐标映射";
        return cv::Point2f(0, 0);
    }

    // 检查系数矩阵大小是否正确（3次多项式需要10个系数）
    if (coeffsX.rows < 10 || coeffsY.rows < 10) {
//        qDebug() << "❌ 多项式系数数量不足，期望10个系数，实际得到:"
//                 << "coeffsX=" << coeffsX.rows << ", coeffsY=" << coeffsY.rows;
        return cv::Point2f(0, 0);
    }

    // 检查系数矩阵类型是否正确
    if (coeffsX.type() != CV_32F || coeffsY.type() != CV_32F) {
//        qDebug() << "❌ 多项式系数类型不正确，期望CV_32F，实际得到:"
//                 << "coeffsX=" << coeffsX.type() << ", coeffsY=" << coeffsY.type();
        return cv::Point2f(0, 0);
    }

    float x = pupilPoint.x;
    float y = pupilPoint.y;

    float mappedX =
        coeffsX.at<float>(0) + coeffsX.at<float>(1) * x + coeffsX.at<float>(2) * y +
        coeffsX.at<float>(3) * x * x + coeffsX.at<float>(4) * x * y + coeffsX.at<float>(5) * y * y +
        coeffsX.at<float>(6) * x * x * x + coeffsX.at<float>(7) * x * x * y +
        coeffsX.at<float>(8) * x * y * y + coeffsX.at<float>(9) * y * y * y;

    float mappedY =
        coeffsY.at<float>(0) + coeffsY.at<float>(1) * x + coeffsY.at<float>(2) * y +
        coeffsY.at<float>(3) * x * x + coeffsY.at<float>(4) * x * y + coeffsY.at<float>(5) * y * y +
        coeffsY.at<float>(6) * x * x * x + coeffsY.at<float>(7) * x * x * y +
        coeffsY.at<float>(8) * x * y * y + coeffsY.at<float>(9) * y * y * y;

    return cv::Point2f(mappedX, mappedY);
}


void MainWindow::computePolynomialMapping() {
    if (allPupilPoints.size() < 10 || allScreenPoints.size() < 10) {
//        qDebug() << "标定点数不足，无法计算拟合";
        return;
    }

    std::vector<float> screenXs, screenYs;
    for (const auto& pt : allScreenPoints) {
        screenXs.push_back(pt.x);
        screenYs.push_back(pt.y);
    }

    coeffsX = polyFit3(allPupilPoints, screenXs);
    coeffsY = polyFit3(allPupilPoints, screenYs);

//    qDebug() << "標定完成";

}


cv::Mat MainWindow::polyFit3(const std::vector<cv::Point2f>& inputPts, const std::vector<float>& outputVals) {
    int N = inputPts.size();
    cv::Mat A(N, 10, CV_32F);  // 每个点构建 10 个多项式特征
    cv::Mat Y(N, 1, CV_32F);

    for (int i = 0; i < N; ++i) {
        float x = inputPts[i].x;
        float y = inputPts[i].y;

        A.at<float>(i, 0) = 1.0f;
        A.at<float>(i, 1) = x;
        A.at<float>(i, 2) = y;
        A.at<float>(i, 3) = x * x;
        A.at<float>(i, 4) = x * y;
        A.at<float>(i, 5) = y * y;
        A.at<float>(i, 6) = x * x * x;
        A.at<float>(i, 7) = x * x * y;
        A.at<float>(i, 8) = x * y * y;
        A.at<float>(i, 9) = y * y * y;

        Y.at<float>(i, 0) = outputVals[i];
    }

    cv::Mat coeffs;

    // 修复：添加数值稳定性检查
    double conditionNumber = cv::norm(A) * cv::norm(A.inv(cv::DECOMP_SVD));
    qDebug() << QString("[polyFit3] 矩阵条件数估计: %.2e").arg(conditionNumber);

    if (conditionNumber > 1e12) {
        qDebug() << "[polyFit3] 警告：矩阵条件数过大，可能数值不稳定";
    }

    bool success = cv::solve(A, Y, coeffs, cv::DECOMP_SVD);  // 求解最小二乘
    if (!success) {
        qDebug() << "[polyFit3] 错误：矩阵求解失败";
        return cv::Mat();
    }

    return coeffs;
}

void MainWindow::cal_calibration(const std::vector<cv::Point2f>& scenePoints,
                                 const std::vector<cv::Point2f>& vectorPoints)
{
    for (int i = 0; i < 12; i++) {
    A1(i,0)=1.0;
    A1(i,1) = vectorPoints[i].x;
    A1(i, 2) = vectorPoints[i].y;
    A1(i, 3) = vectorPoints[i].x * vectorPoints[i].y;
    A1(i, 4) = vectorPoints[i].x * vectorPoints[i].x;
    A1(i, 5) = vectorPoints[i].y * vectorPoints[i].y;



//        B(i,0)=1.0;
//        B(i,1) = vectors[i].y;
//        B(i, 2) = vectors[i].x;
//        B(i, 3) = vectors[i].x * vectors[i].y;
//        B(i, 4) = vectors[i].y * vectors[i].y;
//        B(i, 5) = vectors[i].x * vectors[i].x;


    // Y坐标变换矩阵 B（修正：与A1使用相同的坐标顺序）
     B(i,0) = 1.0;
     B(i,1) = vectorPoints[i].x;     // 修正：使用x坐标（原来是y）
     B(i,2) = vectorPoints[i].y;     // 修正：使用y坐标（原来是x）
     B(i,3) = vectorPoints[i].x * vectorPoints[i].y;
     B(i,4) = vectorPoints[i].x * vectorPoints[i].x;  // 修正：x²项（原来是y²）
     B(i,5) = vectorPoints[i].y * vectorPoints[i].y;  // 修正：y²项（原来是x²）


    b1(i) = scenePoints[i].x;
    b2(i) = scenePoints[i].y;

}



// 修复：添加数值稳定性检查
auto qr1 = A1.colPivHouseholderQr();
auto qr2 = B.colPivHouseholderQr();

// 检查矩阵条件数（通过秩判断）
int rank1 = qr1.rank();
int rank2 = qr2.rank();
qDebug() << QString("[cal_calibration] 矩阵A1秩: %1/%2, 矩阵B秩: %3/%4").arg(rank1).arg(A1.cols()).arg(rank2).arg(B.cols());

if (rank1 < A1.cols() || rank2 < B.cols()) {
    qDebug() << "[cal_calibration] 警告：矩阵秩不足，可能导致数值不稳定";
}

x1 = qr1.solve(b1);
x2 = qr2.solve(b2);

FILE* fp1;
fp1 = fopen(".//matrix.txt", "w");
for (int i = 0; i < 6; ++i) {
    fprintf(fp1, "%lf %lf \n", x1[i], x2[i]);
}

fclose(fp1);

//    qDebug() << "标定完成，矩阵已保存。";
}


//void MainWindow::cal_calibration(CvPoint2D64f scenecalipoints[], CvPoint2D64f vectors[]){

//    for (int i = 0; i < 12; i++) {


//        A1(i,0)=1.0;
//        A1(i,1) = vectors[i].x;
//        A1(i, 2) = vectors[i].y;
//        A1(i, 3) = vectors[i].x * vectors[i].y;
//        A1(i, 4) = vectors[i].x * vectors[i].x;
//        A1(i, 5) = vectors[i].y * vectors[i].y;



//        B(i,0)=1.0;
//        B(i,1) = vectors[i].y;
//        B(i, 2) = vectors[i].x;
//        B(i, 3) = vectors[i].x * vectors[i].y;
//        B(i, 4) = vectors[i].y * vectors[i].y;
//        B(i, 5) = vectors[i].x * vectors[i].x;




//        b1(i) = scenecalipoints[i].x;
//        b2(i) = scenecalipoints[i].y;

//    }



//    x1 = A1.colPivHouseholderQr().solve(b1);
//    x2 = B.colPivHouseholderQr().solve(b2);

//    FILE* fp1;
//    fp1 = fopen(".//matrix.txt", "w");
//    for (int i = 0; i < 6; ++i) {
//        fprintf(fp1, "%lf %lf \n", x1[i], x2[i]);
//    }

//    fclose(fp1);

//}




// 优化的欧拉角缓存函数
cv::Vec3d MainWindow::getCachedEulerAngles(const cv::Mat& rotationMatrix) {
    // 🔧 关键修复：确保输入矩阵有效且类型一致
    if (rotationMatrix.empty() || rotationMatrix.rows != 3 || rotationMatrix.cols != 3) {
//        qDebug() << "❌ getCachedEulerAngles: 输入旋转矩阵无效";
        return cv::Vec3d(0, 0, 0);
    }

    // 确保数据类型一致
    cv::Mat rotationMatrix64;
    rotationMatrix.convertTo(rotationMatrix64, CV_64F);

    // 检查是否为同一个矩阵（避免重复计算）
    bool canUseCache = m_eulerAnglesValid && !m_lastRotationMatrix.empty() &&
                      m_lastRotationMatrix.rows == 3 && m_lastRotationMatrix.cols == 3;

    if (canUseCache) {
        // 🔧 安全的矩阵比较
        cv::Mat lastMatrix64;
        m_lastRotationMatrix.convertTo(lastMatrix64, CV_64F);

        if (cv::norm(rotationMatrix64, lastMatrix64, cv::NORM_L2) < 1e-10) {
            return m_cachedEulerAngles;
        }
    }

    // 重新计算并缓存
    m_cachedEulerAngles = rotationMatrixToEulerAngles(rotationMatrix64);
    rotationMatrix64.copyTo(m_lastRotationMatrix);
    m_eulerAnglesValid = true;

    return m_cachedEulerAngles;
}

void MainWindow::processDetectedMarkers(const std::map<int, std::vector<cv::Point2f>>& markerMap,
                                       cv::Mat& displayFrame)
{
    if (markerMap.empty()) {
//        qDebug() << "⚠️ 未检测到任何标记，保持上一次有效数据";
        return;
    }

//    qDebug() << "🎯 混合策略: 检测到" << markerMap.size() << "个ArUco标记";

    // 🔧 优化策略：根据标记数量采用不同处理方式
    if (markerMap.size() >= 4) {
        // 3个或更多标记：进行位姿计算
        bool epnpSuccess = tryEPNPMethod(markerMap, displayFrame);
        if (epnpSuccess) {
//            qDebug() << "✅ EPNP/P3P主策略成功，更新位姿数据";
            // 位姿数据已更新，AR系统会使用新数据
            return;
        }
//        qDebug() << "⚠️ EPNP/P3P主策略失败，保持上一次有效数据";
    } else {
        // 1-2个标记：保持上一次有效数据，避免跳变
//        qDebug() << "🔄 标记数量不足3个(" << markerMap.size() << "个)，保持上一次有效位姿数据以避免跳变";

        // 绘制检测到的标记（提供视觉反馈）
        for (const auto& pair : markerMap) {
            cv::Point2f center(0, 0);
            for (const auto& corner : pair.second) {
                center += corner;
            }
            center *= (1.0f / pair.second.size());

            // 用不同颜色标识少量标记
            cv::circle(displayFrame, center, 5, cv::Scalar(0, 255, 255), -1);  // 黄色圆圈
            cv::putText(displayFrame, "ID:" + std::to_string(pair.first) + "(Hold)",
                       cv::Point(center.x + 10, center.y),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 2);
        }
    }

    // 注意：无论哪种情况，都不更新位姿数据，AR系统继续使用上一次的有效数据
    // 这确保了系统的稳定性和连续性
}

// 🚀 主策略：传统EPNP/P3P方法（保持原有高精度）
bool MainWindow::tryEPNPMethod(const std::map<int, std::vector<cv::Point2f>>& markerMap, cv::Mat& displayFrame)
{
    // 检查是否有标准的4个标记（0,1,2,3）
    std::vector<int> requiredIds = {0, 1, 2, 3};
    bool hasAllStandardMarkers = true;
    for (int id : requiredIds) {
        if (markerMap.find(id) == markerMap.end()) {
            hasAllStandardMarkers = false;
            break;
        }
    }

//    qDebug() << "🔧 EPNP主策略: 标准4标记" << (hasAllStandardMarkers ? "完整" : "不完整")
//             << ", 总标记数:" << markerMap.size();

    // 构建3D-2D点对
    std::vector<cv::Point3f> objectPoints;
    std::vector<cv::Point2f> imagePoints;
    std::vector<int> usedIds;

    float screenWidth = 0.31f;
    float screenHeight = 0.199f;

    // 定义标准标记位置
    std::map<int, cv::Point3f> standardPositions = {
        {0, cv::Point3f(-screenWidth/2, -screenHeight/2, 0)},  // ID 0: 左上
        {1, cv::Point3f(screenWidth/2, -screenHeight/2, 0)},   // ID 1: 右上
        {2, cv::Point3f(-screenWidth/2, screenHeight/2, 0)},   // ID 2: 左下
        {3, cv::Point3f(screenWidth/2, screenHeight/2, 0)}     // ID 3: 右下
    };

    // 优先使用标准4个标记
    if (hasAllStandardMarkers) {
        for (int id : requiredIds) {
            objectPoints.push_back(standardPositions[id]);

            // 计算标记中心点
            cv::Point2f center(0, 0);
            for (const auto& corner : markerMap.at(id)) {
                center += corner;
            }
            center *= (1.0f / markerMap.at(id).size());
            imagePoints.push_back(center);
            usedIds.push_back(id);
        }
    } else {
        // 使用可用的标记（降级到3个点）
        for (const auto& pair : markerMap) {
            int id = pair.first;
            if (standardPositions.find(id) != standardPositions.end()) {
                objectPoints.push_back(standardPositions[id]);

                cv::Point2f center(0, 0);
                for (const auto& corner : pair.second) {
                    center += corner;
                }
                center *= (1.0f / pair.second.size());
                imagePoints.push_back(center);
                usedIds.push_back(id);
            }
        }
    }

    if (objectPoints.size() < 3) {
//        qDebug() << "❌ EPNP主策略失败: 可用标记点不足3个";
        return false;
    }

    // 相机参数
    cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) <<
        464, 0, 326,
        0, 462, 225.7,
        0, 0, 1
    );
    cv::Mat distCoeffs = (cv::Mat_<double>(5, 1) << 0, 0, 0, 0, 0);

    // PnP求解
    cv::Mat rvec, tvec;
    bool success = false;

    if (objectPoints.size() >= 4) {
        success = cv::solvePnP(objectPoints, imagePoints, cameraMatrix, distCoeffs,
                              rvec, tvec, false, cv::SOLVEPNP_EPNP);
//        qDebug() << "🔧 EPNP主策略: 使用EPNP算法，点数:" << objectPoints.size();
    } else if (objectPoints.size() == 3) {
        // 对于3个点，使用ITERATIVE算法而非P3P（P3P需要4个点）
        success = cv::solvePnP(objectPoints, imagePoints, cameraMatrix, distCoeffs,
                              rvec, tvec, false, cv::SOLVEPNP_ITERATIVE);
//        qDebug() << "🔧 EPNP主策略: 使用ITERATIVE算法，点数:" << objectPoints.size();
    } else {
//        qDebug() << "❌ EPNP主策略: 点数不足，无法进行PnP求解，点数:" << objectPoints.size();
        return false;
    }

    if (success) {
        // 处理位姿结果（复用原有逻辑）
        processEPNPResult(rvec, tvec, displayFrame, usedIds);
        return true;
    }

//    qDebug() << "❌ EPNP主策略: PnP求解失败";
    return false;
}

// 🚀 补充策略：单ArUco位姿方法
bool MainWindow::trySingleArUcoMethod(const std::map<int, std::vector<cv::Point2f>>& markerMap, cv::Mat& displayFrame)
{
//    qDebug() << "🔧 单ArUco补充策略: 开始处理" << markerMap.size() << "个标记";

    // 🔧 彻底修复：安全地收集标记角点
    std::vector<std::vector<cv::Point2f>> markerCorners;
    std::vector<int> markerIds;

    for (const auto& pair : markerMap) {
        // 验证角点数据
        if (pair.second.size() == 4) {  // ArUco标记必须有4个角点
            markerCorners.push_back(pair.second);
            markerIds.push_back(pair.first);
//            qDebug() << "🔧 收集标记" << pair.first << "，角点数:" << pair.second.size();
        } else {
//            qDebug() << "⚠️ 跳过标记" << pair.first << "，角点数无效:" << pair.second.size();
        }
    }

    if (markerCorners.empty()) {
//        qDebug() << "❌ 单ArUco补充策略: 无有效的标记角点";
        return false;
    }

    // 🔧 彻底修复：安全的相机参数
    cv::Mat cameraMatrix, distCoeffs;
    try {
        cameraMatrix = (cv::Mat_<double>(3, 3) <<
            464, 0, 326,
            0, 462, 225.7,
            0, 0, 1
        );
        distCoeffs = (cv::Mat_<double>(5, 1) << 0, 0, 0, 0, 0);

        // 验证相机参数
        if (cameraMatrix.empty() || cameraMatrix.rows != 3 || cameraMatrix.cols != 3) {
//            qDebug() << "❌ 相机矩阵无效";
            return false;
        }
        if (distCoeffs.empty() || distCoeffs.rows != 5) {
//            qDebug() << "❌ 畸变系数无效";
            return false;
        }
    } catch (const cv::Exception& e) {
//        qDebug() << "❌ 相机参数创建异常:" << e.what();
        return false;
    }

    // 🔧 彻底修复：安全的位姿计算
    std::vector<cv::Mat> rvecs, tvecs;
    try {
//        qDebug() << "🔧 开始estimatePoseSingleMarkers，标记数:" << markerCorners.size();

        // 验证输入数据
        for (size_t i = 0; i < markerCorners.size(); ++i) {
            if (markerCorners[i].size() != 4) {
//                qDebug() << "❌ 标记" << i << "角点数不是4个:" << markerCorners[i].size();
                return false;
            }
        }

        cv::aruco::estimatePoseSingleMarkers(markerCorners, 0.10, cameraMatrix, distCoeffs, rvecs, tvecs);

//        qDebug() << "🔧 estimatePoseSingleMarkers完成，rvecs数:" << rvecs.size() << "，tvecs数:" << tvecs.size();

    } catch (const cv::Exception& e) {
//        qDebug() << "❌ estimatePoseSingleMarkers异常:" << e.what();
        return false;
    } catch (const std::exception& e) {
//        qDebug() << "❌ estimatePoseSingleMarkers标准异常:" << e.what();
        return false;
    } catch (...) {
//        qDebug() << "❌ estimatePoseSingleMarkers未知异常";
        return false;
    }

    if (rvecs.empty() || tvecs.empty()) {
//        qDebug() << "❌ 单ArUco补充策略: estimatePoseSingleMarkers返回空结果";
        return false;
    }

    // 🔧 彻底修复：完整的向量大小验证
    if (rvecs.size() != markerIds.size() || tvecs.size() != markerIds.size() ||
        rvecs.size() != tvecs.size() || rvecs.size() != markerCorners.size()) {
//        qDebug() << "❌ 单ArUco策略向量大小完全不匹配:";
//        qDebug() << "  markerIds=" << markerIds.size();
//        qDebug() << "  markerCorners=" << markerCorners.size();
//        qDebug() << "  rvecs=" << rvecs.size();
//        qDebug() << "  tvecs=" << tvecs.size();
        return false;
    }

    // 🔧 彻底修复：安全的位姿验证和融合
    std::map<int, cv::Mat> validRvecs, validTvecs;
    for (size_t i = 0; i < rvecs.size() && i < tvecs.size() && i < markerIds.size(); ++i) {
        try {
            // 🔧 多重安全检查：确保Mat对象完全有效
            if (rvecs[i].empty() || tvecs[i].empty()) {
//                qDebug() << "❌ 标记" << markerIds[i] << "的位姿Mat为空(补充策略)";
                continue;
            }

            // 检查Mat尺寸
            if (rvecs[i].rows != 3 || rvecs[i].cols != 1 || tvecs[i].rows != 3 || tvecs[i].cols != 1) {
//                qDebug() << "❌ 标记" << markerIds[i] << "的位姿Mat尺寸无效";
//                qDebug() << "  rvec尺寸:" << rvecs[i].rows << "x" << rvecs[i].cols;
//                qDebug() << "  tvec尺寸:" << tvecs[i].rows << "x" << tvecs[i].cols;
                continue;
            }

            // 安全计算距离
            double distance;
            try {
                distance = cv::norm(tvecs[i]);
            } catch (const cv::Exception& e) {
//                qDebug() << "❌ 标记" << markerIds[i] << "距离计算异常:" << e.what();
                continue;
            }

            if (distance > 0.1 && distance < 2.0) {
                // 安全克隆Mat对象
                cv::Mat clonedRvec, clonedTvec;
                try {
                    clonedRvec = rvecs[i].clone();
                    clonedTvec = tvecs[i].clone();

                    // 验证克隆结果
                    if (clonedRvec.empty() || clonedTvec.empty()) {
//                        qDebug() << "❌ 标记" << markerIds[i] << "Mat克隆失败";
                        continue;
                    }

                    validRvecs[markerIds[i]] = clonedRvec;
                    validTvecs[markerIds[i]] = clonedTvec;
//                    qDebug() << "✅ 标记" << markerIds[i] << "位姿有效，距离:" << distance << "m";

                } catch (const cv::Exception& e) {
//                    qDebug() << "❌ 标记" << markerIds[i] << "Mat克隆异常:" << e.what();
                    continue;
                }
            } else {
//                qDebug() << "⚠️ 标记" << markerIds[i] << "距离超出范围:" << distance << "m";
            }

        } catch (const cv::Exception& e) {
//            qDebug() << "❌ 标记" << markerIds[i] << "位姿处理异常:" << e.what();
            continue;
        } catch (const std::exception& e) {
//            qDebug() << "❌ 标记" << markerIds[i] << "位姿处理标准异常:" << e.what();
            continue;
        } catch (...) {
//            qDebug() << "❌ 标记" << markerIds[i] << "位姿处理未知异常";
            continue;
        }
    }

    if (validRvecs.empty()) {
//        qDebug() << "❌ 单ArUco补充策略: 无有效位姿";
        return false;
    }

    // 🔧 彻底修复：安全的位姿融合
    cv::Mat finalRvec, finalTvec;
    bool poseCalculated = false;

    try {
        poseCalculated = fuseSingleArUcoPoses(validRvecs, validTvecs, finalRvec, finalTvec);

        if (poseCalculated) {
            // 验证融合结果
            if (finalRvec.empty() || finalTvec.empty()) {
//                qDebug() << "❌ 位姿融合返回空Mat";
                return false;
            }

            if (finalRvec.rows != 3 || finalRvec.cols != 1 || finalTvec.rows != 3 || finalTvec.cols != 1) {
//                qDebug() << "❌ 位姿融合返回Mat尺寸无效";
                return false;
            }

            // 处理单ArUco位姿结果
            processSingleArUcoResult(finalRvec, finalTvec, displayFrame, markerIds);
            return true;
        }

    } catch (const cv::Exception& e) {
//        qDebug() << "❌ 位姿融合OpenCV异常:" << e.what();
        return false;
    } catch (const std::exception& e) {
//        qDebug() << "❌ 位姿融合标准异常:" << e.what();
        return false;
    } catch (...) {
//        qDebug() << "❌ 位姿融合未知异常";
        return false;
    }

//    qDebug() << "❌ 单ArUco补充策略: 位姿融合失败";
    return false;
}

// 🔧 单ArUco位姿融合函数 - 彻底安全版本
bool MainWindow::fuseSingleArUcoPoses(const std::map<int, cv::Mat>& validRvecs,
                                     const std::map<int, cv::Mat>& validTvecs,
                                     cv::Mat& finalRvec, cv::Mat& finalTvec) {

    try {
        if (validRvecs.empty() || validTvecs.empty()) {
//            qDebug() << "❌ fuseSingleArUcoPoses: 输入为空";
            return false;
        }

        if (validRvecs.size() != validTvecs.size()) {
//            qDebug() << "❌ fuseSingleArUcoPoses: rvecs和tvecs数量不匹配";
            return false;
        }

//        qDebug() << "🔧 融合" << validRvecs.size() << "个有效的单ArUco位姿";

    if (validRvecs.size() >= 4) {
        // 策略A：3个或更多标记 - 使用平均融合
        std::vector<cv::Mat> allRvecs, allTvecs;
        for (const auto& pair : validRvecs) {
            int markerId = pair.first;
            // 🔧 安全检查：确保对应的tvec存在
            if (validTvecs.find(markerId) != validTvecs.end()) {
                allRvecs.push_back(pair.second);
                allTvecs.push_back(validTvecs.at(markerId));
            } else {
//                qDebug() << "⚠️ 标记" << markerId << "的tvec缺失，跳过";
            }
        }

        // 🔧 验证收集到的向量
        if (allRvecs.empty() || allTvecs.empty() || allRvecs.size() != allTvecs.size()) {
//            qDebug() << "❌ 位姿融合失败：收集的位姿向量无效";
            return false;
        }

        finalRvec = cv::Mat::zeros(3, 1, CV_64F);
        finalTvec = cv::Mat::zeros(3, 1, CV_64F);

        for (size_t i = 0; i < allRvecs.size(); ++i) {
            // 🔧 验证Mat对象有效性
            if (allRvecs[i].empty() || allTvecs[i].empty()) {
//                qDebug() << "⚠️ 跳过无效的位姿Mat，索引:" << i;
                continue;
            }

            cv::Mat tempRvec, tempTvec;
            allRvecs[i].convertTo(tempRvec, CV_64F);
            allTvecs[i].convertTo(tempTvec, CV_64F);
            finalRvec += tempRvec;
            finalTvec += tempTvec;
        }

        if (allRvecs.size() > 0) {
            finalRvec /= static_cast<double>(allRvecs.size());
            finalTvec /= static_cast<double>(allTvecs.size());
        } else {
//            qDebug() << "❌ 位姿融合失败：无有效的位姿数据";
            return false;
        }

//        qDebug() << "✅ 使用" << validRvecs.size() << "个标记进行鲁棒位姿融合";

    } else if (validRvecs.size() == 2) {
        // 策略B：2个标记 - 简单平均
        auto it1 = validRvecs.begin();
        auto it2 = std::next(it1);

        // 🔧 安全检查：确保对应的tvec存在
        if (validTvecs.find(it1->first) == validTvecs.end() ||
            validTvecs.find(it2->first) == validTvecs.end()) {
//            qDebug() << "❌ 位姿融合失败：对应的tvec缺失";
            return false;
        }

        cv::Mat rvec1, rvec2, tvec1, tvec2;
        it1->second.convertTo(rvec1, CV_64F);
        it2->second.convertTo(rvec2, CV_64F);
        validTvecs.at(it1->first).convertTo(tvec1, CV_64F);
        validTvecs.at(it2->first).convertTo(tvec2, CV_64F);

        finalRvec = (rvec1 + rvec2) * 0.5;
        finalTvec = (tvec1 + tvec2) * 0.5;

//        qDebug() << "✅ 使用2个标记进行位姿融合";

    } else {
        // 策略C：1个标记 - 直接使用
        auto it = validRvecs.begin();

        // 🔧 安全检查：确保对应的tvec存在
        if (validTvecs.find(it->first) == validTvecs.end()) {
//            qDebug() << "❌ 位姿融合失败：标记" << it->first << "的tvec缺失";
            return false;
        }

        it->second.convertTo(finalRvec, CV_64F);
        validTvecs.at(it->first).convertTo(finalTvec, CV_64F);

//        qDebug() << "✅ 使用单个标记" << it->first << "计算位姿";
    }

    return true;

    } catch (const cv::Exception& e) {
//        qDebug() << "❌ fuseSingleArUcoPoses: OpenCV异常:" << e.what();
        return false;
    } catch (const std::exception& e) {
//        qDebug() << "❌ fuseSingleArUcoPoses: 标准异常:" << e.what();
        return false;
    } catch (...) {
//        qDebug() << "❌ fuseSingleArUcoPoses: 未知异常";
        return false;
    }
}





// 🔧 EPNP结果处理函数
void MainWindow::processEPNPResult(const cv::Mat& rvec, const cv::Mat& tvec,
                                  cv::Mat& displayFrame, const std::vector<int>& usedIds) {

//    qDebug() << "🎯 EPNP主策略位姿处理，使用标记ID:" << usedIds;

    // 🎯 新增：计算透视变换矩阵
    if (usedIds.size() >= 4) {
        // 获取场景图像中的ArUco标记坐标
        std::vector<cv::Point2f> imagePoints;
        for (int id : usedIds) {
            for (const auto& marker : markers) {
                if (marker.id == id) {
                    imagePoints.push_back(cv::Point2f(marker.center.x, marker.center.y));
                    break;
                }
            }
        }


       calculatePerspectiveMatrix(imagePoints);

    }

    // 原有代码：转换为头部相对屏幕的位姿
    cv::Mat screenRotationMatrix;
    cv::Rodrigues(rvec, screenRotationMatrix);

    // 🔧 关键修复：确保输入参数类型一致
    cv::Mat rvec64, tvec64;
    rvec.convertTo(rvec64, CV_64F);
    tvec.convertTo(tvec64, CV_64F);

    cv::Mat screenRotationMatrix64;
    cv::Rodrigues(rvec64, screenRotationMatrix64);

    cv::Mat headToScreenRotation = screenRotationMatrix64.t();
    cv::Mat headToScreenTranslation = -screenRotationMatrix64.t() * tvec64;

    // 确保数据类型一致性
    headToScreenTranslation.convertTo(headToScreenTranslation, CV_64F);

    // 存储头部位姿
    headToScreenRotation.convertTo(m_headRotationMatrix, CV_32F);
    /* headToScreenTranslation.convertTo(m_headTranslationVector, CV_32F); */

    // 计算欧拉角
    cv::Vec3d headEulerAngles = getCachedEulerAngles(headToScreenRotation);

    double roll = headEulerAngles[0];
    double pitch = headEulerAngles[1];
    double yaw = headEulerAngles[2];

//    qDebug() << "📊 EPNP主策略结果:";
//    qDebug() << "Distance:" << cv::norm(headToScreenTranslation) << "m";
//    qDebug() << "Position (x,y,z):" << headToScreenTranslation.at<double>(0)
//             << headToScreenTranslation.at<double>(1) << headToScreenTranslation.at<double>(2);
//    qDebug() << "Euler angles - Roll:" << roll * 180.0/CV_PI << "°, Pitch:" << pitch * 180.0/CV_PI
//             << "°, Yaw:" << yaw * 180.0/CV_PI << "°";

    // 绘制头部位姿信息
    drawPoseInfo(displayFrame, headToScreenTranslation, headEulerAngles);

    // 存储计算结果
    /* m_headEulerAngles = headEulerAngles; */
}

// 🔧 单ArUco结果处理函数
void MainWindow::processSingleArUcoResult(const cv::Mat& rvec, const cv::Mat& tvec,
                                         cv::Mat& displayFrame, const std::vector<int>& markerIds) {

//    qDebug() << "🎯 单ArUco补充策略位姿处理，基于标记ID:" << markerIds;

    // 单ArUco位姿已经是相机相对标记的位姿
    // 需要转换为头部相对屏幕的位姿

    // 🔧 关键修复：确保输入参数类型一致
    cv::Mat rvec64, tvec64;
    rvec.convertTo(rvec64, CV_64F);
    tvec.convertTo(tvec64, CV_64F);

    cv::Mat rotationMatrix64;
    cv::Rodrigues(rvec64, rotationMatrix64);

    // 假设单个标记代表屏幕的局部坐标系，需要变换到屏幕全局坐标系
    // 这里简化处理，直接使用单标记位姿
    cv::Mat headToScreenRotation = rotationMatrix64.t();
    cv::Mat headToScreenTranslation = -rotationMatrix64.t() * tvec64;

    // 确保数据类型一致性
    headToScreenTranslation.convertTo(headToScreenTranslation, CV_64F);

    // 存储头部位姿
    headToScreenRotation.convertTo(m_headRotationMatrix, CV_32F);
    /* headToScreenTranslation.convertTo(m_headTranslationVector, CV_32F); */

    // 计算欧拉角
    cv::Vec3d headEulerAngles = getCachedEulerAngles(headToScreenRotation);

    double roll = headEulerAngles[0];
    double pitch = headEulerAngles[1];
    double yaw = headEulerAngles[2];

//    qDebug() << "📊 单ArUco补充策略结果:";
//    qDebug() << "Distance:" << cv::norm(headToScreenTranslation) << "m";
//    qDebug() << "Position (x,y,z):" << headToScreenTranslation.at<double>(0)
//             << headToScreenTranslation.at<double>(1) << headToScreenTranslation.at<double>(2);
//    qDebug() << "Euler angles - Roll:" << roll * 180.0/CV_PI << "°, Pitch:" << pitch * 180.0/CV_PI
//             << "°, Yaw:" << yaw * 180.0/CV_PI << "°";

    // 绘制头部位姿信息
    drawPoseInfo(displayFrame, headToScreenTranslation, headEulerAngles);

    // 存储计算结果
    /* m_headEulerAngles = headEulerAngles; */
}


// 绘制头部位姿信息到图像上（用于眼动跟踪应用）
// 修正：移除未使用的参数
void MainWindow::drawPoseInfo(cv::Mat& frame, const cv::Mat& headTranslation,
                             const cv::Vec3d& eulerAngles)
{
    // 🔧 安全性检查
    if (frame.empty() || headTranslation.empty()) {
//        qDebug() << "❌ drawPoseInfo: 输入参数无效";
        return;
    }

    if (headTranslation.rows != 3 || headTranslation.cols != 1) {
//        qDebug() << "❌ drawPoseInfo: headTranslation矩阵尺寸无效";
        return;
    }

    // 设置文本参数
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    double fontScale = 0.6;
    int thickness = 2;
    cv::Scalar textColor(255, 255, 255); // 白色文字

    // 🔧 安全访问头部位置数据
    double posX = 0.0, posY = 0.0, posZ = 0.0;
    double distance = 0.0;

    try {
        posX = headTranslation.at<double>(0);
        posY = headTranslation.at<double>(1);
        posZ = headTranslation.at<double>(2);
        distance = cv::norm(headTranslation);
    } catch (const cv::Exception& e) {
//        qDebug() << "❌ drawPoseInfo: 矩阵访问异常:" << e.what();
        return;
    }

    // 准备显示头部位姿信息（相对于屏幕）
    std::vector<std::string> texts;
    texts.push_back("Head Pose for Eye Tracking");
    texts.push_back(cv::format("Distance: %.3f m", distance));
    texts.push_back(cv::format("Head Position (x,y,z):"));
    texts.push_back(cv::format("  (%.3f, %.3f, %.3f) m", posX, posY, posZ));
    texts.push_back(cv::format("Head Orientation:"));
    texts.push_back(cv::format("  Roll:  %+.1f°", eulerAngles[0] * 180.0/CV_PI));
    texts.push_back(cv::format("  Pitch: %+.1f°", eulerAngles[1] * 180.0/CV_PI));
    texts.push_back(cv::format("  Yaw:   %+.1f°", eulerAngles[2] * 180.0/CV_PI));

    // 计算文本区域
    int lineHeight = 25;
    int startY = 30;
    int startX = 10;
    int maxWidth = 0;

    // 计算最大文本宽度
    for (const auto& text : texts) {
        cv::Size textSize = cv::getTextSize(text, fontFace, fontScale, thickness, nullptr);
        maxWidth = max(maxWidth, textSize.width); // 修正：使用max
    }

    // 绘制半透明背景
    cv::Rect bgRect(startX - 5, startY - 20, maxWidth + 15, static_cast<int>(texts.size()) * lineHeight + 10);
    cv::Mat overlay;
    frame.copyTo(overlay);
    cv::rectangle(overlay, bgRect, cv::Scalar(0, 0, 0), -1);
    cv::addWeighted(frame, 0.7, overlay, 0.3, 0, frame);

    // 绘制边框
    cv::rectangle(frame, bgRect, cv::Scalar(100, 100, 100), 1);

    // 绘制文本
    for (size_t i = 0; i < texts.size(); ++i) {
        cv::Point textPos(startX, startY + static_cast<int>(i) * lineHeight);
        cv::putText(frame, texts[i], textPos, fontFace, fontScale, textColor, thickness);
    }
}


// 从旋转矩阵提取欧拉角的实现（ZYX内在旋转，返回roll, pitch, yaw）
// 改进：增强数值稳定性和错误处理
cv::Vec3d MainWindow::rotationMatrixToEulerAngles(const cv::Mat& R) {
    // 🔧 增强输入验证
    if (R.empty() || R.rows != 3 || R.cols != 3) {
//        qDebug() << "❌ rotationMatrixToEulerAngles: 输入旋转矩阵无效";
        return cv::Vec3d(0, 0, 0);
    }

    // 🔧 安全的数据类型转换
    cv::Mat rotation;
    try {
        R.convertTo(rotation, CV_64F);

        // 验证转换后的矩阵
        if (rotation.empty() || rotation.rows != 3 || rotation.cols != 3) {
//            qDebug() << "❌ rotationMatrixToEulerAngles: 类型转换后矩阵无效";
            return cv::Vec3d(0, 0, 0);
        }
    } catch (const cv::Exception& e) {
//        qDebug() << "❌ rotationMatrixToEulerAngles: 类型转换异常:" << e.what();
        return cv::Vec3d(0, 0, 0);
    }

    // 验证旋转矩阵的有效性（正交性检查）
    cv::Mat shouldBeIdentity = rotation * rotation.t();
    cv::Mat identity = cv::Mat::eye(3, 3, CV_64F);
    double orthogonalityError = cv::norm(shouldBeIdentity, identity, cv::NORM_L2);

    if (orthogonalityError > 0.1) {  // 容忍一定误差
//        qDebug() << "Warning: Rotation matrix may not be orthogonal, error:" << orthogonalityError;
    }

    // 🔧 安全的矩阵元素访问
    double sy;
    try {
        sy = sqrt(rotation.at<double>(0,0) * rotation.at<double>(0,0) +
                 rotation.at<double>(1,0) * rotation.at<double>(1,0));
    } catch (const cv::Exception& e) {
//        qDebug() << "❌ rotationMatrixToEulerAngles: 矩阵访问异常:" << e.what();
        return cv::Vec3d(0, 0, 0);
    }

    // 使用更实用的阈值，避免数值不稳定
    bool singular = sy < 1e-4;  // 从1e-6改为1e-4

    double roll, pitch, yaw;
    try {
        if (!singular) {
            // 修正：正确的欧拉角定义，符合头部运动语义
            // ZYX内在旋转顺序：先Z轴(Yaw)，再Y轴(Pitch)，最后X轴(Roll)
            yaw =  atan2(-rotation.at<double>(2,0), sy);
            pitch =atan2(rotation.at<double>(2,1), rotation.at<double>(2,2));
            roll = atan2(rotation.at<double>(1,0), rotation.at<double>(0,0));

        } else {
            // 万向锁情况处理：使用更稳定的计算方法
            yaw = 0; // 在万向锁情况下，yaw角度不确定，设为0
            pitch = atan2(-rotation.at<double>(2,0), sy);
            roll = atan2(rotation.at<double>(1,0), rotation.at<double>(0,0));

            // 只在调试模式下输出警告，避免影响性能
            static int warningCount = 0;
            if (++warningCount % 100 == 0) {  // 每100次警告一次
//                qDebug() << "Warning: Gimbal lock detected (count:" << warningCount << ")";
            }
        }
    } catch (const cv::Exception& e) {
//        qDebug() << "❌ rotationMatrixToEulerAngles: 欧拉角计算异常:" << e.what();
        return cv::Vec3d(0, 0, 0);
    }

    // 角度范围检查（可选）
    if (std::abs(roll) > CV_PI || std::abs(pitch) > CV_PI/2 || std::abs(yaw) > CV_PI) {
//        qDebug() << "Warning: Unusual Euler angles - Roll:" << roll * 180/CV_PI
//                 << "Pitch:" << pitch * 180/CV_PI << "Yaw:" << yaw * 180/CV_PI;
    }
    if (pitch > M_PI/2) {
        pitch = pitch - M_PI;  // 177.518° 会变成 -2.482°
    } else if (pitch < -M_PI/2) {
        pitch = pitch + M_PI;
    }
    return cv::Vec3d(roll, pitch, yaw);  // 返回顺序：roll, pitch, yaw
}




// 可选：保留透视变换功能
void MainWindow::calculatePerspectiveMatrix(const std::vector<cv::Point2f>& imagePoints)
{
    int screenWidth = static_cast<int>(m_screenWidth);
    int screenHeight = static_cast<int>(m_screenHeight);



    std::vector<cv::Point2f> dstPoints = {
        cv::Point2f(0, 0),
        cv::Point2f(screenWidth, 0),
        cv::Point2f(0, screenHeight),
        cv::Point2f(screenWidth, screenHeight)
    };

    cv::Mat perspectiveMatrix = cv::getPerspectiveTransform(imagePoints, dstPoints);
    m_perspectiveMatrix = perspectiveMatrix;
}

qreal MainWindow::getDevicePixelRatio() const
{
    QScreen *screen = QGuiApplication::primaryScreen();
    return screen ? screen->devicePixelRatio() : 1.0;
}

int MainWindow::scaleValue(int value) const
{
    return static_cast<int>(value * getDevicePixelRatio());
}

// mainwindow.cpp

void MainWindow::updateImage()
{
    // 记录处理流程的开始时间，用于性能分析


    // 仅在两个摄像头都准备好新帧时才继续
    // eyeImage 和 sceneImage 由 updateCamera1Label/updateCamera2Label 槽函数异步更新
    if (!eyeImage.empty() && !sceneImage.empty()) {

        // 根据冻结状态选择图像源
        cv::Mat currentEyeImage, currentSceneImage;

        if (m_imageProcessingPaused && m_hasFrozenImages) {
            // 使用冻结的图像
            currentEyeImage = m_frozenEyeImage;
            currentSceneImage = m_frozenSceneImage;
        } else {
            // 使用实时图像
            currentEyeImage = eyeImage;
            currentSceneImage = sceneImage;
        }

        // 标定模式时使用高精度标志
        bool isForCalibration = m_singlePointCalibrationMode;

        // 【核心】发出信号，请求子线程处理。
        // 使用 clone() 来传递数据的深拷贝，确保线程安全。
        emit frameProcessingRequested(currentEyeImage.clone(), currentSceneImage.clone(), isForCalibration);
    }
}




// 在 mainwindow.cpp 中实现
void MainWindow::showCalibrationPoint(const cv::Point2f& point) {
    // 如果校准点标签不存在，创建它
    if (!calibrationPointLabel) {
        calibrationPointLabel = new QLabel(this);
        calibrationPointLabel->setFixedSize(20, 20);
        calibrationPointLabel->setStyleSheet("background-color: red; border-radius: 10px;");
        calibrationPointLabel->raise(); // 确保在最上层
    }

    // 设置校准点位置
    calibrationPointLabel->move(point.x - 10, point.y - 10);
    calibrationPointLabel->setVisible(true);
    calibrationPointLabel->raise();

    // 强制界面更新
    QApplication::processEvents();
}

void MainWindow::showCalibrationInstruction(const QString& instruction) {
    // 如果指令标签不存在，创建它
    if (!instructionLabel) {
        instructionLabel = new QLabel(this);
        instructionLabel->setAlignment(Qt::AlignCenter);
        instructionLabel->setStyleSheet(
            "QLabel {"
            "background-color: rgba(0, 0, 0, 180);"
            "color: white;"
            "font-size: 16px;"
            "font-weight: bold;"
            "padding: 20px;"
            "border-radius: 10px;"
            "}"
        );
        instructionLabel->setWordWrap(true);
        instructionLabel->setMinimumSize(400, 100);
    }

    // 设置指令文本
    instructionLabel->setText(instruction);

    // 居中显示
    int x = (this->width() - instructionLabel->width()) / 2;
    int y = this->height() / 4;  // 在屏幕上方1/4处显示
    instructionLabel->move(x, y);
    instructionLabel->setVisible(true);
    instructionLabel->raise();

    // 强制界面更新
    QApplication::processEvents();
}

void MainWindow::hideCalibrationUI() {
    if (calibrationPointLabel) {
        calibrationPointLabel->setVisible(false);
    }

    if (instructionLabel) {
        instructionLabel->setVisible(false);
    }

    // 强制界面更新
    QApplication::processEvents();
}

void MainWindow::showMessage(const QString& message) {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("校准结果");
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

// B键和C键功能的新函数实现
void MainWindow::startSinglePointCalibration() {
    // 清理之前的标定数据
    rightPupil = PointVector();
    Markgaze = PointVector();
    markers.clear();

    // 设置标定模式
    m_singlePointCalibrationMode = true;
    CALIBRATIONPOINTS9 = 1;

    // 加载标定标记图像
    cv::Mat calibImage = cv::imread("CalibMeCollectionMarker.png");
    if (calibImage.empty()) {
        QMessageBox::warning(this, "错误", "无法加载标定标记图像 CalibMeCollectionMarker.png");
        m_calibrationActive = false;
        m_singlePointCalibrationMode = false;
        return;
    }

    // 显示标定界面
    showCalibrationMarker(calibImage);

//    qDebug() << "🎯 单点标定模式已启动";
}

void MainWindow::finishSinglePointCalibration() {
    if (rightPupil.size() < 4) {
        QMessageBox::information(this, "提示", "标定数据不足，需要至少4个数据点");
        return;
    }

    // 执行标定计算
    bool calibrationSuccess = calibrate();

    if (calibrationSuccess) {
        QMessageBox::information(this, "成功", "单点演示标定完成！");
        m_singlePointCalibrationMode = false;

        // 清理标定数据
        rightPupil = PointVector();
    } else {
        QMessageBox::warning(this, "失败", "标定计算失败，请重试");
    }

    // 关闭标定界面
    hideCalibrationMarker();

//    qDebug() << "🏁 单点标定结果:" << (calibrationSuccess ? "成功" : "失败");
}

void MainWindow::showCalibrationMarker(const cv::Mat& calibImage) {
    // 转换为Qt图像格式
    QImage qImage;
    if (calibImage.channels() == 3) {
        cv::Mat rgbImage;
        cv::cvtColor(calibImage, rgbImage, cv::COLOR_BGR2RGB);
        qImage = QImage(rgbImage.data, rgbImage.cols, rgbImage.rows, rgbImage.step, QImage::Format_RGB888);
    } else {
        qImage = QImage(calibImage.data, calibImage.cols, calibImage.rows, calibImage.step, QImage::Format_Grayscale8);
    }

    // 创建显示标签窗口
    if (labelm) {
        labelm->close();
        delete labelm;
    }

    labelm = new QLabel();
    labelm->installEventFilter(this);  // 安装事件过滤器处理鼠标点击
    labelm->setPixmap(QPixmap::fromImage(qImage));
    labelm->setWindowTitle("单点标定 - 点击标记进行标定");
    labelm->show();
    labelm->setAttribute(Qt::WA_DeleteOnClose);
}

void MainWindow::hideCalibrationMarker() {
    if (labelm) {
        labelm->close();
        labelm = nullptr;
    }
}

void MainWindow::updateStatusIndicators() {
    QString statusText = "";

    if (m_imageProcessingPaused) {
        statusText += "冻结模式 ";
    }

    if (m_calibrationActive) {
        statusText += "标定模式 ";
    }

    if (!statusText.isEmpty()) {
        this->statusBar()->showMessage(statusText, 0);
    } else {
        this->statusBar()->clearMessage();
    }
}

cv::Point2f MainWindow::processCalibrationSamples(const std::vector<cv::Point2f>& samples) {
    if (samples.size() < 10) {
//        qDebug() << "采样点数量不足:" << samples.size();
        return cv::Point2f(-1, -1);  // 返回无效点
    }

    // 使用中位数绝对偏差(MAD)去除异常值
    std::vector<cv::Point2f> validSamples;

    // 计算中位数
    std::vector<float> xValues, yValues;
    for (const auto& p : samples) {
        xValues.push_back(p.x);
        yValues.push_back(p.y);
    }

    std::sort(xValues.begin(), xValues.end());
    std::sort(yValues.begin(), yValues.end());

    float medianX = xValues[xValues.size() / 2];
    float medianY = yValues[yValues.size() / 2];

    // 计算MAD (中位数绝对偏差)
    std::vector<float> deviationsX, deviationsY;
    for (const auto& p : samples) {
        deviationsX.push_back(std::abs(p.x - medianX));
        deviationsY.push_back(std::abs(p.y - medianY));
    }

    std::sort(deviationsX.begin(), deviationsX.end());
    std::sort(deviationsY.begin(), deviationsY.end());

    float madX = deviationsX[deviationsX.size() / 2];
    float madY = deviationsY[deviationsY.size() / 2];

    // 防止MAD为0的情况
    if (madX < 1e-6) madX = 1.0f;
    if (madY < 1e-6) madY = 1.0f;

    // 去除异常值 (大于3倍MAD的点)
    float thresholdX = 3.0f * madX;
    float thresholdY = 3.0f * madY;

    for (const auto& p : samples) {
        if (std::abs(p.x - medianX) <= thresholdX &&
            std::abs(p.y - medianY) <= thresholdY) {
            validSamples.push_back(p);
        }
    }

    if (validSamples.size() < 3) {
//        qDebug() << "有效采样点过少:" << validSamples.size() << "，使用中位数";
        return cv::Point2f(medianX, medianY);
    }

    // 计算有效样本的均值
    cv::Point2f avg(0, 0);
    for (const auto& p : validSamples) {
        avg += p;
    }
    avg *= (1.0f / validSamples.size());

//    qDebug() << "原始采样点数:" << samples.size() << "有效点数:" << validSamples.size()
//             << "中位数:(" << medianX << "," << medianY << ") 均值:(" << avg.x << "," << avg.y << ")";

    return avg;
}


void MainWindow::clickCalibButton()
{

    if(0 == flag_calibration_status){
        calibDlg *calibdlg = new calibDlg();
        connect(calibdlg, &calibDlg::calibrationFinished, this, &MainWindow::onCalibrationReceived);
        calibdlg->show();

//        qDebug() << "开始眼动校准...";
        // 初始化校准器


    }else if (flag_calibration_status == 1) { // 演示模式
//        qDebug() << "启动演示标定流程...";

        // 【关键】演示标定流程不应该关心'unknowns'或拟合方法
        // 它只关心要标定的点数
        CALIBRATIONPOINTS9 = ui->comboBox->currentText().toUInt();

        // 直接启动 ShowCal 窗口
        calWin = new ShowCal(CALIBRATIONPOINTS9, this);
        connect(calWin, &ShowCal::calibrationFinished, this, &MainWindow::onCalibrationReceived);
        calWin->show();
    }

}


void MainWindow::clickReloadButton()
{
    if (0 == flag_calibration_status) {
        // ================== 修改后的精确模式加载逻辑 ==================
//        qDebug() << "正在为精确模式加载标定参数...";

        FILE* fp = fopen(".//jingque_matrix.txt", "r");
        if (fp == NULL) {
            QMessageBox::warning(this, "加载失败", "找不到精确标定参数文件 (jingque_matrix.txt)！\n请先进行一次精确标定。");
            return;
        }

        int file_unknowns = 0;
        int file_plType = 0;

        // 读取文件头信息（可选，但推荐）
        fscanf(fp, " unknowns: %d\n", &file_unknowns);
        fscanf(fp, " plType: %d\n", &file_plType);

        // 验证参数数量是否匹配
        if (file_unknowns != unknowns) {
            QMessageBox::critical(this, "加载错误",
                QString("参数文件与当前配置不匹配！\n文件参数个数: %1\n当前需要: %2")
                .arg(file_unknowns).arg(unknowns));
            fclose(fp);
            return;
        }

        // 调整矩阵大小以确保匹配
        rcx = Mat::zeros(unknowns, 1, CV_64F);
        rcy = Mat::zeros(unknowns, 1, CV_64F);

        bool load_success = true;
        for (int i = 0; i < unknowns; ++i) {
            double temp_rcx, temp_rcy;
            if (fscanf(fp, "%lf %lf", &temp_rcx, &temp_rcy) == 2) {
                rcx.at<double>(i) = temp_rcx;
                rcy.at<double>(i) = temp_rcy;
            } else {
                load_success = false;
                break; // 读取失败，退出循环
            }
        }

        fclose(fp);

        if (load_success) {
            calibration_success = true; // 标记为已标定/已加载
            monoRightCalibrated = true;
            QMessageBox::information(this, "加载成功", "精确标定参数已成功加载！");
//            qDebug() << "精确标定参数加载成功。";
        } else {
            QMessageBox::critical(this, "加载错误", "读取参数文件时发生错误，文件可能已损坏。");
        }
//        FILE* fp ;//C://Users//Hewie//Desktop//613
//        fp = fopen(".//matrix.txt", "r" );
//        if(fp==NULL)
//        {
//            QMessageBox::information(this,"info","can not open matrix.txt!");
//        }
//        //     fseek( fp, 0L, SEEK_SET );
//        /*double temp[3][3]={0};

//        for(int i=0; i<3;i++)
//        {

//            fscanf( fp, "%lf %lf %lf", &temp[i][0],&temp[i][1],&temp[i][2]);

//            this->map_matrix[i][0]=temp[i][0];
//            this->map_matrix[i][1]=temp[i][1];
//            this->map_matrix[i][2]=temp[i][2];
//        }*/

//        double temp[6][2]={0};

//        for(int i=0; i<6;i++)
//        {

//            fscanf( fp, "%lf %lf ", &temp[i][0],&temp[i][1]);

//            this->map_matrix[i][0]=temp[i][0];
//            this->map_matrix[i][1]=temp[i][1];
//        }

//        fclose(fp);


    }else if(1 == flag_calibration_status){


        FILE* fp = fopen(".//yanshi_matrix.txt", "r");
           if (fp == NULL) {
               QMessageBox::information(this, "info", "无法打开 yanshi_matrix.txt 文件！");
               return;
           }

           double temp[3][3] = {0};
           int rowsRead = 0;

           // ShowCal现在统一输出3x3矩阵格式，直接读取3x3矩阵
           for (int i = 0; i < 3; ++i) {
               if (fscanf(fp, "%lf %lf %lf", &temp[i][0], &temp[i][1], &temp[i][2]) == 3) {
                   rowsRead++;
               } else {
                   QMessageBox::warning(this, "警告", QString("矩阵第%1行格式错误！").arg(i+1));
                   fclose(fp);
                   return;
               }
           }

           if (rowsRead == 3) {
               // 加载3x3变换矩阵
               for (int i = 0; i < 3; ++i)
                   for (int j = 0; j < 3; ++j)
                       this->map_yanshi[i][j] = temp[i][j];

               // 检查是否为仿射变换（最后一行是否为 0 0 1）
               if (abs(temp[2][0]) < 1e-6 && abs(temp[2][1]) < 1e-6 && abs(temp[2][2] - 1.0) < 1e-6) {
//                   qDebug() << "加载仿射变换矩阵成功";
               } else {
//                   qDebug() << "加载透视变换矩阵成功";
               }
           } else {
               QMessageBox::warning(this, "警告", "变换矩阵加载失败！");
           }

           fclose(fp);
    }
    //QMessageBox::information(this,"infomation","重载成功");
}



CvPoint2D32f MainWindow::Homography_map_point(CvPoint2D32f p, double map_affine[2][3])
{
    CvPoint2D32f p2;
    p2.x = map_affine[0][0] * p.x + map_affine[0][1] * p.y + map_affine[0][2];
    p2.y = map_affine[1][0] * p.x + map_affine[1][1] * p.y + map_affine[1][2];
    return p2;
}

CvPoint2D32f MainWindow::Homography_map_point_homography(CvPoint2D32f p, double H[3][3]) {
    double x = p.x;
    double y = p.y;
    double w = H[2][0]*x + H[2][1]*y + H[2][2];
    CvPoint2D32f p2;
    p2.x = (H[0][0]*x + H[0][1]*y + H[0][2]) / w;
    p2.y = (H[1][0]*x + H[1][1]*y + H[1][2]) / w;
    return p2;
}


CvPoint2D32f  MainWindow::Homography_map_point1(CvPoint2D32f  p,double map_matrix[][3])
{
    CvPoint2D32f  p2;
    double z = map_matrix[2][0]*p.x + map_matrix[2][1]*p.y + map_matrix[2][2];
    p2.x = (float)((map_matrix[0][0]*p.x + map_matrix[0][1]*p.y + map_matrix[0][2])/z);
    p2.y = (float)((map_matrix[1][0]*p.x + map_matrix[1][1]*p.y + map_matrix[1][2])/z);
    return p2;
}



CvPoint2D32f  MainWindow::Zuixiaoercheng_map_point(CvPoint2D32f  p,double map_matrix[6][2])
{
//    CvPoint2D32f  p2;
//    p2.x =map_matrix[0][0] + map_matrix[1][0] * p.x + map_matrix[2][0] * p.y + map_matrix[3][0] * p.x * p.y + map_matrix[4][0] * p.x * p.x + map_matrix[5][0] * p.y * p.y ;
//    p2.y =map_matrix[0][1] + map_matrix[1][1] * p.y + map_matrix[2][1] * p.x + map_matrix[3][1] * p.x * p.y + map_matrix[4][1] * p.y * p.y + map_matrix[5][1] * p.x * p.x;
//    return p2;


    CvPoint2D32f p2;

     // X坐标变换（保持原有逻辑）
     p2.x = map_matrix[0][0] + map_matrix[1][0] * p.x + map_matrix[2][0] * p.y +
            map_matrix[3][0] * p.x * p.y + map_matrix[4][0] * p.x * p.x + map_matrix[5][0] * p.y * p.y;

     // Y坐标变换（修正：统一坐标使用顺序）
     p2.y = map_matrix[0][1] + map_matrix[1][1] * p.x + map_matrix[2][1] * p.y +
            map_matrix[3][1] * p.x * p.y + map_matrix[4][1] * p.x * p.x + map_matrix[5][1] * p.y * p.y;

     return p2;
}

//CvPoint2D32f MainWindow::Zuixiaoercheng_map_point(CvPoint2D32f p, double map_matrix[6][2])
//{
//    CvPoint2D32f p2;
//    double x = p.x, y = p.y;

//    // 标准的二次多项式映射
//    p2.x = map_matrix[0][0] + map_matrix[1][0] * x + map_matrix[2][0] * y +
//           map_matrix[3][0] * x * y + map_matrix[4][0] * x * x + map_matrix[5][0] * y * y;

//    p2.y = map_matrix[0][1] + map_matrix[1][1] * x + map_matrix[2][1] * y +
//           map_matrix[3][1] * x * y + map_matrix[4][1] * x * x + map_matrix[5][1] * y * y;

//    return p2;
//}

//CvPoint2D32f MainWindow::Zuixiaoercheng_map_point(CvPoint2D32f p, double map_matrix[6][2]) {
//    double x = p.x, y = p.y;
//    CvPoint2D32f p2;
//    p2.x = map_matrix[0][0] + map_matrix[1][0]*x + map_matrix[2][0]*y +
//           map_matrix[3][0]*x*y + map_matrix[4][0]*x*x + map_matrix[5][0]*y*y;

//    p2.y = map_matrix[0][1] + map_matrix[1][1]*x + map_matrix[2][1]*y +
//           map_matrix[3][1]*x*y + map_matrix[4][1]*x*x + map_matrix[5][1]*y*y;

//    return p2;
//}



void MainWindow::compute_angle(cv::Point2f gaze)
{
    yaw=qAtan((gaze.x-eyeImage.cols/2.0)*l_perPix/z_length)*(180.0/3.1415926);
    pitch=qAtan((eyeImage.rows/2.0-gaze.y)*l_perPix/qSqrt(z_length*z_length+((gaze.x-eyeImage.cols/2.0)*l_perPix)*(gaze.x-eyeImage.cols/2.0)*l_perPix))*(180.0/3.1415926);
}





void MainWindow::showCalibrationWindow()
{
    // 【修复】标定开始时重置并禁用所有误差校正，确保收集纯净的原始数据
    resetCumulativeErrorCorrection();
    resetGazeCorrection();
    m_cumulativeErrorCorrectionEnabled = false;
    m_gazeCorrectionEnabled = false;

    if(!m_calibError)
    {
        m_waitingForSpace=true;
        m_recording = true;
        m_currentPoint = 0;
        m_gazeCount = 0;
        m_samplingActive = false;
        m_gazePoints.clear();
        m_originalGazePoints.clear();
        m_errors.clear();
        m_calibError = new CalibError(m_screenWidth, m_screenHeight, this);
        m_calibError->installEventFilter(this);

        // 连接CalibError的误差计算信号
//        connect(m_calibError, &CalibError::firstPointCompleted, this, &MainWindow::onFirstPointCompleted);
//        connect(m_calibError, &CalibError::allPointsCompleted, this, &MainWindow::onAllPointsCompleted);
//        connect(m_calibError, &CalibError::errorCalculationRequest, this, &MainWindow::onErrorCalculationRequest);
//        connect(m_calibError, &CalibError::pointCompleted, this, &MainWindow::onPointCompleted);
//        connect(m_calibError, &CalibError::cycleReset, this, &MainWindow::onCycleReset);
    }
    predefinedScreenPoints.clear();
    predefinedScreenPoints = generateCalibrationPoints(CALIBRATIONPOINTS9);

    // 初始化 m_calibrationPoints 为 CalibError 提供标定点数据
    m_calibrationPoints.clear();
    for (const auto& pt : predefinedScreenPoints) {
        m_calibrationPoints.append(pt);
    }

    m_calibError->setGeometry(0, 0, m_screenWidth, m_screenHeight);
    m_calibError->resetForNewTest(); // 清空数据
    m_calibError->show();
}


void MainWindow::on_TestButton_clicked()
{
    // 【修复】精确测试开始时禁用误差校正，确保收集纯净的原始数据
    resetCumulativeErrorCorrection();
    resetGazeCorrection();
    m_cumulativeErrorCorrectionEnabled = false;
    m_gazeCorrectionEnabled = false;

//    if(0 == flag_calibration_status)
    {   m_waitingForSpace=true;
        m_recording = true;
        m_currentPoint = 0;
        m_gazeCount = 0;
        m_samplingActive = false;
        m_gazePoints.clear();
        m_originalGazePoints.clear();
        m_errors.clear();
        //m_calibrationImage.fill(Qt::white);

        // 打开标定窗口
        showCalibrationWindow();
    }

}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        switch (keyEvent->key()) {
            case Qt::Key_D:
            m_dKeyPressCount++;
//            qDebug() << "🔢 D键统计 - 总按压:" << m_dKeyPressCount
//                     << " 有效采集:" << m_validCollectionCount
//                     << " 失败采集:" << m_failedCollectionCount;
                this->on_calibImgCollectBtn_clicked();
                isCollecting = true;
                collectedCount = 0;
                currentSamplePoints.clear();
//                qDebug() << "开始采集瞳孔中心...";

                break;
            case Qt::Key_Escape:

                break;



            case Qt::Key_C:
                gmark = !gmark;

                if (gmark) {
                     on_MarkerBtn_clicked();  // 显示标记，开始采集数据
                } else {
                    on_Markcalib_clicked();   // 执行CalibMe标定算法
                    if(labelm) labelm->close();
                }

                // this->on_TestButton_clicked();
                break;

            case Qt::Key_B:

                break;

            case Qt::Key_T:
                // T键：绘制屏幕视线坐标（实时显示当前注视点位置）
                qDebug() << "[T键] 开启视线坐标绘制模式";
                setFreeGazeMode(!m_freeGazeMode);
                break;

            case Qt::Key_Space:
                // 条件更新：只有在等待空格，并且当前点未超出范围时允许采样
                // 在循环模式下，允许currentPoint重置后继续
                if (!m_recording || m_samplingActive || !m_waitingForSpace)
                {
                    // 如果当前点超出范围且不在等待状态，直接返回
                    if (m_currentPoint >= m_calibrationPoints.size() && !m_waitingForSpace) {
                        return true;
                    }
                    return true;
                }

                // 启动当前点采样
                m_samplingActive = true;
                m_waitingForSpace = false;  // 空格已按，不再等待
                m_gazeCount = 0;

                if (m_gazePoints.size() <= m_currentPoint) {
                    m_gazePoints.push_back(QVector<cv::Point2f>());
                    m_originalGazePoints.push_back(QVector<cv::Point2f>());
                    m_errors.push_back(QVector<double>());
                }

//                qDebug() << "开始采样点" << m_currentPoint + 1;
                break;
        case Qt::Key_E:
                // E键：保留用于其他功能，目前在2D模式下执行单点校正
            if (flag_calibration_status == 0) {
                performSinglePointCorrection();
            }
            break;

        case Qt::Key_G:
            // G键：专用于假3D单点零点校正，独立于多点标定
            if (flag_calibration_status == 2 && worker && !worker->isReal3DModeEnabled()) {
                performFake3DCalibration();
                qDebug() << "[G键校正] 执行单点零点偏移校正";
            }
            break;

            default:
                break;
        }

    }

    return QWidget::eventFilter(obj, event);
}

void MainWindow::onCalibrationReceived()
{
//   qDebug() << "Received calibrationFinished signal!";
   clickReloadButton(); // 加载刚刚保存的矩阵
   yanshiCalibrated = true; // **关键：设置标定成功标志**
   QMessageBox::information(this, "标定完成", "演示模式标定已完成并自动重载！");
}

void MainWindow::displayResult(const Mat &img_matches, double yaw, double pitch, double roll) {
//    qDebug() << "Received result: Yaw =" << yaw << ", Pitch =" << pitch << ", Roll =" << roll;
    imshow("ORB Feature Matches (Filtered by Essential Matrix)", img_matches);
    waitKey(1);


    // 假设 yaw [-45°, 45°] → x [0, screenWidth]
    // 假设 pitch [-30°, 30°] → y [0, screenHeight]
    double x = ((-pitch + 20.0) / 40.0) * m_screenWidth;
    double y = ((roll + 10.0) / 20.0) * m_screenHeight; // pitch 向上为负，屏幕 y 向下为正

//    // 限制 gaze 不越界
//    x = clamp(x, 0.0, (double)m_screenWidth);
//    y = clamp(y, 0.0, (double)m_screenHeight);

//    qDebug() << "Gaze point on screen: (" << x << "," << y << ")";

}

void MainWindow::handleError(const QString &msg) {
//    qDebug() << "Error from worker:" << msg;
    if (msg.contains("Error:")) {
        worker->stop();
//        qDebug() << "Critical error detected, stopping worker";
    }
}

void MainWindow::on_TestResultButton_clicked()
{



//    if(0 == flag_calibration_status){
//        testResultDlg *testDlg = new testResultDlg();
//        testDlg->show();
//    }


}

void MainWindow::on_calibPointBtn_clicked()
{
    calibPointDlg *calibPoint = new calibPointDlg();
    calibPoint->show();
}

void MainWindow::on_calibImgCollectBtn_clicked()
{

    num = num % CALIBRATIONPOINTS9;
    ui->label_num->setText(QString::number(num+1));
    if(0 == flag_calibration_status){
        // for(int i = 0; i <5; ++i){
        char eyeName[128],sceneName[128];
        QDir().mkpath("./calibImage");
        sprintf((char*)&eyeName[0],".//calibImage//eye_%d.png",num);
        sprintf((char*)&sceneName[0],".//calibImage//scene_%d.png",num);
        cvtColor(sceneImage,sceneImage,CV_RGB2BGR);
        //cvtColor(eyeImage,eyeImage,CV_RGB2BGR);
        cv::imwrite(eyeName,orig_eye);
        cv::imwrite(sceneName,sceneImage);


        if(num == CALIBRATIONPOINTS9-1){
//           QMessageBox::information(this,"infomation","已经采集完毕，请标定");
            num_yanshi=0;
        }
        num++;



        this->update();
    }else if (flag_calibration_status == 1) {

        int totalPoints = (CALIBRATIONPOINTS9 == 10) ? 12 : CALIBRATIONPOINTS9;
        if (num_yanshi >= totalPoints) {
//            qDebug() << "所有演示标定点图像已请求采集。";
            return;
        }

        ui->label_num->setText(QString::number(num_yanshi + 1));
//        qDebug() << "请求采集演示标定帧，点:" << num_yanshi + 1;

        // 【关键】发出一个带有“标定”标志的特殊处理请求
        emit frameProcessingRequested(eyeImage.clone(), sceneImage.clone(), true);
    }else if (flag_calibration_status == 2) {
        // 3D模式：与精确模式类似的处理
        ui->label_num->setText(QString::number(num+1));
//        qDebug() << "3D模式：请求采集标定数据，点:" << num + 1;

        // 发出处理请求，触发数据采集
        emit frameProcessingRequested(eyeImage.clone(), sceneImage.clone(), true);

        if(num == CALIBRATIONPOINTS9-1){
            num = 0;
        } else {
            num++;
        }
    }
}

void MainWindow::on_setOrignalBtn_clicked()
{
    this->num_yanshi = 0;
    this->num = 0;
    ui->label_num->setText("0");
    m_samplingActive = false;
    collectedCount = 0;           // 重置计数器
    calibrationIndex = 0;
//    QMessageBox::information(this,"infomation","SUCCESS");
}

//void MainWindow::on_pushButton_clicked()
//{

//}



void MainWindow::on_pointcalibButton_2_clicked()
{

//    qDebug() << "开始新的多点标定，正在清理旧数据...";
    allPupilPoints.clear();
    allScreenPoints.clear();
    m_predictedPoints.clear();
    m_errorVectors.clear();
    rightPupil = PointVector(); // 假设 PointVector() 会创建一个新的空对象
    Markgaze = PointVector();   // 同上

    // 从UI获取用户选择的点数和拟合方法对应的未知数数量
    CALIBRATIONPOINTS9 = ui->comboBox->currentText().toUInt();
    // 'unknowns' 变量会在 on_comboBox_2_currentIndexChanged 中被设置

    // 【健壮性检查】
    // 检查采集的点数是否足够支持当前选择的多项式复杂度
    if (CALIBRATIONPOINTS9 < unknowns && flag_calibration_status==0) {
        QMessageBox::critical(this, "配置错误",
            QString("您选择的标定点数 (%1个) 不足以支持所选的拟合方法 (需要 %2个系数)。\n\n"
                    "请选择更多的标定点，或选择一个更简单的拟合方法。")
            .arg(CALIBRATIONPOINTS9)
            .arg(unknowns));
        return; // 阻止后续操作，不显示标定窗口
    }

    // 如果检查通过，则正常显示标定点窗口
    showPoint = new ShowPoint(CALIBRATIONPOINTS9, this);
    showPoint->show();
    showPoint->installEventFilter(this);
}

void MainWindow::on_MarkerBtn_clicked()
{
    //   QMutexLocker locker(&cfgMutex);
    rightPupil = PointVector();  // 创建新对象，旧数据被释放
    Markgaze = PointVector();    // 同理
    markers.clear();
    //    std::cout << "rightPupil size: " << rightPupil.size() << std::endl;  // 应输出 0
    //    std::cout << "Markgaze size: " << Markgaze.size() << std::endl;      // 应输出 0


    if(gmark)
    {
        Mat tm=imread("CalibMeCollectionMarker.png");

        CALIBRATIONPOINTS9=1;

        QImage image(tm.data, tm.cols, tm.rows, tm.step, QImage::Format_RGB888);

        // 创建Qt窗口和标签
        labelm = new QLabel();
        labelm->installEventFilter(this);
        labelm->setPixmap(QPixmap::fromImage(image));
        labelm->show();
        // 设置窗口在关闭时自动删除
        labelm->setAttribute(Qt::WA_DeleteOnClose);

    }

}

void MainWindow::handleCamera1Image(Timestamp t, Mat frame)
{

    eyeImage= frame.clone();
//    flip(eyeImage,eyeImage,0);
    timestamp = t;
    //m_grabber->getCurrentFrame(); // 从 FrameGrabber 获取当前帧
    //updateImage
}

void MainWindow::handleCamera2Image(Timestamp t, Mat frame)
{

    sceneImage= frame.clone();

    // 注释：共享内存更新移至主处理循环，避免重复调用和数据不同步

}

//cv::Mat MainWindow::generateErrorPlot() {
//    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
//    int screenWidth = screenRect.width();
//    int screenHeight = screenRect.height();

//    // 屏幕物理参数（基于用户提供的屏幕规格）
//    const double MM_PER_PIXEL = 0.125391;   // 毫米/像素 (321mm / 2560px)
//    const double VIEWING_DISTANCE_MM = 500.0; // 观看距离60cm

//    // 配置参数 - 动态调整文本区域宽度
//    const int TEXT_AREA_WIDTH = min(320, screenWidth / 4);  // 限制文本区域最大为屏幕宽度的1/4
//    const int TEXT_START_X = 10;            // 文本起始X坐标
//    const int TEXT_START_Y = 30;            // 文本起始Y坐标
//    const int LINE_HEIGHT = min(22, screenHeight / 35); // 动态行高，确保35行文本能完全显示
//    const double CIRCLE_ALPHA = 0.3;        // 圆形透明度
//    const cv::Scalar CIRCLE_COLOR = cv::Scalar(100, 200, 255); // 圆形颜色（浅橙色）
//    const cv::Scalar TEXT_COLOR = cv::Scalar(50, 50, 50);      // 文本颜色（深灰色）
//    const cv::Scalar SEPARATOR_COLOR = cv::Scalar(200, 200, 200); // 分割线颜色
//    const cv::Scalar METRIC_COLOR = cv::Scalar(0, 100, 200);   // 指标文本颜色（深蓝色）

//    // 创建画布 - 使用屏幕尺寸，避免截断
//    cv::Mat errorPlot(screenHeight, screenWidth, CV_8UC3, cv::Scalar(255, 255, 255));

//    // 绘制分割线
//    cv::line(errorPlot, cv::Point(TEXT_AREA_WIDTH, 0),
//             cv::Point(TEXT_AREA_WIDTH, screenHeight), SEPARATOR_COLOR, 1);

//    // 计算每个点的误差并存储
//    std::vector<double> errorMagnitudes;
//    double totalErrorMagnitude = 0.0;

//    for (size_t i = 0; i < allScreenPoints.size(); ++i) {
//        cv::Point2f error = allScreenPoints[i] - m_predictedPoints[i];
//        double magnitude = cv::norm(error);
//        errorMagnitudes.push_back(magnitude);
//        totalErrorMagnitude += magnitude;
//    }

//    // 计算眼动跟踪指标
//    double avgErrorMagnitude = totalErrorMagnitude / allScreenPoints.size();



//    // 转换为物理单位
//    double avgErrorMM = avgErrorMagnitude * MM_PER_PIXEL;


//    // 转换为视角度数
//    double avgErrorDegrees = atan(avgErrorMM / VIEWING_DISTANCE_MM) * 180.0 / M_PI;


//    // 创建透明圆形的overlay图层
//    cv::Mat overlay = errorPlot.clone();

//    // 绘制透明误差圆形区域
//    // 计算绘图区域（右侧区域，去除文本区域）
//    float plotAreaWidth = screenWidth - TEXT_AREA_WIDTH;
//    float plotAreaHeight = screenHeight;

//    for (size_t i = 0; i < allScreenPoints.size(); ++i) {
//        cv::Point2f originalPoint = allScreenPoints[i];

//        // 将原始屏幕坐标映射到绘图区域
//        cv::Point2f adjustedScreenPoint;
//        adjustedScreenPoint.x = TEXT_AREA_WIDTH + (originalPoint.x * plotAreaWidth / screenWidth);
//        adjustedScreenPoint.y = originalPoint.y * plotAreaHeight / screenHeight;

//        // 绘制误差圆形（半径为误差大小）
//        int radius = static_cast<int>(errorMagnitudes[i]);
//        if (radius > 0) {
//            cv::circle(overlay, adjustedScreenPoint, radius, CIRCLE_COLOR, -1);
//        }
//    }

//    // 应用透明效果
//    cv::addWeighted(errorPlot, 1.0 - CIRCLE_ALPHA, overlay, CIRCLE_ALPHA, 0, errorPlot);

//    // 绘制真实点（红色圆圈）
//    for (const auto& screenPoint : allScreenPoints) {
//        cv::Point2f adjustedPoint;
//        adjustedPoint.x = TEXT_AREA_WIDTH + (screenPoint.x * plotAreaWidth / screenWidth);
//        adjustedPoint.y = screenPoint.y * plotAreaHeight / screenHeight;
//        cv::circle(errorPlot, adjustedPoint, 5, cv::Scalar(0, 0, 255), -1);
//    }

//    // 绘制预测点（绿色圆圈）
//    for (const auto& predictedPoint : m_predictedPoints) {
//        cv::Point2f adjustedPoint;
//        adjustedPoint.x = TEXT_AREA_WIDTH + (predictedPoint.x * plotAreaWidth / screenWidth);
//        adjustedPoint.y = predictedPoint.y * plotAreaHeight / screenHeight;
//        cv::circle(errorPlot, adjustedPoint, 3, cv::Scalar(0, 255, 0), -1);
//    }

//    // 绘制误差向量（蓝色箭头）
//    for (size_t i = 0; i < allScreenPoints.size(); ++i) {
//        cv::Point2f adjustedStart, adjustedEnd;
//        adjustedStart.x = TEXT_AREA_WIDTH + (m_predictedPoints[i].x * plotAreaWidth / screenWidth);
//        adjustedStart.y = m_predictedPoints[i].y * plotAreaHeight / screenHeight;
//        adjustedEnd.x = TEXT_AREA_WIDTH + (allScreenPoints[i].x * plotAreaWidth / screenWidth);
//        adjustedEnd.y = allScreenPoints[i].y * plotAreaHeight / screenHeight;
//        cv::arrowedLine(errorPlot, adjustedStart, adjustedEnd, cv::Scalar(255, 0, 0), 1, 8, 0, 0.1);
//    }

//    // 绘制平均误差向量（紫色箭头）
//    cv::Point2f center(TEXT_AREA_WIDTH + plotAreaWidth / 2, plotAreaHeight / 2);
//    cv::Point2f averageErrorEnd = center + m_averageError;
//    cv::arrowedLine(errorPlot, center, averageErrorEnd, cv::Scalar(255, 0, 255), 2, 8, 0, 0.1);

//    // 在左侧添加文本信息
//    int currentY = TEXT_START_Y;

//    // 标题
//    cv::putText(errorPlot, "Eye Tracking Analysis",
//                cv::Point(TEXT_START_X, currentY),
//                cv::FONT_HERSHEY_SIMPLEX, 0.7, TEXT_COLOR, 2);
//    currentY += LINE_HEIGHT + 10;

//    // 分割线
//    cv::line(errorPlot, cv::Point(TEXT_START_X, currentY - 5),
//             cv::Point(TEXT_AREA_WIDTH - 10, currentY - 5), TEXT_COLOR, 1);
//    currentY += 10;

//    // 每个点的误差信息（像素和毫米）
//    for (size_t i = 0; i < errorMagnitudes.size(); ++i) {
//        double errorMM = errorMagnitudes[i] * MM_PER_PIXEL;
//        std::string errorText = "P" + std::to_string(i + 1) + ": " +
//                               std::to_string(static_cast<int>(errorMagnitudes[i] * 10) / 10.0) + "px/" +
//                               std::to_string(static_cast<int>(errorMM * 100) / 100.0) + "mm";
//        cv::putText(errorPlot, errorText,
//                    cv::Point(TEXT_START_X, currentY),
//                    cv::FONT_HERSHEY_SIMPLEX, 0.45, TEXT_COLOR, 1);
//        currentY += LINE_HEIGHT;
//    }

//    // 添加空行
//    currentY += 10;

//    // 分割线
//    cv::line(errorPlot, cv::Point(TEXT_START_X, currentY - 5),
//             cv::Point(TEXT_AREA_WIDTH - 10, currentY - 5), METRIC_COLOR, 1);
//    currentY += 5;

//    // 准确度指标（平均误差）
//    std::string accuracyText1 = "Accuracy (avg error):";
//    cv::putText(errorPlot, accuracyText1,
//                cv::Point(TEXT_START_X, currentY),
//                cv::FONT_HERSHEY_SIMPLEX, 0.5, METRIC_COLOR, 1);
//    currentY += LINE_HEIGHT;

//    std::string accuracyText2 = std::to_string(static_cast<int>(avgErrorMagnitude * 10) / 10.0) + " px / " +
//                               std::to_string(static_cast<int>(avgErrorMM * 100) / 100.0) + " mm";
//    cv::putText(errorPlot, accuracyText2,
//                cv::Point(TEXT_START_X + 10, currentY),
//                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);
//    currentY += LINE_HEIGHT;

//    std::string accuracyText3 = std::to_string(static_cast<int>(avgErrorDegrees * 1000) / 1000.0) + " degrees";
//    cv::putText(errorPlot, accuracyText3,
//                cv::Point(TEXT_START_X + 10, currentY),
//                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);
//    currentY += LINE_HEIGHT + 5;



//    // 添加图例说明（移到右侧图表区域）
//    int legendX = TEXT_AREA_WIDTH + 10;
//    int legendY = 20;
//    cv::putText(errorPlot, "Red: True Points", cv::Point(legendX, legendY),
//                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
//    cv::putText(errorPlot, "Green: Predicted Points", cv::Point(legendX, legendY + 20),
//                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
//    cv::putText(errorPlot, "Blue: Error Vectors", cv::Point(legendX, legendY + 40),
//                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);
//    cv::putText(errorPlot, "Orange: Error Circles", cv::Point(legendX, legendY + 60),
//                cv::FONT_HERSHEY_SIMPLEX, 0.5, CIRCLE_COLOR, 1);

//    return errorPlot;
//}

cv::Mat MainWindow::generateErrorPlot() {
    // ==== 屏幕物理参数（保持你的设定） ====
    const double MM_PER_PIXEL = 0.125391;      // 毫米/像素 (321mm / 2560px)
    const double VIEWING_DISTANCE_MM = 500.0;  // 观看距离 500mm

    // ==== 画布参数：改为紧凑型小画布 ====
    const int PLOT_W = 800;    // 右侧误差图宽
    const int PLOT_H = 600;    // 右侧误差图高
    const int TEXT_AREA_WIDTH = 280; // 左侧文本栏
    const int CANVAS_W = TEXT_AREA_WIDTH + PLOT_W;
    const int CANVAS_H = max(PLOT_H, 540);

    // 文本与样式
    const int TEXT_START_X = 12;
    const int TEXT_START_Y = 28;
    const int LINE_HEIGHT  = 22;
    const cv::Scalar TEXT_COLOR(50, 50, 50);
    const cv::Scalar SEPARATOR_COLOR(200, 200, 200);
    const cv::Scalar METRIC_COLOR(0, 100, 200);

    // 绘制样式（更饱满）
    const double CIRCLE_ALPHA = 0.5;
    const cv::Scalar CIRCLE_COLOR(80, 180, 255); // 浅蓝
    const cv::Scalar TRUE_PT_COLOR(0, 0, 255);   // 红
    const cv::Scalar PRED_PT_COLOR(0, 255, 0);   // 绿
    const cv::Scalar VEC_COLOR(255, 0, 0);       // 蓝
    const cv::Scalar AVG_VEC_COLOR(255, 0, 255); // 紫

    // ==== 如果你的点是以“原屏幕分辨率”采集的，需要这个原始屏幕宽高 ====
    // 若外部已知，可替换为固定值；默认取当前主屏（仅用于坐标映射比例）
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    const int SCREEN_W = screenRect.width();
    const int SCREEN_H = screenRect.height();

    // 创建紧凑画布
    cv::Mat canvas(CANVAS_H, CANVAS_W, CV_8UC3, cv::Scalar(255, 255, 255));

    // 左右分割线
    cv::line(canvas, cv::Point(TEXT_AREA_WIDTH, 0),
             cv::Point(TEXT_AREA_WIDTH, CANVAS_H), SEPARATOR_COLOR, 1);

    // ====== 计算误差 ======
    std::vector<double> errorMagnitudes;
    errorMagnitudes.reserve(allScreenPoints.size());
    double totalErrorMagnitude = 0.0;

    for (size_t i = 0; i < allScreenPoints.size(); ++i) {
        cv::Point2f d = allScreenPoints[i] - m_predictedPoints[i]; // 均为“原屏坐标”下的像素偏差
        double mag = cv::norm(d);
        errorMagnitudes.push_back(mag);
        totalErrorMagnitude += mag;
    }

    double avgErrorMagnitude = allScreenPoints.empty() ? 0.0
                            : totalErrorMagnitude / static_cast<double>(allScreenPoints.size());

    double avgErrorMM = avgErrorMagnitude * MM_PER_PIXEL;
    double avgErrorDegrees = atan(avgErrorMM / VIEWING_DISTANCE_MM) * 180.0 / M_PI;

    // ====== p95 自适应半径缩放 ======
    double scale_px = 1.0;
    int MIN_RADIUS = 6;  // 最小半径，保证小误差可见
    int MAX_RADIUS = max(24, int(PLOT_W * 0.06)); // 上限半径

    if (!errorMagnitudes.empty()) {
        std::vector<double> errs = errorMagnitudes;
        std::sort(errs.begin(), errs.end());
        size_t idx = errs.size() * 95 / 100;
        if (idx >= errs.size()) idx = errs.size() - 1;
        double p95 = errs[idx];
        // 目标：让 p95 误差的可视半径 ≈ PLOT_W 的 3%
        double targetP95Radius = max(12.0, PLOT_W * 0.03);
        scale_px = (p95 > 1e-6) ? (targetP95Radius / p95) : 1.0;
    }

    // ====== 绘制透明误差圆（右侧紧凑绘图区域） ======
    cv::Mat overlay = canvas.clone();

    // 将“原屏坐标”映射到紧凑绘图区域
    auto mapToPlot = [&](const cv::Point2f& p) -> cv::Point2f {
        cv::Point2f q;
        q.x = TEXT_AREA_WIDTH + (p.x * PLOT_W / SCREEN_W);
        q.y = p.y * PLOT_H / SCREEN_H;
        // 注意纵向放在画布上端对齐，若想垂直居中，可加一个偏移 (CANVAS_H - PLOT_H)/2
        q.y += (CANVAS_H - PLOT_H) * 0.5f;
        return q;
    };

    for (size_t i = 0; i < allScreenPoints.size(); ++i) {
        cv::Point2f center = mapToPlot(allScreenPoints[i]);
        int radius = int(std::round(errorMagnitudes[i] * scale_px));
        radius = max(MIN_RADIUS, min(radius, MAX_RADIUS));
        cv::circle(overlay, center, radius, CIRCLE_COLOR, -1, cv::LINE_AA);
    }
    cv::addWeighted(canvas, 1.0 - CIRCLE_ALPHA, overlay, CIRCLE_ALPHA, 0, canvas);

    // 真值点（红）与预测点（绿）
    for (const auto& pt : allScreenPoints) {
        cv::circle(canvas, mapToPlot(pt), 5, TRUE_PT_COLOR, -1, cv::LINE_AA);
    }
    for (const auto& pt : m_predictedPoints) {
        cv::circle(canvas, mapToPlot(pt), 3, PRED_PT_COLOR, -1, cv::LINE_AA);
    }

    // 误差向量（蓝箭头）
    for (size_t i = 0; i < allScreenPoints.size(); ++i) {
        cv::Point2f s = mapToPlot(m_predictedPoints[i]);
        cv::Point2f e = mapToPlot(allScreenPoints[i]);
        cv::arrowedLine(canvas, s, e, VEC_COLOR, 1, cv::LINE_AA, 0, 0.15);
    }

    // 平均误差向量（以绘图区中心为起点）
    cv::Point2f plotCenter(TEXT_AREA_WIDTH + PLOT_W * 0.5f, (CANVAS_H - PLOT_H) * 0.5f + PLOT_H * 0.5f);
    // m_averageError 假定是“原屏坐标”下的像素偏移，需要映射到紧凑绘图区尺度
    cv::Point2f avgVec(m_averageError.x * (float(PLOT_W) / SCREEN_W),
                       m_averageError.y * (float(PLOT_H) / SCREEN_H));
    cv::arrowedLine(canvas, plotCenter, plotCenter + avgVec, AVG_VEC_COLOR, 2, cv::LINE_AA, 0, 0.15);

    // ====== 左侧文本 ======
    int y = TEXT_START_Y;

//    cv::putText(canvas, "Eye Tracking Analysis (Compact)",
//                cv::Point(TEXT_START_X, y), cv::FONT_HERSHEY_SIMPLEX, 0.65, TEXT_COLOR, 1, cv::LINE_AA);
    y += LINE_HEIGHT + 8;

    cv::line(canvas, cv::Point(TEXT_START_X, y), cv::Point(TEXT_AREA_WIDTH - 10, y), SEPARATOR_COLOR, 1);
    y += 10;

    // 仅输出前若干点的误差，避免文本溢出（可调 MAX_LIST）
    const int MAX_LIST = min<int>(30, int(errorMagnitudes.size()));
    for (int i = 0; i < MAX_LIST; ++i) {
        double px = errorMagnitudes[i];
        double mm = px * MM_PER_PIXEL;
        std::ostringstream oss;
        oss.setf(std::ios::fixed); oss.precision(1);
        oss << "P" << (i + 1) << ": " << px << " px / ";
        oss.precision(2);
        oss << mm << " mm";
        cv::putText(canvas, oss.str(), cv::Point(TEXT_START_X, y),
                    cv::FONT_HERSHEY_SIMPLEX, 0.48, TEXT_COLOR, 1, cv::LINE_AA);
        y += LINE_HEIGHT;
        if (y > CANVAS_H - 80) break; // 超出则截断
    }

    y += 6;
    cv::line(canvas, cv::Point(TEXT_START_X, y), cv::Point(TEXT_AREA_WIDTH - 10, y), METRIC_COLOR, 1);
    y += 8;

    // 概要指标
    {
        cv::putText(canvas, "Accuracy (avg error):",
                    cv::Point(TEXT_START_X, y),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, METRIC_COLOR, 1, cv::LINE_AA);
        y += LINE_HEIGHT;

        // 平均误差 px / mm
        std::ostringstream oss1;
        oss1.setf(std::ios::fixed);
        oss1.precision(1);
        oss1 << avgErrorMagnitude << " px / ";
        oss1.precision(2);
        oss1 << avgErrorMM << " mm";
        cv::putText(canvas, oss1.str(), cv::Point(TEXT_START_X + 12, y),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1, cv::LINE_AA);
        y += LINE_HEIGHT;

        // 平均误差 角度
        std::ostringstream oss2;
        oss2.setf(std::ios::fixed);
        oss2.precision(3);
        oss2 << avgErrorDegrees << " deg";
        cv::putText(canvas, oss2.str(), cv::Point(TEXT_START_X + 12, y),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1, cv::LINE_AA);
        y += LINE_HEIGHT;
    }

    // 右侧图例（绘图区左上）
    int legendX = TEXT_AREA_WIDTH + 10;
    int legendY = int((CANVAS_H - PLOT_H) * 0.5) + 18;
    cv::putText(canvas, "Red: True Points",    cv::Point(legendX, legendY+10),       cv::FONT_HERSHEY_SIMPLEX, 1, TRUE_PT_COLOR, 1, cv::LINE_AA);
    cv::putText(canvas, "Green: Predicted",    cv::Point(legendX, legendY + 40),  cv::FONT_HERSHEY_SIMPLEX, 1, PRED_PT_COLOR, 1, cv::LINE_AA);
    cv::putText(canvas, "Blue: Error Vectors", cv::Point(legendX, legendY + 75),  cv::FONT_HERSHEY_SIMPLEX, 1, VEC_COLOR,     1, cv::LINE_AA);
    cv::putText(canvas, "Cyan: Error Circles", cv::Point(legendX, legendY + 110),  cv::FONT_HERSHEY_SIMPLEX, 1, CIRCLE_COLOR,  1, cv::LINE_AA);

    return canvas;
}


void MainWindow::showErrorPlot() {
    qDebug() << "🔴 [调用追踪] showErrorPlot() 被调用 - 2D精确标定误差显示";
    cv::Mat errorPlot = generateErrorPlot();

    cv::cvtColor(errorPlot, errorPlot, cv::COLOR_BGR2RGB);
    imshow("errorPlot",errorPlot);
    waitKey(1);
}


//POLYFIT.cpp
// 文件: mainwindow.cpp

bool MainWindow::calibrate()
{
    // 【修复】3D模式且假3D已标定时，跳过传统2D标定
    if (flag_calibration_status == 2 && worker && !worker->isReal3DModeEnabled() && m_fake3D_multiPointCalibrated) {
        qDebug() << "假3D模式已标定，跳过传统2D标定计算";
        return true;  // 返回成功，避免执行下面的传统标定逻辑，但不显示误差图
    }

    // 【数据验证】检查传统标定数据是否足够
    if (allPupilPoints.size() < unknowns || allScreenPoints.size() < unknowns) {
        qDebug() << QString("传统标定数据不足：瞳孔点%1个，屏幕点%2个，需要至少%3个")
                    .arg(allPupilPoints.size()).arg(allScreenPoints.size()).arg(unknowns);
//        return false;
    }

    // 【新增的关键修复】
    // 在进行任何计算之前，根据当前选择的拟合方法所需的系数数量 (unknowns)，
    // 强制重新创建 rcx 和 rcy 矩阵。
    // 这会为它们分配正确大小的新内存，从而解决了从简单->复杂切换时内存越界的问题。
    rcx = cv::Mat1d(unknowns, 1);
    rcy = cv::Mat1d(unknowns, 1);

    // 调用底层的多项式拟合函数，分别为X和Y坐标计算系数
    monoRightCalibrated = calibrate(rightPupil.x, rightPupil.y, Markgaze.x, rcx)
            && calibrate(rightPupil.x, rightPupil.y, Markgaze.y, rcy);

    // 如果标定成功，则进行后续处理
    if (monoRightCalibrated) {
            calibration_success = true;
            m_predictedPoints.clear();
            m_errorVectors.clear();

            // 使用刚刚计算出的系数，来预测每个瞳孔点对应的屏幕位置
            for (const auto& pupilPoint : allPupilPoints) {
                m_predictedPoints.push_back(evaluate(pupilPoint));
            }

            // 计算平均误差幅度，用于评估标定质量
            double totalErrorMagnitude = 0.0;
            for (size_t i = 0; i < allScreenPoints.size(); ++i) {
                cv::Point2f error = allScreenPoints[i] - m_predictedPoints[i];
                totalErrorMagnitude += cv::norm(error);
            }
            double avgErrorMagnitude = totalErrorMagnitude / allScreenPoints.size();
//            qDebug() << "标定完成，平均误差幅度: " << avgErrorMagnitude << " 像素";

            // 如果是精确标定模式，则保存参数并显示误差图
            if(0 == flag_calibration_status)
            {
//                qDebug() << "精确标定成功，正在保存标定参数...";

                if (saveJingqueCalibrationParams()) {
                    QMessageBox::information(this, "保存成功", "精确标定参数已成功保存到 jingque_matrix.txt");
                } else {
                    QMessageBox::warning(this, "保存失败", "无法保存精确标定参数，请检查文件权限。");
                }
                // 显示误差分析图
                showErrorPlot();
             }
             // 如果是3D标定模式，则根据子模式选择相应处理
             else if(2 == flag_calibration_status)
             {
//                 qDebug() << "3D标定成功，正在保存标定参数...";

                 // 检查是否为假3D模式
                 if (worker && !worker->isReal3DModeEnabled() && m_fake3D_multiPointCalibrated) {
                     // 假3D模式：不保存传统标定参数，不显示误差图
                     qDebug() << "假3D多点标定完成，跳过传统标定保存和误差图显示";
                 } else {
                     // 真3D模式或传统3D模式：使用原有逻辑
                     if (saveJingqueCalibrationParams()) {
                         QMessageBox::information(this, "保存成功", "3D标定参数已成功保存到 jingque_matrix.txt");
                     } else {
                         QMessageBox::warning(this, "保存失败", "无法保存3D标定参数，请检查文件权限。");
                     }

                     // 显示误差分析图
                     showErrorPlot();
                 }
             }
        }

    return monoRightCalibrated;
}



cv::Point2f MainWindow::evaluate(cv::Point2f gcenter)
{
    // 3D注视模式分支，完全兼容现有逻辑
    if (flag_calibration_status == 2 && m_gaze3DCalculator) {
        return calculateGaze3D(gcenter);
    }

    // 保持现有2D计算逻辑完全不变
    cv::Point2f estimate(-1, -1);

    estimate.x = evaluate(
                gcenter.x,
                gcenter.y,
                rcx);
    estimate.y = evaluate(
                gcenter.x,
                gcenter.y,
                rcy);

    return estimate;
}

float MainWindow::evaluate(float x, float y, Mat1d &c)
{

    if (unknowns > c.rows) {
            // 如果需要的多于实际有的，说明新算法还未成功标定，数据不匹配。
            // 此时不进行任何计算，直接返回0，安全地阻止崩溃。
            return 0.0f;
        }

    double xx = x*x;
    double yy = y*y;
    double xy = x*y;
    double xyy = x*yy;
    double yxx = y*xx;
    double xxyy = xx*yy;
    double xxx = x*xx;
    double yyy = y*yy;

    float estimate;
    //    plType = POLY_1_X_Y_XY_XX_YY;
    switch (plType) {
    case POLY_1_X_Y_XY:
        estimate = c.at<double>(0) +
                x*c.at<double>(1) +
                y*c.at<double>(2) +
                xy*c.at<double>(3);
        break;

    case POLY_1_X_Y_XY_XX_YY:
        estimate = c.at<double>(0) +
                x*c.at<double>(1) +
                y*c.at<double>(2) +
                xy*c.at<double>(3) +
                xx*c.at<double>(4) +
                yy*c.at<double>(5);
        break;


    case POLY_1_X_Y_XY_XX_YY_XXYY:
        estimate = c.at<double>(0) +
                x*c.at<double>(1) +
                y*c.at<double>(2) +
                xy*c.at<double>(3) +
                xx*c.at<double>(4) +
                yy*c.at<double>(5) +
                xxyy*c.at<double>(6);
        break;

    case POLY_1_X_Y_XY_XX_YY_XYY_YXX:
        estimate = c.at<double>(0) +
                x*c.at<double>(1) +
                y*c.at<double>(2) +
                xy*c.at<double>(3) +
                xx*c.at<double>(4) +
                yy*c.at<double>(5) +
                xyy*c.at<double>(6) +
                yxx*c.at<double>(7);
        break;

    case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY:
        estimate = c.at<double>(0) +
                x*c.at<double>(1) +
                y*c.at<double>(2) +
                xy*c.at<double>(3) +
                xx*c.at<double>(4) +
                yy*c.at<double>(5) +
                xyy*c.at<double>(6) +
                yxx*c.at<double>(7) +
                xxyy*c.at<double>(8);
        break;

    case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXX_YYY:
        estimate = c.at<double>(0) +
                x*c.at<double>(1) +
                y*c.at<double>(2) +
                xy*c.at<double>(3) +
                xx*c.at<double>(4) +
                yy*c.at<double>(5) +
                xyy*c.at<double>(6) +
                yxx*c.at<double>(7) +
                xxx*c.at<double>(8) +
                yyy*c.at<double>(9);
        break;

    default:
        break;
    }

    return estimate;
}

cv::Point2f MainWindow::calculateGazeFromCornealVector(cv::Point2f cornealVector, cv::Mat1d &cx, cv::Mat1d &cy)
{
    cv::Point2f gazePosition;

    // Check if calibration matrices are valid
    if (cx.empty() || cy.empty()) {
//        qDebug() << "[calculateGazeFromCornealVector] 标定矩阵为空";
        return cv::Point2f(-1, -1);
    }

    // Calculate gaze X coordinate using corneal vector and X calibration matrix
    gazePosition.x = evaluate(cornealVector.x, cornealVector.y, cx);

    // Calculate gaze Y coordinate using corneal vector and Y calibration matrix
    gazePosition.y = evaluate(cornealVector.x, cornealVector.y, cy);

    return gazePosition;
}

bool MainWindow::calibrate(Mat &x, Mat &y, Mat &z, Mat1d &c)
{
    // Possible terms; not using more than third degree atm
    Mat xx, yy, xy, xyy, yxx, xxyy, xxx, yyy, A;

    //    qDebug() <<"  x.rows"<<x.rows<<x.cols<<y.rows<<y.cols<<endl;
    multiply(x, x, xx);
    multiply(y, y, yy);
    multiply(x, y, xy);
    multiply(x, yy, xyy);
    multiply(y, xx, yxx);
    multiply(xx, yy, xxyy);
    multiply(x, xx, xxx);
    multiply(y, yy, yyy);
    //    unknowns = 6;
    A = Mat::ones(z.rows, unknowns, CV_64F);
    //    plType = POLY_1_X_Y_XY_XX_YY;
    switch (plType) {
    case POLY_1_X_Y_XY:
        // A.col(0) was initialized with one
        x.copyTo(A.col(1));
        y.copyTo(A.col(2));
        xy.copyTo(A.col(3));
        break;

    case POLY_1_X_Y_XY_XX_YY:
        // A.col(0) was initialized with one
        x.copyTo(A.col(1));
        y.copyTo(A.col(2));
        xy.copyTo(A.col(3));
        xx.copyTo(A.col(4));
        yy.copyTo(A.col(5));
        break;

    case POLY_1_X_Y_XY_XX_YY_XXYY:
        // A.col(0) was initialized with one
        x.copyTo(A.col(1));
        y.copyTo(A.col(2));
        xy.copyTo(A.col(3));
        xx.copyTo(A.col(4));
        yy.copyTo(A.col(5));
        xxyy.copyTo(A.col(6));
        break;

    case POLY_1_X_Y_XY_XX_YY_XYY_YXX:
        // A.col(0) was initialized with one
        x.copyTo(A.col(1));
        y.copyTo(A.col(2));
        xy.copyTo(A.col(3));
        xx.copyTo(A.col(4));
        yy.copyTo(A.col(5));
        xyy.copyTo(A.col(6));
        yxx.copyTo(A.col(7));
        break;

    case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY:
        // A.col(0) was initialized with one
        x.copyTo(A.col(1));
        y.copyTo(A.col(2));
        xy.copyTo(A.col(3));
        xx.copyTo(A.col(4));
        yy.copyTo(A.col(5));
        xyy.copyTo(A.col(6));
        yxx.copyTo(A.col(7));
        xxyy.copyTo(A.col(8));
        break;

    case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXX_YYY:
        // A.col(0) was initialized with one
        x.copyTo(A.col(1));
        y.copyTo(A.col(2));
        xy.copyTo(A.col(3));
        xx.copyTo(A.col(4));
        yy.copyTo(A.col(5));
        xyy.copyTo(A.col(6));
        yxx.copyTo(A.col(7));
        xxx.copyTo(A.col(8));
        yyy.copyTo(A.col(9));
        break;

    default:
        break;
    }

    z.convertTo(z, A.type());

    bool result = true;
    //    qDebug() <<"flag_calibration_sflag_calibration_statustatus"<<endl;
    result &= solve(A, z, c, DECOMP_SVD);
    //    qDebug() <<"flag_calibration_status  c.rows"<<c.rows<<endl;
    //    qDebug() <<"flag_calibration_status A.rows"<<A.rows<<A.cols<<z.rows<<" c.rows"<<c.rows<<endl;

    for (int i=0; i<c.rows; i++)
        if ( std::isnan(c(i)) ) {
            //            errorMsg = "found NaN coefficients.";
            result = false;
        }

    return result;
}

bool MainWindow::calibrateWithValidData(Mat &x, Mat &y, Mat &z, Mat1d &c)
{

    // 首先找出所有有效数据点
    std::vector<int> validIndices;
    for (int i = 0; i < x.rows; i++) {
        // 修复：改为 at<float>
        if (x.at<float>(i, 0) != -9999 && y.at<float>(i, 0) != -9999) {
            validIndices.push_back(i);
        }
    }

    int validCount = validIndices.size();
    if (validCount < max(6, unknowns)) {
//        qDebug() << "[calibrateWithValidData] 有效数据点不足:" << validCount << "需要至少" << unknowns << "个";
        return false;
    }

    // 创建只包含有效数据的新矩阵
    Mat validX(validCount, 1, CV_64F);
    Mat validY(validCount, 1, CV_64F);
    Mat validZ(validCount, 1, CV_64F);

    for (int i = 0; i < validCount; i++) {
        int idx = validIndices[i];
        // 修复：从float读取，转换为double存储
        validX.at<double>(i, 0) = static_cast<double>(x.at<float>(idx, 0));
        validY.at<double>(i, 0) = static_cast<double>(y.at<float>(idx, 0));
        validZ.at<double>(i, 0) = static_cast<double>(z.at<float>(idx, 0));
    }

//    qDebug() << "[calibrateWithValidData] 使用" << validCount << "个有效数据点进行标定";

    return calibrate(validX, validY, validZ, c);
}


// 修复后的 evaluateCalibrationError 函数
// 文件位置：mainwindow.cpp 第4268行开始
// 修复内容：将所有 at<double> 访问改为 at<float> 访问

float MainWindow::evaluateCalibrationError(cv::Mat &x, cv::Mat &y, cv::Mat &targetX, cv::Mat &targetY, cv::Mat1d &cx, cv::Mat1d &cy)
{
    float totalError = 0;
    int validCount = 0;

    for (int i = 0; i < x.rows && i < targetX.rows && i < targetY.rows; i++) {
        // 跳过无效数据 - 修复：使用 at<float> 而不是 at<double>
        if (x.at<float>(i, 0) == -9999 || y.at<float>(i, 0) == -9999) {
            continue;
        }

        // 使用标定矩阵预测结果 - 修复：使用 at<float> 而不是 at<double>
        float predX = evaluate(x.at<float>(i, 0), y.at<float>(i, 0), cx);
        float predY = evaluate(x.at<float>(i, 0), y.at<float>(i, 0), cy);

        // 计算与目标的误差 - 修复：使用 at<float> 而不是 at<double>
        float actualX = targetX.at<float>(i, 0);
        float actualY = targetY.at<float>(i, 0);

        float error = sqrt((predX - actualX) * (predX - actualX) + (predY - actualY) * (predY - actualY));
        totalError += error;
        validCount++;
    }

    if (validCount == 0) {
//        qDebug() << "[evaluateCalibrationError] 没有有效数据用于误差评估";
        return -1;
    }

    float avgError = totalError / validCount;
//    qDebug() << "[evaluateCalibrationError] 使用" << validCount << "个点，平均误差:" << avgError << "像素";

    return avgError;
}

//void MainWindow::buildDepthMap(vector<CollectionTuple> &tuples)
//{
//    depthMap = Mat(tuples[0].field.height, tuples[0].field.width, CV_32F, -1.0);
//    Rect rect(0,0, depthMap.cols, depthMap.rows);

//    Subdiv2D subdiv(Rect(0, 0, tuples[0].field.width, tuples[0].field.height));
//    for (unsigned int i=0; i<tuples.size(); i++) {
//        Point point = to2D(tuples[i].field.collectionMarker.center);
//        depthMap.at<float>(point) = tuples[i].field.collectionMarker.center.z;
//        subdiv.insert(point);
//    }

//    vector<Vec6f> triangleList;
//    subdiv.getTriangleList(triangleList);
//    vector<Point> pt(3);

//    for( unsigned int i = 0; i < triangleList.size(); i++ )
//    {
//        Vec6f t = triangleList[i];
//        vector<Point> vertices;
//        vertices.push_back(Point(cvRound(t[0]), cvRound(t[1])));
//        vertices.push_back(Point(cvRound(t[2]), cvRound(t[3])));
//        vertices.push_back(Point(cvRound(t[4]), cvRound(t[5])));
//        if ( rect.contains(vertices[0]) && rect.contains(vertices[1]) && rect.contains(vertices[2])) {
//            Rect aoi = boundingRect(vertices);
//            float mu = (depthMap.at<float>(vertices[0]) + depthMap.at<float>(vertices[1]) + depthMap.at<float>(vertices[2])) / 3;
//            for (int y=aoi.y; y<aoi.y+aoi.height; y++)
//                for (int x=aoi.x; x<aoi.x+aoi.width; x++)
//                    if ( pointPolygonTest(vertices, cv::Point2f(x,y), false) >= 0)
//                        depthMap.at<float>(y,x) = mu;
//        }
//    }
//    int validCount = 0;
//    double validAcc = 0;
//    for (int y=0; y<depthMap.rows; y++)
//        for (int x=0; x<depthMap.cols; x++)
//            if (depthMap.at<float>(y,x) >= 0) {
//                validCount++;
//                validAcc += depthMap.at<float>(y,x);
//            }
//    if (true){  // use depthMap interpolation
//        Mat mask;
//        threshold(depthMap, mask, -1, 255, cv::THRESH_BINARY_INV);
//        mask.convertTo(mask, CV_8U);
//        depthMap.setTo(validAcc/validCount, mask);
//    } else // use mean value
//        depthMap.setTo(validAcc/validCount);
//}

void MainWindow::on_comboBox_activated(int index)
{
    cv::destroyWindow("errorPlot");
    CALIBRATIONPOINTS9 = ui->comboBox->currentText().toUInt();


    this->on_setOrignalBtn_clicked();
    rightPupil = PointVector();
    leftPupil = PointVector();

    Markgaze = PointVector();    // 同理
    predefinedScreenPoints.clear();
    predefinedScreenPoints = generateCalibrationPoints(CALIBRATIONPOINTS9);

    if(CALIBRATIONPOINTS9!=1)
        on_pointcalibButton_2_clicked();


//    ShowPoint *calibrationWindow = new ShowPoint(CALIBRATIONPOINTS9, this);
//    showPoint->show();
//    showPoint->installEventFilter(this);



//    showPoint = new ShowPoint(CALIBRATIONPOINTS9, this);
//    showPoint->show();
//    showPoint->installEventFilter(this);

}

void MainWindow::on_comboBox_2_currentIndexChanged(int index)
{
    plType = static_cast<PlType>(index);
    switch(index)
    {
    case POLY_1_X_Y_XY:
        unknowns = 4;
        break;
    case POLY_1_X_Y_XY_XX_YY:
        unknowns = 6;
        break;
    case POLY_1_X_Y_XY_XX_YY_XXYY:
        unknowns = 7;
        break;
    case POLY_1_X_Y_XY_XX_YY_XYY_YXX:
        unknowns = 8;
        break;
    case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY:
        unknowns = 9;
        break;
    default:
        break;
    }
//    on_Markcalib_clicked();

}

void MainWindow::GetGradient1(cv::Mat src, Mat &Ixx, Mat &Ixy, Mat &Iyy){
    Mat sobelx = (Mat_<float>(3,3) << -1, 0, 1, -2, 0, 2, -1, 0, 1);
    Mat sobely = (Mat_<float>(3,3) << -1, -2, -1, 0, 0, 0, 1, 2, 1);

    Mat Ix, Iy;
    Point point(-1, -1);
    filter2D(src, Ix, -1, sobelx, point, 0, BORDER_CONSTANT);
    filter2D(src, Iy, -1, sobely, point, 0, BORDER_CONSTANT);

    Ixx = Ix.mul(Ix);
    Iyy = Iy.mul(Iy);
    Ixy = Ix.mul(Iy);
}

cv::Mat MainWindow::CalcMinEigenVal1(Mat Ixx, Mat Ixy, Mat Iyy) {
    Mat res = Mat::zeros(Ixx.size(),CV_32FC1);

    for(int i = 0; i < Ixx.rows; i++) {
        for(int j = 0; j < Ixx.cols; j++) {
            float a = Ixx.at<float>(i, j);
            float b = Ixy.at<float>(i, j);
            float c = Iyy.at<float>(i, j);
            res.at<float>(i,j) = (a+c) - sqrt((a-c)*(a-c) + b*b);
        }
    }
    return res;
}

cv::Mat MainWindow::CornersShow1(cv::Mat src, std::vector<cv::KeyPoint> corners){
    Mat dst = src.clone();
    for(int i=0; i<corners.size(); i++) {
        circle(dst, corners[i].pt, 3, cv::Scalar(255, 0, 0), -1);  // 画半径为1的圆(画点）
    }

    return dst;
}

vector<float*> MainWindow::CornersChoice1(cv::Mat eig, float thresh){
    vector<float*> tmpCorners;

    double max_val = 0;
    minMaxLoc(eig, 0, &max_val, 0, 0);
    threshold(eig, eig, max_val*thresh, 0, THRESH_TOZERO);

    Mat tmp;
    dilate(eig, tmp, cv::Mat());

    for(int y = 0; y < eig.rows; y++){
        float* eig_data = (float*)eig.ptr(y);
        float* tmp_data = (float*)tmp.ptr(y);

        for(int x = 0; x < eig.cols; x++){
            float val = eig_data[x];
            if(val != 0 && val == tmp_data[x]) {
                tmpCorners.push_back(eig_data + x);
            }
        }
    }

    sort(tmpCorners.begin(), tmpCorners.end(), greaterThanPtr());

    return tmpCorners;
}

void MainWindow::DistanceChoice1(float minDistance, int maxCorners, std::vector<cv::KeyPoint> &corners) {
    int ncorners = 0;
    Mat eig = eig_;
    vector<float*> tmpCorners = tmpCorners_;

    if (minDistance >= 1) {
        int w = eig.cols;
        int h = eig.rows;

        int cell_size = cvRound(minDistance);
        int grid_width = (w + cell_size - 1) / cell_size;
        int grid_height = (h + cell_size - 1) / cell_size;

        vector<vector<cv::Point2f>> grid(grid_width * grid_height);
        double minDistance_squre = minDistance * minDistance;

        for(int i = 0; i < eig.rows*eig.cols&& i < tmpCorners.size(); i++) {
            int ofs = (int)((const uchar*)tmpCorners[i] - eig.ptr());
            int y = (int)(ofs / eig.step);
            int x = (int)((ofs - y*eig.step)/sizeof(float));

            bool good = true;

            int x_cell = x / cell_size;
            int y_cell = y / cell_size;

            int x1 = x_cell - 1;
            int y1 = y_cell - 1;
            int x2 = x_cell + 1;
            int y2 = y_cell + 1;
            x1 = max(0, x1);
            y1 = max(0, y1);
            x2 = min(grid_width - 1, x2);
            y2 = min(grid_height - 1, y2);

            for(int yy = y1; yy <= y2; yy++) {
                for(int xx = x1; xx <= x2; xx++){
                    vector<cv::Point2f> &m = grid[yy * grid_width + xx];
                    if(m.size()) {
                        for(int j = 0; j < m.size(); j++) {
                            float dx = x - m[j].x;
                            float dy = y - m[j].y;

                            if(dx*dx + dy*dy < minDistance_squre) {
                                good = false;
                                goto break_out;
                            }
                        }
                    }
                }
            }
break_out:

            if(good) {
                grid[y_cell*grid_width + x_cell].push_back(Point(x, y));
                cv::KeyPoint kp;
                kp.pt = Point(x, y);
                kp.response = *tmpCorners[i];
                corners.push_back(kp);
                ncorners += 1;
                if(maxCorners > 0 && (int)ncorners == maxCorners )
                    break;
            }
        }
    }
}

void MainWindow::KeyPoint1(cv::Mat eig, vector<float*> tmpCorners, int maxCorners, std::vector<cv::KeyPoint> &corners) {
    int ncorners = 0;
    for(int i = 0; i < eig.rows*eig.cols && i < tmpCorners.size(); i++) {
        int ofs = (int)((const uchar*)tmpCorners[i] - eig.ptr());
        int y = (int)(ofs / eig.step);
        int x = (int)((ofs - y * eig.step) / sizeof(float));

        cv::KeyPoint kp;
        kp.pt = cv::Point2f((float)x, (float)y);
        kp.response = *tmpCorners[i];
        corners.push_back(kp);

        ncorners += 1;
        if( maxCorners > 0 && (int)ncorners == maxCorners )
            break;
    }
}

std::vector<cv::KeyPoint> MainWindow::run1(cv::Mat src, int maxCorners) {
    src.convertTo(src, CV_32FC1, 1/255.0);

    //计算图像梯度提取
    Mat Ixx, Ixy, Iyy;
    GetGradient1(src, Ixx, Ixy, Iyy);

    int kern_size = 5;
    GaussianBlur(Ixx, Ixx, Size(kern_size, kern_size), 0, 0);
    GaussianBlur(Ixy, Ixy, Size(kern_size, kern_size), 0, 0);
    GaussianBlur(Iyy, Iyy, Size(kern_size, kern_size), 0, 0);
    eig_ = CalcMinEigenVal1(Ixx, Ixy, Iyy);
    float thresh = 0.01;
    tmpCorners_ = CornersChoice1(eig_, thresh);
    std::vector<cv::KeyPoint> corners;
    KeyPoint1(eig_, tmpCorners_, maxCorners, corners);

    return corners;
}

double MainWindow::computeR1(Point2i x1, Point2i x2) {
    return norm(x1 - x2);
}

template <typename T> vector<size_t>  MainWindow::sort_indexes1(const vector<T> &v) {
    vector< size_t>  idx(v.size());
    for (size_t i = 0; i != idx.size(); ++i) idx[i] = i;

    // sort indexes based on comparing values in v
    sort(idx.begin(), idx.end(),
         [&v](size_t i1, size_t i2) {return v[i1] >  v[i2]; });
    return idx;
}

std::vector<cv::KeyPoint> MainWindow::ANMS1(std::vector<cv::KeyPoint> kpts, int num) {
    int sz = kpts.size();
    double maxmum = 0;
    vector<double> roblocalmax(kpts.size());
    vector<double> raduis(kpts.size(), INFINITY);
    for (size_t i = 0; i < sz; i++) {
        auto rp = kpts[i].response;
        if (rp > maxmum)
            maxmum = rp;
        roblocalmax[i] = rp*0.9;
    }
    auto max_response = maxmum*0.9;
    for (size_t i = 0; i < sz; i++) {
        double rep = kpts[i].response;
        Point2i p = kpts[i].pt;
        auto& rd = raduis[i];
        if (rep>max_response) {
            rd = INFINITY;
        } else {
            for (size_t j = 0; j < sz; j++) {
                if (roblocalmax[j] > rep) {
                    auto d = computeR1(kpts[j].pt, p);
                    if (rd > d)
                        rd = d;
                }
            }
        }
    }
    auto sorted = sort_indexes1(raduis);
    std::vector<cv::KeyPoint> rpts;

    for (size_t i = 0; i < num; i++) {
        rpts.push_back(kpts[sorted[i]]);
    }
    return std::move(rpts);
}

struct KeypointResponseGreater {
    inline bool operator()(const cv::KeyPoint& kp1, const cv::KeyPoint& kp2) const {
        return kp1.response > kp2.response;
    }
};

std::vector<cv::KeyPoint> MainWindow::ssc1(std::vector<cv::KeyPoint> keyPoints, int numRetPoints, float tolerance, int cols, int rows) {
    sort(keyPoints.begin(), keyPoints.end(), KeypointResponseGreater());

    // several temp expression variables to simplify solution equation
    int exp1 = rows + cols + 2 * numRetPoints;
    long long exp2 =
            ((long long)4 * cols + (long long)4 * numRetPoints +
             (long long)4 * rows * numRetPoints + (long long)rows * rows +
             (long long)cols * cols - (long long)2 * rows * cols +
             (long long)4 * rows * cols * numRetPoints);
    double exp3 = sqrt(exp2);
    double exp4 = numRetPoints - 1;

    double sol1 = -round((exp1 + exp3) / exp4); // first solution
    double sol2 = -round((exp1 - exp3) / exp4); // second solution

    // binary search range initialization with positive solution
    int high = (sol1 > sol2) ? sol1 : sol2;
    int low = floor(sqrt((double)keyPoints.size() / numRetPoints));
    low = max(1, low);

    int width;
    int prevWidth = -1;

    vector<int> ResultVec;
    bool complete = false;
    unsigned int K = numRetPoints;
    unsigned int Kmin = round(K - (K * tolerance));
    unsigned int Kmax = round(K + (K * tolerance));

    vector<int> result;
    result.reserve(keyPoints.size());
    while (!complete) {
        width = low + (high - low) / 2;
        if (width == prevWidth || low > high) { // needed to reassure the same radius is not repeated again
            ResultVec = result; // return the keypoints from the previous iteration
            break;
        }
        result.clear();

        double c = (double)width / 2.0; // initializing Grid
        int numCellCols = floor(cols / c);
        int numCellRows = floor(rows / c);
        vector<vector<bool>> coveredVec(numCellRows + 1, vector<bool>(numCellCols + 1, false));

        for(unsigned int i = 0; i < keyPoints.size(); ++i) {
            int row = floor(keyPoints[i].pt.y / c); // get position of the cell current point is located at
            int col = floor(keyPoints[i].pt.x / c);
            if (coveredVec[row][col] == false) { // if the cell is not covered
                result.push_back(i);
                int rowMin = ((row - floor(width / c)) >= 0) ? (row - floor(width / c)) : 0; // get range which current radius is covering
                int rowMax = ((row + floor(width / c)) <= numCellRows) ? (row + floor(width / c)) : numCellRows;
                int colMin = ((col - floor(width / c)) >= 0) ? (col - floor(width / c)) : 0;
                int colMax = ((col + floor(width / c)) <= numCellCols) ? (col + floor(width / c)) : numCellCols;
                for (int rowToCov = rowMin; rowToCov <= rowMax; ++rowToCov) {
                    for (int colToCov = colMin; colToCov <= colMax; ++colToCov) {
                        if (!coveredVec[rowToCov][colToCov]) {
                            coveredVec[rowToCov][colToCov] = true; // cover cells within the square bounding box with width
                        }
                    }
                }
            }
        }

        if (result.size() >= Kmin && result.size() <= Kmax) { // solution found
            ResultVec = result;
            complete = true;
        } else if (result.size() < Kmin)
            high = width - 1; // update binary search range
        else
            low = width + 1;
        prevWidth = width;
    }
    // retrieve final keypoints
    vector<cv::KeyPoint> kp;
    for (unsigned int i = 0; i < ResultVec.size(); i++)
        kp.push_back(keyPoints[ResultVec[i]]);

    return kp;
}

void MainWindow::on_interact01_clicked()
{
    m_dialog->showFullScreen(); // 或者 dialog->show();
    m_dialog->show();
}

// 接收吸附坐标的槽函数
void MainWindow::receiveAttractedGaze(const cv::Point2f& attractedGaze, bool isAttracting)
{
    static cv::Point2f lastOriginalGaze = cv::Point2f(-1, -1);

    m_lastAttractedGaze = attractedGaze;
    m_attractionActive = isAttracting;
    m_useAttractedGaze = isAttracting;

    // 当吸附激活时，记录原始视线坐标
    if (isAttracting) {
        // 使用校正前的原始坐标（避免使用已校正的坐标）
        cv::Point2f currentOriginalGaze = m_rawGazeBeforeCorrection;

        // 如果原始坐标有效，添加校正样本
        if (currentOriginalGaze.x >= 0 && currentOriginalGaze.y >= 0) {
            addGazeCorrectionSample(currentOriginalGaze, attractedGaze);
            lastOriginalGaze = currentOriginalGaze;
        }

//        qDebug() << "✅ 吸附激活: 原始(" << currentOriginalGaze.x << "," << currentOriginalGaze.y
//                 << ") -> 校正(" << attractedGaze.x << ", " << attractedGaze.y << ")";
    } else {
//        qDebug() << "❌ 吸附停止: (" << attractedGaze.x << ", " << attractedGaze.y << ")";
    }

    // 更新UI显示当前吸附状态
    if (ui) {
        QString status = isAttracting ? "吸附中" : "自由";
        // 如果有状态标签，可以显示吸附状态
    }
}

// 误差校正触发槽函数
void MainWindow::onErrorCorrectionTriggered()
{
//    qDebug() << "🎯 EyeTrackingDialog触发整体误差校正";

    // 执行误差分析和校正
    analyzeCalibrationErrors();
    applySystematicErrorCorrection();

//    qDebug() << "✅ 整体误差校正完成";

    // 输出校正结果
    if (m_systematicErrorCorrectionEnabled) {
//        qDebug() << "📊 系统性误差偏移: (" << m_systematicErrorOffset.x << ", " << m_systematicErrorOffset.y << ")";
//        qDebug() << "📈 平均偏移: (" << m_averageOffset.x << ", " << m_averageOffset.y << ")";
    }
}

// === 新增：实验流程控制槽函数实现 ===
void MainWindow::showInteractionDialog() {
    if (m_dialog) {
        qDebug() << "显示交互界面";
        m_dialog->showFullScreen();
        m_dialog->show();
        m_dialog->raise();
        m_dialog->activateWindow();
    }
}

void MainWindow::hideInteractionDialog() {
    if (m_dialog) {
        qDebug() << "隐藏交互界面";
        m_dialog->hide();
    }
}

void MainWindow::getSelectionCount() {
    // 由于不修改EyeTrackingDialog，我们需要通过定时器或其他方式监控选择计数
    // 这里暂时返回，选择计数将通过定时器在DataTable中直接监控
    qDebug() << "getSelectionCount调用（通过定时器监控）";
}

// CalibError第一个点完成槽函数
void MainWindow::onFirstPointCompleted()
{
//    qDebug() << "🎯 CalibError第一个点完成，执行初步误差计算";

    // 如果有足够数据，计算初步误差
    if (m_gazePoints.size() > 0 && !m_gazePoints[0].isEmpty()) {
        // 计算第一个点的误差
        cv::Point2f firstCalibPoint = cv::Point2f(m_calibrationPoints[0].x, m_calibrationPoints[0].y);
        cv::Point2f firstGazePoint = cv::Point2f(m_gazePoints[0][0].x, m_gazePoints[0][0].y);

        cv::Point2f initialError = firstGazePoint - firstCalibPoint;

        // 应用初步补偿
        if (m_gazeOffsets.size() < MAX_CORRECTION_SAMPLES) {
            m_gazeOffsets.push_back(initialError);
            updateAverageOffset();
            m_gazeCorrectionEnabled = true;

//            qDebug() << "📊 初步误差补偿已应用: (" << initialError.x << ", " << initialError.y << ")";
        }
    }
}

// CalibError所有点完成槽函数
void MainWindow::onAllPointsCompleted()
{
//    qDebug() << "🎯 CalibError所有点完成，执行完整误差补偿和质量评估";

    // 3D模式特殊处理：只有真3D模式才计算区域校正
    if (flag_calibration_status == 2 && !(worker && !worker->isReal3DModeEnabled() && m_fake3D_multiPointCalibrated)) {
        complete3DMultiPointCalibration();
        // 自动调用标定函数（与精确标定一致）
        calibrate();
//        qDebug() << "✅ 3D多点标定完成，区域校正已生成";
    } else if (flag_calibration_status == 2) {
        // 假3D模式：只调用calibrate()进行数据验证，不做区域校正
        calibrate();
    }

    // 执行完整的误差分析和校正
    analyzeCalibrationErrors();
    applySystematicErrorCorrection();

//    qDebug() << "✅ 完整误差补偿完成，开始质量评估";

    // 评估标定质量
    CalibrationQuality quality = evaluateCalibrationQuality();

    // 输出详细的质量信息
//    qDebug() << QString("📊 标定质量评估结果：")
//                .append(QString("\n  质量等级：%1").arg(quality.qualityLevel))
//                .append(QString("\n  平均误差：%1 px").arg(quality.averageError, 0, 'f', 1))
//                .append(QString("\n  最大误差：%1 px").arg(quality.maxError, 0, 'f', 1))
//                .append(QString("\n  最小误差：%1 px").arg(quality.minError, 0, 'f', 1))
//                .append(QString("\n  有效点数：%1/%2").arg(quality.validPoints).arg(m_calibrationPoints.size()))
//                .append(QString("\n  是否可接受：%1").arg(quality.isAcceptable ? "是" : "否"));

    int currentCycle = m_calibError->getCycleCount();

    // 检查是否是第0轮完成
    if (currentCycle == 0) {
        // 第0轮完成，保存最终误差偏置
        m_finalErrorOffset = m_cumulativeErrorOffset;
        m_firstCalibErrorCycleCompleted = true;

//        qDebug() << QString("🎉 第0轮CalibError完成，保存最终误差偏置：(%1,%2)")
//                    .arg(m_finalErrorOffset.x, 0, 'f', 2)
//                    .arg(m_finalErrorOffset.y, 0, 'f', 2);

        // 从第1轮开始使用固定偏置
        m_useFixedErrorOffset = true;

        // 显示结果给用户
        showCalibrationResult(quality);
    } else {
        // 第1轮及以后，不再进行动态误差校正，只显示结果
//        qDebug() << QString("🔄 第%1轮完成，使用第0轮计算的固定误差偏置：(%2,%3)")
//                    .arg(currentCycle)
//                    .arg(m_finalErrorOffset.x, 0, 'f', 2)
//                    .arg(m_finalErrorOffset.y, 0, 'f', 2);

        // 第1轮及以后显示质量报告（假3D模式除外）
        if (flag_calibration_status == 2 && !(worker && !worker->isReal3DModeEnabled() && m_fake3D_multiPointCalibrated)) {
            // 真3D模式显示专门的结果
            show3DCalibrationResult(quality);
        } else if (flag_calibration_status != 2) {
            // 2D模式显示简单信息
            QMessageBox::information(this, QString("第%1轮完成").arg(currentCycle),
                                    QString("第%1轮完成，使用第0轮计算的误差偏置进行补偿。").arg(currentCycle));
        }
        // 假3D模式：不显示标定质量评估对话框
    }
}

// CalibError误差计算请求槽函数
void MainWindow::onErrorCalculationRequest()
{
//    qDebug() << "🔄 CalibError请求误差计算";

    // 根据当前状态决定计算类型
    if (m_gazePoints.size() == 1) {
        // 第一个点完成后的计算
        onFirstPointCompleted();
    } else if (m_gazePoints.size() >= m_calibrationPoints.size()) {
        // 所有点完成后的计算
        onAllPointsCompleted();
    }
}

// 单个点完成槽函数 - 基于累积误差进行实时校正
void MainWindow::onPointCompleted(int pointIndex, const QPointF& targetPoint, const QPointF& gazePoint)
{
//    qDebug() << QString("📊 处理点 %1 的误差：目标(%2,%3) -> 注视(%4,%5)")
//                .arg(pointIndex+1)
//                .arg(targetPoint.x(), 0, 'f', 1)
//                .arg(targetPoint.y(), 0, 'f', 1)
//                .arg(gazePoint.x(), 0, 'f', 1)
//                .arg(gazePoint.y(), 0, 'f', 1);

    // 只在第一轮进行动态误差校正
    if (!m_firstCalibErrorCycleCompleted) {
        // 第一轮：使用原始gaze点来计算真实误差
        if (pointIndex < m_originalGazePoints.size() && !m_originalGazePoints[pointIndex].isEmpty()) {
            // 计算原始gaze点的平均值（过滤离散点）
            cv::Point2f avgOriginalGaze = calculateFilteredAverage(m_originalGazePoints[pointIndex]);

            // 计算真实误差向量 (原始注视点 - 目标点)
            cv::Point2f targetCV(targetPoint.x(), targetPoint.y());
            cv::Point2f errorVector = avgOriginalGaze - targetCV;

            // 添加到累积误差列表
            m_accumulatedErrors.push_back(errorVector);
            m_accumulatedTargets.push_back(targetCV);

//            qDebug() << QString("📈 第一轮点 %1 原始误差向量：(%2,%3) [基于 %4 个原始样本]")
//                        .arg(pointIndex+1)
//                        .arg(errorVector.x, 0, 'f', 2)
//                        .arg(errorVector.y, 0, 'f', 2)
//                        .arg(m_originalGazePoints[pointIndex].size());

            // 重新计算基于所有已完成点的累积误差偏移
            updateCumulativeErrorOffset();

            // 启用累积误差校正
            if (!m_cumulativeErrorCorrectionEnabled && m_accumulatedErrors.size() >= 1) {
                m_cumulativeErrorCorrectionEnabled = true;
//                qDebug() << "✨ 启用累积误差校正系统";
            }

            // 更新现有的视线校正系统
            if (m_gazeOffsets.size() < MAX_CORRECTION_SAMPLES) {
                m_gazeOffsets.push_back(errorVector);
                updateAverageOffset();
                m_gazeCorrectionEnabled = true;
            }

//            qDebug() << QString("🎯 第一轮累积误差偏移更新：(%1,%2) 基于 %3 个点")
//                        .arg(m_cumulativeErrorOffset.x, 0, 'f', 2)
//                        .arg(m_cumulativeErrorOffset.y, 0, 'f', 2)
//                        .arg(m_accumulatedErrors.size());

            // 特殊处理：如果是第一个点，立即应用校正让其显示在中心位置
            if (m_accumulatedErrors.size() == 1) {
//                qDebug() << "🎯 第一个点完成，立即应用校正将其移动到中心位置";

                // 计算校正后的注视点位置（应该在目标点附近）
                cv::Point2f originalGaze = avgOriginalGaze;
                cv::Point2f correctedGaze = originalGaze - errorVector;  // 反向校正

//                qDebug() << QString("📍 第一点校正：原始(%1,%2) -> 校正后(%3,%4) [误差偏移:(%5,%6)]")
//                            .arg(originalGaze.x, 0, 'f', 2).arg(originalGaze.y, 0, 'f', 2)
//                            .arg(correctedGaze.x, 0, 'f', 2).arg(correctedGaze.y, 0, 'f', 2)
//                            .arg(errorVector.x, 0, 'f', 2).arg(errorVector.y, 0, 'f', 2);

                // 触发界面更新，让第一个点显示在校正后的位置
                // 这里可能需要调用相关的界面更新函数
                emit gazePointUpdated(QPointF(correctedGaze.x, correctedGaze.y));
            }
        } else {
//            qDebug() << QString("⚠️  第一轮点 %1 没有原始gaze数据，跳过累积误差计算").arg(pointIndex+1);
        }
    } else {
        // 第二轮及以后：不再进行动态误差校正，只记录信息
//        qDebug() << QString("🔄 第%1轮点%2完成，使用固定误差偏置(%3,%4)，不再动态调整")
//                    .arg(m_calibError->getCycleCount())
//                    .arg(pointIndex+1)
//                    .arg(m_finalErrorOffset.x, 0, 'f', 2)
//                    .arg(m_finalErrorOffset.y, 0, 'f', 2);
    }
}

// 更新累积误差偏移 - 基于所有已完成点的误差（过滤离散点）
void MainWindow::updateCumulativeErrorOffset()
{
    if (m_accumulatedErrors.empty()) {
        m_cumulativeErrorOffset = cv::Point2f(0, 0);
        return;
    }

    // 如果只有一个点，直接使用
    if (m_accumulatedErrors.size() == 1) {
        m_cumulativeErrorOffset = m_accumulatedErrors[0];
        return;
    }

    // 过滤离散点：去除偏差过大的点
    std::vector<cv::Point2f> filteredErrors;

    // 第一步：计算初步平均值
    cv::Point2f totalError(0, 0);
    for (const cv::Point2f& error : m_accumulatedErrors) {
        totalError += error;
    }
    cv::Point2f preliminaryMean = totalError / static_cast<float>(m_accumulatedErrors.size());

    // 第二步：计算标准偏差
    float sumSquaredDiff = 0.0f;
    for (const cv::Point2f& error : m_accumulatedErrors) {
        float diffX = error.x - preliminaryMean.x;
        float diffY = error.y - preliminaryMean.y;
        sumSquaredDiff += (diffX * diffX + diffY * diffY);
    }
    float stdDev = std::sqrt(sumSquaredDiff / static_cast<float>(m_accumulatedErrors.size()));

    // 第三步：过滤离散点（超过2倍标准偏差的点）
    const float OUTLIER_THRESHOLD = 2.0f;  // 可调整的阈值
    int outlierCount = 0;

    for (const cv::Point2f& error : m_accumulatedErrors) {
        float distance = cv::norm(error - preliminaryMean);
        if (distance <= OUTLIER_THRESHOLD * stdDev) {
            filteredErrors.push_back(error);
        } else {
            outlierCount++;
//            qDebug() << QString("🚫 过滤离散点：误差(%1,%2)，距离%.2f > 阈值%.2f")
//                        .arg(error.x, 0, 'f', 2)
//                        .arg(error.y, 0, 'f', 2)
//                        .arg(distance)
//                        .arg(OUTLIER_THRESHOLD * stdDev);
        }
    }

    // 第四步：如果过滤后点数太少，使用原始数据
    if (filteredErrors.size() < m_accumulatedErrors.size() * 0.5f || filteredErrors.empty()) {
//        qDebug() << "⚠️ 过滤后点数太少，使用原始数据";
        filteredErrors = m_accumulatedErrors;
        outlierCount = 0;
    }

    // 第五步：计算过滤后的平均值
    cv::Point2f filteredTotalError(0, 0);
    for (const cv::Point2f& error : filteredErrors) {
        filteredTotalError += error;
    }

    m_cumulativeErrorOffset = filteredTotalError / static_cast<float>(filteredErrors.size());

    if (outlierCount > 0) {
//        qDebug() << QString("🔍 离散点过滤：原始%1个点，过滤%2个离散点，使用%3个点计算平均误差")
//                    .arg(m_accumulatedErrors.size())
//                    .arg(outlierCount)
//                    .arg(filteredErrors.size());
    }

//    qDebug() << QString("📊 累积误差统计：总计 %1 个点，平均偏移(%2,%3)")
//                .arg(m_accumulatedErrors.size())
//                .arg(m_cumulativeErrorOffset.x, 0, 'f', 3)
//                .arg(m_cumulativeErrorOffset.y, 0, 'f', 3);

    // 分析误差趋势
    if (m_accumulatedErrors.size() >= 3) {
        // 计算误差的标准偏差，评估校正的稳定性
        cv::Point2f variance(0, 0);
        for (const cv::Point2f& error : m_accumulatedErrors) {
            cv::Point2f diff = error - m_cumulativeErrorOffset;
            variance.x += diff.x * diff.x;
            variance.y += diff.y * diff.y;
        }
        variance /= static_cast<float>(m_accumulatedErrors.size());

        float stdDevX = sqrt(variance.x);
        float stdDevY = sqrt(variance.y);

//        qDebug() << QString("📈 误差稳定性：X标准差=%.2f, Y标准差=%.2f")
//                    .arg(stdDevX).arg(stdDevY);
    }
}

// 重置累积误差校正
void MainWindow::resetCumulativeErrorCorrection()
{
    m_accumulatedErrors.clear();
    m_accumulatedTargets.clear();
    m_cumulativeErrorOffset = cv::Point2f(0, 0);
    m_cumulativeErrorCorrectionEnabled = false;

//    qDebug() << "🔄 累积误差校正已重置";
}

// CalibError轮次重置槽函数
void MainWindow::onCycleReset()
{
//    qDebug() << "🔄 R键重置：清空累积误差数据，重新开始动态误差校正";

    // 重置累积误差校正数据
    resetCumulativeErrorCorrection();

    // 重置轮次相关状态
    m_finalErrorOffset = cv::Point2f(0, 0);
    m_useFixedErrorOffset = false;
    m_firstCalibErrorCycleCompleted = false;

    // 重置误差校正状态
    resetGazeCorrection();

    // 强制重置所有静态变量
    forceResetAllStaticVariables();

//    qDebug() << "✅ 已重置所有误差校正数据，重新开始第0轮动态误差校正";

    QMessageBox::information(this, "重置完成", "已重置轮数和误差偏差，重新开始第0轮动态误差校正。");
}

// 强制重置所有静态变量
void MainWindow::forceResetAllStaticVariables()
{
//    qDebug() << "🔄 强制重置所有静态变量";

    // 由于已简化applyGazeCorrection函数，不再需要复杂的静态变量管理
    // 静态变量会在下次调用时自动重置

//    qDebug() << "✅ 已强制重置所有静态变量";
}

// 手动重置累积误差校正（用户主动调用）
void MainWindow::manualResetCumulativeErrorCorrection()
{
//    qDebug() << "🔄 用户手动重置累积误差校正";
    resetCumulativeErrorCorrection();
//    qDebug() << "✅ 手动重置完成，可以开始新的标定会话";
}

// ================== 标定质量评估功能 ==================

// 评估标定质量
MainWindow::CalibrationQuality MainWindow::evaluateCalibrationQuality()
{
    CalibrationQuality quality;
    quality.averageError = 0.0f;
    quality.maxError = 0.0f;
    quality.minError = 9999.0f;
    quality.validPoints = 0;
    quality.isAcceptable = false;
    quality.qualityLevel = "未知";

    if (m_calibrationPoints.isEmpty() || m_gazePoints.isEmpty()) {
//        qDebug() << "❌ 无标定数据，无法评估质量";
        return quality;
    }

    float totalError = 0.0f;
    std::vector<float> errors;

    // 计算每个标定点的平均误差
    for (int i = 0; i < m_calibrationPoints.size() && i < m_gazePoints.size(); ++i) {
        if (!m_gazePoints[i].isEmpty()) {
            // 计算平均注视点位置
            cv::Point2f avgGaze(0, 0);
            for (const cv::Point2f& gp : m_gazePoints[i]) {
                avgGaze.x += gp.x;
                avgGaze.y += gp.y;
            }
            avgGaze.x /= m_gazePoints[i].size();
            avgGaze.y /= m_gazePoints[i].size();

            // 计算误差距离
            cv::Point2f target(m_calibrationPoints[i].x, m_calibrationPoints[i].y);
            float error = cv::norm(avgGaze - target);

            errors.push_back(error);
            totalError += error;
            quality.validPoints++;

            if (error > quality.maxError) {
                quality.maxError = error;
            }
            if (error < quality.minError) {
                quality.minError = error;
            }
        }
    }

    // 计算平均误差
    if (quality.validPoints > 0) {
        quality.averageError = totalError / quality.validPoints;
    }

    // 判断质量等级
    if (quality.averageError <= EXCELLENT_THRESHOLD && quality.maxError <= EXCELLENT_THRESHOLD * 1.5f) {
        quality.qualityLevel = "优秀";
        quality.isAcceptable = true;
    } else if (quality.averageError <= GOOD_THRESHOLD && quality.maxError <= GOOD_THRESHOLD * 1.5f) {
        quality.qualityLevel = "良好";
        quality.isAcceptable = true;
    } else if (quality.averageError <= ACCEPTABLE_THRESHOLD && quality.maxError <= MAX_ACCEPTABLE_ERROR) {
        quality.qualityLevel = "可接受";
        quality.isAcceptable = true;
    } else {
        quality.qualityLevel = "不合格";
        quality.isAcceptable = false;
    }

    // 检查有效点数
    if (quality.validPoints < MIN_VALID_POINTS) {
        quality.qualityLevel = "不合格（有效点不足）";
        quality.isAcceptable = false;
    }

    return quality;
}

// 判断标定是否可接受
bool MainWindow::isCalibrationAcceptable(const CalibrationQuality& quality)
{
    return quality.isAcceptable &&
           quality.validPoints >= MIN_VALID_POINTS &&
           quality.averageError <= ACCEPTABLE_THRESHOLD &&
           quality.maxError <= MAX_ACCEPTABLE_ERROR;
}

// 显示标定结果
void MainWindow::showCalibrationResult(const CalibrationQuality& quality)
{
    QString title = quality.isAcceptable ? "标定成功" : "标定失败";
    QString icon = quality.isAcceptable ? "🎉" : "⚠️";

    // 根据标定模式调整标题
    if (flag_calibration_status == 2) {
        title = QString("3D注视标定") + (quality.isAcceptable ? "成功" : "失败");
    }

    QString message = QString("%1 标定质量评估\n\n"
                             "🎯 质量等级：%2\n"
                             "📊 平均误差：%3 像素\n"
                             "🔺 最大误差：%4 像素\n"
                             "🔻 最小误差：%5 像素\n"
                             "🎯 有效点数：%6/%7\n")
                     .arg(icon)
                     .arg(quality.qualityLevel)
                     .arg(quality.averageError, 0, 'f', 1)
                     .arg(quality.maxError, 0, 'f', 1)
                     .arg(quality.minError, 0, 'f', 1)
                     .arg(quality.validPoints)
                     .arg(m_calibrationPoints.size());

    // 3D模式添加额外信息
    if (flag_calibration_status == 2 && m_3DRegionalCorrectionEnabled) {
        message += QString("🌐 区域校正：已生成%1个校正点\n").arg(m_3DCalibrationOffsets.size());
    }

    message += "\n";

    if (quality.isAcceptable) {
        message += "✅ 标定质量合格，可以开始使用系统。";
    } else {
        message += "❌ 标定质量不合格，建议重新标定。\n\n";
        message += "💡 改进建议：\n";
        message += "  • 确保头部保持稳定\n";
        message += "  • 确保眼部对准标定点\n";
        message += "  • 检查环境光线是否适合\n";
        message += "  • 检查摄像头位置和焦距";
    }

    // 使用QMessageBox显示结果
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(quality.isAcceptable ? QMessageBox::Information : QMessageBox::Warning);

    if (quality.isAcceptable) {
        msgBox.setStandardButtons(QMessageBox::Ok);
    } else {
        msgBox.setStandardButtons(QMessageBox::Retry | QMessageBox::Ignore);
        msgBox.setDefaultButton(QMessageBox::Retry);
    }

    int result = msgBox.exec();

    // 处理用户选择
    if (!quality.isAcceptable && result == QMessageBox::Retry) {
//        qDebug() << "🔄 用户选择重新标定";
        // 重置相关数据，准备重新标定
        manualResetCumulativeErrorCorrection();

        // 重新打开标定窗口
        if (m_calibError) {
            m_calibError->show();
            m_calibError->raise();
            m_calibError->activateWindow();
        }
    } else if (quality.isAcceptable || result == QMessageBox::Ignore) {
//        qDebug() << "✅ 标定结果被接受，继续使用";
    }
}

// 添加视线校正样本 - 基于偏差计算
void MainWindow::addGazeCorrectionSample(const cv::Point2f& originalGaze, const cv::Point2f& correctedGaze)
{
    // 计算偏差：校正坐标 - 原始坐标
    cv::Point2f offset = correctedGaze - originalGaze;

    m_gazeOffsets.push_back(offset);
    m_correctionSampleCount++;

    // 限制样本数量，保持最新的样本
    if (m_gazeOffsets.size() > MAX_CORRECTION_SAMPLES) {
        m_gazeOffsets.erase(m_gazeOffsets.begin());
    }

//    qDebug() << "📍 添加偏差样本" << m_correctionSampleCount
//             << ": 原始(" << originalGaze.x << "," << originalGaze.y << ")"
//             << " -> 校正(" << correctedGaze.x << "," << correctedGaze.y << ")"
//             << " 偏差(" << offset.x << "," << offset.y << ")";

    // 更新平均偏差
    updateAverageOffset();

    // 当有足够样本时，启用校正
    if (m_correctionSampleCount >= 3) {
        m_gazeCorrectionEnabled = true;
    }
}

// 更新平均偏差
void MainWindow::updateAverageOffset()
{
    if (m_gazeOffsets.empty()) {
        m_averageOffset = cv::Point2f(0, 0);
        return;
    }

    cv::Point2f sum(0, 0);
    for (const auto& offset : m_gazeOffsets) {
        sum += offset;
    }

    m_averageOffset = sum * (1.0f / m_gazeOffsets.size());

//    qDebug() << "📊 平均偏差更新为: (" << m_averageOffset.x << "," << m_averageOffset.y
//             << ") 基于" << m_gazeOffsets.size() << "个样本";
}

// 应用视线校正 - 支持区域性校正的增强版本
cv::Point2f MainWindow::applyGazeCorrection(const cv::Point2f& rawGaze)
{
    cv::Point2f correctionOffset(0, 0);

    // 只使用累积误差校正
    if (m_cumulativeErrorCorrectionEnabled && (!m_accumulatedErrors.empty() || m_useFixedErrorOffset)) {
        if (m_useFixedErrorOffset) {
            // 使用固定的最终误差偏置
            correctionOffset = -m_finalErrorOffset;  // 负号是因为我们要反向校正

            // 输出调试信息（限制频率）
            static int fixedDebugCounter = 0;
            if (fixedDebugCounter % 30 == 0) {  // 每30帧输出一次
//                qDebug() << QString("🔒 应用固定误差偏置：(%1,%2)")
//                            .arg(correctionOffset.x, 0, 'f', 2)
//                            .arg(correctionOffset.y, 0, 'f', 2);
            }
            fixedDebugCounter++;
        } else {
            // 使用动态计算的累积误差偏置
            correctionOffset = -m_cumulativeErrorOffset;  // 负号是因为我们要反向校正

            // 输出调试信息（限制频率）
            static int debugCounter = 0;
            if (debugCounter % 30 == 0) {  // 每30帧输出一次
//                qDebug() << QString("🔧 应用动态累积误差校正：(%1,%2) [基于%3个点]")
//                            .arg(correctionOffset.x, 0, 'f', 2)
//                            .arg(correctionOffset.y, 0, 'f', 2)
//                            .arg(m_accumulatedErrors.size());
            }
            debugCounter++;
        }
    }

    // 应用校正
    cv::Point2f correctedGaze = rawGaze + correctionOffset;

    // 边界检查
    correctedGaze.x = max(0.0f, min(static_cast<float>(m_screenWidth), correctedGaze.x));
    correctedGaze.y = max(0.0f, min(static_cast<float>(m_screenHeight), correctedGaze.y));

    return correctedGaze;
}

// 重置视线校正
void MainWindow::resetGazeCorrection()
{
    m_gazeOffsets.clear();
    m_averageOffset = cv::Point2f(0, 0);
    m_gazeCorrectionEnabled = false;
    m_correctionSampleCount = 0;

    // 同时重置累积误差校正
    resetCumulativeErrorCorrection();

//    qDebug() << "🔄 视线校正和累积误差校正已重置";
}

// 计算过滤离散点后的平均值
cv::Point2f MainWindow::calculateFilteredAverage(const QVector<cv::Point2f>& points)
{
    if (points.isEmpty()) {
        return cv::Point2f(0, 0);
    }

    if (points.size() == 1) {
        return points[0];
    }

    // 第一步：计算初步平均值
    cv::Point2f totalPoint(0, 0);
    for (const cv::Point2f& pt : points) {
        totalPoint += pt;
    }
    cv::Point2f preliminaryMean = totalPoint / static_cast<float>(points.size());

    // 第二步：计算标准偏差
    float sumSquaredDiff = 0.0f;
    for (const cv::Point2f& pt : points) {
        float diffX = pt.x - preliminaryMean.x;
        float diffY = pt.y - preliminaryMean.y;
        sumSquaredDiff += (diffX * diffX + diffY * diffY);
    }
    float stdDev = std::sqrt(sumSquaredDiff / static_cast<float>(points.size()));

    // 第三步：过滤离散点（超过1.5倍标准偏差的点）
    const float OUTLIER_THRESHOLD = 1.5f;  // 对单点数据使用更严格的阈值
    std::vector<cv::Point2f> filteredPoints;
    int outlierCount = 0;

    for (const cv::Point2f& pt : points) {
        float distance = cv::norm(pt - preliminaryMean);
        if (distance <= OUTLIER_THRESHOLD * stdDev) {
            filteredPoints.push_back(pt);
        } else {
            outlierCount++;
        }
    }

    // 第四步：如果过滤后点数太少，使用原始数据
    if (filteredPoints.size() < points.size() * 0.6f || filteredPoints.empty()) {
        filteredPoints.clear();
        for (const cv::Point2f& pt : points) {
            filteredPoints.push_back(pt);
        }
        outlierCount = 0;
    }

    // 第五步：计算过滤后的平均值
    cv::Point2f filteredTotal(0, 0);
    for (const cv::Point2f& pt : filteredPoints) {
        filteredTotal += pt;
    }

    cv::Point2f result = filteredTotal / static_cast<float>(filteredPoints.size());

    if (outlierCount > 0) {
//        qDebug() << QString("🔍 单点数据过滤：原始%1个样本，过滤%2个离散点，使用%3个样本")
//                    .arg(points.size())
//                    .arg(outlierCount)
//                    .arg(filteredPoints.size());
    }

    return result;
}

// ================== 系统性误差分析和修复功能 ==================

// 计算全局系统性误差
cv::Point2f MainWindow::calculateSystematicError()
{
    if (m_calibrationPoints.isEmpty() || m_gazePoints.isEmpty()) {
//        qDebug() << "❌ 无标定数据，无法计算系统性误差";
        return cv::Point2f(0, 0);
    }

    cv::Point2f totalError(0, 0);
    int totalSamples = 0;

    // 计算所有标定点的平均误差向量
    for (int i = 0; i < m_calibrationPoints.size() && i < m_gazePoints.size(); ++i) {
        cv::Point2f targetPoint = m_calibrationPoints[i];

        // 计算该点所有样本的平均gaze
        cv::Point2f avgGaze(0, 0);
        for (const auto& gazePoint : m_gazePoints[i]) {
            avgGaze += gazePoint;
        }

        if (!m_gazePoints[i].isEmpty()) {
            avgGaze *= (1.0f / m_gazePoints[i].size());

            // 计算误差向量 (目标点 - 实际gaze点)
            cv::Point2f errorVector = targetPoint - avgGaze;
            totalError += errorVector;
            totalSamples++;
        }
    }

    cv::Point2f systematicError = totalSamples > 0 ? totalError * (1.0f / totalSamples) : cv::Point2f(0, 0);

//    qDebug() << "📊 计算得到系统性误差: (" << systematicError.x << ", " << systematicError.y << ")";
//    qDebug() << "📊 基于" << totalSamples << "个标定点，总样本数:" << totalSamples;

    return systematicError;
}

// 获取区域性误差偏移（9宫格）
cv::Point2f MainWindow::getRegionalErrorOffset(const cv::Point2f& point)
{
    if (m_regionalErrorOffsets.size() != 9) {
        return m_systematicErrorOffset; // 回退到全局偏移
    }

    // 将屏幕分为9个区域（3x3网格）
    float regionWidth = m_screenWidth / 3.0f;
    float regionHeight = m_screenHeight / 3.0f;

    int regionX = min(2, static_cast<int>(point.x / regionWidth));
    int regionY = min(2, static_cast<int>(point.y / regionHeight));
    int regionIndex = regionY * 3 + regionX;

    return m_regionalErrorOffsets[regionIndex];
}

// 分析标定误差模式
void MainWindow::analyzeCalibrationErrors()
{
    if (m_calibrationPoints.isEmpty() || m_gazePoints.isEmpty()) {
//        qDebug() << "❌ 无标定数据，无法分析误差模式";
        return;
    }

//    qDebug() << "📈 开始分析标定误差模式...";

    // 1. 计算全局系统性误差
    m_systematicErrorOffset = calculateSystematicError();

    // 2. 计算区域性误差（9宫格）
    m_regionalErrorOffsets.clear();
    m_regionalErrorOffsets.resize(9, cv::Point2f(0, 0));

    std::vector<int> regionCounts(9, 0);

    for (int i = 0; i < m_calibrationPoints.size() && i < m_gazePoints.size(); ++i) {
        cv::Point2f targetPoint = m_calibrationPoints[i];

        // 确定该点所属区域
        float regionWidth = m_screenWidth / 3.0f;
        float regionHeight = m_screenHeight / 3.0f;
        int regionX = min(2, static_cast<int>(targetPoint.x / regionWidth));
        int regionY = min(2, static_cast<int>(targetPoint.y / regionHeight));
        int regionIndex = regionY * 3 + regionX;

        // 计算该点的平均gaze
        cv::Point2f avgGaze(0, 0);
        for (const auto& gazePoint : m_gazePoints[i]) {
            avgGaze += gazePoint;
        }

        if (!m_gazePoints[i].isEmpty()) {
            avgGaze *= (1.0f / m_gazePoints[i].size());

            // 计算该区域的误差向量
            cv::Point2f errorVector = targetPoint - avgGaze;
            m_regionalErrorOffsets[regionIndex] += errorVector;
            regionCounts[regionIndex]++;
        }
    }

    // 计算各区域平均误差
    for (int i = 0; i < 9; ++i) {
        if (regionCounts[i] > 0) {
            m_regionalErrorOffsets[i] *= (1.0f / regionCounts[i]);
        } else {
            m_regionalErrorOffsets[i] = m_systematicErrorOffset; // 使用全局偏移
        }
    }

    // 3. 输出分析结果
//    qDebug() << "📊 误差分析完成:";
//    qDebug() << "   全局系统性误差: (" << m_systematicErrorOffset.x << ", " << m_systematicErrorOffset.y << ")";

    for (int i = 0; i < 9; ++i) {
        // 调试输出已禁用，不需要额外变量
//        int row = i / 3, col = i % 3;
//        qDebug() << QString("   区域[%1,%2]: (%3, %4) 样本数:%5")
//                    .arg(row).arg(col)
//                    .arg(m_regionalErrorOffsets[i].x, 0, 'f', 2)
//                    .arg(m_regionalErrorOffsets[i].y, 0, 'f', 2)
//                    .arg(regionCounts[i]);
    }

    // 4. 计算误差改善估计（暂时禁用）
    // double avgErrorMagnitude = cv::norm(m_systematicErrorOffset);
//    qDebug() << QString("📈 预计误差改善: %.2f 像素").arg(avgErrorMagnitude);

    m_systematicErrorCorrectionEnabled = true;
//    qDebug() << "✅ 系统性误差校正已启用";
}

// 应用系统性误差校正
void MainWindow::applySystematicErrorCorrection()
{
    if (!m_systematicErrorCorrectionEnabled) {
        return;
    }

    // 分析当前标定数据的误差模式
    analyzeCalibrationErrors();

//    qDebug() << "🔧 应用系统性误差校正...";

    // 将误差校正应用到现有的视线校正系统
    if (!m_gazeCorrectionEnabled) {
        // 如果视线校正系统未启用，直接使用系统性误差作为校正
        m_averageOffset = m_systematicErrorOffset;
        m_gazeCorrectionEnabled = true;
//        qDebug() << "🎯 系统性误差校正已合并到视线校正系统";
    } else {
        // 如果视线校正系统已启用，合并系统性误差校正
        m_averageOffset = (m_averageOffset + m_systematicErrorOffset) * 0.5f;
//        qDebug() << "🔀 系统性误差校正已与现有校正合并";
    }

//    qDebug() << "📊 最终校正偏移: (" << m_averageOffset.x << ", " << m_averageOffset.y << ")";
}



// 头部姿态标定函数实现 - 暂时注释掉，缺少相关变量定义
/*
void MainWindow::calibrateHeadPose() {
    if (m_headEulerAngles[0] == 0 && m_headEulerAngles[1] == 0 && m_headEulerAngles[2] == 0) {
//        qDebug() << "头部姿态数据未准备好，无法标定";
        return;
    }

    /* // 将当前头部姿态设置为参考位置
    m_referenceHeadPose = HeadPose::fromVec3d(m_headEulerAngles);

    // 保存当前头部位置作为参考位置
    if (!m_headTranslationVector.empty()) {
        m_referenceHeadPosition.x = m_headTranslationVector.at<float>(0);
        m_referenceHeadPosition.y = m_headTranslationVector.at<float>(1);
        m_referenceHeadPosition.z = m_headTranslationVector.at<float>(2);
    } else {
        m_referenceHeadPosition = cv::Point3f(0, 0, 0.6f); // 默认60cm距离
    }

    m_headPoseCalibrated = true;

//    qDebug() << "头部姿态标定完成 - 参考姿态(弧度):"
             << "Pitch:" << m_referenceHeadPose.pitch_rad
             << "Yaw:" << m_referenceHeadPose.yaw_rad
             << "Roll:" << m_referenceHeadPose.roll_rad;
//    qDebug() << "参考位置(米):"
             << "X:" << m_referenceHeadPosition.x
             << "Y:" << m_referenceHeadPosition.y
             << "Z:" << m_referenceHeadPosition.z;
}
*/












// Find the two glint contours with fixed distance and similar Y
//                cv::Point glintCenter1(-1, -1), glintCenter2(-1, -1);
//                if(gvar==1) //冻结画面
//                {           //glint
//                        int sx = point.x;  // Assume the pupil is at the image center
//                        int sy=point.y;
//                        int pupil_diameter = data.pupil.size.width*3;  // Approximate pupil diameter in pixels (adjust based on your setup)
//                        int pupil_radius = pupil_diameter / 2;
//                        int iris_diameter = 3 * pupil_diameter;  // Estimate iris diameter (3x pupil diameter)
//                        int iris_radius = iris_diameter / 2;

//                        // Create a mask for the pupil region (inverted, to exclude the pupil)
//                        Mat pupil_mask = Mat::zeros(eyeImage.size(), CV_8U);
//                        cv::circle(pupil_mask, Point(sx, sy), pupil_radius, cv::Scalar(255), FILLED);

//                        Mat iris_mask = pupil_mask; // Invert pupil mask to get iris mask
//                        //imshow("iris_mask gray", iris_mask);
//                        // Thresholding and masking
//                        Mat gray;
//                        if (eyeImage.channels() > 1) {
//                            cvtColor(eyeImage, gray, COLOR_BGR2GRAY);
//                        } else {
//                            gray = eyeImage.clone();
//                        }


//                        Mat masked_thresh;
//                        gray.copyTo(masked_thresh, iris_mask); // Apply iris mask to the thresholded image
//                        imshow("masked_thresh Threshold", masked_thresh);

//                        Mat thresh;
//                        double threshValue = 180; // Fixed threshold value (adjust if needed)
//                        threshold(masked_thresh, thresh, threshValue, 255, cv::THRESH_BINARY);

//                        // 创建结构元素，这里使用矩形结构元素
//                           cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));

//                           // 膨胀操作
//                           cv::Mat dilated;
//                           cv::dilate(thresh, dilated, element);

//                           // 腐蚀操作
//                           cv::Mat eroded;
//                           cv::erode(dilated, thresh, element);

//                        imshow("Masked gray", thresh);


//                        // Find contours in the thresholded and masked image
//                        std::vector<std::vector<cv::Point>> contours;
//                        cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);




//                        double yTolerance = 3.0; // Max Y difference (pixels)

//                        for (size_t i = 0; i < contours.size(); ++i) {
//                            double area1 = cv::contourArea(contours[i]);
//                            if (area1 < 70 || area1 > 400) continue;

//                            double perimeter1 = cv::arcLength(contours[i], true);
//                            double circularity1 = (perimeter1 > 0) ? (4 * CV_PI * area1) / (perimeter1 * perimeter1) : 0.0;
//                            if (circularity1 < 0.5) continue;

//                            qDebug()<<"area1"<<area1<<"cir"<<circularity1<<endl;
//                            cv::Moments m1 = cv::moments(contours[i]);

//                            cv::Point center1(m1.m10 / m1.m00, m1.m01 / m1.m00);
//                            glintCenter1 = center1;
//                            qDebug()<<"center1"<<center1<<endl;
//                            cv::circle(eyeImage, glintCenter1, 3, cv::Scalar(255, 0, 0), -1);  // single glint
////                            cv::Mat mask1 = cv::Mat::zeros(gray.size(), CV_8U);
////                            cv::drawContours(mask1, contours, i, cv::Scalar(255), cv::FILLED);

//                               //double glint
////                            for (size_t j = i + 1; j < contours.size(); ++j) {
////                                double area2 = cv::contourArea(contours[j]);
////                                if (area2 < 10 || area2 > 150) continue;
////                                  qDebug()<<"area2"<<area2<<endl;
////                                // Compute circularity
////                                double perimeter1 = cv::arcLength(contours[i], true);
////                                double circularity1 = (perimeter1 > 0) ? (4 * CV_PI * area1) / (perimeter1 * perimeter1) : 0.0;
////                                if (circularity1 < 0.6) continue;

////                                cv::Moments m2 = cv::moments(contours[j]);
////                                if (m2.m00 == 0) continue;
////                                cv::Point center2(m2.m10 / m2.m00, m2.m01 / m2.m00);

////                                // Check Y-coordinate similarity
////                                qDebug()<<"distanceY"<<std::abs(center1.y - center2.y)<<endl;
////                                if (std::abs(center1.y - center2.y) > yTolerance) continue;

////                                // Check distance constraint
////                                double distance = std::abs(center1.x - center2.x);
////                                qDebug()<<"distance"<<distance<<endl;

////                                double minDist = 5;
////                                double maxDist = 30;
////                                if (distance < minDist || distance > maxDist) continue;


////                                glintCenter1 = center1;
////                                glintCenter2 = center2;

////                            }
//                        }

//                        // Draw glint centers on the original image
////                        if (glintCenter1.x != -1 && glintCenter1.y != -1 && glintCenter2.x != -1 && glintCenter2.y != -1) {
////                            cv::Mat drawImage;
////                            if (eyeImage.channels() == 1) {
////                                cv::cvtColor(eyeImage, drawImage, cv::COLOR_GRAY2BGR);
////                            } else {
////                                drawImage = eyeImage;
////                            }

////                            if(glintCenter1.x<glintCenter2.x)
////                            {
////                                cv::circle(eyeImage, glintCenter1, 3, cv::Scalar(255, 0, 0), -1);  // Blue, filled circle (left glint)
////                                cv::circle(eyeImage, glintCenter2, 3, cv::Scalar(0, 0, 255), -1);  // Red, filled circle (right glint)
////                            }
////                            else
////                            {

////                                cv::circle(eyeImage, glintCenter2, 3, cv::Scalar(255, 0, 0), -1);  // Blue, filled circle (left glint)
////                                cv::circle(eyeImage, glintCenter1, 3, cv::Scalar(0, 0, 255), -1);  // Red, filled circle (right glint)

////                            }

////                            // Additional Visualization (Optional)
////                            cv::line(eyeImage, glintCenter1, glintCenter2, cv::Scalar(0, 255, 0), 1);  // Draw a green line between glints
////                        }
//                    }





// ================== 多项式拟合标定误差评估 ==================

// 评估多项式标定的初始误差
float MainWindow::evaluatePolynomialCalibrationError()
{
    if (predefinedScreenPoints.empty() || allPupilPoints.empty()) {
//        qDebug() << "❌ 无多项式标定数据，无法评估误差";
        return -1.0f;
    }

    if (predefinedScreenPoints.size() != allPupilPoints.size()) {
//        qDebug() << "❌ 标定数据不匹配，无法评估误差";
        return -1.0f;
    }

    // 验证多项式系数是否已初始化
    if (coeffsX.empty() || coeffsY.empty()) {
//        qDebug() << "❌ 多项式系数未初始化，无法评估误差。请先调用computePolynomialMapping()";
        return -1.0f;
    }

    if (coeffsX.rows < 10 || coeffsY.rows < 10) {
//        qDebug() << "❌ 多项式系数数量不足，无法评估误差";
        return -1.0f;
    }

    float totalError = 0.0f;
    int validPoints = 0;

    // 对每个标定点计算预测误差
    for (size_t i = 0; i < predefinedScreenPoints.size(); ++i) {
        const cv::Point2f& actualScreen = predefinedScreenPoints[i];
        const cv::Point2f& pupilPoint = allPupilPoints[i];

        // 使用多项式模型预测屏幕坐标
        cv::Point2f predictedScreen = mapToScreenUsingPolynomial(pupilPoint);

        // 计算误差距离
        float error = cv::norm(actualScreen - predictedScreen);

        // 过滤异常值（可能是数据错误）
        if (error < 500.0f) {  // 假设误差不会超过500像素
            totalError += error;
            validPoints++;
        }
    }

    if (validPoints == 0) {
//        qDebug() << "❌ 无有效的标定点，无法计算误差";
        return -1.0f;
    }

    float averageError = totalError / validPoints;

//    qDebug() << QString("📊 多项式标定误差评估：平均误差 %1 px，有效点数 %2/%3")
//                .arg(averageError, 0, 'f', 1)
//                .arg(validPoints)
//                .arg(predefinedScreenPoints.size());

    return averageError;
}

// 检查多项式标定质量
bool MainWindow::checkPolynomialCalibrationQuality(float averageError)
{
    if (averageError < 0) {
        return false;  // 评估失败
    }

    QString qualityLevel;
    bool isAcceptable = false;

    if (averageError <= POLYNOMIAL_GOOD_THRESHOLD) {
        qualityLevel = "良好";
        isAcceptable = true;
    } else if (averageError <= POLYNOMIAL_ACCEPTABLE_THRESHOLD) {
        qualityLevel = "可接受";
        isAcceptable = true;
    } else if (averageError <= POLYNOMIAL_MAX_ERROR) {
        qualityLevel = "勉强可用";
        isAcceptable = true;  // 勉强可用，但会警告
    } else {
        qualityLevel = "不合格";
        isAcceptable = false;
    }

    // 显示结果给用户
    QString title = isAcceptable ? "多项式标定结果" : "多项式标定失败";
    QString icon = isAcceptable ? "🎉" : "⚠️";

    QString message = QString("%1 多项式标定质量评估\n\n"
                             "🎯 质量等级：%2\n"
                             "📊 平均误差：%3 像素\n")
                     .arg(icon)
                     .arg(qualityLevel)
                     .arg(averageError, 0, 'f', 1);

    if (isAcceptable) {
        if (averageError <= POLYNOMIAL_GOOD_THRESHOLD) {
            message += "✅ 标定质量良好，可以正常使用系统。";
        } else {
            message += "⚠️ 标定质量可接受，但建议在条件允许时重新标定。";
        }
    } else {
        message += "❌ 标定质量不合格，强烈建议重新标定。\n\n";
        message += "💡 改进建议：\n";
        message += "  • 检查眼部跟踪精度\n";
        message += "  • 确保标定时精准注视\n";
        message += "  • 检查光线和摄像头设置\n";
        message += "  • 减少头部移动";
    }

    // 使用QMessageBox显示结果
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(isAcceptable ?
                   (averageError <= POLYNOMIAL_GOOD_THRESHOLD ? QMessageBox::Information : QMessageBox::Warning)
                   : QMessageBox::Critical);

    if (isAcceptable) {
        msgBox.setStandardButtons(QMessageBox::Ok);
    } else {
        msgBox.setStandardButtons(QMessageBox::Retry   |  QMessageBox::Ignore);
        msgBox.setDefaultButton(QMessageBox::Retry);
    }

    int result = msgBox.exec();

    // 处理用户选择
    if (!isAcceptable && result == QMessageBox::Retry) {
//        qDebug() << "🔄 用户选择重新进行多项式标定";
        // 重置标定索引，准备重新标定
        calibrationIndex = 0;
        allPupilPoints.clear();
        allScreenPoints.clear();
        rightPupil = PointVector();
        Markgaze = PointVector();
        return false;  // 表示需要重新标定
    }

    return isAcceptable;
}

// DataTable and GraphPlot related implementations
void MainWindow::onCreateDataTable() {
    if (m_isDestroying) {  // ✅ 析构期间忽略此调用
            qDebug() << "析构期间忽略 onCreateDataTable 调用";
            return;
        }
    printf("MainWindow::onCreateDataTable called, m_dataTable is %s\n",
           m_dataTable ? "not null" : "null");

    if (m_dataTable == nullptr) {
        printf("Creating new DataTable...\n");
        m_dataTable = new DataTable(false, this);
        m_dataTable->setWindowTitle(tr("数据表格"));
        QFont dataTitleFont("Microsoft YaHei", 14, QFont::Bold);
        m_dataTable->setFont(dataTitleFont);
        QIcon dataTableIcon(":/icons/iconSet/histogram.png");
        if (!dataTableIcon.isNull()) {
            QIcon finalTableIcon;
            finalTableIcon.addPixmap(dataTableIcon.pixmap(16, 16));
            finalTableIcon.addPixmap(dataTableIcon.pixmap(24, 24));
            finalTableIcon.addPixmap(dataTableIcon.pixmap(32, 32));
            m_dataTable->setWindowIcon(finalTableIcon);
        }

        if (m_dataSplitter) {
            m_dataSplitter->addWidget(m_dataTable);
            // 设置适当的大小策略，让DataTable在dock窗口中正确显示
            m_dataTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            if (m_dataDock && !m_dataDock->isVisible()) {
                m_dataDock->show();
            }
        }

        // 【关键】确保所有这些连接都存在
        // 调试：确认连接建立
        printf("Establishing DataTable connections...\n");

        // 建立处理结果连接
        connect(this, &MainWindow::processingResultUpdated, m_dataTable, &DataTable::onProcessingResult, Qt::QueuedConnection);


        connect(this, &MainWindow::framecountUpdated, m_dataTable, &DataTable::onCameraFramecount);
        connect(m_dataTable, &DataTable::createGraphPlot, this, &MainWindow::onCreateGraphPlot);

        // 添加实验流程控制信号连接
        connect(m_dataTable, &DataTable::requestShowInteractionDialog, this, &MainWindow::showInteractionDialog);
        connect(m_dataTable, &DataTable::requestHideInteractionDialog, this, &MainWindow::hideInteractionDialog);
        connect(m_dataTable, &DataTable::requestSelectionCount, this, &MainWindow::getSelectionCount);

        // 【修复】让DataTable嵌入到数据分析窗口中，而不是独立显示
        // 移除makeWindowIndependent()调用，保持DataTable在dock窗口中
        m_dataTable->setVisible(true);

        // 确保DataTable不会被自动隐藏
        m_dataTable->setAttribute(Qt::WA_DeleteOnClose, false);

        // 如果有parent dock widget，也要显示它
        if (m_dataDock) {
            m_dataDock->show();
            m_dataDock->setVisible(true);
            printf("DataTable dock shown, dock isVisible: %s\n",
                   m_dataDock->isVisible() ? "true" : "false");
        }

        printf("DataTable created and shown, isVisible: %s\n",
               m_dataTable->isVisible() ? "true" : "false");

    } else {
        if (m_dataDock && !m_dataDock->isVisible()) {
            m_dataDock->show();
        }
    }

    // ==================== 【新增】创建独立的摄像头控制窗口 ====================
    // 所有摄像头相关功能都在独立窗口中，原有控件窗口保持纯净
    if (!m_cameraDock) {
        // 创建独立的摄像头控制窗口
        createCameraDockWindow();
//        qDebug() << "独立的摄像头控制窗口已创建";
    }

    // ==================== 【新增】创建独立的AI状态窗口 ====================
    if (!m_aiStatusDock) {
        // 创建独立的AI状态窗口
        createAIStatusDockWindow();
//        qDebug() << "独立的AI状态窗口已创建";
    }
    // ================================================================
}



void MainWindow::onCreateGraphPlot(const QString &value) {
    // 默认创建Tab模式的GraphPlot
    onCreateGraphPlotAsTab(value);
}

void MainWindow::onCreateGraphPlotAsTab(const QString &value) {
    // Create GraphPlot widget
    GraphPlot* graphPlot = new GraphPlot(value, false, false, this);
    QString windowTitle = tr("图表 - ") + value;
    graphPlot->setWindowTitle(windowTitle);
    QFont graphTabTitleFont("Microsoft YaHei", 14, QFont::Bold);
    graphPlot->setFont(graphTabTitleFont);
    QIcon graphPlotIcon(":/icons/iconSet/histogram.png");
    if (!graphPlotIcon.isNull()) {
        QIcon finalPlotIcon;
        finalPlotIcon.addPixmap(graphPlotIcon.pixmap(16, 16));
        finalPlotIcon.addPixmap(graphPlotIcon.pixmap(24, 24));
        finalPlotIcon.addPixmap(graphPlotIcon.pixmap(32, 32));
        graphPlot->setWindowIcon(finalPlotIcon);
    }
    graphPlot->setMinimumSize(250, 200);

    // 添加到Tab容器中
    if (m_dataTabWidget) {
        m_dataTabWidget->addTab(graphPlot, windowTitle);
        // 只在图表分析窗口隐藏时才显示，避免干扰tab切换
        if (m_graphTabDock && !m_graphTabDock->isVisible()) {
            m_graphTabDock->show();
        }
    }

    connect(this, &MainWindow::processingResultUpdated, graphPlot, static_cast<void (GraphPlot::*)(const ProcessingResult &)>(&GraphPlot::appendData));

    // 2. 连接独立的帧计数器
    connect(this, &MainWindow::framecountUpdated, graphPlot, static_cast<void (GraphPlot::*)(const int &)>(&GraphPlot::appendData));

    // 3. 连接扩展特征数据（从DataTable）
    if (m_dataTable) {
        connect(m_dataTable, &DataTable::extendedFeatureData, graphPlot, &GraphPlot::appendExtendedFeatureData);
    }

    m_graphPlots.append(graphPlot);

//    qDebug() << "GraphPlot created in Tab mode:" << windowTitle;
}

void MainWindow::onCreateGraphPlotAsWindow(const QString &value) {
    // Create GraphPlot widget
    GraphPlot* graphPlot = new GraphPlot(value, false, false, this);
    QString windowTitle = tr("图表 - ") + value;
    graphPlot->setWindowTitle(windowTitle);
    QFont graphWindowTitleFont("Microsoft YaHei", 14, QFont::Bold);
    graphPlot->setFont(graphWindowTitleFont);
    QIcon graphPlotIcon(":/icons/iconSet/histogram.png");
    if (!graphPlotIcon.isNull()) {
        QIcon finalPlotIcon;
        finalPlotIcon.addPixmap(graphPlotIcon.pixmap(16, 16));
        finalPlotIcon.addPixmap(graphPlotIcon.pixmap(24, 24));
        finalPlotIcon.addPixmap(graphPlotIcon.pixmap(32, 32));
        graphPlot->setWindowIcon(finalPlotIcon);
    }
    graphPlot->setMinimumSize(250, 200);

    // 作为独立窗口显示
    graphPlot->setParent(nullptr);
    graphPlot->setWindowFlags(Qt::Window);
    graphPlot->setAttribute(Qt::WA_DeleteOnClose, true);
    graphPlot->resize(600, 400);

    // 设置窗口位置（稍微偏移避免重叠）
    static int windowCount = 0;
    int offsetX = windowCount * 30;
    int offsetY = windowCount * 30;
    graphPlot->move(100 + offsetX, 100 + offsetY);
    windowCount++;

    graphPlot->show();

    // Connect all signals - let GraphPlot decide which data to use based on plotValue
    connect(this, SIGNAL(pupilDataUpdated(quint64,const PupilData&,const QString&)),
            graphPlot, SLOT(appendData(quint64,const PupilData&,const QString&)));
    connect(this, SIGNAL(processingResultUpdated(const ProcessingResult&)),
            graphPlot, SLOT(appendData(const ProcessingResult&)));
    connect(this, SIGNAL(cameraFPSUpdated(double)),
            graphPlot, SLOT(appendData(double)));
    connect(this, SIGNAL(processingFPSUpdated(double)),
            graphPlot, SLOT(appendData(double)));
    connect(this, SIGNAL(framecountUpdated(int)),
            graphPlot, SLOT(appendData(int)));

    // 连接扩展特征数据（从DataTable）
    if (m_dataTable) {
        connect(m_dataTable, &DataTable::extendedFeatureData, graphPlot, &GraphPlot::appendExtendedFeatureData);
    }

    // Store reference
    m_graphPlots.append(graphPlot);

//    qDebug() << "GraphPlot created as independent window:" << windowTitle;
}

void MainWindow::onCloseGraphPlot(GraphPlot* graphPlot) {
    if (graphPlot && m_graphPlots.contains(graphPlot)) {
        // Remove from tracking list
        m_graphPlots.removeAll(graphPlot);

        // QSplitter会在widget被删除时自动处理，无需手动移除
        // 直接删除widget即可
        graphPlot->deleteLater();

//        qDebug() << "GraphPlot removed from splitter and deleted";
    }
}


void MainWindow::emitPupilData(quint64 timestamp, const PupilData &pupil) {
    emit pupilDataUpdated(timestamp, pupil, QString(""));
}

void MainWindow::emitCameraFPS(double fps) {
    emit cameraFPSUpdated(fps);
}

void MainWindow::emitProcessingFPS(double fps) {
    emit processingFPSUpdated(fps);
}

void MainWindow::emitFramecount(int framecount) {
    emit framecountUpdated(framecount);
}

// onGraphPlotClosed function removed to prevent crashes

void MainWindow::saveWindowPositions() {
    // 增强的窗口状态保存系统
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir dir(exeDir);
    dir.cdUp();
    QString settingsPath = dir.absolutePath() + "/EyeTracker_WindowConfig.ini";
    QDir().mkpath(QFileInfo(settingsPath).absolutePath());
    QSettings settings(settingsPath, QSettings::IniFormat);

    // 保存主窗口状态
    settings.setValue("mainWindow/geometry", saveGeometry());
    settings.setValue("mainWindow/windowState", saveState());
    settings.setValue("mainWindow/isMaximized", isMaximized());
    settings.setValue("mainWindow/isMinimized", isMinimized());

    // 保存每个dock窗口的详细状态
    if (m_dataDock) {
        settings.setValue("docks/dataDock/geometry", m_dataDock->saveGeometry());
        settings.setValue("docks/dataDock/floating", m_dataDock->isFloating());
        settings.setValue("docks/dataDock/visible", m_dataDock->isVisible());
        settings.setValue("docks/dataDock/width", m_dataDock->width());
        settings.setValue("docks/dataDock/height", m_dataDock->height());
    }

    if (m_controlsDock) {
        settings.setValue("docks/controlsDock/geometry", m_controlsDock->saveGeometry());
        settings.setValue("docks/controlsDock/floating", m_controlsDock->isFloating());
        settings.setValue("docks/controlsDock/visible", m_controlsDock->isVisible());
        settings.setValue("docks/controlsDock/width", m_controlsDock->width());
        settings.setValue("docks/controlsDock/height", m_controlsDock->height());
    }

    if (m_cameraDock) {
        settings.setValue("docks/cameraDock/geometry", m_cameraDock->saveGeometry());
        settings.setValue("docks/cameraDock/floating", m_cameraDock->isFloating());
        settings.setValue("docks/cameraDock/visible", m_cameraDock->isVisible());
        settings.setValue("docks/cameraDock/width", m_cameraDock->width());
        settings.setValue("docks/cameraDock/height", m_cameraDock->height());
    }

    if (m_aiStatusDock) {
        settings.setValue("docks/aiStatusDock/geometry", m_aiStatusDock->saveGeometry());
        settings.setValue("docks/aiStatusDock/floating", m_aiStatusDock->isFloating());
        settings.setValue("docks/aiStatusDock/visible", m_aiStatusDock->isVisible());
        settings.setValue("docks/aiStatusDock/width", m_aiStatusDock->width());
        settings.setValue("docks/aiStatusDock/height", m_aiStatusDock->height());
    }

    if (m_graphTabDock) {
        settings.setValue("docks/graphTabDock/geometry", m_graphTabDock->saveGeometry());
        settings.setValue("docks/graphTabDock/floating", m_graphTabDock->isFloating());
        settings.setValue("docks/graphTabDock/visible", m_graphTabDock->isVisible());
        settings.setValue("docks/graphTabDock/width", m_graphTabDock->width());
        settings.setValue("docks/graphTabDock/height", m_graphTabDock->height());
    }

    // 记录保存时间戳
    settings.setValue("general/lastSaved", QDateTime::currentDateTime());
    settings.sync();
}

// 删除错误的第一个restoreWindowPositions函数

void MainWindow::restoreWindowPositions() {
    // 增强的窗口状态恢复系统
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir dir(exeDir);
    dir.cdUp();
    QString settingsPath = dir.absolutePath() + "/EyeTracker_WindowConfig.ini";
    QSettings settings(settingsPath, QSettings::IniFormat);

    if (!QFileInfo(settingsPath).exists()) {
        return;
    }

    // 恢复主窗口状态
    QByteArray geometry = settings.value("mainWindow/geometry").toByteArray();
    QByteArray windowState = settings.value("mainWindow/windowState").toByteArray();
    bool wasMaximized = settings.value("mainWindow/isMaximized", false).toBool();

    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }

    if (!windowState.isEmpty()) {
        restoreState(windowState);
    }

    // 延迟恢复dock窗口的详细状态，确保主窗口布局稳定后再应用
    // 增加延迟时间到1000ms，确保Qt布局系统完全初始化
    QTimer::singleShot(1000, this, [this]() {
        // 在lambda内部重新创建QSettings对象
        QString exeDir = QCoreApplication::applicationDirPath();
        QDir dir(exeDir);
        dir.cdUp();
        QString settingsPath = dir.absolutePath() + "/EyeTracker_WindowConfig.ini";
        QSettings settings(settingsPath, QSettings::IniFormat);

        // 恢复每个dock窗口的状态
        if (m_dataDock && settings.contains("docks/dataDock/geometry")) {
            QByteArray dockGeometry = settings.value("docks/dataDock/geometry").toByteArray();
            bool floating = settings.value("docks/dataDock/floating", false).toBool();
            bool visible = settings.value("docks/dataDock/visible", true).toBool();
            int width = settings.value("docks/dataDock/width", 400).toInt();
            int height = settings.value("docks/dataDock/height", 600).toInt();

            if (!dockGeometry.isEmpty()) {
                m_dataDock->restoreGeometry(dockGeometry);
            }
            m_dataDock->setFloating(floating);
            m_dataDock->setVisible(visible);
            // 使用resizeDocks方法而不是resize，更适合停靠窗口
            if (!floating && width > 0 && height > 0) {
                QTimer::singleShot(100, this, [this, width, height]() {
                    resizeDocks({m_dataDock}, {width}, Qt::Horizontal);
                    QTimer::singleShot(50, this, [this, height]() {
                        resizeDocks({m_dataDock}, {height}, Qt::Vertical);
                    });
                });
            }
        }

        if (m_controlsDock && settings.contains("docks/controlsDock/geometry")) {
            QByteArray dockGeometry = settings.value("docks/controlsDock/geometry").toByteArray();
            bool floating = settings.value("docks/controlsDock/floating", false).toBool();
            bool visible = settings.value("docks/controlsDock/visible", true).toBool();
            int width = settings.value("docks/controlsDock/width", 300).toInt();
            int height = settings.value("docks/controlsDock/height", 400).toInt();

            if (!dockGeometry.isEmpty()) {
                m_controlsDock->restoreGeometry(dockGeometry);
            }
            m_controlsDock->setFloating(floating);
            m_controlsDock->setVisible(visible);
            // 使用resizeDocks方法而不是resize，更适合停靠窗口
            if (!floating && width > 0 && height > 0) {
                QTimer::singleShot(150, this, [this, width, height]() {
                    resizeDocks({m_controlsDock}, {width}, Qt::Horizontal);
                    QTimer::singleShot(50, this, [this, height]() {
                        resizeDocks({m_controlsDock}, {height}, Qt::Vertical);
                    });
                });
            }
        }

        // 恢复AI状态窗口几何状态
        if (m_aiStatusDock && settings.contains("docks/aiStatusDock/geometry")) {
            QByteArray dockGeometry = settings.value("docks/aiStatusDock/geometry").toByteArray();
            bool floating = settings.value("docks/aiStatusDock/floating", false).toBool();
            // 强制显示AI状态窗口，忽略保存的visible状态
            bool visible = true;  // 始终显示AI状态窗口
            int width = settings.value("docks/aiStatusDock/width", 400).toInt();
            int height = settings.value("docks/aiStatusDock/height", 400).toInt();

            if (!dockGeometry.isEmpty()) {
                m_aiStatusDock->restoreGeometry(dockGeometry);
            }
            m_aiStatusDock->setFloating(floating);
            m_aiStatusDock->setVisible(visible);
            m_aiStatusDock->show();  // 强制显示
            m_aiStatusDock->raise(); // 提升到前面
            // 使用resizeDocks方法而不是resize，更适合停靠窗口
            if (!floating && width > 0 && height > 0) {
                QTimer::singleShot(200, this, [this, width, height]() {
                    resizeDocks({m_aiStatusDock}, {width}, Qt::Horizontal);
                    QTimer::singleShot(50, this, [this, height]() {
                        resizeDocks({m_aiStatusDock}, {height}, Qt::Vertical);
                    });
                });
            }
        }

        // 恢复摄像头控制窗口几何状态
        if (m_cameraDock && settings.contains("docks/cameraDock/geometry")) {
            QByteArray dockGeometry = settings.value("docks/cameraDock/geometry").toByteArray();
            bool floating = settings.value("docks/cameraDock/floating", false).toBool();
            // 强制显示摄像头控制窗口，忽略保存的visible状态
            bool visible = true;  // 始终显示摄像头控制窗口
            int width = settings.value("docks/cameraDock/width", 300).toInt();
            int height = settings.value("docks/cameraDock/height", 300).toInt();

            if (!dockGeometry.isEmpty()) {
                m_cameraDock->restoreGeometry(dockGeometry);
            }
            m_cameraDock->setFloating(floating);
            m_cameraDock->setVisible(visible);
            m_cameraDock->show();  // 强制显示
            m_cameraDock->raise(); // 提升到前面
            // 使用resizeDocks方法而不是resize，更适合停靠窗口
            if (!floating && width > 0 && height > 0) {
                QTimer::singleShot(250, this, [this, width, height]() {
                    resizeDocks({m_cameraDock}, {width}, Qt::Horizontal);
                    QTimer::singleShot(50, this, [this, height]() {
                        resizeDocks({m_cameraDock}, {height}, Qt::Vertical);
                    });
                });
            }
        }
    });

    if (wasMaximized) {
        showMaximized();
    }

    // 应用响应式布局
    setupResponsiveLayout();
}

// 简化的dock恢复：移除复杂逻辑，使用响应式布局

void MainWindow::restoreDockWidgetPositions() {
    // 不再需要复杂的dock恢复逻辑，响应式布局会自动处理
    setupResponsiveLayout();
}

void MainWindow::restoreDockDetailedPositions() {
    // 简化：不再需要详细位置恢复，使用响应式布局
    setupResponsiveLayout();
}


// 清理所有残留的函数代码

void MainWindow::restoreDockSizes() {
    // 简化：不再需要复杂的dock尺寸恢复，使用响应式布局
    setupResponsiveLayout();
}

// 重复的restoreDockWidgetPositions函数在下面，需要删除或简化

// 删除重复的restoreDockWidgetPositions函数

void MainWindow::restoreCompleteWindowState() {
    // 窗口状态恢复已在构造函数中处理，这里不需要做任何事情

    // 强制显示核心窗口并应用响应式布局
    QTimer::singleShot(100, this, [this]() {
        if (m_controlsDock) {
            m_controlsDock->show();
            m_controlsDock->raise();
        }

        // 应用响应式布局
        setupResponsiveLayout();
    });
}

void MainWindow::onToggleDataTable() {
    if (m_dataTable == nullptr) {
        // If DataTable doesn't exist, create it
        onCreateDataTable();
    } else {
        // 在splitter模式下，切换整个数据分析dock的显示状态
        if (m_dataDock) {
            if (m_dataDock->isVisible()) {
                m_dataDock->hide();
            } else {
                // 只在数据分析窗口隐藏时才显示，避免干扰tab切换
                if (!m_dataDock->isVisible()) {
                    m_dataDock->show();
                }
            }
        }
    }
}

void MainWindow::onShowAllGraphPlots() {
    // 在splitter模式下，显示所有GraphPlot就是显示整个数据分析dock
    if (m_dataDock) {
        m_dataDock->show();
        m_dataDock->raise();
    }
}

void MainWindow::onHideAllGraphPlots() {
    // 隐藏数据分析dock（包含所有GraphPlot）
    if (m_dataDock) {
        m_dataDock->hide();
    }
}

// 语法错误修复完成，所有函数都已清理

void MainWindow::updateViewMenu() {
    // This method could be used to dynamically update the view menu
    // For now, we'll implement a basic check for window visibility
    // and could be expanded to show individual controls for each graph plot

    // Future enhancement: Add individual menu items for each graph plot
    // that show their current visibility status
}

void MainWindow::onRestoreAllWindows() {
    // Show all image windows
    if (m_sceneDock) {
        m_sceneDock->show();
        m_sceneDock->raise();
    }

    // Show all dock windows
    if (m_controlsDock) {
        m_controlsDock->show();
        m_controlsDock->raise();
    }
    if (m_dataDock) {
        m_dataDock->show();
        m_dataDock->raise();
    }
    if (m_graphTabDock) {
        m_graphTabDock->show();
        m_graphTabDock->raise();
    }
    if (m_cameraDock) {
        m_cameraDock->show();
        m_cameraDock->raise();
    }

    // Apply responsive layout
    setupResponsiveLayout();
}

void MainWindow::onToggleSceneWindow() {
    if (m_sceneDock) {
        m_sceneDock->setVisible(!m_sceneDock->isVisible());
    }
}

void MainWindow::onToggleEyeWindow() {
    if (m_eyeDock) {
        m_eyeDock->setVisible(!m_eyeDock->isVisible());
    }
}

void MainWindow::onToggleControlsWindow() {
    if (m_controlsDock) {
        m_controlsDock->setVisible(!m_controlsDock->isVisible());
    }
}

void MainWindow::saveSessionState() {
    // 保存会话状态到exe上一层目录
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir dir(exeDir);
    dir.cdUp(); // 回到上一层目录
    QString settingsPath = dir.absolutePath() + "/EyeTracker_WindowConfig.ini";
    QDir().mkpath(QFileInfo(settingsPath).absolutePath());
    QSettings settings(settingsPath, QSettings::IniFormat);

    // Save DataTable state
    bool dataTableExists = (m_dataTable != nullptr);
    bool dataTableVisible = false;
    if (dataTableExists) {
        QDockWidget* dock = qobject_cast<QDockWidget*>(m_dataTable->parent());
        dataTableVisible = (dock && dock->isVisible());
    }

    settings.setValue("session/dataTableExists", dataTableExists);
    settings.setValue("session/dataTableVisible", dataTableVisible);

    // 保存控件和摄像头窗口的显示状态
    if (m_controlsDock) {
        settings.setValue("session/controlsDockVisible", m_controlsDock->isVisible());
    }
    if (m_cameraDock) {
        settings.setValue("session/cameraDockVisible", m_cameraDock->isVisible());
    }
    if (m_aiStatusDock) {
        settings.setValue("session/aiStatusDockVisible", m_aiStatusDock->isVisible());
    }

    settings.sync();
}

void MainWindow::restoreSessionState() {
    // 从exe上一层目录读取配置文件
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir dir(exeDir);
    dir.cdUp();
    QString settingsPath = dir.absolutePath() + "/EyeTracker_WindowConfig.ini";
    QSettings settings(settingsPath, QSettings::IniFormat);

    // 【修复】默认创建DataTable，确保数据分析窗口在启动时显示
    bool dataTableExists = settings.value("session/dataTableExists", true).toBool(); // 默认为true
    bool dataTableVisible = settings.value("session/dataTableVisible", true).toBool(); // 默认为true

    if (dataTableExists) {
        if (!m_dataTable) {
            onCreateDataTable();
        }
        if (m_dataDock && dataTableVisible) {
            m_dataDock->show();
        }
    }

    // 恢复控件和摄像头窗口的显示状态
    if (m_controlsDock) {
        bool visible = settings.value("session/controlsDockVisible", true).toBool();
        m_controlsDock->setVisible(visible);
    }
    if (m_cameraDock) {
        bool visible = settings.value("session/cameraDockVisible", false).toBool();
        m_cameraDock->setVisible(visible);
    }
    if (m_aiStatusDock) {
        bool visible = settings.value("session/aiStatusDockVisible", true).toBool();  // 默认显示
        m_aiStatusDock->setVisible(visible);
    }
}

void MainWindow::debugWindowState() {
    // This function is implemented below
}





// Handle application close event to save window layout
// void MainWindow::closeEvent(QCloseEvent *event) {
//     // Save window positions and layout before closing
//     saveWindowPositions();
//     saveSessionState();

//     // Accept the close event
//     event->accept();
// }

// Glint detection implementation

bool MainWindow::detectGlints(const Mat &eyeImage, const Rect &coarseROI, const cv::Point2f &pupilCenter, float /*pupilRadius*/, float &x1, float &y1, float &x2, float &y2, Mat &debugImg)
{
    Mat roi = eyeImage(coarseROI), gray;
    if (roi.channels() > 1)
        cvtColor(roi, gray, COLOR_BGR2GRAY);
    else
        gray = roi;

    // 渐进式检测策略：从严格到宽松
    // 修改为保存亮度、面积、位置信息和圆形度
    struct GlintCandidate {
        float brightness;
        float area;
        float circularity;
        cv::Point2f center;
    };
    std::vector<GlintCandidate> candidates;
    candidates.reserve(50);

    // 计算统计信息用于调试
    cv::Scalar meanVal, stdDev;
    cv::meanStdDev(gray, meanVal, stdDev);
    double minVal, maxVal;
    cv::minMaxLoc(gray, &minVal, &maxVal);

//    qDebug() << "[Glint Debug] ROI size:" << gray.cols << "x" << gray.rows;
//    qDebug() << "[Glint Debug] Pupil radius:" << pupilRadius;
//    qDebug() << "[Glint Debug] Brightness - Mean:" << meanVal[0] << "Max:" << maxVal << "Min:" << minVal;

    // 多层次阈值检测（从高到低）
    std::vector<int> thresholds = {200, 180, 160, 140, 120, 100, 80};  // 更宽松的阈值范围
    Mat lastBinary;  // 保存最后一次二值化结果用于调试

    for (int thresh : thresholds) {
        Mat binary;
        threshold(gray, binary, thresh, 255, THRESH_BINARY);
        lastBinary = binary.clone();  // 保存用于调试显示

        std::vector<std::vector<Point>> contours;
        findContours(binary, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

//        qDebug() << "[Glint Debug] Threshold" << thresh << "found" << contours.size() << "contours";

        // 宽松的面积约束（从严格到宽松）
        float minArea = 1.0f;  // 最小1像素
        float maxArea = (thresh >= 160) ? 200.0f : 1000.0f;  // 高阈值时更严格
        const float MIN_CIRCULARITY = 0.6f;  // 圆形度阈值，亮斑必须是圆形

        for (size_t i = 0; i < contours.size(); ++i) {
            double area = contourArea(contours[i]);
            if (area >= minArea && area <= maxArea) {
                // 计算圆形度 = (4 * π * 面积) / (周长²)
                double perimeter = arcLength(contours[i], true);
                float circularity = 0.0f;
                if (perimeter > 0) {
                    circularity = (4.0f * M_PI * area) / (perimeter * perimeter);
                }

                // 圆形度检验：只有圆形度达标的才是真正的亮斑
                if (circularity >= MIN_CIRCULARITY) {
                    Moments m = moments(contours[i]);
                    if (m.m00 > 0) {
                        cv::Point2f center(m.m10/m.m00, m.m01/m.m00);
                        int cx = static_cast<int>(std::round(center.x));
                        int cy = static_cast<int>(std::round(center.y));
                        if (cx >= 0 && cx < gray.cols && cy >= 0 && cy < gray.rows && gray.type() == CV_8UC1) {
                            uchar brightness = gray.at<uchar>(cy, cx);
                            candidates.push_back({(float)brightness, (float)area, circularity, center});
//                            qDebug() << "[Glint Debug] Valid Candidate" << i << "- pos:" << center.x << "," << center.y
//                                    << "brightness:" << brightness << "area:" << area << "circularity:" << circularity << "thresh:" << thresh;
                        }
                    }
                } else {
//                    qDebug() << "[Glint Debug] Rejected candidate" << i << "- low circularity:" << circularity << "(min:" << MIN_CIRCULARITY << ")";
                }
            }
        }

            // 如果找到足够候选点就停止 - 但如果有4个以上可以考虑更精确的选择
        if (candidates.size() >= 2) {
//            qDebug() << "[Glint Debug] Found" << candidates.size() << "candidates, stopping at threshold" << thresh;
            break;
        }
    }

//    qDebug() << "[Glint Debug] Total candidates found:" << candidates.size();

    if (candidates.empty()) {
//        qDebug() << "[Glint Debug] No candidates found - creating debug image with all pixels above 80";
        // 返回一个调试图显示所有亮点
        threshold(gray, lastBinary, 80, 255, THRESH_BINARY);
        cvtColor(lastBinary, debugImg, COLOR_GRAY2BGR);
        putText(debugImg, "No glints found!", cv::Point(5, 15),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,0,255), 1);
        return false;
    }

    // 新的亮斑选择策略：支持从4个亮斑中选择面积最大的两个
    const float MIN_DISTANCE = 15.0f;
    const float MAX_DISTANCE = 100.0f;
    const float MAX_Y_DIFF = 5.0f;

    x1 = x2 = y1 = y2 = 0;
    bool foundPair = false;

    if (candidates.size() >= 4) {
//        qDebug() << "[Glint Debug] 检测到" << candidates.size() << "个候选亮斑，使用面积+圆形度综合评分策略";

        // 按综合评分排序：面积 * 圆形度，优先选择又大又圆的亮斑
        std::sort(candidates.begin(), candidates.end(),
                 [](const GlintCandidate& a, const GlintCandidate& b) {
                     float scoreA = a.area * a.circularity;
                     float scoreB = b.area * b.circularity;
                     return scoreA > scoreB;
                 });

        // 尝试所有面积最大的候选点组合
        for (size_t i = 0; i < candidates.size() && !foundPair; ++i) {
            for (size_t j = i + 1; j < candidates.size() && !foundPair; ++j) {
                cv::Point2f p1 = candidates[i].center;
                cv::Point2f p2 = candidates[j].center;
                float distance = norm(p1 - p2);
                float y_diff = abs(p1.y - p2.y);

                // 调试评分计算（已禁用）
//                float scoreI = candidates[i].area * candidates[i].circularity;
//                float scoreJ = candidates[j].area * candidates[j].circularity;
//                qDebug() << "[Glint Debug] 综合评分：候选" << i << "(面积:" << candidates[i].area
//                         << " 圆形度:" << candidates[i].circularity << " 评分:" << scoreI
//                         << ") 和候选" << j << "(面积:" << candidates[j].area
//                         << " 圆形度:" << candidates[j].circularity << " 评分:" << scoreJ
//                         << ") 距离:" << distance << " Y差:" << y_diff;

                if (distance >= MIN_DISTANCE && distance <= MAX_DISTANCE && y_diff <= MAX_Y_DIFF) {
                    x1 = p1.x + coarseROI.x;
                    y1 = p1.y + coarseROI.y;
                    x2 = p2.x + coarseROI.x;
                    y2 = p2.y + coarseROI.y;
                    foundPair = true;
//                    qDebug() << "[Glint Debug] 选择综合评分最高的有效对：面积" << candidates[i].area << "(圆形度:" << candidates[i].circularity << ") 和面积" << candidates[j].area << "(圆形度:" << candidates[j].circularity << ")";
                }
            }
        }
    }

    if (!foundPair && candidates.size() >= 2) {
//        qDebug() << "[Glint Debug] 面积策略未找到有效对，回退到亮度策略";

        // 回退到原来的亮度优先策略
        std::sort(candidates.begin(), candidates.end(),
                 [](const GlintCandidate& a, const GlintCandidate& b) {
                     return a.brightness > b.brightness;
                 });

        size_t sortCount = min((size_t)5, candidates.size());
        cv::Point2f p1 = candidates[0].center;
        float bestScore = 0;

        x1 = p1.x + coarseROI.x;
        y1 = p1.y + coarseROI.y;
        x2 = x1; y2 = y1;

//        qDebug() << "[Glint Debug] 最亮候选点: pos(" << p1.x << "," << p1.y
//                 << ") 亮度:" << candidates[0].brightness << " 面积:" << candidates[0].area;

        for (size_t i = 1; i < sortCount; ++i) {
            cv::Point2f p2 = candidates[i].center;
            float distance = norm(p1 - p2);
            float y_diff = abs(p1.y - p2.y);

//            qDebug() << "[Glint Debug] 检查候选点" << i << " 距离:" << distance << " Y差:" << y_diff;

            if (distance >= MIN_DISTANCE && distance <= MAX_DISTANCE && y_diff <= MAX_Y_DIFF) {
                float score = candidates[i].brightness;
                if (score > bestScore) {
                    bestScore = score;
                    x2 = p2.x + coarseROI.x;
                    y2 = p2.y + coarseROI.y;
                    foundPair = true;
//                    qDebug() << "[Glint Debug] 新的最佳对: 距离" << distance << " Y差" << y_diff << " 亮度" << score;
                }
            } else if (y_diff > MAX_Y_DIFF) {
//                qDebug() << "[Glint Debug] 候选点" << i << "被拒绝: Y差值过大 (" << y_diff << " > " << MAX_Y_DIFF << ")";
            }
        }
    }

    if (!foundPair && candidates.size() == 1) {
//        qDebug() << "[Glint Debug] 只找到1个候选亮斑";
        cv::Point2f singleGlint(candidates[0].center.x + coarseROI.x, candidates[0].center.y + coarseROI.y);
        // 使用传入的准确瞳孔中心位置

        // 根据单个亮斑相对于瞳孔的位置确定是glint1还是glint2
        if (singleGlint.x <= pupilCenter.x) {
            // 亮斑在瞳孔左边（或中心），是glint1（左边亮斑）
            x1 = singleGlint.x;
            y1 = singleGlint.y;
            x2 = 0; y2 = 0; // glint2无效
//            qDebug() << "[Glint Debug] 单个亮斑定义为glint1（左边）";
        } else {
            // 亮斑在瞳孔右边，是glint2（右边亮斑）
            x1 = 0; y1 = 0; // glint1无效
            x2 = singleGlint.x;
            y2 = singleGlint.y;
//            qDebug() << "[Glint Debug] 单个亮斑定义为glint2（右边）";
        }
    }

    // 对两个亮斑进行位置排序：确保glint1在左边，glint2在右边
    if (foundPair && x1 > 0 && x2 > 0) {
        if (x1 > x2) {
            // 交换位置，确保x1(glint1)在左边，x2(glint2)在右边
            std::swap(x1, x2);
            std::swap(y1, y2);
//            qDebug() << "[Glint Debug] 交换亮斑位置：glint1(" << x1 << "," << y1 << ") glint2(" << x2 << "," << y2 << ")";
        } else {
//            qDebug() << "[Glint Debug] 亮斑位置正确：glint1(" << x1 << "," << y1 << ") glint2(" << x2 << "," << y2 << ")";
        }
    }

    // 调试图像
    cvtColor(lastBinary, debugImg, COLOR_GRAY2BGR);

    // 绘制所有候选点
    for (size_t i = 0; i < min((size_t)5, candidates.size()); ++i) {
        cv::Point2f pt = candidates[i].center;
        cv::Scalar color = (i == 0) ? cv::Scalar(0,255,0) : cv::Scalar(255,0,0);  // 绿色：最佳，蓝色：其他
        circle(debugImg, pt, 3, color, -1);
        char info[50];
        snprintf(info, sizeof(info), "B:%d A:%d C:%.2f", (int)candidates[i].brightness, (int)candidates[i].area, candidates[i].circularity);
        putText(debugImg, info, cv::Point(pt.x + 5, pt.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.25, color, 1);
    }

    // 绘制最终结果
    circle(debugImg, cv::Point2f(x1-coarseROI.x, y1-coarseROI.y), 5, cv::Scalar(0,255,0), 2);  // 绿色大圈
    if (x2 != x1 || y2 != y1) {
        circle(debugImg, cv::Point2f(x2-coarseROI.x, y2-coarseROI.y), 5, cv::Scalar(0,255,0), 2);
        line(debugImg, cv::Point2f(x1-coarseROI.x, y1-coarseROI.y),
             cv::Point2f(x2-coarseROI.x, y2-coarseROI.y), cv::Scalar(255,255,0), 2);

        float finalDistance = norm(cv::Point2f(x1, y1) - cv::Point2f(x2, y2));
        float finalYDiff = abs(y1 - y2);
        putText(debugImg, "Dist: " + std::to_string((int)finalDistance), cv::Point(5, 45),
                cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,255,0), 1);
        putText(debugImg, "Y-diff: " + std::to_string((int)finalYDiff), cv::Point(5, 60),
                cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,255,0), 1);
    }

    // 添加关键信息
    putText(debugImg, "Candidates: " + std::to_string(candidates.size()), cv::Point(5, 15),
            cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,255,255), 1);
    std::string strategy = (candidates.size() >= 4) ? "Area*Circularity strategy" : "Brightness-first strategy";
    putText(debugImg, "Range: 15-100px, Y-diff<=5px", cv::Point(5, 30),
            cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,255,255), 1);
    putText(debugImg, strategy, cv::Point(5, 75),
            cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255,255,255), 1);

    bool finalResult = (x2 != x1 || y2 != y1) || candidates.size() == 1;
//    qDebug() << "[Glint Debug] Final result: foundPair =" << finalResult << " (候选点数:" << candidates.size() << ")";
    return finalResult;
}

// ========== 双亮斑瞳孔角膜映射系统核心函数实现 ==========

void MainWindow::addDualGlintCalibrationSample(const cv::Point2f& pupilCenter,
                                              const cv::Point2f& glint1, bool hasGlint1,
                                              const cv::Point2f& glint2, bool hasGlint2,
                                              const cv::Point2f& screenTarget)
{
    // 存储标定数据
    m_dualGlint_pupilCenters.push_back(pupilCenter);
    m_dualGlint_glint1Positions.push_back(glint1);
    m_dualGlint_glint2Positions.push_back(glint2);
    m_dualGlint_screenTargets.push_back(screenTarget);
    m_dualGlint_hasGlint1.push_back(hasGlint1);
    m_dualGlint_hasGlint2.push_back(hasGlint2);

//    qDebug() << "[DualGlint] 添加标定样本" << m_dualGlint_pupilCenters.size()
//            << " - 瞳孔:(" << pupilCenter.x << "," << pupilCenter.y << ")"
//            << " 目标:(" << screenTarget.x << "," << screenTarget.y << ")"
//            << " 亮斑1:" << (hasGlint1 ? "有效" : "无效")
//            << " 亮斑2:" << (hasGlint2 ? "有效" : "无效");
}

void MainWindow::computeDualGlintMappings()
{
    const int minSamples = 6; // 至少需要6个样本进行多项式拟合

    // 统计有效的亮斑样本数量
    int glint1ValidCount = 0, glint2ValidCount = 0;
    for (size_t i = 0; i < m_dualGlint_hasGlint1.size(); ++i) {
        if (m_dualGlint_hasGlint1[i]) glint1ValidCount++;
        if (m_dualGlint_hasGlint2[i]) glint2ValidCount++;
    }

//    qDebug() << "[DualGlint] 开始计算映射矩阵 - 总样本:" << m_dualGlint_pupilCenters.size()
//            << " 亮斑1有效:" << glint1ValidCount << " 亮斑2有效:" << glint2ValidCount;

    // 重置映射状态
    m_dualGlintMapping.glint1MappingValid = false;
    m_dualGlintMapping.glint2MappingValid = false;

    // 计算亮斑1的映射
    if (glint1ValidCount >= minSamples) {
        std::vector<cv::Point2f> vectors1, targets1;

        for (size_t i = 0; i < m_dualGlint_pupilCenters.size(); ++i) {
            if (m_dualGlint_hasGlint1[i]) {
                cv::Point2f vector = pupilToGlintVector(m_dualGlint_pupilCenters[i], m_dualGlint_glint1Positions[i]);
                vectors1.push_back(vector);
                targets1.push_back(m_dualGlint_screenTargets[i]);
            }
        }

        // 使用多项式拟合计算映射矩阵
        if (computeMappingMatrix(vectors1, targets1, m_dualGlintMapping.mappingMatrix_Glint1)) {
            m_dualGlintMapping.glint1MappingValid = true;
            m_dualGlintMapping.glint1Accuracy = evaluateMappingAccuracy(vectors1, targets1, m_dualGlintMapping.mappingMatrix_Glint1);
//            qDebug() << "[DualGlint] 亮斑1映射计算成功，精度:" << m_dualGlintMapping.glint1Accuracy;
        } else {
//            qDebug() << "[DualGlint] 亮斑1映射计算失败";
        }
    }

    // 计算亮斑2的映射
    if (glint2ValidCount >= minSamples) {
        std::vector<cv::Point2f> vectors2, targets2;

        for (size_t i = 0; i < m_dualGlint_pupilCenters.size(); ++i) {
            if (m_dualGlint_hasGlint2[i]) {
                cv::Point2f vector = pupilToGlintVector(m_dualGlint_pupilCenters[i], m_dualGlint_glint2Positions[i]);
                vectors2.push_back(vector);
                targets2.push_back(m_dualGlint_screenTargets[i]);
            }
        }

        // 使用多项式拟合计算映射矩阵
        if (computeMappingMatrix(vectors2, targets2, m_dualGlintMapping.mappingMatrix_Glint2)) {
            m_dualGlintMapping.glint2MappingValid = true;
            m_dualGlintMapping.glint2Accuracy = evaluateMappingAccuracy(vectors2, targets2, m_dualGlintMapping.mappingMatrix_Glint2);
//            qDebug() << "[DualGlint] 亮斑2映射计算成功，精度:" << m_dualGlintMapping.glint2Accuracy;
        } else {
//            qDebug() << "[DualGlint] 亮斑2映射计算失败";
        }
    }
}

cv::Point2f MainWindow::calculateDualGlintGaze(const cv::Point2f& pupilCenter,
                                             const cv::Point2f& glint1, bool hasGlint1,
                                             const cv::Point2f& glint2, bool hasGlint2)
{
    cv::Point2f result(0, 0);
    int validMappings = 0;

    // 使用亮斑1映射
    if (hasGlint1 && m_dualGlintMapping.glint1MappingValid) {
        cv::Point2f vector1 = pupilToGlintVector(pupilCenter, glint1);
        cv::Point2f gaze1 = mapVectorToScreen(vector1, m_dualGlintMapping.mappingMatrix_Glint1);
        result += gaze1;
        validMappings++;

//        qDebug() << "[DualGlint] 亮斑1映射结果: (" << gaze1.x << "," << gaze1.y << ")";
    }

    // 使用亮斑2映射
    if (hasGlint2 && m_dualGlintMapping.glint2MappingValid) {
        cv::Point2f vector2 = pupilToGlintVector(pupilCenter, glint2);
        cv::Point2f gaze2 = mapVectorToScreen(vector2, m_dualGlintMapping.mappingMatrix_Glint2);
        result += gaze2;
        validMappings++;

//        qDebug() << "[DualGlint] 亮斑2映射结果: (" << gaze2.x << "," << gaze2.y << ")";
    }

    // 求平均或使用单一映射
    if (validMappings > 0) {
        result.x /= validMappings;
        result.y /= validMappings;

//        qDebug() << "[DualGlint] 最终结果 (使用" << validMappings << "个映射): (" << result.x << "," << result.y << ")";
    } else {
//        qDebug() << "[DualGlint] 警告：没有有效的映射可用";
    }

    return result;
}

cv::Point2f MainWindow::pupilToGlintVector(const cv::Point2f& pupil, const cv::Point2f& glint)
{
    return cv::Point2f(glint.x - pupil.x, glint.y - pupil.y);
}

cv::Point2f MainWindow::mapVectorToScreen(const cv::Point2f& vector, const cv::Mat& mappingMatrix)
{
    if (mappingMatrix.empty() || mappingMatrix.rows < 6) {
//        qDebug() << "[DualGlint] 错误：映射矩阵无效";
        return cv::Point2f(0, 0);
    }

    // 使用多项式映射：[1, x, y, x*y, x^2, y^2] * coeffs
    cv::Mat features = (cv::Mat_<double>(6, 1) <<
        1.0, vector.x, vector.y, vector.x*vector.y, vector.x*vector.x, vector.y*vector.y);

    cv::Mat result = mappingMatrix * features;

    return cv::Point2f(result.at<double>(0, 0), result.at<double>(1, 0));
}

// 辅助函数：计算映射矩阵
bool MainWindow::computeMappingMatrix(const std::vector<cv::Point2f>& inputVectors,
                                    const std::vector<cv::Point2f>& outputTargets,
                                    cv::Mat& mappingMatrix)
{
    if (inputVectors.size() != outputTargets.size() || inputVectors.size() < 6) {
//        qDebug() << "[DualGlint] 错误：样本数量不足或不匹配";
        return false;
    }

    int n = static_cast<int>(inputVectors.size());

    // 构建特征矩阵 A: [1, x, y, x*y, x^2, y^2]
    cv::Mat A(n, 6, CV_64F);
    cv::Mat bX(n, 1, CV_64F);
    cv::Mat bY(n, 1, CV_64F);

    for (int i = 0; i < n; ++i) {
        float x = inputVectors[i].x;
        float y = inputVectors[i].y;

        A.at<double>(i, 0) = 1.0;
        A.at<double>(i, 1) = x;
        A.at<double>(i, 2) = y;
        A.at<double>(i, 3) = x * y;
        A.at<double>(i, 4) = x * x;
        A.at<double>(i, 5) = y * y;

        bX.at<double>(i, 0) = outputTargets[i].x;
        bY.at<double>(i, 0) = outputTargets[i].y;
    }

    // 求解最小二乘：A * coeffs = b
    cv::Mat coeffsX, coeffsY;
    bool solvedX = cv::solve(A, bX, coeffsX, cv::DECOMP_SVD);
    bool solvedY = cv::solve(A, bY, coeffsY, cv::DECOMP_SVD);

    if (solvedX && solvedY) {
        // 组合X和Y的系数到映射矩阵
        mappingMatrix = cv::Mat(2, 6, CV_64F);
        for (int i = 0; i < 6; ++i) {
            mappingMatrix.at<double>(0, i) = coeffsX.at<double>(i, 0);
            mappingMatrix.at<double>(1, i) = coeffsY.at<double>(i, 0);
        }
        return true;
    }

//    qDebug() << "[DualGlint] 错误：矩阵求解失败";
    return false;
}

// 辅助函数：评估映射精度
float MainWindow::evaluateMappingAccuracy(const std::vector<cv::Point2f>& inputVectors,
                                        const std::vector<cv::Point2f>& outputTargets,
                                        const cv::Mat& mappingMatrix)
{
    float totalError = 0.0f;
    int n = static_cast<int>(inputVectors.size());

    for (int i = 0; i < n; ++i) {
        cv::Point2f predicted = mapVectorToScreen(inputVectors[i], mappingMatrix);
        cv::Point2f actual = outputTargets[i];

        float error = sqrt((predicted.x - actual.x) * (predicted.x - actual.x) +
                          (predicted.y - actual.y) * (predicted.y - actual.y));
        totalError += error;
    }

    return totalError / n;
}

// 🚀 亮斑跟踪算法实现 - 复用PupilTrackingMethod

// 真正的集成式亮斑跟踪（解决跳变问题）
// 🆔 亮斑ID分配规则：
// - 双亮斑：左边是G1（第一个），右边是G2（第二个）
// - 单亮斑：瞳孔左边是G2（第二个），瞳孔右边是G1（第一个）
bool MainWindow::integratedGlintTracking(const cv::Mat &eyeImage, const cv::Rect &coarseROI,
                                        const cv::Point2f &pupilCenter, float pupilRadius,
                                        float &x1, float &y1, float &x2, float &y2, cv::Mat &debugImg)
{
    // 初始化输出
    x1 = x2 = y1 = y2 = 0;
    bool foundAnyGlint = false;

    // 创建调试图像
    if (eyeImage.channels() == 1) {
        cv::cvtColor(eyeImage, debugImg, cv::COLOR_GRAY2BGR);
    } else {
        debugImg = eyeImage.clone();
    }

    // 🎯 真正的跟踪策略：使用历史位置信息进行智能匹配
    std::vector<cv::Point2f> detectedGlints;
    if (detectGlintsSimplified(eyeImage, coarseROI, pupilCenter, pupilRadius, detectedGlints)) {

        if (!detectedGlints.empty()) {
            foundAnyGlint = true;

            if (detectedGlints.size() >= 2) {
                // 🔧 智能配对：使用距离匹配而不是简单排序
                cv::Point2f glint1, glint2;
                bool hasValidTracking = (m_lastGlint1.valid && m_lastGlint2.valid);

                if (hasValidTracking) {
                    // 使用历史位置进行最近邻匹配
                    float dist1_to_last1 = cv::norm(detectedGlints[0] - m_lastGlint1.center);
                    float dist1_to_last2 = cv::norm(detectedGlints[0] - m_lastGlint2.center);
                    float dist2_to_last1 = cv::norm(detectedGlints[1] - m_lastGlint1.center);
                    float dist2_to_last2 = cv::norm(detectedGlints[1] - m_lastGlint2.center);

                    // 选择总距离最小的配对方案
                    if ((dist1_to_last1 + dist2_to_last2) < (dist1_to_last2 + dist2_to_last1)) {
                        glint1 = detectedGlints[0];
                        glint2 = detectedGlints[1];
                    } else {
                        glint1 = detectedGlints[1];
                        glint2 = detectedGlints[0];
                    }

                    // 🚨 运动连续性验证：检查是否有大跳变
                    const float MAX_MOVEMENT_PER_FRAME = 25.0f;
                    float movement1 = cv::norm(glint1.y - m_lastGlint1.center.y);
                    float movement2 = cv::norm(glint2.y - m_lastGlint2.center.y);

                    if (movement1 > MAX_MOVEMENT_PER_FRAME || movement2 > MAX_MOVEMENT_PER_FRAME) {
//                        qDebug() << "[GlintTrack] 警告：检测到大跳变 - G1移动:" << movement1 << "px, G2移动:" << movement2 << "px";

                        if (detectGlintsSimplified(eyeImage, coarseROI, pupilCenter, pupilRadius, detectedGlints))
                        {
                            if (detectedGlints[0].x > detectedGlints[1].x)
                            {
                                glint1 = detectedGlints[1];
                                glint2 = detectedGlints[0];
                            } else {
                                glint1 = detectedGlints[0];
                                glint2 = detectedGlints[1];
                            }

//                            qDebug() << "[GlintTrack] 重新检测";
                        }
                    }
                } else {
                    // 首次检测：按X坐标排序
                    if (detectedGlints[0].x > detectedGlints[1].x) {
                        glint1 = detectedGlints[1];
                        glint2 = detectedGlints[0];
                    } else {
                        glint1 = detectedGlints[0];
                        glint2 = detectedGlints[1];
                    }
                }

                x1 = glint1.x; y1 = glint1.y;
                x2 = glint2.x; y2 = glint2.y;

                // 更新历史位置
                m_lastGlint1 = GlintAsCircle(glint1, 3.0f, 0.9f);
                m_lastGlint2 = GlintAsCircle(glint2, 3.0f, 0.9f);
                m_lastGlint1.valid = m_lastGlint2.valid = true;

                // 可视化（确保ID标识清晰）
                cv::circle(debugImg, cv::Point(x1, y1), 5, cv::Scalar(0, 255, 0), 2);  // 绿色-G1
                cv::circle(debugImg, cv::Point(x2, y2), 5, cv::Scalar(0, 0, 255), 2);  // 红色-G2
                cv::putText(debugImg, "G1-L", cv::Point(x1 + 8, y1 - 8),  // L=Left
                           cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 1);
                cv::putText(debugImg, "G2-R", cv::Point(x2 + 8, y2 - 8),  // R=Right
                           cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 255), 1);

                // 验证ID分配正确性
                if (x1 > x2) {
//                    qDebug() << "[GlintTrack] 错误：G1应该在左边，但G1.x=" << x1 << " > G2.x=" << x2;
                }

            } else {
                // 只检测到一个亮斑 - 智能分配给最接近的历史位置
                cv::Point2f singleGlint = detectedGlints[0];

                if (m_lastGlint1.valid && m_lastGlint2.valid) {
                    // 分配给距离更近的历史位置，并验证运动合理性
                    float dist1 = cv::norm(singleGlint - m_lastGlint1.center);
                    float dist2 = cv::norm(singleGlint - m_lastGlint2.center);
                    const float MAX_SINGLE_MOVEMENT = 30.0f;

                    // 验证移动合理性
                    bool validMovement1 = (dist1 <= MAX_SINGLE_MOVEMENT);
                    bool validMovement2 = (dist2 <= MAX_SINGLE_MOVEMENT);

                    if (validMovement1 && dist1 < dist2) {
                        x1 = singleGlint.x; y1 = singleGlint.y;
                        m_lastGlint1 = GlintAsCircle(singleGlint, 3.0f, 0.9f);
                        m_lastGlint1.valid = true;
                        cv::circle(debugImg, cv::Point(x1, y1), 5, cv::Scalar(0, 255, 0), 2);
                        cv::putText(debugImg, "G1", cv::Point(x1 + 8, y1 - 8),
                                   cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 1);
                    } else if (validMovement2 && dist2 < dist1) {
                        x2 = singleGlint.x; y2 = singleGlint.y;
                        m_lastGlint2 = GlintAsCircle(singleGlint, 3.0f, 0.9f);
                        m_lastGlint2.valid = true;
                        cv::circle(debugImg, cv::Point(x2, y2), 5, cv::Scalar(0, 0, 255), 2);
                        cv::putText(debugImg, "G2", cv::Point(x2 + 8, y2 - 8),
                                   cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 255), 1);
                    } else {
                        // 移动过大，忽略该帧或使用降低置信度的容错
//                        qDebug() << "[GlintTrack] 单点移动过大，忽略 - Dist1:" << dist1 << ", Dist2:" << dist2;
                        if (dist1 < dist2 && dist1 < MAX_SINGLE_MOVEMENT * 1.5f) {
                            // 放宽一些限制的容错
                            x1 = singleGlint.x; y1 = singleGlint.y;
                            m_lastGlint1 = GlintAsCircle(singleGlint, 3.0f, 0.7f);  // 降低置信度
                            m_lastGlint1.valid = true;
                            cv::circle(debugImg, cv::Point(x1, y1), 5, cv::Scalar(0, 128, 255), 2);  // 橙色警告
                            cv::putText(debugImg, "G1!", cv::Point(x1 + 8, y1 - 8),
                                       cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 128, 255), 1);
                        } else if (dist2 < MAX_SINGLE_MOVEMENT * 1.5f) {
                            x2 = singleGlint.x; y2 = singleGlint.y;
                            m_lastGlint2 = GlintAsCircle(singleGlint, 3.0f, 0.7f);
                            m_lastGlint2.valid = true;
                            cv::circle(debugImg, cv::Point(x2, y2), 5, cv::Scalar(0, 128, 255), 2);
                            cv::putText(debugImg, "G2!", cv::Point(x2 + 8, y2 - 8),
                                       cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 128, 255), 1);
                        }
                    }
                } else {
                    // 首次检测单个亮斑：按正确的ID规则分配
                    // 规则：单亮斑在瞳孔左边是G2，在瞳孔右边是G1
                    if (singleGlint.x <= pupilCenter.x) {
                        // 瞳孔左边 → G2（第二个）
                        x2 = singleGlint.x; y2 = singleGlint.y;
                        m_lastGlint2 = GlintAsCircle(singleGlint, 3.0f, 0.9f);
                        m_lastGlint2.valid = true;
                        cv::circle(debugImg, cv::Point(x2, y2), 5, cv::Scalar(0, 0, 255), 2);
                        cv::putText(debugImg, "G2", cv::Point(x2 + 8, y2 - 8),
                                   cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 255), 1);
//                        qDebug() << "[GlintTrack] ✓ 单亮斑ID正确：瞳孔左边 → G2";
                    } else {
                        // 瞳孔右边 → G1（第一个）
                        x1 = singleGlint.x; y1 = singleGlint.y;
                        m_lastGlint1 = GlintAsCircle(singleGlint, 3.0f, 0.9f);
                        m_lastGlint1.valid = true;
                        cv::circle(debugImg, cv::Point(x1, y1), 5, cv::Scalar(0, 255, 0), 2);
                        cv::putText(debugImg, "G1", cv::Point(x1 + 8, y1 - 8),
                                   cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 1);
//                        qDebug() << "[GlintTrack] ✓ 单亮斑ID正确：瞳孔右边 → G1";
                    }
                }
            }
        }
    } else {
        // 检测失败：渐进式清除历史状态（避免突然重置）
        static int consecutiveFailures = 0;
        consecutiveFailures++;

        if (consecutiveFailures > 5) {  // 连续5帧失败才清除状态
            m_lastGlint1.valid = m_lastGlint2.valid = false;
            consecutiveFailures = 0;
//            qDebug() << "[GlintTrack] 连续检测失败，清除历史状态";
        } else {
//            qDebug() << "[GlintTrack] 检测失败 (" << consecutiveFailures << "/5)";
        }
    }

    // 增强状态信息显示
    std::string status = foundAnyGlint ? "Tracking" : "None";
    std::string trackingInfo = std::string("G1:") + (m_lastGlint1.valid ? "OK" : "--") +
                              " G2:" + (m_lastGlint2.valid ? "OK" : "--");

    cv::putText(debugImg, "Status: " + status, cv::Point(5, 15),
               cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);
    cv::putText(debugImg, trackingInfo, cv::Point(5, 35),
               cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 0), 1);

    return foundAnyGlint;
}

// 混合跟踪+检测主函数
bool MainWindow::hybridGlintDetectionAndTracking(const cv::Mat &eyeImage, const cv::Rect &coarseROI,
                                                const cv::Point2f &pupilCenter, float pupilRadius,
                                                float &x1, float &y1, float &x2, float &y2, cv::Mat &debugImg)
{
    const int MAX_LOST_FRAMES = 3;
    bool foundAnyGlint = false;

    // 初始化输出变量
    x1 = x2 = y1 = y2 = 0;

    // 🎯 策略1: 优先使用跟踪（只有当跟踪器已激活且有有效的上一帧数据时）
    if (m_glint1TrackingActive && m_lastGlint1.valid) {
        GlintAsCircle trackedGlint1;
        cv::Rect searchROI1 = cv::Rect(m_lastGlint1.center.x - 20, m_lastGlint1.center.y - 20, 40, 40);
        searchROI1 &= cv::Rect(0, 0, eyeImage.cols, eyeImage.rows);

        if (trackGlintUsingPupilTracker(eyeImage, m_lastGlint1, trackedGlint1, searchROI1)) {
            // 跟踪成功
            x1 = trackedGlint1.center.x;
            y1 = trackedGlint1.center.y;
            m_lastGlint1 = trackedGlint1;
            m_glintTrackingLostFrames1 = 0;
            foundAnyGlint = true;

//            qDebug() << "[GlintTrack] Glint1跟踪成功: (" << x1 << "," << y1 << ")";
        } else {
            // 跟踪失败
            m_glintTrackingLostFrames1++;
//            qDebug() << "[GlintTrack] Glint1跟踪失败, 丢失帧数:" << m_glintTrackingLostFrames1;
            if (m_glintTrackingLostFrames1 >= MAX_LOST_FRAMES) {
                m_glint1TrackingActive = false;
//                qDebug() << "[GlintTrack] Glint1跟踪丢失，停止跟踪";
            }
        }
    }

    if (m_glint2TrackingActive && m_lastGlint2.valid) {
        GlintAsCircle trackedGlint2;
        cv::Rect searchROI2 = cv::Rect(m_lastGlint2.center.x - 20, m_lastGlint2.center.y - 20, 40, 40);
        searchROI2 &= cv::Rect(0, 0, eyeImage.cols, eyeImage.rows);

        if (trackGlintUsingPupilTracker(eyeImage, m_lastGlint2, trackedGlint2, searchROI2)) {
            // 跟踪成功
            x2 = trackedGlint2.center.x;
            y2 = trackedGlint2.center.y;
            m_lastGlint2 = trackedGlint2;
            m_glintTrackingLostFrames2 = 0;
            foundAnyGlint = true;

//            qDebug() << "[GlintTrack] Glint2跟踪成功: (" << x2 << "," << y2 << ")";
        } else {
            // 跟踪失败
            m_glintTrackingLostFrames2++;
//            qDebug() << "[GlintTrack] Glint2跟踪失败, 丢失帧数:" << m_glintTrackingLostFrames2;
            if (m_glintTrackingLostFrames2 >= MAX_LOST_FRAMES) {
                m_glint2TrackingActive = false;
//                qDebug() << "[GlintTrack] Glint2跟踪丢失，停止跟踪";
            }
        }
    }

    // 🎯 策略2: 如果跟踪失败或未初始化，使用简化检测
    if (!m_glint1TrackingActive || !m_glint2TrackingActive) {
        std::vector<cv::Point2f> detectedGlints;
//        qDebug() << "[GlintTrack] 开始检测, 跟踪状态 - Glint1:" << m_glint1TrackingActive << ", Glint2:" << m_glint2TrackingActive;

        if (detectGlintsSimplified(eyeImage, coarseROI, pupilCenter, pupilRadius, detectedGlints)) {

            // 根据检测结果初始化或重新初始化跟踪器
            if (!detectedGlints.empty()) {
                foundAnyGlint = true;
//                qDebug() << "[GlintTrack] 检测到" << detectedGlints.size() << "个亮斑";

                if (detectedGlints.size() >= 2) {
                    // 检测到两个亮斑，按X坐标排序
                    if (detectedGlints[0].x > detectedGlints[1].x) {
                        std::swap(detectedGlints[0], detectedGlints[1]);
                    }

                    if (!m_glint1TrackingActive) {
                        x1 = detectedGlints[0].x;
                        y1 = detectedGlints[0].y;
                        m_lastGlint1 = GlintAsCircle(detectedGlints[0], 3.0f, 0.8f);
                        m_lastGlint1.valid = true;  // 确保设置为有效
                        m_glint1TrackingActive = true;
                        m_glintTrackingLostFrames1 = 0;
//                        qDebug() << "[GlintTrack] Glint1跟踪初始化: (" << x1 << "," << y1 << ")";
                    }

                    if (!m_glint2TrackingActive) {
                        x2 = detectedGlints[1].x;
                        y2 = detectedGlints[1].y;
                        m_lastGlint2 = GlintAsCircle(detectedGlints[1], 3.0f, 0.8f);
                        m_lastGlint2.valid = true;  // 确保设置为有效
                        m_glint2TrackingActive = true;
                        m_glintTrackingLostFrames2 = 0;
//                        qDebug() << "[GlintTrack] Glint2跟踪初始化: (" << x2 << "," << y2 << ")";
                    }
                } else {
                    // 只检测到一个亮斑
                    cv::Point2f singleGlint = detectedGlints[0];

                    // 根据位置决定是glint1还是glint2
                    if (singleGlint.x <= pupilCenter.x) {
                        // 左边亮斑
                        if (!m_glint1TrackingActive) {
                            x1 = singleGlint.x;
                            y1 = singleGlint.y;
                            m_lastGlint1 = GlintAsCircle(singleGlint, 3.0f, 0.8f);
                            m_lastGlint1.valid = true;  // 确保设置为有效
                            m_glint1TrackingActive = true;
                            m_glintTrackingLostFrames1 = 0;
//                            qDebug() << "[GlintTrack] 单个Glint1跟踪初始化: (" << x1 << "," << y1 << ")";
                        }
                    } else {
                        // 右边亮斑
                        if (!m_glint2TrackingActive) {
                            x2 = singleGlint.x;
                            y2 = singleGlint.y;
                            m_lastGlint2 = GlintAsCircle(singleGlint, 3.0f, 0.8f);
                            m_lastGlint2.valid = true;  // 确保设置为有效
                            m_glint2TrackingActive = true;
                            m_glintTrackingLostFrames2 = 0;
//                            qDebug() << "[GlintTrack] 单个Glint2跟踪初始化: (" << x2 << "," << y2 << ")";
                        }
                    }
                }
            } else {
//                qDebug() << "[GlintTrack] 检测失败，未找到亮斑";
            }
        } else {
//            qDebug() << "[GlintTrack] detectGlintsSimplified返回失败";
        }
    }

    // 创建调试图像
    if (eyeImage.channels() == 1) {
        cv::cvtColor(eyeImage, debugImg, cv::COLOR_GRAY2BGR);
    } else {
        debugImg = eyeImage.clone();
    }

    // 绘制跟踪状态
    if (m_glint1TrackingActive) {
        cv::circle(debugImg, cv::Point(x1, y1), 5, cv::Scalar(0, 255, 0), 2);  // 绿色：跟踪中
        cv::putText(debugImg, "G1-Track", cv::Point(x1 + 8, y1 - 8),
                   cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 1);
    }

    if (m_glint2TrackingActive) {
        cv::circle(debugImg, cv::Point(x2, y2), 5, cv::Scalar(0, 255, 0), 2);  // 绿色：跟踪中
        cv::putText(debugImg, "G2-Track", cv::Point(x2 + 8, y2 - 8),
                   cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 1);
    }

    // 状态信息
    std::string status = "Track: ";
    status += m_glint1TrackingActive ? "G1+" : "G1-";
    status += m_glint2TrackingActive ? "G2+" : "G2-";
    cv::putText(debugImg, status, cv::Point(5, 15),
               cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);

    return foundAnyGlint;
}

// 使用PupilTrackingMethod跟踪单个亮斑
bool MainWindow::trackGlintUsingPupilTracker(const cv::Mat &eyeROI, const GlintAsCircle &lastGlint,
                                           GlintAsCircle &currentGlint, const cv::Rect &searchROI)
{
    if (!glintTrackingMethod || !lastGlint.valid) {
//        qDebug() << "[GlintTrack] 跟踪条件检查失败 - Method:" << (glintTrackingMethod != nullptr) << ", Valid:" << lastGlint.valid;
        return false;
    }

    try {
        // 将亮斑转换为Pupil对象
        Pupil lastPupilAsGlint = lastGlint.toPupil();
        Pupil currentPupilAsGlint;

        // 使用PupilTrackingMethod进行跟踪
        // 注意：PuReST需要灰度图像
        cv::Mat grayROI;
        if (eyeROI.channels() > 1) {
            cv::cvtColor(eyeROI, grayROI, cv::COLOR_BGR2GRAY);
        } else {
            grayROI = eyeROI;
        }

//        qDebug() << "[GlintTrack] 调用PuReST跟踪 - 输入位置:(" << lastPupilAsGlint.center.x << "," << lastPupilAsGlint.center.y
//                 << "), ROI:(" << searchROI.x << "," << searchROI.y << "," << searchROI.width << "," << searchROI.height << ")";

        // 调用跟踪算法（最小直径3px，最大直径15px，适合亮斑）
        glintTrackingMethod->run(grayROI, searchROI, lastPupilAsGlint, currentPupilAsGlint, 3.0f, 15.0f);

//        qDebug() << "[GlintTrack] PuReST返回结果 - Valid:" << currentPupilAsGlint.valid() << ", Confidence:" << currentPupilAsGlint.confidence;

        // 验证跟踪结果
        if (currentPupilAsGlint.valid() && currentPupilAsGlint.confidence > 0.3f) {
            // 转换回亮斑对象
            currentGlint = GlintAsCircle::fromPupil(currentPupilAsGlint);

//            qDebug() << "[GlintTrack] 跟踪结果位置:(" << currentGlint.center.x << "," << currentGlint.center.y << ")";

            // 额外验证：检查位置变化是否合理（亮斑移动不应该太快）
            float distance = cv::norm(currentGlint.center - lastGlint.center);
            if (distance < 30.0f) {  // 放宽移动距离限制到30像素
//                qDebug() << "[GlintTrack] 跟踪成功，移动距离:" << distance << "px";
                return true;
            } else {
//                qDebug() << "[GlintTrack] 跟踪结果位置变化过大: " << distance << "px";
            }
        } else {
//            qDebug() << "[GlintTrack] PuReST跟踪失败 - 结果无效或置信度过低";
        }

    } catch (const std::exception& /*e*/) {
//        qDebug() << "[GlintTrack] 跟踪异常: " << e.what();
    }

    return false;
}

// 简化的亮斑检测（只在跟踪失败时使用）
bool MainWindow::detectGlintsSimplified(const cv::Mat &eyeImage, const cv::Rect &coarseROI,
                                       const cv::Point2f &/*pupilCenter*/, float /*pupilRadius*/,
                                       std::vector<cv::Point2f> &glintCenters)
{
    glintCenters.clear();

    // qDebug() << "[GlintTrack] 开始简化检测";

    // 检查ROI有效性
    if (coarseROI.width <= 0 || coarseROI.height <= 0 ||
        coarseROI.x < 0 || coarseROI.y < 0 ||
        coarseROI.x + coarseROI.width > eyeImage.cols ||
        coarseROI.y + coarseROI.height > eyeImage.rows) {
//        qDebug() << "[GlintTrack] ROI无效，图像尺寸:(" << eyeImage.cols << "," << eyeImage.rows << ")";
        return false;
    }

    cv::Mat roi = eyeImage(coarseROI);
    cv::Mat gray;
    if (roi.channels() > 1) {
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = roi;
    }

    // 使用适中阈值进行亮斑检测
    cv::Mat binary;
    cv::threshold(gray, binary, 160, 255, cv::THRESH_BINARY);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // qDebug() << "[GlintTrack] 找到" << contours.size() << "个轮廓";

    // 恢复重要约束：面积、圆形度、亮度检验
    const float MIN_AREA = 80.0f;
    const float MAX_AREA = 1200.0f;
    const float MIN_CIRCULARITY = 0.6f;  // 圆形度约束

    struct GlintCandidate {
        float brightness;
        float area;
        float circularity;
        cv::Point2f center;
    };

    std::vector<GlintCandidate> candidates;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area >= MIN_AREA && area <= MAX_AREA) {
            // 计算圆形度 = (4 * π * 面积) / (周长²)
            double perimeter = cv::arcLength(contour, true);
            float circularity = 0.0f;
            if (perimeter > 0) {
                circularity = (4.0f * M_PI * area) / (perimeter * perimeter);
            }

            // 圆形度检验：只有圆形度达标的才是真正的亮斑
            if (circularity >= MIN_CIRCULARITY) {
                cv::Moments m = cv::moments(contour);
                if (m.m00 > 0) {
                    cv::Point2f localCenter(m.m10/m.m00, m.m01/m.m00);
                    cv::Point2f globalCenter(localCenter.x + coarseROI.x, localCenter.y + coarseROI.y);

                    // 检查中心点是否在ROI内且获取亮度
                    if (localCenter.x >= 0 && localCenter.x < gray.cols &&
                        localCenter.y >= 0 && localCenter.y < gray.rows) {
                        uchar brightness = gray.at<uchar>((int)localCenter.y, (int)localCenter.x);
                        candidates.push_back({(float)brightness, (float)area, circularity, globalCenter});
                    }
                }
            }
        }
    }

    // 恢复智能筛选：距离约束和位置验证
    const float MIN_DISTANCE = 20.0f;
    const float MAX_DISTANCE = 100.0f;
    const float MAX_Y_DIFF = 10.0f;  // 允许的Y轴差异

    std::vector<cv::Point2f> finalCenters;

    if (candidates.size() >= 2) {
        // 按综合评分排序：面积 * 圆形度 * 亮度
        std::sort(candidates.begin(), candidates.end(),
                 [](const GlintCandidate& a, const GlintCandidate& b) {
                     float scoreA = a.area * a.circularity * (a.brightness / 255.0f);
                     float scoreB = b.area * b.circularity * (b.brightness / 255.0f);
                     return scoreA > scoreB;
                 });

        // 寻找符合距离约束的最佳对
        bool foundPair = false;
        for (size_t i = 0; i < candidates.size() && !foundPair; ++i) {
            for (size_t j = i + 1; j < candidates.size() && !foundPair; ++j) {
                cv::Point2f p1 = candidates[i].center;
                cv::Point2f p2 = candidates[j].center;
                float distance = cv::norm(p1 - p2);
                float y_diff = abs(p1.y - p2.y);

                // 检查距离和Y轴差异约束
                if (distance >= MIN_DISTANCE && distance <= MAX_DISTANCE && y_diff <= MAX_Y_DIFF) {
                    // 按X坐标排序确保左右一致性
                    if (p1.x < p2.x) {
                        finalCenters.push_back(p1);
                        finalCenters.push_back(p2);
                    } else {
                        finalCenters.push_back(p2);
                        finalCenters.push_back(p1);
                    }
                    foundPair = true;
                }
            }
        }

        // 如果没找到配对，选择最佳单个点
        if (!foundPair && !candidates.empty()) {
            finalCenters.push_back(candidates[0].center);
        }
    } else if (candidates.size() == 1) {
        finalCenters.push_back(candidates[0].center);
    }

    glintCenters = finalCenters;

    // qDebug() << "[GlintTrack] 简化检测最终找到" << glintCenters.size() << "个亮斑";

    return !glintCenters.empty();
}

void MainWindow::loadLatestCalibration()
{
    if (!m_calibrator) return;

    // 查找最新的标定文件
    QDir currentDir(".");
    QStringList filters;
    filters << "pupil_corneal_calibration_*.json";
    QStringList calibFiles = currentDir.entryList(filters, QDir::Files, QDir::Time);

    if (!calibFiles.isEmpty()) {
        QString latestFile = calibFiles.first(); // 按时间排序，第一个是最新的
        if (m_calibrator->loadCalibration(latestFile)) {
//            qDebug() << "✅ 成功加载瞳孔-角膜标定文件:" << latestFile;
//            qDebug() << "📊 标定质量 - 平均误差:" << m_calibrator->getMeanError()
//                    << "像素, 最大误差:" << m_calibrator->getMaxError() << "像素";
        } else {
//            qDebug() << "❌ 加载瞳孔-角膜标定文件失败:" << latestFile;
        }
    } else {
//        qDebug() << "📁 未找到瞳孔-角膜标定文件，需要重新标定";
    }
}

void MainWindow::saveCurrentCalibration()
{
    if (!m_calibrator || !m_calibrator->validateCalibration()) {
//        qDebug() << "❌ 无有效标定数据可保存";
        return;
    }

    QString filename = QString("./pupil_corneal_calibration_%1.json")
                      .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    if (m_calibrator->saveCalibration(filename)) {
//        qDebug() << "💾 瞳孔-角膜标定已保存到:" << filename;
    } else {
//        qDebug() << "❌ 标定保存失败:" << filename;
    }
}

// T模式相关函数实现
// mainwindow.cpp

void MainWindow::setFreeGazeMode(bool enabled)
{
    m_freeGazeMode = enabled;
    if (enabled) {
//        qDebug() << "T模式开启：自由注视模式 + EyeTrackingDialog交互";
        // --- 修正：使用 m_gazeTrailQt ---
        m_gazeTrailQt.clear();
        m_gazeTrailTimer->start();

        // 显示通用视线覆盖层（可以在任何窗口上面显示）
        if (m_gazeOverlay) {
            m_gazeOverlay->setGeometry(this->geometry());
            m_gazeOverlay->show();
        }

        // 【修复】同步CalibError的T模式状态
        if (m_calibError) {
            qDebug() << "MainWindow T模式开启，同步启用CalibError T模式";
            m_calibError->setFreeGazeMode(true);
        }

        // 自动显示EyeTrackingDialog进行交互
//        if (m_dialog) {
//            m_dialog->showFullScreen();
//            m_dialog->show();
//            qDebug() << "EyeTrackingDialog已自动启动，通用视线系统已启用";
//        }
    } else {
//        qDebug() << "T模式关闭";

        // 关闭EyeTrackingDialog
        if (m_dialog) {
            m_dialog->hide();
        }
        m_gazeTrailTimer->stop();
        // --- 修正：使用 m_gazeTrailQt ---
        m_gazeTrailQt.clear();

        if (m_gazeOverlay) {
            m_gazeOverlay->hide();
        }

        // 【修复】同步CalibError的T模式状态
        if (m_calibError) {
            qDebug() << "MainWindow T模式关闭，同步关闭CalibError T模式";
            m_calibError->setFreeGazeMode(false);
        }
    }
}



// mainwindow.cpp

void MainWindow::updateFreeGaze(const cv::Point2f& gazePoint)
{
    if (!m_freeGazeMode) return;

    m_currentFreeGaze = gazePoint;
    QPointF qGazePoint(gazePoint.x, gazePoint.y);

    // --- 修正：使用 m_gazeTrailQt ---
    m_gazeTrailQt.enqueue(qGazePoint);
    m_lastGazeTime = QDateTime::currentMSecsSinceEpoch();
    const int FREE_GAZE_TRAIL_MAX_POINTS = 50;
    while (m_gazeTrailQt.size() > FREE_GAZE_TRAIL_MAX_POINTS) {
        m_gazeTrailQt.dequeue();
    }

    // 【修复】同时更新CalibError的T模式（如果存在且处于T模式）
    if (m_calibError && m_calibError->isFreeGazeMode()) {
        qDebug() << "MainWindow传递fake3D坐标给CalibError T模式:" << gazePoint.x << "," << gazePoint.y
                 << "屏幕大小:" << m_screenWidth << "x" << m_screenHeight
                 << "CalibError窗口大小:" << m_calibError->width() << "x" << m_calibError->height();
        m_calibError->updateFreeGaze(gazePoint);
    }

    // 将更新后的数据传递给覆盖窗口
    if (m_gazeOverlay && m_gazeOverlay->isVisible()) {
        // --- 修正：传递 m_gazeTrailQt ---
        m_gazeOverlay->updateGazeData(qGazePoint, m_gazeTrailQt);
    }
}



// mainwindow.cpp

void MainWindow::addGazeTrailPoint(const cv::Point2f& point)
{
    qDebug() << "[addGazeTrailPoint] 被调用 - 坐标:" << point.x << "," << point.y;
    QPointF qPoint(point.x, point.y);

    // --- 修正：使用 m_gazeTrailQt ---
    m_gazeTrailQt.enqueue(qPoint);
    m_lastGazeTime = QDateTime::currentMSecsSinceEpoch();

    const int FREE_GAZE_TRAIL_MAX_POINTS = 50;
    while (m_gazeTrailQt.size() > FREE_GAZE_TRAIL_MAX_POINTS) {
        m_gazeTrailQt.dequeue();
    }
}


// mainwindow.cpp

void MainWindow::updateGazeTrail()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    const int FREE_GAZE_TRAIL_DURATION = 2000; // 2秒

    if (currentTime - m_lastGazeTime > FREE_GAZE_TRAIL_DURATION) {
        // --- 修正：使用 m_gazeTrailQt ---
        if (!m_gazeTrailQt.isEmpty()) {
            m_gazeTrailQt.clear();
            // 清空后，也需要通知覆盖窗口更新，否则旧轨迹会残留
            if (m_gazeOverlay && m_gazeOverlay->isVisible()) {
                m_gazeOverlay->updateGazeData(QPointF(-1,-1), m_gazeTrailQt);
            }
        }
    }
    // 不再需要 this->update()
}


void MainWindow::drawFreeGazeTrail(QPainter& painter)
{
    if (m_gazeTrailQt.isEmpty()) return;

    painter.setRenderHint(QPainter::Antialiasing);

    // 将队列转换为列表以便遍历
    // 将队列转换为列表以便遍历
    QList<cv::Point2f> trailList;
    for (const auto& point : m_gazeTrailQt) {
        // --- 修正部分 ---
        // 在添加之前，手动创建一个 cv::Point2f 对象
        trailList.append(cv::Point2f(point.x(), point.y()));
    }

    // 绘制轨迹线
    if (trailList.size() > 1) {
        QPen trailPen;
        trailPen.setWidth(2);  // 与EyeTrackingDialog一致
        trailPen.setCapStyle(Qt::RoundCap);
        trailPen.setJoinStyle(Qt::RoundJoin);

        for (int i = 1; i < trailList.size(); ++i) {
            const cv::Point2f& prev = trailList[i-1];
            const cv::Point2f& curr = trailList[i];

            // 计算透明度，越新的线段越不透明
            qreal opacity = static_cast<qreal>(i) / trailList.size();
            QColor lineColor(100, 200, 255, static_cast<int>(opacity * 255));  // 与EyeTrackingDialog一致
            trailPen.setColor(lineColor);
            painter.setPen(trailPen);

            painter.drawLine(QPointF(prev.x, prev.y), QPointF(curr.x, curr.y));
        }
    }

    // 绘制轨迹点（小圆点）
    for (int i = 0; i < trailList.size(); ++i) {
        const cv::Point2f& point = trailList[i];
        qreal opacity = static_cast<qreal>(i + 1) / trailList.size();

        QColor pointColor(150, 220, 255, static_cast<int>(opacity * 200));  // 与EyeTrackingDialog一致
        painter.setBrush(QBrush(pointColor));
        painter.setPen(Qt::NoPen);

        int size = static_cast<int>(3 + opacity * 2);  // 与EyeTrackingDialog一致
        QPointF qPoint(point.x, point.y);
        painter.drawEllipse(qPoint, size, size);
    }

    // 绘制当前注视点（与EyeTrackingDialog完全一致）
    if (m_currentFreeGaze.x >= 0 && m_currentFreeGaze.y >= 0) {
        const int GAZE_INDICATOR_SIZE = 25;  // 与EyeTrackingDialog一致
        QPointF gazePoint(m_currentFreeGaze.x, m_currentFreeGaze.y);

        // 外圈 - 红色边框
        QColor outerColor = QColor(255, 50, 50, 200);
        QPen pen(outerColor, 3);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(gazePoint, GAZE_INDICATOR_SIZE, GAZE_INDICATOR_SIZE);

        // 内圈填充
        QColor innerColor = QColor(255, 100, 100, 80);
        QBrush brush(innerColor);
        painter.setBrush(brush);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(gazePoint, GAZE_INDICATOR_SIZE - 8, GAZE_INDICATOR_SIZE - 8);

        // 中心点
        QColor centerColor = QColor(255, 0, 0, 255);
        painter.setBrush(QBrush(centerColor));
        painter.drawEllipse(gazePoint, 3, 3);
    }
}

// ========== 3D注视计算实现 ==========
// 完全独立的实现，不影响现有功能

MainWindow::Gaze3DCalculator::Gaze3DCalculator() {
    // m_rayHistory is now part of MainWindow class, not this nested class
}

cv::Point2f MainWindow::calculateGaze3D(const cv::Point2f& pupilCenter) {
    // 【关键修改】优先检查是否启用了真3D模式，完全禁用假3D
    bool workerExists = (worker != nullptr);
    bool real3DEnabled = workerExists ? worker->isReal3DModeEnabled() : false;
//    qDebug() << QString("🎯 [MainWindow] calculateGaze3D调用 - Worker存在: %1, 真3D启用: %2")
//                .arg(workerExists ? "是" : "否")
//                .arg(real3DEnabled ? "是" : "否");

    if (worker && worker->isReal3DModeEnabled()) {
        if (worker->isReal3DModelBuilt()) {
//            qDebug() << "[真3D] 使用Worker的真3D系统计算注视点";

            // 从最新的处理结果获取瞳孔数据
            QMutexLocker locker(&m_resultMutex);
            if (m_lastProcessingResult.pupilFound && m_lastProcessingResult.pupilData.confidence > 0.3) {
                // 直接使用PupilData对象传递给Worker
                const PupilData& pupilData = m_lastProcessingResult.pupilData;

                // 使用eyeImage调用Worker的真3D计算
                cv::Point2f result = worker->computeReal3DGaze(eyeImage, pupilData);

//                qDebug() << QString("[真3D] 计算结果: (%.1f, %.1f)").arg(result.x).arg(result.y);
                return result;
            } else {
//                qDebug() << "[真3D] 瞳孔数据无效，返回原始位置";
                return pupilCenter;
            }
        } else {
//            qDebug() << QString("[真3D] 模型构建中... 进度: %1/%2")
//                        .arg(worker->get3DModelProgress()).arg(30);
            // 【关键修改】真3D模式下，即使模型未完成也不使用假3D，避免冲突
            return pupilCenter;
        }
    } else {
//        qDebug() << "[虚假3D] 真3D未启用，使用虚假3D系统";

        // 【完全重写】基于temp05 Python代码的假3D算法
//        qDebug() << "[假3D] 使用temp05风格的假3D算法";

        try {
            // 1. 存储瞳孔椭圆用于射线计算（模拟Python的ray_lines）
            QMutexLocker locker(&m_resultMutex);
            if (m_lastProcessingResult.pupilFound && m_lastProcessingResult.pupilData.confidence > 0.3) {
                const PupilData& pupilData = m_lastProcessingResult.pupilData;

                // 2. 更新椭圆历史（模拟Python的ray_lines.append(final_rotated_rect)）
                updateEllipseHistory(pupilData);

                // 3. 【关键修改】假3D模式优先使用真3D的眼球球心
                cv::Point2f sphereCenter;
                bool useReal3DCenter = false;

                // 【关键修改】尝试获取真3D眼球球心，不检查真3D模式是否启用
                if (worker && worker->isReal3DModelBuilt()) {
                    cv::Point2f real3DCenter = worker->getReal3DEyeCenter();
                    if (real3DCenter.x > 0 && real3DCenter.y > 0) {
                        sphereCenter = real3DCenter;
                        useReal3DCenter = true;
//                        qDebug() << "[假3D] ✅ 使用真3D眼球球心:" << sphereCenter.x << "," << sphereCenter.y;
                    } else {
//                        qDebug() << "[假3D] ⚠️ 真3D球心无效:" << real3DCenter.x << "," << real3DCenter.y;
                    }
                } else {
//                    qDebug() << "[假3D] ⚠️ Worker不存在或3D模型未构建";
                }

                // 如果无法获取真3D球心，回退到假3D计算
                if (!useReal3DCenter) {
                    sphereCenter = computeAverageIntersection();
//                    qDebug() << "[假3D] 使用假3D计算的球心:" << sphereCenter.x << "," << sphereCenter.y;
                }

                // 4. 更新眼球中心平均值（模拟Python的update_and_average_point）
                if (sphereCenter.x != 0 || sphereCenter.y != 0) {
                    // 只有计算出有效的眼球中心才更新
                    m_currentEyeSphereCenter = updateAndAveragePoint(sphereCenter);
                } else {
                    // 如果没有有效的射线交点，使用瞳孔中心作为临时的眼球中心
                    if (m_currentEyeSphereCenter.x == 320 && m_currentEyeSphereCenter.y == 240) {
                        // 首次运行，使用瞳孔中心初始化
                        m_currentEyeSphereCenter = pupilCenter;
//                        qDebug() << "[假3D] 使用瞳孔中心初始化眼球中心:" << pupilCenter.x << "," << pupilCenter.y;
                    }
                }

                // 5. 计算3D注视向量（模拟Python的compute_gaze_vector）
                auto gaze3D = computeGazeVector(pupilCenter, m_currentEyeSphereCenter, eyeImage.cols, eyeImage.rows);

            if (gaze3D.valid) {
                // 【修正】使用完整的3D方向向量，通过mapGaze3DToScreen转换为屏幕坐标
                cv::Point2f screenGaze = mapGaze3DToScreen(gaze3D.direction, m_screenWidth, m_screenHeight);

                // 应用3D区域校正（如果已启用）
                if (m_3DRegionalCorrectionEnabled && !m_3DCalibrationOffsets.empty()) {
                    screenGaze = apply3DRegionalCorrection(screenGaze);
                }

                // 输出调试信息
//                qDebug() << QString("[假3D] 瞳孔:(%1,%2) 眼球中心:(%3,%4) 方向:(%5,%6,%7) -> 注视:(%8,%9)")
//                            .arg(pupilCenter.x).arg(pupilCenter.y)
//                            .arg(m_currentEyeSphereCenter.x).arg(m_currentEyeSphereCenter.y)
//                            .arg(gaze3D.direction.x).arg(gaze3D.direction.y).arg(gaze3D.direction.z)
//                            .arg(screenGaze.x).arg(screenGaze.y);

                return screenGaze;
            }
            } // 关闭 if (m_lastProcessingResult.pupilFound...) 块
        } catch (const std::exception& e) {
//            qDebug() << "[虚假3D] 计算异常:" << e.what();
        } catch (...) {
//            qDebug() << "[虚假3D] 未知计算异常";
        }
    }

    return pupilCenter; // 如果所有方法都失败，返回原始瞳孔中心
}

// ==================== 【新增】假3D算法辅助函数实现（基于temp05 Python） ====================

void MainWindow::updateEllipseHistory(const PupilData& pupilData) {
    // 模拟Python中ray_lines.append(final_rotated_rect)
    // 将瞳孔椭圆数据转换为RotatedRect并添加到射线历史中
    cv::RotatedRect ellipse;
    ellipse.center = pupilData.center;
    ellipse.size = cv::Size2f(pupilData.size.width, pupilData.size.height);  // PupilData继承自RotatedRect
    ellipse.angle = pupilData.angle;

    m_rayHistory.push_back(ellipse);

    // 限制历史椭圆数量，保留最近的椭圆（与Python版本一致）
    const size_t MAX_RAY_HISTORY_LOCAL = 100;  // Python中没有明确限制，这里设置合理值
    if (m_rayHistory.size() > MAX_RAY_HISTORY_LOCAL) {
        m_rayHistory.erase(m_rayHistory.begin());
    }

//    qDebug() << QString("[假3D] 椭圆历史更新：当前数量 %1").arg(m_rayHistory.size());
}

cv::Point2f MainWindow::computeAverageIntersection() {
    // 模拟Python的compute_average_intersection函数
    if (m_rayHistory.size() < 2) {
//        qDebug() << "[假3D] 射线历史不足，无法计算交点";
        return cv::Point2f(0, 0);
    }

    const int N = 8;  // 随机选择的射线数量（与Python版本一致）
    std::vector<cv::RotatedRect> selectedRays;

    // 随机选择N个射线（模拟Python的random.sample）
    // 使用简单的随机算法代替std::shuffle以避免编译器兼容性问题
    if (m_rayHistory.size() > 1) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        for (size_t i = m_rayHistory.size() - 1; i > 0; --i) {
            size_t j = std::rand() % (i + 1);
            std::swap(m_rayHistory[i], m_rayHistory[j]);
        }
    }

    int actualN = (N < static_cast<int>(m_rayHistory.size())) ? N : static_cast<int>(m_rayHistory.size());
    selectedRays.assign(m_rayHistory.begin(), m_rayHistory.begin() + actualN);

    std::vector<cv::Point2f> intersections;

    // 计算射线对的交点（模拟Python的find_line_intersection）
    for (int i = 0; i < selectedRays.size() - 1; i++) {
        const cv::RotatedRect& ray1 = selectedRays[i];
        const cv::RotatedRect& ray2 = selectedRays[i + 1];

        // 检查角度差异（模拟Python的角度检查）
        float angleDiff = std::abs(ray1.angle - ray2.angle);
        if (angleDiff >= 2.0f) {  // 至少2度差异
            cv::Point2f intersection = findLineIntersection(ray1, ray2);

            // 检查交点是否在图像范围内（模拟Python的边界检查）
            if (intersection.x >= 0 && intersection.x < 640 &&
                intersection.y >= 0 && intersection.y < 480) {
                intersections.push_back(intersection);
                m_storedIntersections.push_back(intersection);
            }
        }
    }

    // 修剪存储的交点（模拟Python的prune_intersections）
    if (m_storedIntersections.size() > MAX_INTERSECTIONS) {
        m_storedIntersections.erase(m_storedIntersections.begin(),
            m_storedIntersections.end() - MAX_INTERSECTIONS);
    }

    if (m_storedIntersections.empty()) {
//        qDebug() << "[假3D] 没有有效交点";
        return cv::Point2f(0, 0);
    }

    // 计算平均交点（模拟Python的np.mean）
    float avgX = 0, avgY = 0;
    for (const auto& pt : m_storedIntersections) {
        avgX += pt.x;
        avgY += pt.y;
    }
    avgX /= m_storedIntersections.size();
    avgY /= m_storedIntersections.size();

//    qDebug() << QString("[假3D] 计算得到平均交点：(%.1f, %.1f)，基于%2个交点")
//                .arg(avgX).arg(avgY).arg(m_storedIntersections.size());

    return cv::Point2f(avgX, avgY);
}

cv::Point2f MainWindow::updateAndAveragePoint(const cv::Point2f& newPoint) {
    // 模拟Python的update_and_average_point函数
    m_eyeCenterHistory.push_back(newPoint);

    // 保持历史点数量限制（模拟Python的N参数）
    if (m_eyeCenterHistory.size() > MAX_CENTER_HISTORY) {
        m_eyeCenterHistory.erase(m_eyeCenterHistory.begin());
    }

    if (m_eyeCenterHistory.empty()) {
        return cv::Point2f(0, 0);
    }

    // 计算平均点（模拟Python的np.mean）
    float avgX = 0, avgY = 0;
    for (const auto& pt : m_eyeCenterHistory) {
        avgX += pt.x;
        avgY += pt.y;
    }
    avgX /= m_eyeCenterHistory.size();
    avgY /= m_eyeCenterHistory.size();

//    qDebug() << QString("[假3D] 眼球中心更新：(%.1f, %.1f)，基于%2个历史点")
//                .arg(avgX).arg(avgY).arg(m_eyeCenterHistory.size());

    return cv::Point2f(avgX, avgY);
}

MainWindow::GazeVector3D MainWindow::computeGazeVector(const cv::Point2f& pupilCenter,
                                                     const cv::Point2f& sphereCenter,
                                                     int screenWidth, int screenHeight) {
    // 【完全按照temp05 Python代码实现】模拟compute_gaze_vector函数
    GazeVector3D result;
    result.valid = false;

    try {
        // 相机和投影设置（与Python版本完全一致）
        const float fov_y_deg = 45.0f;
        const float aspect_ratio = static_cast<float>(screenWidth) / screenHeight;
        const float far_clip = 100.0f;
        const cv::Point3f camera_position(0.0f, 0.0f, 3.0f);

        // 计算远平面尺寸
        const float fov_y_rad = fov_y_deg * M_PI / 180.0f;
        const float half_height_far = tan(fov_y_rad / 2) * far_clip;
        const float half_width_far = half_height_far * aspect_ratio;

        // 转换屏幕坐标到NDC（模拟Python的坐标转换）
        float ndc_x = (2.0f * pupilCenter.x) / screenWidth - 1.0f;
        float ndc_y = 1.0f - (2.0f * pupilCenter.y) / screenHeight;

        // 投影瞳孔中心到远平面世界坐标
        float far_x = ndc_x * half_width_far;
        float far_y = ndc_y * half_height_far;
        float far_z = camera_position.z - far_clip;
        cv::Point3f far_point(far_x, far_y, far_z);

        // 计算射线方向
        cv::Point3f ray_origin = camera_position;
        cv::Point3f ray_direction = far_point - camera_position;
        float ray_length = sqrt(ray_direction.x*ray_direction.x +
                               ray_direction.y*ray_direction.y +
                               ray_direction.z*ray_direction.z);
        ray_direction /= ray_length;
        ray_direction = -ray_direction;

        // 球体参数（模拟Python的球体设置）
        const float inner_radius = 1.0f / 1.05f;
        float sphere_offset_x = (sphereCenter.x / screenWidth) * 2.0f - 1.0f;
        float sphere_offset_y = 1.0f - (sphereCenter.y / screenHeight) * 2.0f;
        cv::Point3f sphere_center_3d(sphere_offset_x * 1.5f, sphere_offset_y * 1.5f, 0.0f);

        // 计算射线-球面交点（模拟Python的射线-球面相交算法）
        cv::Point3f origin = ray_origin;
        cv::Point3f direction = -ray_direction;
        cv::Point3f L = origin - sphere_center_3d;

        float a = direction.x*direction.x + direction.y*direction.y + direction.z*direction.z;
        float b = 2 * (direction.x*L.x + direction.y*L.y + direction.z*L.z);
        float c = L.x*L.x + L.y*L.y + L.z*L.z - inner_radius * inner_radius;

        float discriminant = b * b - 4 * a * c;

        cv::Point3f target_direction;
        float t = -1;

        if (discriminant < 0) {
            // 最近点逼近（模拟Python的切点逼近）
            t = -(direction.x*L.x + direction.y*L.y + direction.z*L.z) /
                (direction.x*direction.x + direction.y*direction.y + direction.z*direction.z);
            cv::Point3f intersection_point = origin + t * direction;
            cv::Point3f intersection_local = intersection_point - sphere_center_3d;
            float local_length = sqrt(intersection_local.x*intersection_local.x +
                                    intersection_local.y*intersection_local.y +
                                    intersection_local.z*intersection_local.z);
            target_direction = intersection_local / local_length;
        } else {
            // 计算有效交点（与Python代码完全一致）
            float sqrt_disc = sqrt(discriminant);
            float t1 = (-b - sqrt_disc) / (2 * a);
            float t2 = (-b + sqrt_disc) / (2 * a);

            if (t1 > 0 && t2 > 0) {
                t = (t1 < t2) ? t1 : t2;
            } else if (t1 > 0) {
                t = t1;
            } else if (t2 > 0) {
                t = t2;
            }

            if (t <= 0) {
                return result;  // 无效交点
            }

            // Final intersection point （Python line 722）
            cv::Point3f intersection_point = origin + t * direction;

            // Convert to local space relative to sphere center （Python line 724-726）
            cv::Point3f intersection_local = intersection_point - sphere_center_3d;
            float local_length = sqrt(intersection_local.x*intersection_local.x +
                                    intersection_local.y*intersection_local.y +
                                    intersection_local.z*intersection_local.z);
            target_direction = intersection_local / local_length;
        }

        // 【关键：完整实现Python的旋转矩阵计算】
        // Local green ring direction （Python line 728-730）
        cv::Point3f circle_local_center(0.0f, 0.0f, inner_radius);
        float circle_norm = sqrt(circle_local_center.x*circle_local_center.x +
                               circle_local_center.y*circle_local_center.y +
                               circle_local_center.z*circle_local_center.z);
        circle_local_center /= circle_norm;

        // Compute rotation to align local +Z to target （Python line 732-741）
        cv::Point3f rotation_axis = cv::Point3f(
            circle_local_center.y * target_direction.z - circle_local_center.z * target_direction.y,
            circle_local_center.z * target_direction.x - circle_local_center.x * target_direction.z,
            circle_local_center.x * target_direction.y - circle_local_center.y * target_direction.x
        );

        float rotation_axis_norm = sqrt(rotation_axis.x*rotation_axis.x +
                                      rotation_axis.y*rotation_axis.y +
                                      rotation_axis.z*rotation_axis.z);

        if (rotation_axis_norm < 1e-6f) {
            result.direction = circle_local_center;
            result.valid = true;
            return result;
        }

        rotation_axis /= rotation_axis_norm;
        float dot = circle_local_center.x*target_direction.x +
                   circle_local_center.y*target_direction.y +
                   circle_local_center.z*target_direction.z;
        dot = max(-1.0f, min(1.0f, dot));  // clip
        float angle_rad = acos(dot);

        // Rotation matrix from axis-angle （Python line 743-753）
        float cos_a = cos(angle_rad);
        float sin_a = sin(angle_rad);
        float t_val = 1 - cos_a;
        float x_ = rotation_axis.x;
        float y_ = rotation_axis.y;
        float z_ = rotation_axis.z;

        // 构建旋转矩阵
        cv::Matx33f rotation_matrix(
            t_val * x_ * x_ + cos_a,        t_val * x_ * y_ - sin_a * z_,  t_val * x_ * z_ + sin_a * y_,
            t_val * x_ * y_ + sin_a * z_,   t_val * y_ * y_ + cos_a,       t_val * y_ * z_ - sin_a * x_,
            t_val * x_ * z_ - sin_a * y_,   t_val * y_ * z_ + sin_a * x_,  t_val * z_ * z_ + cos_a
        );

        // Rotate +Z vector to get gaze direction （Python line 755-758）
        cv::Point3f gaze_local(0.0f, 0.0f, inner_radius);
        cv::Point3f gaze_rotated = cv::Point3f(
            rotation_matrix(0,0) * gaze_local.x + rotation_matrix(0,1) * gaze_local.y + rotation_matrix(0,2) * gaze_local.z,
            rotation_matrix(1,0) * gaze_local.x + rotation_matrix(1,1) * gaze_local.y + rotation_matrix(1,2) * gaze_local.z,
            rotation_matrix(2,0) * gaze_local.x + rotation_matrix(2,1) * gaze_local.y + rotation_matrix(2,2) * gaze_local.z
        );

        float gaze_length = sqrt(gaze_rotated.x*gaze_rotated.x +
                                gaze_rotated.y*gaze_rotated.y +
                                gaze_rotated.z*gaze_rotated.z);
        gaze_rotated /= gaze_length;

        // 返回完整计算的结果
        result.direction = gaze_rotated;
        result.valid = true;

//        qDebug() << QString("[假3D] 完整3D计算成功：方向(%.3f, %.3f, %.3f)")
//                    .arg(result.direction.x).arg(result.direction.y).arg(result.direction.z);

    } catch (const std::exception& e) {
//        qDebug() << "[假3D] 3D向量计算异常:" << e.what();
    } catch (...) {
//        qDebug() << "[假3D] 3D向量计算未知异常";
    }

    return result;
}

// 辅助函数：计算两条射线的交点（基于椭圆的正交射线）
cv::Point2f MainWindow::findLineIntersection(const cv::RotatedRect& ellipse1, const cv::RotatedRect& ellipse2) {
    // 模拟Python中find_line_intersection的实现
    // 计算椭圆正交射线的交点

    // 将角度转换为弧度
    float angle1_rad = ellipse1.angle * M_PI / 180.0f;
    float angle2_rad = ellipse2.angle * M_PI / 180.0f;

    // 计算射线方向向量（正交于椭圆长轴）
    cv::Point2f dir1(-sin(angle1_rad), cos(angle1_rad));
    cv::Point2f dir2(-sin(angle2_rad), cos(angle2_rad));

    // 椭圆中心作为射线起点
    cv::Point2f p1 = ellipse1.center;
    cv::Point2f p2 = ellipse2.center;

    // 计算射线交点（直线方程求解）
    float det = dir1.x * dir2.y - dir1.y * dir2.x;

    if (std::abs(det) < 1e-6) {
        // 射线平行，返回中点
        return cv::Point2f((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
    }

    cv::Point2f dp = p2 - p1;
    float t = (dp.x * dir2.y - dp.y * dir2.x) / det;

    cv::Point2f intersection = p1 + t * dir1;

    return intersection;
}

// ================================================================================================

MainWindow::Gaze3DCalculator::GazeVector3D MainWindow::Gaze3DCalculator::computeGaze3D(
    const cv::Point2f& pupilCenter, const cv::Point2f& sphereCenter,
    int screenWidth, int screenHeight) {

    // 相机参数（与Python版本保持一致）
    const float fov_y_deg = 45.0f;
    const float aspect_ratio = static_cast<float>(screenWidth) / screenHeight;
    const float far_clip = 100.0f;
    const cv::Point3f camera_position(0.0f, 0.0f, 3.0f);

    // 计算远平面尺寸
    const float fov_y_rad = fov_y_deg * M_PI / 180.0f;
    const float half_height_far = tan(fov_y_rad / 2) * far_clip;
    const float half_width_far = half_height_far * aspect_ratio;

    // 转换到NDC坐标
    const float ndc_x = (2.0f * pupilCenter.x) / screenWidth - 1.0f;
    const float ndc_y = 1.0f - (2.0f * pupilCenter.y) / screenHeight;

    // 计算远平面点
    const cv::Point3f far_point(
        ndc_x * half_width_far,
        ndc_y * half_height_far,
        camera_position.z - far_clip
    );

    // 计算射线方向
    cv::Point3f ray_direction = far_point - camera_position;
    const float ray_length = sqrt(ray_direction.x*ray_direction.x +
                                 ray_direction.y*ray_direction.y +
                                 ray_direction.z*ray_direction.z);
    if (ray_length > 0) {
        ray_direction.x /= ray_length;
        ray_direction.y /= ray_length;
        ray_direction.z /= ray_length;
    }
    // Python中使用-ray_direction进行球面相交计算
    cv::Point3f corrected_ray_direction = ray_direction;
    corrected_ray_direction.x = -corrected_ray_direction.x;
    corrected_ray_direction.y = -corrected_ray_direction.y;
    corrected_ray_direction.z = -corrected_ray_direction.z;

    // 眼球球心偏移计算
    const float sphere_offset_x = (sphereCenter.x / screenWidth) * 2.0f - 1.0f;
    const float sphere_offset_y = 1.0f - (sphereCenter.y / screenHeight) * 2.0f;
    const cv::Point3f sphere_center_3d(sphere_offset_x * 1.5f, sphere_offset_y * 1.5f, 0.0f);

    // 计算球面相交（使用Python中的inner_radius = 1.0/1.05）
    const float inner_radius = 1.0f / 1.05f;  // ≈ 0.952 (不是0.12!)

//    qDebug() << QString("[3D计算] 相机位置:(%1,%2,%3)")
//                .arg(camera_position.x).arg(camera_position.y).arg(camera_position.z);
//    qDebug() << QString("[3D计算] 射线方向:(%1,%2,%3)")
//                .arg(corrected_ray_direction.x).arg(corrected_ray_direction.y).arg(corrected_ray_direction.z);
//    qDebug() << QString("[3D计算] 球心:(%1,%2,%3) 半径:%4")
//                .arg(sphere_center_3d.x).arg(sphere_center_3d.y).arg(sphere_center_3d.z).arg(inner_radius);

    // 直接计算射线和球面的交点
      const cv::Point3f intersection = computeSphereIntersection(
          camera_position, ray_direction, sphere_center_3d, inner_radius);

      if (intersection.x == 0 && intersection.y == 0 && intersection.z == 0) {
//          qDebug() << "[3D计算] 射线与球体不相交，返回无效向量";
          return GazeVector3D(); // 返回无效结果
      }

      // 计算从球心指向交点的方向向量
      cv::Point3f gaze_direction = intersection - sphere_center_3d;

      // 归一化方向向量
      float gaze_length = cv::norm(gaze_direction);
      if (gaze_length > 1e-6) {
          gaze_direction /= gaze_length;
      } else {
//          qDebug() << "[3D计算] 注视方向向量长度为0，返回无效";
          return GazeVector3D();
      }

//      qDebug() << QString("[3D计算] 简化后最终方向:(%1,%2,%3)")
//                  .arg(gaze_direction.x).arg(gaze_direction.y).arg(gaze_direction.z);

      // 返回球心和计算出的方向
      return GazeVector3D(sphere_center_3d, gaze_direction);
}

cv::Point3f MainWindow::Gaze3DCalculator::computeSphereIntersection(
    const cv::Point3f& rayOrigin, const cv::Point3f& rayDirection,
    const cv::Point3f& sphereCenter, float sphereRadius) {

    // 射线-球面相交计算
    const cv::Point3f L = rayOrigin - sphereCenter;

    const float a = rayDirection.x*rayDirection.x + rayDirection.y*rayDirection.y + rayDirection.z*rayDirection.z;
    const float b = 2 * (rayDirection.x*L.x + rayDirection.y*L.y + rayDirection.z*L.z);
    const float c = L.x*L.x + L.y*L.y + L.z*L.z - sphereRadius*sphereRadius;

    const float discriminant = b*b - 4*a*c;

//    qDebug() << QString("[球面相交] L=(%1,%2,%3)").arg(L.x).arg(L.y).arg(L.z);
//    qDebug() << QString("[球面相交] a=%1, b=%2, c=%3").arg(a).arg(b).arg(c);
//    qDebug() << QString("[球面相交] discriminant=%1").arg(discriminant);

    if (discriminant < 0) {
//        qDebug() << "[球面相交] 判别式小于0，射线与球体不相交";
        return cv::Point3f(0, 0, 0); // 无相交，将在上层函数中处理
    }

    const float sqrt_disc = sqrt(discriminant);
    const float t1 = (-b - sqrt_disc) / (2*a);
    const float t2 = (-b + sqrt_disc) / (2*a);

    // 选择最近的正t值
    float t = -1;
    if (t1 > 0 && t2 > 0) {
        t = (t1 < t2) ? t1 : t2;
    } else if (t1 > 0) {
        t = t1;
    } else if (t2 > 0) {
        t = t2;
    }

    if (t > 0) {
        cv::Point3f result(
            rayOrigin.x + t * rayDirection.x,
            rayOrigin.y + t * rayDirection.y,
            rayOrigin.z + t * rayDirection.z
        );
//        qDebug() << QString("[球面相交] 找到交点，t=%1，交点:(%2,%3,%4)").arg(t).arg(result.x).arg(result.y).arg(result.z);
        return result;
    }

//    qDebug() << QString("[球面相交] 无有效t值，t1=%1，t2=%2").arg(t1).arg(t2);
    return cv::Point3f(0, 0, 0);
}

cv::Point2f MainWindow::Gaze3DCalculator::project3DToScreen(
    const GazeVector3D& gaze3D, int screenWidth, int screenHeight,
    float maxAzimuthDeg, float maxElevationDeg) {

    if (!gaze3D.valid) {
        return cv::Point2f(-1, -1);
    }

    // 注意：由于这是在Gaze3DCalculator类中，我们无法直接访问MainWindow的成员变量
    // 这里我们暂时使用绝对角度映射，后续可以通过参数传递或重构来改进

    // 参考3DTracker的做法：基于角度的屏幕映射
    // 计算水平和垂直角度
    const cv::Point3f& dir = gaze3D.direction;

    // 计算方位角（水平角度）
    float azimuth_rad = atan2(dir.x, -dir.z);  // 使用-z作为前向

    // 计算仰角（垂直角度）
    float magnitude = sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    float elevation_rad = 0;
    if (magnitude > 1e-6) {
        elevation_rad = asin(dir.y / magnitude);
    }

    // 使用传入的可配置视场角范围
    const float max_azimuth_rad = maxAzimuthDeg * M_PI / 180.0f;
    const float max_elevation_rad = maxElevationDeg * M_PI / 180.0f;

    // 将角度映射到屏幕坐标 [-1, 1] 范围
    float normalized_x = azimuth_rad / max_azimuth_rad;
    float normalized_y = elevation_rad / max_elevation_rad;

    // 限制在有效范围内
    normalized_x = max(-1.0f, min(1.0f, normalized_x));
    normalized_y = max(-1.0f, min(1.0f, normalized_y));

    // 转换到屏幕像素坐标
   float screen_x = (normalized_x + 1.0f) / 2.0f * screenWidth;
    float screen_y = (1.0f - normalized_y) / 2.0f * screenHeight;

    // 确保在屏幕范围内
    const float clamped_x = max(0.0f, min(static_cast<float>(screenWidth-1), screen_x));
    const float clamped_y = max(0.0f, min(static_cast<float>(screenHeight-1), screen_y));

//    qDebug() << QString("[屏幕映射] 方位角:%1°, 仰角:%2°, 屏幕坐标:(%3,%4)")
//                .arg(azimuth_rad * 180 / M_PI, 0, 'f', 1)
//                .arg(elevation_rad * 180 / M_PI, 0, 'f', 1)
//                .arg(clamped_x, 0, 'f', 1)
//                .arg(clamped_y, 0, 'f', 1);

    return cv::Point2f(screen_x, screen_y);
}

// ========== 新增：3D视线角度计算 ==========
// mainwindow.cpp, Gaze3DCalculator::computeGazeAngles, line 4980 附近
MainWindow::Gaze3DCalculator::GazeAngles MainWindow::Gaze3DCalculator::computeGazeAngles(const GazeVector3D& gaze3D) {
    if (!gaze3D.valid) {
        return GazeAngles();
    }

    const cv::Point3f& dir = gaze3D.direction;
    float magnitude = cv::norm(dir);

    if (magnitude < 1e-6) {
        return GazeAngles();
    }

    // 方位角 (Azimuth, Yaw) - XZ平面上的角度，绕Y轴旋转
    // 使用 atan2(x, -z) 是因为我们通常认为-Z是向前看的方向
    float azimuth_rad = atan2(dir.x, -dir.z);

    // 仰角 (Elevation, Pitch) - Y分量与XZ平面投影长度的比值
    float horizontal_dist = sqrt(dir.x * dir.x + dir.z * dir.z);
    float elevation_rad = atan2(dir.y, horizontal_dist);

    return GazeAngles(azimuth_rad * 180.0f / M_PI,
                      elevation_rad * 180.0f / M_PI,
                      magnitude);
}


// ========== 改进的屏幕映射功能（基于角度映射，参考3DTracker理念）==========
cv::Point2f MainWindow::Gaze3DCalculator::mapToScreen(const GazeVector3D& gaze3D,
                                                     int screenWidth, int screenHeight) {
    // 使用与project3DToScreen相同的逻辑，确保一致性
    // 这里使用默认参数，实际调用时可以传入动态参数
    return project3DToScreen(gaze3D, screenWidth, screenHeight);
}

// ========== 新增：数据输出功能 ==========
void MainWindow::Gaze3DCalculator::writeGazeData(const GazeVector3D& gaze3D,
                                                 const GazeAngles& angles,
                                                 const cv::Point2f& screenPoint) {
    // 修改：即使数据无效也要尝试写入，用于调试
    if (!gaze3D.valid || !angles.valid) {
//        qDebug() << QString("[writeGazeData] 数据无效 - 3D:%1, 角度:%2").arg(gaze3D.valid).arg(angles.valid);
        // 继续执行而不是直接返回，以便调试
    }

    static QString lastFilePath = "";
    QString filePath = "gaze_vector_data.txt";

    // 检查文件是否可写
    bool canWrite = true;
    try {
        QFile testFile(filePath);
        if (!testFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
            canWrite = false;
        } else {
            testFile.close();
        }
    } catch (...) {
        canWrite = false;
    }

    if (canWrite) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);

            // 写入CSV格式：球心坐标(3) + 方向向量(3) + 角度(2) + 屏幕坐标(2)
            out << QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10\n")
                   .arg(gaze3D.origin.x, 0, 'f', 6)
                   .arg(gaze3D.origin.y, 0, 'f', 6)
                   .arg(gaze3D.origin.z, 0, 'f', 6)
                   .arg(gaze3D.direction.x, 0, 'f', 6)
                   .arg(gaze3D.direction.y, 0, 'f', 6)
                   .arg(gaze3D.direction.z, 0, 'f', 6)
                   .arg(angles.azimuth_deg, 0, 'f', 2)
                   .arg(angles.elevation_deg, 0, 'f', 2)
                   .arg(screenPoint.x, 0, 'f', 2)
                   .arg(screenPoint.y, 0, 'f', 2);

            file.close();
        }
    }
}

// ========== 动态眼球中心跟踪（参考3DTracker的update_and_average_point）==========
// mainwindow.cpp, 替换旧的 updateEyeSphereCenter 函数 (line 5000 附近)
void MainWindow::updateEyeSphereCenter(const cv::Point2f& pupilCenter) {
    if (pupilCenter.x < 0 || pupilCenter.y < 0) {
//        qDebug() << "[眼球中心跟踪] 瞳孔中心无效，跳过更新";
        return;
    }

    // 如果历史记录为空，用当前瞳孔中心直接初始化，避免从(0,0)开始
    if (m_eyeCenterHistory.empty()) {
        m_currentEyeSphereCenter = pupilCenter;
    }

    // 将当前瞳孔中心加入历史记录
    m_eyeCenterHistory.push_back(pupilCenter);

    // 保持历史记录在合理范围内
    if (m_eyeCenterHistory.size() > MAX_EYE_CENTER_HISTORY) {
        int numToRemove = m_eyeCenterHistory.size() - MAX_EYE_CENTER_HISTORY;
        m_eyeCenterHistory.erase(m_eyeCenterHistory.begin(), m_eyeCenterHistory.begin() + numToRemove);
    }

    // 计算历史数据的简单平均值
    float sum_x = 0, sum_y = 0;
    for (const auto& point : m_eyeCenterHistory) {
        sum_x += point.x;
        sum_y += point.y;
    }
    cv::Point2f newAvgCenter(sum_x / m_eyeCenterHistory.size(), sum_y / m_eyeCenterHistory.size());

    // --- 核心修改：使用低通滤波（EMA）平滑更新，打破正反馈 ---
    const float alpha = 0.005f; // 平滑系数，值越小越平滑，越能抵抗噪声漂移
//    m_currentEyeSphereCenter.x = alpha * newAvgCenter.x + (1.0f - alpha) * m_currentEyeSphereCenter.x;
//    m_currentEyeSphereCenter.y = alpha * newAvgCenter.y + (1.0f - alpha) * m_currentEyeSphereCenter.y;

    // 调试日志
    static int debug_counter = 0;
    if (++debug_counter % 60 == 0) {
//        qDebug() << QString("[眼球中心跟踪] 平滑更新后中心:(%.1f,%.1f), 基于%1个样本")
//                        .arg(m_currentEyeSphereCenter.x)
//                        .arg(m_currentEyeSphereCenter.y)
//                        .arg(m_eyeCenterHistory.size());
    }
}

// 【修复】3D状态重置函数
void MainWindow::reset3DStates() {
//    qDebug() << "[状态重置] 重置3D相关状态...";

    // 重置假3D标定状态
    m_fake3D_isCalibrated = false;
    m_fake3D_zeroPitchOffset = 0.0f;
    m_fake3D_zeroYawOffset = 0.0f;
    m_lastFake3DScreenCoords = cv::Point2f(-1, -1);  // 【修复】重置屏幕坐标缓存

    // 重置假3D多点标定数据
    m_fake3D_multiPointCalibrated = false;
    m_fake3D_calibAngles.clear();
    m_fake3D_calibScreens.clear();
    m_fake3D_transformMatrix.release();

    // 重置真3D标定状态
    m_is3DZeroCalibrated = false;
    m_zeroAzimuthOffset = 0.0f;
    m_zeroElevationOffset = 0.0f;

    // 重置3D区域校正
    m_3DRegionalCorrectionEnabled = false;
    m_3DCalibrationOffsets.clear();
    m_3DCalibrationScreenPoints.clear();
    m_3DCalibrationPupilPoints.clear();
    m_is3DCalibrated = false;

    // 重置眼球中心历史
    m_eyeCenterHistory.clear();
    m_currentEyeSphereCenter = cv::Point2f(320, 240);

    // 重置Worker的3D模式
    if (worker) {
        worker->enableReal3DMode(false);
        worker->resetReal3DModel();
    }

    // 重置3D计算器
    if (m_gaze3DCalculator) {
        // 这里可以添加3D计算器的重置逻辑
    }

//    qDebug() << "[状态重置] ✅ 3D状态重置完成";
}

// 【新增】假3D独立标定系统
void MainWindow::performFake3DCalibration() {
    qDebug() << "[假3D标定] ===== 开始单点零位校准 =====";
    qDebug() << "[假3D标定] 当前瞳孔中心:" << pupilCenter.x << "," << pupilCenter.y;

    // 检查瞳孔检测状态 - 使用成员变量 pupilCenter
    if (pupilCenter.x <= 0 || pupilCenter.y <= 0) {
        QMessageBox::warning(this, "标定失败", "无法检测到瞳孔，请确保眼睛在摄像头视野内。");
        return;
    }

    // 检查是否在假3D模式且有真3D模型
    if (!worker || !worker->isReal3DModelBuilt()) {
        QMessageBox::warning(this, "标定失败", "需要先构建真3D模型，请切换到真3D模式完成建模。");
        return;
    }

    // 获取真3D眼球几何信息
    qDebug() << "[假3D标定] 开始获取眼球几何信息...";
    auto eyeGeometry = worker->getReal3DEyeballGeometry();
    qDebug() << "[假3D标定] 几何信息获取完成，valid=" << eyeGeometry.valid;

    if (!eyeGeometry.valid) {
        qDebug() << "[假3D标定] 失败：无法获取眼球几何信息";
        QMessageBox::warning(this, "标定失败", "无法获取眼球模型信息，请重试。");
        return;
    }
    qDebug() << "[假3D标定] 眼球几何信息获取成功，焦距:" << eyeGeometry.focal_length;

    // 使用当前眼图计算标定角度
    cv::Mat currentEyeImage = eyeImage.clone();  // 获取当前眼图

    // 获取当前瞳孔位置
    cv::Point2f currentPupilPos = pupilCenter;
    qDebug() << "[假3D标定] 当前瞳孔位置:" << currentPupilPos.x << "," << currentPupilPos.y;

    // 模拟调用draw3DGazeVisualization中的角度计算逻辑
    // 1. 动态获取图像中心
    double image_center_x = currentEyeImage.cols / 2.0;
    double image_center_y = currentEyeImage.rows / 2.0;

    // 2. 计算射线方向（与推理阶段完全一致，不进行归一化）
    double focal_length = eyeGeometry.focal_length;
    cv::Point3f ray_direction(
        (currentPupilPos.x - image_center_x) / focal_length,
        (currentPupilPos.y - image_center_y) / focal_length,
        1.0
    );

    // 3. 计算射线与眼球的交点
    cv::Point3f eye_center = eyeGeometry.center_3d;
    double eye_radius = eyeGeometry.radius;
    cv::Point3f ray_origin(0, 0, 0);
    cv::Point3f oc = ray_origin - eye_center;

    double a = ray_direction.dot(ray_direction);
    double b = 2.0 * oc.dot(ray_direction);
    double c = oc.dot(oc) - eye_radius * eye_radius;
    double discriminant = b*b - 4*a*c;
    qDebug() << "[假3D标定] 射线与眼球交点计算: discriminant=" << discriminant;

    if (discriminant >= 0 && a != 0) {
        // 使用与推理阶段完全一致的交点计算方法
        double t = (-b + sqrt(discriminant)) / (2 * a);

        qDebug() << "[假3D标定] 交点参数: t=" << t;

        if (t > 0) {
            // 计算3D瞳孔位置和法线向量
            cv::Point3f pupil_3d = ray_origin + static_cast<float>(t) * ray_direction;
            cv::Point3f normal_vector = pupil_3d - eye_center;
            double normal_length = sqrt(normal_vector.x*normal_vector.x +
                                       normal_vector.y*normal_vector.y +
                                       normal_vector.z*normal_vector.z);

            if (normal_length > 1e-6) {
                // 【关键修复】使用与运行时完全一致的角度计算方式
                double nx = normal_vector.x / normal_length;
                double ny = normal_vector.y / normal_length;
                double nz = normal_vector.z / normal_length;

                // 【修复】使用与draw3DGazeVisualization中完全相同的角度计算公式
                double alpha = std::atan2(nz, ny) * 180.0 / CV_PI;  // Pitch: 上下方向 (ny控制上下)
                double beta = std::atan2(nz, nx) * 180.0 / CV_PI;   // Yaw: 左右方向 (nx控制左右)

                qDebug() << "[假3D标定] 计算得到角度: alpha=" << alpha << "°, beta=" << beta << "°";

                // 记录当前角度作为零点偏移
                m_fake3D_zeroPitchOffset = static_cast<float>(alpha);
                m_fake3D_zeroYawOffset = static_cast<float>(beta);
                m_fake3D_isCalibrated = true;

                // 立即验证标定结果 - 测试零点校正后的映射
                QScreen* screen = QGuiApplication::primaryScreen();
                QRect screenGeometry = screen->geometry();
                int screenWidth = screenGeometry.width();
                int screenHeight = screenGeometry.height();

                // 【关键修复】验证时要应用零点偏移校正
                double calibrated_alpha = alpha - m_fake3D_zeroPitchOffset;
                double calibrated_beta = beta - m_fake3D_zeroYawOffset;
                cv::Point2f testCenter = mapFake3DAnglesToScreen(calibrated_alpha, calibrated_beta, screenWidth, screenHeight);

                // qDebug() << QString("[标定验证] 原始角度(%.2f°,%.2f°) -> 校正后(%.2f°,%.2f°) -> 映射到屏幕(%.0f,%.0f)")
                //             .arg(alpha).arg(beta)
                //             .arg(calibrated_alpha).arg(calibrated_beta)
                //             .arg(testCenter.x).arg(testCenter.y);
                // qDebug() << QString("[标定验证] 屏幕中心为(%.0f,%.0f)，校正后应该接近中心")
                //             .arg(screenWidth/2.0).arg(screenHeight/2.0);

//                qDebug() << QString("[假3D标定] 成功！零点角度: Pitch=%.2f°, Yaw=%.2f°")
//                            .arg(pitch).arg(yaw);
                // QMessageBox::information(this, "假3D标定成功",
                //     QString("假3D零点位置已校准！\nPitch偏移: %.2f°\nYaw偏移: %.2f°")
                //     .arg(alpha).arg(beta));
                return;
            } else {
                qDebug() << "[假3D标定] 失败：法线长度过小 " << normal_length;
            }
        } else {
            qDebug() << "[假3D标定] 失败：无有效交点 t=" << t;
        }
    } else {
        qDebug() << "[假3D标定] 失败：判别式为负 discriminant=" << discriminant;
    }

    qDebug() << "[假3D标定] 失败：几何计算错误";
    QMessageBox::warning(this, "标定失败", "几何计算出现问题，请重试。");
}

// 【新增】假3D角度到屏幕坐标的映射函数
cv::Point2f MainWindow::mapFake3DAnglesToScreen(double pitch, double yaw, int screenWidth, int screenHeight) {
    // 检查是否已标定
    if (!m_fake3D_isCalibrated) {
//        qDebug() << "[假3D映射] 警告：未进行假3D标定，返回无效坐标";
        return cv::Point2f(-1, -1);
    }

    // 使用传入的角度（调用者负责应用零点偏移）
    double calibrated_pitch = pitch;
    double calibrated_yaw = yaw;

    // 【调整映射范围】降低灵敏度，增大角度范围
    // 原来范围太小导致过于敏感，现在增大范围
    const double max_pitch_deg = 23.0;    // 垂直视角范围 ±25度 (总共50度覆盖全屏高度)
    const double max_yaw_deg = 35.0;      // 水平视角范围 ±35度 (总共70度覆盖全屏宽度)

    // 将角度归一化到 [-1, 1] 范围
    double normalized_pitch = max(-1.0, min(1.0, calibrated_pitch / max_pitch_deg));
    double normalized_yaw = max(-1.0, min(1.0, calibrated_yaw / max_yaw_deg));

    // 映射到屏幕坐标
    // 【修复方向问题】反转yaw和pitch的映射方向
    // Yaw(beta)控制水平位置：反转X方向
    double screen_x = (1.0 - normalized_yaw) / 2.0 * screenWidth;
    // Pitch(alpha)控制垂直位置：反转Y方向
    double screen_y = (1.0 - normalized_pitch) / 2.0 * screenHeight;

    // 【修复后的映射逻辑】标定后，0偏移应该映射到屏幕中心
    // 如果 calibrated_pitch = 0, calibrated_yaw = 0，那么：
    // normalized_pitch = 0, normalized_yaw = 0
    // screen_x = (1-0)/2 * width = width/2 ✓ 正确！
    // screen_y = (1-0)/2 * height = height/2 ✓ 正确！
    // 现在：向右看(yaw>0) → screen_x小(屏幕左侧) - 修复方向
    //      向左看(yaw<0) → screen_x大(屏幕右侧) - 修复方向
    //      向上看(pitch>0) → screen_y小(屏幕上方) - 修复方向
    //      向下看(pitch<0) → screen_y大(屏幕下方) - 修复方向

    // 【调试】启用详细调试输出查看映射过程（每10帧输出一次）
    // static int debug_counter = 0;
    // if (++debug_counter % 10 == 0) {
    //     qDebug() << QString("[假3D映射详细] 原始角度(P:%.1f°,Y:%.1f°) -> 标定偏移(P:%.1f°,Y:%.1f°) -> 校正角度(P:%.1f°,Y:%.1f°)")
    //                 .arg(pitch).arg(yaw)
    //                 .arg(m_fake3D_zeroPitchOffset).arg(m_fake3D_zeroYawOffset)
    //                 .arg(calibrated_pitch).arg(calibrated_yaw);
    //     qDebug() << QString("[假3D映射详细] 视角范围(P:±%.1f°=%.1f°总,Y:±%.1f°=%.1f°总) -> 归一化(P:%.3f,Y:%.3f) -> 屏幕(%.0f,%.0f)")
    //                 .arg(max_pitch_deg).arg(max_pitch_deg*2)
    //                 .arg(max_yaw_deg).arg(max_yaw_deg*2)
    //                 .arg(normalized_pitch).arg(normalized_yaw)
    //                 .arg(screen_x).arg(screen_y);

    //     // 【测试】强制输出一个中心坐标来验证映射公式
    //     if (debug_counter == 10) {
    //         double test_norm_x = 0.0, test_norm_y = 0.0;  // 归一化中心值
    //         double test_x = (test_norm_x + 1.0) / 2.0 * screenWidth;
    //         double test_y = (1.0 - test_norm_y) / 2.0 * screenHeight;
    //         qDebug() << QString("[测试] 归一化中心(0,0) -> 屏幕中心应该是(%.0f,%.0f)，实际屏幕中心(%.0f,%.0f)")
    //                     .arg(test_x).arg(test_y)
    //                     .arg(screenWidth/2.0).arg(screenHeight/2.0);
    //     }
    // }

    return cv::Point2f(static_cast<float>(screen_x), static_cast<float>(screen_y));
}

// 【新增】计算瞳孔位置对应的假3D角度
cv::Point2f MainWindow::calculateFake3DAnglesForScreenPoint(const cv::Point2f& screenPoint) {
    // 根据屏幕坐标计算期望的假3D角度
    // 这个函数应该实现从屏幕坐标到角度的反向映射

    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    // 【关键修复】确保与mapFake3DAnglesToScreen完全对应
    // mapFake3DAnglesToScreen中的逻辑：
    // screen_x = (normalized_yaw + 1.0) / 2.0 * screenWidth
    // screen_y = (normalized_pitch + 1.0) / 2.0 * screenHeight

    // 反向计算：
    double normalized_yaw = (screenPoint.x / screenWidth) * 2.0 - 1.0;
    double normalized_pitch = (screenPoint.y / screenHeight) * 2.0 - 1.0;

    // 使用相同的视角范围：水平±16.5°，垂直±9°
    const double max_yaw_deg = 16.5;     // 水平范围
    const double max_pitch_deg = 9.0;    // 垂直范围

    // 计算对应的角度
    double yaw = normalized_yaw * max_yaw_deg;      // 左右角度
    double pitch = normalized_pitch * max_pitch_deg; // 上下角度

    // qDebug() << QString("[假3D角度计算] 屏幕(%.1f,%.1f) -> 角度(P:%.2f°,Y:%.2f°)")
    //             .arg(screenPoint.x).arg(screenPoint.y).arg(pitch).arg(yaw);

    return cv::Point2f(pitch, yaw);
}

// 【新增】获取多项式方法名称
QString MainWindow::getPolynomialMethodName(PlType plType) {
    switch(plType) {
    case POLY_1_X_Y_XY:
        return "POLY_1_X_Y_XY";
    case POLY_1_X_Y_XY_XX_YY:
        return "POLY_1_X_Y_XY_XX_YY";
    case POLY_1_X_Y_XY_XX_YY_XXYY:
        return "POLY_1_X_Y_XY_XX_YY_XXYY";
    case POLY_1_X_Y_XY_XX_YY_XYY_YXX:
        return "POLY_1_X_Y_XY_XX_YY_XYY_YXX";
    case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY:
        return "POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY";
    default:
        return "UNKNOWN";
    }
}

// 【新增】填充假3D多项式系数矩阵
void MainWindow::fillFake3DPolynomialCoefficients(cv::Mat& A, int row, float pitch, float yaw, PlType plType, int unknowns) {
    int halfParams = unknowns;  // 单轴参数数量
    int totalParams = unknowns * 2;  // 双轴总参数数量

    // 预计算常用项
    float pitch2 = pitch * pitch;
    float yaw2 = yaw * yaw;
    float pitch_yaw = pitch * yaw;
    float pitch2_yaw = pitch2 * yaw;
    float pitch_yaw2 = pitch * yaw2;
    float pitch2_yaw2 = pitch2 * yaw2;

    // 初始化所有系数为0
    for (int j = 0; j < totalParams; j++) {
        A.at<float>(2*row, j) = 0.0f;
        A.at<float>(2*row+1, j) = 0.0f;
    }

    // 根据多项式类型填充X方程系数（前半部分）和Y方程系数（后半部分）
    switch(plType) {
    case POLY_1_X_Y_XY: // 4参数: 1, x, y, xy
        // X方程
        A.at<float>(2*row, 0) = 1.0f;
        A.at<float>(2*row, 1) = pitch;
        A.at<float>(2*row, 2) = yaw;
        A.at<float>(2*row, 3) = pitch_yaw;
        // Y方程
        A.at<float>(2*row+1, 4) = 1.0f;
        A.at<float>(2*row+1, 5) = pitch;
        A.at<float>(2*row+1, 6) = yaw;
        A.at<float>(2*row+1, 7) = pitch_yaw;
        break;

    case POLY_1_X_Y_XY_XX_YY: // 6参数: 1, x, y, xy, x², y²
        // X方程
        A.at<float>(2*row, 0) = 1.0f;
        A.at<float>(2*row, 1) = pitch;
        A.at<float>(2*row, 2) = yaw;
        A.at<float>(2*row, 3) = pitch_yaw;
        A.at<float>(2*row, 4) = pitch2;
        A.at<float>(2*row, 5) = yaw2;
        // Y方程
        A.at<float>(2*row+1, 6) = 1.0f;
        A.at<float>(2*row+1, 7) = pitch;
        A.at<float>(2*row+1, 8) = yaw;
        A.at<float>(2*row+1, 9) = pitch_yaw;
        A.at<float>(2*row+1, 10) = pitch2;
        A.at<float>(2*row+1, 11) = yaw2;
        break;

    case POLY_1_X_Y_XY_XX_YY_XXYY: // 7参数: 1, x, y, xy, x², y², x²y²
        // X方程
        A.at<float>(2*row, 0) = 1.0f;
        A.at<float>(2*row, 1) = pitch;
        A.at<float>(2*row, 2) = yaw;
        A.at<float>(2*row, 3) = pitch_yaw;
        A.at<float>(2*row, 4) = pitch2;
        A.at<float>(2*row, 5) = yaw2;
        A.at<float>(2*row, 6) = pitch2_yaw2;
        // Y方程
        A.at<float>(2*row+1, 7) = 1.0f;
        A.at<float>(2*row+1, 8) = pitch;
        A.at<float>(2*row+1, 9) = yaw;
        A.at<float>(2*row+1, 10) = pitch_yaw;
        A.at<float>(2*row+1, 11) = pitch2;
        A.at<float>(2*row+1, 12) = yaw2;
        A.at<float>(2*row+1, 13) = pitch2_yaw2;
        break;

    case POLY_1_X_Y_XY_XX_YY_XYY_YXX: // 8参数: 1, x, y, xy, x², y², xy², yx²
        // X方程
        A.at<float>(2*row, 0) = 1.0f;
        A.at<float>(2*row, 1) = pitch;
        A.at<float>(2*row, 2) = yaw;
        A.at<float>(2*row, 3) = pitch_yaw;
        A.at<float>(2*row, 4) = pitch2;
        A.at<float>(2*row, 5) = yaw2;
        A.at<float>(2*row, 6) = pitch_yaw2;
        A.at<float>(2*row, 7) = pitch2_yaw;
        // Y方程
        A.at<float>(2*row+1, 8) = 1.0f;
        A.at<float>(2*row+1, 9) = pitch;
        A.at<float>(2*row+1, 10) = yaw;
        A.at<float>(2*row+1, 11) = pitch_yaw;
        A.at<float>(2*row+1, 12) = pitch2;
        A.at<float>(2*row+1, 13) = yaw2;
        A.at<float>(2*row+1, 14) = pitch_yaw2;
        A.at<float>(2*row+1, 15) = pitch2_yaw;
        break;

    case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY: // 9参数: 1, x, y, xy, x², y², xy², yx², x²y²
        // X方程
        A.at<float>(2*row, 0) = 1.0f;
        A.at<float>(2*row, 1) = pitch;
        A.at<float>(2*row, 2) = yaw;
        A.at<float>(2*row, 3) = pitch_yaw;
        A.at<float>(2*row, 4) = pitch2;
        A.at<float>(2*row, 5) = yaw2;
        A.at<float>(2*row, 6) = pitch_yaw2;
        A.at<float>(2*row, 7) = pitch2_yaw;
        A.at<float>(2*row, 8) = pitch2_yaw2;
        // Y方程
        A.at<float>(2*row+1, 9) = 1.0f;
        A.at<float>(2*row+1, 10) = pitch;
        A.at<float>(2*row+1, 11) = yaw;
        A.at<float>(2*row+1, 12) = pitch_yaw;
        A.at<float>(2*row+1, 13) = pitch2;
        A.at<float>(2*row+1, 14) = yaw2;
        A.at<float>(2*row+1, 15) = pitch_yaw2;
        A.at<float>(2*row+1, 16) = pitch2_yaw;
        A.at<float>(2*row+1, 17) = pitch2_yaw2;
        break;

    default:
        qDebug() << "[假3D多项式] 错误：不支持的多项式类型";
        break;
    }
}

// 【新增】应用假3D多项式计算
float MainWindow::applyFake3DPolynomial(float pitch, float yaw, PlType plType, int unknowns, bool isXCoord) {
    if (m_fake3D_transformMatrix.empty()) {
        qDebug() << "【多项式计算错误】矩阵为空";
        return 0.0f;
    }

    // 计算系数偏移量（Y坐标使用后半部分系数）
    int offset = isXCoord ? 0 : unknowns;

    // 【调试】检查矩阵访问范围
    if (offset + unknowns > m_fake3D_transformMatrix.rows) {
        QString errorMsg = QString("【多项式计算错误】\n")
            .append(QString("矩阵尺寸: %1x%2\n").arg(m_fake3D_transformMatrix.rows).arg(m_fake3D_transformMatrix.cols))
            .append(QString("请求访问范围: offset=%1, unknowns=%2\n").arg(offset).arg(unknowns))
            .append(QString("计算%1坐标").arg(isXCoord ? "X" : "Y"));
        QMessageBox::critical(this, "多项式计算错误", errorMsg);
        return 0.0f;
    }

    // 预计算常用项
    float pitch2 = pitch * pitch;
    float yaw2 = yaw * yaw;
    float pitch_yaw = pitch * yaw;
    float pitch2_yaw = pitch2 * yaw;
    float pitch_yaw2 = pitch * yaw2;
    float pitch2_yaw2 = pitch2 * yaw2;

    float result = 0.0f;

    // 根据多项式类型计算结果
    switch(plType) {
    case POLY_1_X_Y_XY: // 4参数: 1, x, y, xy
        result = m_fake3D_transformMatrix.at<float>(offset + 0) +       // 常数项
                 m_fake3D_transformMatrix.at<float>(offset + 1) * pitch + // x项
                 m_fake3D_transformMatrix.at<float>(offset + 2) * yaw +   // y项
                 m_fake3D_transformMatrix.at<float>(offset + 3) * pitch_yaw; // xy项
        break;

    case POLY_1_X_Y_XY_XX_YY: // 6参数: 1, x, y, xy, x², y²
        result = m_fake3D_transformMatrix.at<float>(offset + 0) +       // 常数项
                 m_fake3D_transformMatrix.at<float>(offset + 1) * pitch + // x项
                 m_fake3D_transformMatrix.at<float>(offset + 2) * yaw +   // y项
                 m_fake3D_transformMatrix.at<float>(offset + 3) * pitch_yaw + // xy项
                 m_fake3D_transformMatrix.at<float>(offset + 4) * pitch2 + // x²项
                 m_fake3D_transformMatrix.at<float>(offset + 5) * yaw2;   // y²项
        break;

    case POLY_1_X_Y_XY_XX_YY_XXYY: // 7参数: 1, x, y, xy, x², y², x²y²
        result = m_fake3D_transformMatrix.at<float>(offset + 0) +       // 常数项
                 m_fake3D_transformMatrix.at<float>(offset + 1) * pitch + // x项
                 m_fake3D_transformMatrix.at<float>(offset + 2) * yaw +   // y项
                 m_fake3D_transformMatrix.at<float>(offset + 3) * pitch_yaw + // xy项
                 m_fake3D_transformMatrix.at<float>(offset + 4) * pitch2 + // x²项
                 m_fake3D_transformMatrix.at<float>(offset + 5) * yaw2 +   // y²项
                 m_fake3D_transformMatrix.at<float>(offset + 6) * pitch2_yaw2; // x²y²项
        break;

    case POLY_1_X_Y_XY_XX_YY_XYY_YXX: // 8参数: 1, x, y, xy, x², y², xy², yx²
        result = m_fake3D_transformMatrix.at<float>(offset + 0) +       // 常数项
                 m_fake3D_transformMatrix.at<float>(offset + 1) * pitch + // x项
                 m_fake3D_transformMatrix.at<float>(offset + 2) * yaw +   // y项
                 m_fake3D_transformMatrix.at<float>(offset + 3) * pitch_yaw + // xy项
                 m_fake3D_transformMatrix.at<float>(offset + 4) * pitch2 + // x²项
                 m_fake3D_transformMatrix.at<float>(offset + 5) * yaw2 +   // y²项
                 m_fake3D_transformMatrix.at<float>(offset + 6) * pitch_yaw2 + // xy²项
                 m_fake3D_transformMatrix.at<float>(offset + 7) * pitch2_yaw;  // x²y项
        break;

    case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY: // 9参数: 1, x, y, xy, x², y², xy², yx², x²y²
        result = m_fake3D_transformMatrix.at<float>(offset + 0) +       // 常数项
                 m_fake3D_transformMatrix.at<float>(offset + 1) * pitch + // x项
                 m_fake3D_transformMatrix.at<float>(offset + 2) * yaw +   // y项
                 m_fake3D_transformMatrix.at<float>(offset + 3) * pitch_yaw + // xy项
                 m_fake3D_transformMatrix.at<float>(offset + 4) * pitch2 + // x²项
                 m_fake3D_transformMatrix.at<float>(offset + 5) * yaw2 +   // y²项
                 m_fake3D_transformMatrix.at<float>(offset + 6) * pitch_yaw2 + // xy²项
                 m_fake3D_transformMatrix.at<float>(offset + 7) * pitch2_yaw + // x²y项
                 m_fake3D_transformMatrix.at<float>(offset + 8) * pitch2_yaw2; // x²y²项
        break;

    default:
        qDebug() << "[假3D多项式应用] 错误：不支持的多项式类型";
        result = 0.0f;
        break;
    }

    return result;
}

// 【新增】计算假3D多点映射矩阵
void MainWindow::computeFake3DMultiPointMapping() {
    if (m_fake3D_calibAngles.size() < 3 || m_fake3D_calibScreens.size() < 3) {
        qDebug() << "[假3D多点标定] 错误：标定点数量不足，至少需要3个点";
        return;
    }

    if (m_fake3D_calibAngles.size() != m_fake3D_calibScreens.size()) {
        qDebug() << "[假3D多点标定] 错误：角度数据和屏幕坐标数量不匹配";
        return;
    }

    int n = m_fake3D_calibAngles.size();
    qDebug() << QString("[假3D多点标定] 开始计算映射矩阵，使用%1个标定点").arg(n);

    // 从UI获取用户选择的拟合方法和所需参数数量
    int selectedParams = unknowns * 2;  // unknowns是单轴参数数量，双轴需要翻倍

    // 检查点数是否足够支持选择的拟合方法
    if (n * 2 < selectedParams) {
        QString errorMsg = QString("选择的拟合方法需要%1个系数，但只有%2个数据点（%3个方程）。\n\n"
                                  "请选择更多标定点或更简单的拟合方法。")
                              .arg(selectedParams)
                              .arg(n)
                              .arg(n * 2);
        QMessageBox::warning(this, "假3D标定错误", errorMsg);
        qDebug() << QString("[假3D多点标定] 错误：数据点不足，需要%1参数但只有%2方程").arg(selectedParams).arg(n * 2);
        return;
    }

    qDebug() << QString("[假3D多点标定] 使用UI选择的拟合方法：%1参数").arg(selectedParams);

    // 创建系数矩阵，使用UI选择的拟合方法
    cv::Mat A = cv::Mat(2 * n, selectedParams, CV_32F);
    cv::Mat b = cv::Mat(2 * n, 1, CV_32F);

    QString methodName = getPolynomialMethodName(plType);
    qDebug() << QString("[假3D多点标定] 使用UI选择的拟合方法：%1 (%2参数)").arg(methodName).arg(selectedParams);

    for (int i = 0; i < n; i++) {
        float pitch = m_fake3D_calibAngles[i].x;
        float yaw = m_fake3D_calibAngles[i].y;

        // 根据选择的拟合类型填充系数矩阵
        fillFake3DPolynomialCoefficients(A, i, pitch, yaw, plType, unknowns);

        // 设置目标值
        b.at<float>(2*i) = m_fake3D_calibScreens[i].x;     // X坐标
        b.at<float>(2*i+1) = m_fake3D_calibScreens[i].y;   // Y坐标
    }

    // 修复：添加数值稳定性检查
    double conditionNumber = cv::norm(A) * cv::norm(A.inv(cv::DECOMP_SVD));
    qDebug() << QString("[假3D多点标定] 矩阵条件数估计: %.2e").arg(conditionNumber);

    if (conditionNumber > 1e12) {
        qDebug() << "[假3D多点标定] 警告：矩阵条件数过大，可能导致数值不稳定";
        QMessageBox::warning(this, "假3D标定警告",
                           QString("检测到数值不稳定性（条件数: %.2e）\n建议增加更多标定点或选择更简单的拟合方法。").arg(conditionNumber));
    }

    // 求解最小二乘系统 A * x = b
    bool success = cv::solve(A, b, m_fake3D_transformMatrix, cv::DECOMP_SVD);

    if (success && !m_fake3D_transformMatrix.empty()) {
        m_fake3D_multiPointCalibrated = true;
        qDebug() << "[假3D多点标定] ✅ 映射矩阵计算成功";

        // 输出训练数据进行调试
        qDebug() << "[假3D调试] 训练数据:";
        for (int i = 0; i < n; i++) {
            qDebug() << QString("  点%1: 角度(%.2f°,%.2f°) -> 屏幕(%.0f,%.0f)")
                        .arg(i+1)
                        .arg(m_fake3D_calibAngles[i].x).arg(m_fake3D_calibAngles[i].y)
                        .arg(m_fake3D_calibScreens[i].x).arg(m_fake3D_calibScreens[i].y);
        }

        // 输出系数矩阵
        qDebug() << "[假3D调试] 计算得到的映射系数:";
        for (int i = 0; i < m_fake3D_transformMatrix.rows; i++) {
            qDebug() << QString("  系数[%1] = %2").arg(i).arg(m_fake3D_transformMatrix.at<float>(i));
        }

        // 验证标定质量
        double totalError = 0.0;
        for (int i = 0; i < n; i++) {
            cv::Point2f predicted = applyFake3DMultiPointMapping(
                m_fake3D_calibAngles[i].x, m_fake3D_calibAngles[i].y);
            cv::Point2f actual = m_fake3D_calibScreens[i];
            double error = cv::norm(predicted - actual);
            totalError += error;

            qDebug() << QString("[假3D验证%1] 角度(P:%.2f°,Y:%.2f°) -> 预测屏幕(%.0f,%.0f) vs 目标屏幕(%.0f,%.0f), 误差%.1fpx")
                        .arg(i+1).arg(m_fake3D_calibAngles[i].x).arg(m_fake3D_calibAngles[i].y)
                        .arg(predicted.x).arg(predicted.y)
                        .arg(actual.x).arg(actual.y).arg(error);
        }

        double avgError = totalError / n;
        qDebug() << QString("[假3D多点标定] 平均误差: %.1f像素").arg(avgError);

        // 【移除】这里不显示误差图，交给perform3DCalibration统一处理
        // showFake3DCalibrationResult(avgError);

    } else {
        qDebug() << "[假3D多点标定] ❌ 映射矩阵计算失败";
        m_fake3D_multiPointCalibrated = false;
    }
}

// 【新增】应用假3D多点映射矩阵
cv::Point2f MainWindow::applyFake3DMultiPointMapping(double pitch, double yaw) {
    // 【调试】检查多点标定状态
//    static bool debug_shown = false;
//    if (!debug_shown) {
//        QString debugMsg = QString("【多点推理调试】\n")
//            .append(QString("m_fake3D_multiPointCalibrated: %1\n").arg(m_fake3D_multiPointCalibrated ? "true" : "false"))
//            .append(QString("m_fake3D_transformMatrix.empty(): %1\n").arg(m_fake3D_transformMatrix.empty() ? "true" : "false"))
//            .append(QString("矩阵尺寸: %1x%2\n").arg(m_fake3D_transformMatrix.rows).arg(m_fake3D_transformMatrix.cols))
//            .append(QString("当前plType: %1\n").arg(plType))
//            .append(QString("当前unknowns: %1\n").arg(unknowns))
//            .append(QString("标定数据点数: %1").arg(m_fake3D_calibAngles.size()));
//        QMessageBox::information(this, "多点推理状态检查", debugMsg);
//        debug_shown = true;
//    }

    if (!m_fake3D_multiPointCalibrated || m_fake3D_transformMatrix.empty()) {
        // 回退到单点映射
        QScreen* screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->geometry();
        cv::Point2f fallbackResult = mapFake3DAnglesToScreen(pitch, yaw, screenGeometry.width(), screenGeometry.height());
//        qDebug() << QString("【多点推理回退】角度(%.2f°,%.2f°) -> 屏幕(%.0f,%.0f)")
//                    .arg(pitch).arg(yaw).arg(fallbackResult.x).arg(fallbackResult.y);
        return fallbackResult;
    }

    float p = static_cast<float>(pitch);
    float y = static_cast<float>(yaw);

    // 应用多项式计算（与训练时使用相同的逻辑）
    float screen_x = applyFake3DPolynomial(p, y, plType, unknowns, true);   // 计算X坐标
    float screen_y = applyFake3DPolynomial(p, y, plType, unknowns, false);  // 计算Y坐标

    // 【增强调试】详细输出推理过程
//    static int debug_count = 0;
//    debug_count++;
//    if (debug_count <= 5 || debug_count % 30 == 0) {  // 前5次和每30次输出
//        QString detailMsg = QString("【多点推理过程】\n")
//            .append(QString("输入: Pitch=%.3f°, Yaw=%.3f°\n").arg(pitch).arg(yaw))
//            .append(QString("输出: X=%.1f, Y=%.1f\n").arg(screen_x).arg(screen_y))
//            .append(QString("使用参数: plType=%1, unknowns=%2").arg(plType).arg(unknowns));

//        if (debug_count <= 3) {
//            QMessageBox::information(this, QString("多点推理第%1次").arg(debug_count), detailMsg);
//        }
//        qDebug() << detailMsg.replace("\n", " | ");
//    }

    return cv::Point2f(screen_x, screen_y);
}

// 【新增】显示假3D标定结果（模仿精确标定的OpenCV窗口方式）
void MainWindow::showFake3DCalibrationResult(double avgError) {
    qDebug() << "🟡 [showFake3DCalibrationResult] 函数开始执行 - 假3D误差显示";
    qDebug() << QString("[showFake3DCalibrationResult] 平均误差: %.1f像素").arg(avgError);

    // 生成假3D误差分析图
    qDebug() << "[showFake3DCalibrationResult] 开始生成误差分析图";
    cv::Mat errorPlot = generateFake3DErrorPlot();
    qDebug() << QString("[showFake3DCalibrationResult] 误差图生成完成，尺寸: %1x%2").arg(errorPlot.cols).arg(errorPlot.rows);

    if (!errorPlot.empty()) {
        cv::cvtColor(errorPlot, errorPlot, cv::COLOR_BGR2RGB);
        qDebug() << "[showFake3DCalibrationResult] 颜色转换完成，准备显示窗口";
        cv::imshow("3D Calibration Error Analysis", errorPlot);
        cv::waitKey(1);
        qDebug() << "[showFake3DCalibrationResult] OpenCV窗口显示完成";
    } else {
        qDebug() << "[showFake3DCalibrationResult] ❌ 误差图为空，无法显示";
    }
}

// 【新增】生成假3D误差分析图（模仿精确标定的generateErrorPlot）
cv::Mat MainWindow::generateFake3DErrorPlot() {
    // 检查数据有效性
    qDebug() << QString("[假3D误差图] 数据检查: calibAngles数量=%1, calibScreens数量=%2")
                .arg(m_fake3D_calibAngles.size()).arg(m_fake3D_calibScreens.size());

    if (m_fake3D_calibAngles.empty() || m_fake3D_calibScreens.empty()) {
        qDebug() << "[假3D误差图] 没有标定数据，尝试使用实时数据生成误差图";

        // 如果没有保存的标定数据，使用当前3D标定数据生成误差图
        if (m_3DCalibrationPupilPoints.empty() || m_3DCalibrationScreenPoints.empty()) {
            qDebug() << "[假3D误差图] 也没有3D标定数据可用于生成误差图";
            return cv::Mat();
        }

        // 使用3D标定数据重新计算假3D角度
        std::vector<cv::Point2f> tempAngles;
        std::vector<cv::Point2f> tempScreens;

        auto eyeGeometry = worker ? worker->getReal3DEyeballGeometry() : Worker::EyeballGeometry{};
        if (!eyeGeometry.valid || !worker) {
            qDebug() << "[假3D误差图] 无法获取眼球几何信息";
            return cv::Mat();
        }

        for (int i = 0; i < m_3DCalibrationPupilPoints.size() && i < m_3DCalibrationScreenPoints.size(); ++i) {
            cv::Point2f pupilPoint = m_3DCalibrationPupilPoints[i];
            cv::Point2f screenPoint = m_3DCalibrationScreenPoints[i];

            // 重新计算角度（使用与数据收集相同的方法）
            double image_center_x = eyeImage.cols / 2.0;
            double image_center_y = eyeImage.rows / 2.0;
            double focal_length = eyeGeometry.focal_length;

            cv::Point3f ray_direction(
                (pupilPoint.x - image_center_x) / focal_length,
                (pupilPoint.y - image_center_y) / focal_length,
                -1.0
            );
            cv::Point3f ray_origin(0, 0, 0);
            cv::Point3f eye_center = eyeGeometry.center_3d;
            double eye_radius = eyeGeometry.radius;

            cv::Point3f oc = ray_origin - eye_center;
            double a = ray_direction.dot(ray_direction);
            double b = 2.0 * oc.dot(ray_direction);
            double c = oc.dot(oc) - eye_radius * eye_radius;
            double discriminant = b * b - 4 * a * c;

            if (discriminant >= 0 && a != 0) {
                double t = (-b + sqrt(discriminant)) / (2 * a);
                if (t > 0) {
                    cv::Point3f pupil_3d = ray_origin + static_cast<float>(t) * ray_direction;
                    cv::Point3f normal_vector = pupil_3d - eye_center;
                    double normal_length = sqrt(normal_vector.x*normal_vector.x +
                                               normal_vector.y*normal_vector.y +
                                               normal_vector.z*normal_vector.z);

                    if (normal_length > 1e-6) {
                        double nx = normal_vector.x / normal_length;
                        double ny = normal_vector.y / normal_length;
                        double nz = normal_vector.z / normal_length;

                        // 【关键修复】使用与推理时完全一致的角度计算公式
                        double alpha = std::atan2(nz, ny) * 180.0 / CV_PI;  // Pitch: 与推理时一致
                        double beta = std::atan2(nz, nx) * 180.0 / CV_PI;   // Yaw: 与推理时一致

                        tempAngles.push_back(cv::Point2f(alpha, beta));
                        tempScreens.push_back(screenPoint);
                    }
                }
            }
        }

        if (tempAngles.empty()) {
            qDebug() << "[假3D误差图] 重新计算角度失败";
            return cv::Mat();
        }

        // 临时使用重新计算的数据（已修复角度计算）
        m_fake3D_calibAngles = tempAngles;
        m_fake3D_calibScreens = tempScreens;

        qDebug() << QString("[假3D误差图] 使用重新计算的数据: %1个点").arg(tempAngles.size());
    }

    // 获取屏幕分辨率
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    int screenWidth = screenRect.width();
    int screenHeight = screenRect.height();

    // 创建画布 - 使用屏幕尺寸，避免截断（与精确标定一致）
    cv::Mat plot(screenHeight, screenWidth, CV_8UC3, cv::Scalar(255, 255, 255));

    // 屏幕物理参数
    const double MM_PER_PIXEL = 0.125391;
    const double VIEWING_DISTANCE_MM = 500.0;

    // 配置参数
    const int TEXT_AREA_WIDTH = min(320, screenWidth / 4);
    const int TEXT_START_X = 10;
    const int TEXT_START_Y = 30;
    const int LINE_HEIGHT = 22;

    // 颜色定义
    const cv::Scalar CIRCLE_COLOR = cv::Scalar(100, 200, 255);  // 浅橙色 (BGR格式)
    const cv::Scalar TEXT_COLOR = cv::Scalar(50, 50, 50);       // 深灰色
    const cv::Scalar SEPARATOR_COLOR = cv::Scalar(200, 200, 200); // 分割线颜色
    const cv::Scalar METRIC_COLOR = cv::Scalar(0, 100, 200);    // 指标文本颜色

    // 绘制分割线
    cv::line(plot, cv::Point(TEXT_AREA_WIDTH, 0), cv::Point(TEXT_AREA_WIDTH, screenHeight), SEPARATOR_COLOR, 1);

    // 计算并绘制误差
    double totalError = 0.0;
    int validPoints = 0;

    // 存储每个点的详细信息用于左侧显示
    std::vector<std::string> pointDetails;

    for (int i = 0; i < m_fake3D_calibAngles.size(); ++i) {
        // 计算预测屏幕坐标
        cv::Point2f predicted = applyFake3DMultiPointMapping(
            m_fake3D_calibAngles[i].x, m_fake3D_calibAngles[i].y);
        cv::Point2f actual = m_fake3D_calibScreens[i];

        // 计算误差
        double error = cv::norm(predicted - actual);
        totalError += error;
        validPoints++;

        // 计算准确度（基于误差的百分比）
        double maxDistance = sqrt(screenWidth*screenWidth + screenHeight*screenHeight);
        double accuracy = max(0.0, (1.0 - error/maxDistance) * 100.0);

        // 存储点的详细信息（匹配精确标定格式）
        double errorMM = error * MM_PER_PIXEL;
        char pointInfo[100];
        snprintf(pointInfo, sizeof(pointInfo), "P%d: %.1fpx/%.1fmm",
                i+1, error, errorMM);
        pointDetails.push_back(std::string(pointInfo));

        // 输出误差信息用于调试
        qDebug() << QString("[Fake3D误差图] 点%1: 实际(%2,%3) 预测(%4,%5) 误差:%6px 准确度:%7%")
                    .arg(i+1).arg(actual.x).arg(actual.y)
                    .arg(predicted.x).arg(predicted.y).arg(error, 0, 'f', 1).arg(accuracy, 0, 'f', 1);

        // 绘制透明误差圆（半径为误差大小）
        if (error > 0 && actual.x >= TEXT_AREA_WIDTH && actual.x < screenWidth &&
            actual.y >= 0 && actual.y < screenHeight) {
            int radius = static_cast<int>(error);

            // 绘制半透明误差圆
            cv::Mat overlay = plot.clone();
            cv::circle(overlay, cv::Point(actual.x, actual.y), radius, CIRCLE_COLOR, -1);
            cv::addWeighted(plot, 0.7, overlay, 0.3, 0, plot);
        }

        // 绘制标定点（红色）
        cv::circle(plot, cv::Point(actual.x, actual.y), 5, cv::Scalar(0, 0, 255), -1);

        // 绘制预测点（绿色）
        if (predicted.x >= TEXT_AREA_WIDTH && predicted.x < screenWidth &&
            predicted.y >= 0 && predicted.y < screenHeight) {
            cv::circle(plot, cv::Point(predicted.x, predicted.y), 3, cv::Scalar(0, 255, 0), -1);
        }

        // 绘制误差向量（蓝色箭头）
        if (predicted.x >= TEXT_AREA_WIDTH && predicted.x < screenWidth &&
            predicted.y >= 0 && predicted.y < screenHeight &&
            actual.x >= TEXT_AREA_WIDTH && actual.x < screenWidth &&
            actual.y >= 0 && actual.y < screenHeight) {
            cv::arrowedLine(plot, cv::Point(predicted.x, predicted.y),
                           cv::Point(actual.x, actual.y), cv::Scalar(255, 0, 0), 1);
        }
    }

    // 绘制文本信息
    int currentY = TEXT_START_Y;

    // 标题
    cv::putText(plot, "Fake3D Calibration Error Analysis",
                cv::Point(TEXT_START_X, currentY), cv::FONT_HERSHEY_SIMPLEX, 0.6, TEXT_COLOR, 1);
    currentY += LINE_HEIGHT + 10;

    // 分割线
    cv::line(plot, cv::Point(TEXT_START_X, currentY - 5),
             cv::Point(TEXT_AREA_WIDTH - 10, currentY - 5), TEXT_COLOR, 1);
    currentY += 10;

    // 统计信息
    if (validPoints > 0) {
        double avgError = totalError / validPoints;
        double avgErrorMM = avgError * MM_PER_PIXEL;
        double avgErrorDegrees = atan(avgErrorMM / VIEWING_DISTANCE_MM) * 180.0 / CV_PI;

        QString mappingMethod = getPolynomialMethodName(plType);

        // 显示统计信息
        cv::putText(plot, QString("Calibration Points: %1").arg(validPoints).toStdString(),
                    cv::Point(TEXT_START_X, currentY), cv::FONT_HERSHEY_SIMPLEX, 0.4, TEXT_COLOR, 1);
        currentY += LINE_HEIGHT;

        cv::putText(plot, QString("Mapping Method: %1").arg(mappingMethod).toStdString(),
                    cv::Point(TEXT_START_X, currentY), cv::FONT_HERSHEY_SIMPLEX, 0.4, TEXT_COLOR, 1);
        currentY += LINE_HEIGHT;

        cv::putText(plot, QString("Parameters: %1").arg(unknowns * 2).toStdString(),
                    cv::Point(TEXT_START_X, currentY), cv::FONT_HERSHEY_SIMPLEX, 0.4, TEXT_COLOR, 1);
        currentY += LINE_HEIGHT + 10;

        // 计算平均准确度
        double totalAccuracy = 0.0;
        double maxDistance = sqrt(screenWidth*screenWidth + screenHeight*screenHeight);
        for (int i = 0; i < m_fake3D_calibAngles.size(); ++i) {
            cv::Point2f predicted = applyFake3DMultiPointMapping(
                m_fake3D_calibAngles[i].x, m_fake3D_calibAngles[i].y);
            cv::Point2f actual = m_fake3D_calibScreens[i];
            double error = cv::norm(predicted - actual);
            double accuracy = max(0.0, (1.0 - error/maxDistance) * 100.0);
            totalAccuracy += accuracy;
        }
        double avgAccuracy = totalAccuracy / validPoints;

        // 匹配精确标定的准确度显示格式
        cv::putText(plot, "Accuracy (avg error):",
                    cv::Point(TEXT_START_X, currentY), cv::FONT_HERSHEY_SIMPLEX, 0.5, METRIC_COLOR, 1);
        currentY += LINE_HEIGHT;

        // 显示每个点的详细信息
        for (size_t i = 0; i < pointDetails.size() && currentY < screenHeight - 40; ++i) {
            cv::putText(plot, pointDetails[i],
                        cv::Point(TEXT_START_X, currentY), cv::FONT_HERSHEY_SIMPLEX, 0.45, TEXT_COLOR, 1);
            currentY += LINE_HEIGHT;
        }

        // 显示平均误差和准确度（匹配精确标定格式）
        currentY += 5;
        cv::putText(plot, QString("%1 px / %2 mm").arg(avgError, 0, 'f', 1).arg(avgErrorMM, 0, 'f', 1).toStdString(),
                    cv::Point(TEXT_START_X + 10, currentY), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);
        currentY += LINE_HEIGHT;

        cv::putText(plot, QString("%1 degrees").arg(avgErrorDegrees, 0, 'f', 3).toStdString(),
                    cv::Point(TEXT_START_X + 10, currentY), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);
        currentY += LINE_HEIGHT;

        // 显示准确度
        cv::putText(plot, QString("Accuracy: %1%").arg(avgAccuracy, 0, 'f', 1).toStdString(),
                    cv::Point(TEXT_START_X + 10, currentY), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 150, 0), 1);
    }

    return plot;
}

// 【新增】计算假3D平均误差
double MainWindow::calculateFake3DAverageError() {
    if (m_fake3D_calibAngles.empty() || m_fake3D_calibScreens.empty() || !m_fake3D_multiPointCalibrated) {
        qDebug() << "[计算假3D平均误差] 假3D数据为空，尝试使用3D标定数据重新计算";

        // 如果假3D数据为空，尝试使用3D标定数据重新计算
        if (m_3DCalibrationPupilPoints.empty() || m_3DCalibrationScreenPoints.empty()) {
            qDebug() << "[计算假3D平均误差] 3D标定数据也为空，返回0";
            return 0.0;
        }

        // 重新计算假3D角度（与generateFake3DErrorPlot中的逻辑相同）
        auto eyeGeometry = worker ? worker->getReal3DEyeballGeometry() : Worker::EyeballGeometry{};
        if (!eyeGeometry.valid || !worker) {
            qDebug() << "[计算假3D平均误差] 无法获取眼球几何信息，返回0";
            return 0.0;
        }

        std::vector<cv::Point2f> tempAngles;
        std::vector<cv::Point2f> tempScreens;

        for (int i = 0; i < m_3DCalibrationPupilPoints.size() && i < m_3DCalibrationScreenPoints.size(); ++i) {
            cv::Point2f pupilPoint = m_3DCalibrationPupilPoints[i];
            cv::Point2f screenPoint = m_3DCalibrationScreenPoints[i];

            // 重新计算角度
            double image_center_x = eyeImage.cols / 2.0;
            double image_center_y = eyeImage.rows / 2.0;
            double focal_length = eyeGeometry.focal_length;

            cv::Point3f ray_direction(
                (pupilPoint.x - image_center_x) / focal_length,
                (pupilPoint.y - image_center_y) / focal_length,
                -1.0
            );
            cv::Point3f ray_origin(0, 0, 0);
            cv::Point3f eye_center = eyeGeometry.center_3d;
            double eye_radius = eyeGeometry.radius;

            cv::Point3f oc = ray_origin - eye_center;
            double a = ray_direction.dot(ray_direction);
            double b = 2.0 * oc.dot(ray_direction);
            double c = oc.dot(oc) - eye_radius * eye_radius;
            double discriminant = b * b - 4 * a * c;

            if (discriminant >= 0 && a != 0) {
                double t = (-b + sqrt(discriminant)) / (2 * a);
                if (t > 0) {
                    cv::Point3f pupil_3d = ray_origin + static_cast<float>(t) * ray_direction;
                    cv::Point3f normal_vector = pupil_3d - eye_center;
                    double normal_length = sqrt(normal_vector.x*normal_vector.x +
                                               normal_vector.y*normal_vector.y +
                                               normal_vector.z*normal_vector.z);

                    if (normal_length > 1e-6) {
                        double nx = normal_vector.x / normal_length;
                        double ny = normal_vector.y / normal_length;
                        double nz = normal_vector.z / normal_length;

                        // 【关键修复】使用与推理时完全一致的角度计算公式
                        double alpha = std::atan2(nz, ny) * 180.0 / CV_PI;  // Pitch: 与推理时一致
                        double beta = std::atan2(nz, nx) * 180.0 / CV_PI;   // Yaw: 与推理时一致

                        tempAngles.push_back(cv::Point2f(alpha, beta));
                        tempScreens.push_back(screenPoint);
                    }
                }
            }
        }

        if (tempAngles.empty()) {
            qDebug() << "[计算假3D平均误差] 重新计算失败，返回0";
            return 0.0;
        }

        // 临时使用重新计算的数据
        m_fake3D_calibAngles = tempAngles;
        m_fake3D_calibScreens = tempScreens;
        m_fake3D_multiPointCalibrated = true;

        qDebug() << QString("[计算假3D平均误差] 使用重新计算的数据: %1个点").arg(tempAngles.size());
    }

    double totalError = 0.0;
    int validPoints = 0;

    for (int i = 0; i < m_fake3D_calibAngles.size(); ++i) {
        // 使用多点映射预测屏幕坐标
        cv::Point2f predicted = applyFake3DMultiPointMapping(
            m_fake3D_calibAngles[i].x, m_fake3D_calibAngles[i].y);
        cv::Point2f actual = m_fake3D_calibScreens[i];

        // 计算误差
        double error = cv::norm(predicted - actual);
        totalError += error;
        validPoints++;
    }

    return validPoints > 0 ? totalError / validPoints : 0.0;
}

// 【修复】收集假3D标定数据
void MainWindow::collectFake3DCalibrationData() {
    qDebug() << "[collectFake3DCalibrationData] 开始收集假3D标定数据";

    // 清空现有数据
    m_fake3D_calibAngles.clear();
    m_fake3D_calibScreens.clear();

//    QMessageBox::information(this, "步骤1.1-数据检查",
//        QString("collectFake3DCalibrationData()开始\nm_3DCalibrationPupilPoints: %1个\nm_3DCalibrationScreenPoints: %2个")
//        .arg(m_3DCalibrationPupilPoints.size()).arg(m_3DCalibrationScreenPoints.size()));

    // 检查3D标定数据是否有效
    if (m_3DCalibrationPupilPoints.empty() || m_3DCalibrationScreenPoints.empty()) {
        qDebug() << "[collectFake3DCalibrationData] 3D标定数据为空，无法收集假3D数据";

        return;
    }

    // 检查worker和眼球几何信息
    if (!worker) {
        qDebug() << "[collectFake3DCalibrationData] worker为空，无法获取眼球几何信息";

        return;
    }

    auto eyeGeometry = worker->getReal3DEyeballGeometry();
    if (!eyeGeometry.valid) {
        qDebug() << "[collectFake3DCalibrationData] 无法获取有效的眼球几何信息";
//        QMessageBox::warning(this, "步骤1.4-眼球几何无效", "无法获取有效的眼球几何信息（假3D模式下需要真3D的眼球模型）");
        return;
    }

//    qDebug() << QString("[collectFake3DCalibrationData] 眼球几何信息: 中心(%.1f,%.1f,%.1f), 半径:%.1f, 焦距:%.1f")
//                .arg(eyeGeometry.center_3d.x).arg(eyeGeometry.center_3d.y).arg(eyeGeometry.center_3d.z)
//                .arg(eyeGeometry.radius).arg(eyeGeometry.focal_length);

//    QMessageBox::information(this, "步骤1.5-开始角度计算",
//        QString("眼球几何信息有效，开始遍历%1个标定点计算角度")
//        .arg(m_3DCalibrationPupilPoints.size()));

    // 遍历3D标定数据，计算对应的假3D角度
    for (int i = 0; i < m_3DCalibrationPupilPoints.size() && i < m_3DCalibrationScreenPoints.size(); ++i) {
        cv::Point2f pupilPoint = m_3DCalibrationPupilPoints[i];
        cv::Point2f screenPoint = m_3DCalibrationScreenPoints[i];

//        qDebug() << QString("[collectFake3DCalibrationData] 处理点%1: 瞳孔(%.1f,%.1f) 屏幕(%.1f,%.1f)")
//                    .arg(i+1).arg(pupilPoint.x).arg(pupilPoint.y).arg(screenPoint.x).arg(screenPoint.y);

        // 计算从瞳孔中心到眼球表面的射线方向（修复后的角度计算）
        double image_center_x = eyeImage.cols / 2.0;
        double image_center_y = eyeImage.rows / 2.0;
        double focal_length = eyeGeometry.focal_length;

        // 【调试】输出角度计算的关键参数
//        if (i < 3) {  // 只显示前3个点的详细信息
//            QString angleCalcMsg = QString("【标定时角度计算-点%1】\\n").arg(i+1)
//                .append(QString("瞳孔位置: (%.2f, %.2f)\\n").arg(pupilPoint.x).arg(pupilPoint.y))
//                .append(QString("图像中心: (%.2f, %.2f)\\n").arg(image_center_x).arg(image_center_y))
//                .append(QString("焦距: %.2f\\n").arg(focal_length))
//                .append(QString("图像尺寸: %1x%2").arg(eyeImage.cols).arg(eyeImage.rows));
//            QMessageBox::information(this, QString("标定角度计算-点%1").arg(i+1), angleCalcMsg);
//        }

        // 计算射线方向（相机坐标系）- 修复Z方向
        cv::Point3f ray_direction(
            (pupilPoint.x - image_center_x) / focal_length,
            (pupilPoint.y - image_center_y) / focal_length,
            1.0  // 修复：改为正方向
        );

        // 计算射线与眼球表面的交点
        cv::Point3f ray_origin(0, 0, 0);
        cv::Point3f eye_center = eyeGeometry.center_3d;
        double eye_radius = eyeGeometry.radius;

        cv::Point3f oc = ray_origin - eye_center;
        double a = ray_direction.dot(ray_direction);
        double b = 2.0 * oc.dot(ray_direction);
        double c = oc.dot(oc) - eye_radius * eye_radius;
        double discriminant = b * b - 4 * a * c;

        if (discriminant >= 0 && a != 0) {
            double t = (-b + sqrt(discriminant)) / (2 * a);
            if (t > 0) {
                cv::Point3f pupil_3d = ray_origin + static_cast<float>(t) * ray_direction;
                cv::Point3f normal_vector = pupil_3d - eye_center;
                double normal_length = sqrt(normal_vector.x*normal_vector.x +
                                           normal_vector.y*normal_vector.y +
                                           normal_vector.z*normal_vector.z);

                if (normal_length > 1e-6) {
                    // 修复：使用正确的球面坐标转换公式
                    double nx = normal_vector.x / normal_length;
                    double ny = normal_vector.y / normal_length;
                    double nz = normal_vector.z / normal_length;

                    // 【关键修复】使用与推理时完全一致的角度计算公式
                    double pitch = std::atan2(nz, ny) * 180.0 / CV_PI;  // Pitch: 与推理时一致
                    double yaw = std::atan2(nz, nx) * 180.0 / CV_PI;   // Yaw: 与推理时一致

                    m_fake3D_calibAngles.push_back(cv::Point2f(pitch, yaw));
                    m_fake3D_calibScreens.push_back(screenPoint);

//                    qDebug() << QString("[收集假3D数据] 点%1: 瞳孔(%.1f,%.1f) → 角度(%.1f°,%.1f°) → 屏幕(%.1f,%.1f)")
//                                .arg(i+1).arg(pupilPoint.x).arg(pupilPoint.y)
//                                .arg(pitch).arg(yaw).arg(screenPoint.x).arg(screenPoint.y);
                } else {
                    qDebug() << QString("[收集假3D数据] 点%1: normal_length太小(%.6f)，跳过").arg(i+1).arg(normal_length);
                }
            } else {
                qDebug() << QString("[收集假3D数据] 点%1: t<=0 (%.6f)，射线方向错误").arg(i+1).arg(t);
            }
        } else {
            qDebug() << QString("[收集假3D数据] 点%1: 射线与球面无交点，discriminant=%.6f, a=%.6f").arg(i+1).arg(discriminant).arg(a);
        }
    }

    qDebug() << QString("[collectFake3DCalibrationData] 成功收集%1个假3D标定数据点")
                .arg(m_fake3D_calibAngles.size());

//    QMessageBox::information(this, "步骤1.6-收集结果",
//        QString("角度计算完成\n成功收集角度数据: %1个\n成功收集屏幕数据: %2个")
//        .arg(m_fake3D_calibAngles.size()).arg(m_fake3D_calibScreens.size()));
}

// ========== 基于3DTracker逆向过程的屏幕映射 ==========
// mainwindow.cpp

// mainwindow.cpp

cv::Point2f MainWindow::mapGaze3DToScreen(const cv::Point3f& gazeDirection, int screenWidth, int screenHeight) {
    // --- 1. 输入验证和归一化 ---
    float magnitude = cv::norm(gazeDirection);
    if (magnitude < 1e-6) {
//        qDebug() << "[3D映射] 警告：输入方向向量为零向量。";
        return m_is3DZeroCalibrated ? cv::Point2f(screenWidth / 2.0f, screenHeight / 2.0f) : cv::Point2f(-1, -1);
    }
    cv::Point3f dir = gazeDirection / magnitude;

    // --- 2. 计算原始角度 (核心修改) ---
    // 尝试使用 asin 来计算水平角，它对输入的变化没有 atan2 那么敏感。
    // asin(x) 直接给出了向量与YZ平面的夹角，可以作为水平角。
    float raw_azimuth_rad = asin(dir.x);

    // 仰角计算保持不变
    float raw_elevation_rad = asin(dir.y);

    // --- 3. 应用零点标定校准 ---
    float calibrated_azimuth_rad = raw_azimuth_rad;
    float calibrated_elevation_rad = raw_elevation_rad;
    if (m_is3DZeroCalibrated) {
        calibrated_azimuth_rad -= m_zeroAzimuthOffset;
        calibrated_elevation_rad -= m_zeroElevationOffset;
    }

    // --- 4. 归一化校准后的角度 (核心修改) ---
    // 【关键修复】基于实际角度数据扩大视角范围
    const float max_azimuth_rad = 120.0f * M_PI / 180.0f;   // 水平视角范围 ±120度 (容纳±94度)
    const float max_elevation_rad = 100.0f * M_PI / 180.0f; // 垂直视角范围 ±100度 (容纳±73度)

       float normalized_x = max(-1.0f, min(1.0f, calibrated_azimuth_rad / max_azimuth_rad));
       float normalized_y = max(-1.0f, min(1.0f, calibrated_elevation_rad / max_elevation_rad));

    // --- 5. 将归一化坐标映射到屏幕像素坐标 (保持修正后的版本) ---
    float screen_x = (normalized_x + 1.0f) / 2.0f * screenWidth;
    float screen_y = (1.0f - normalized_y) / 2.0f * screenHeight;

    // --- 6. 调试日志输出 ---
    static int debug_counter = 0;
    if (++debug_counter % 15 == 0) { // 增加日志频率以更好地观察
//        qDebug() << QString("[3D映射] Dir(%.2f,%.2f,%.2f) -> RawAng(Az:%.1f,El:%.1f) -> Norm(%.2f,%.2f) -> Screen(%.0f,%.0f)")
//                        .arg(dir.x).arg(dir.y).arg(dir.z)
//                        .arg(raw_azimuth_rad * 180/M_PI)
//                        .arg(raw_elevation_rad * 180/M_PI)
//                        .arg(normalized_x)
//                        .arg(normalized_y)
//                        .arg(screen_x)
//                        .arg(screen_y);
    }

    return cv::Point2f(screen_x, screen_y);
}



// ========== 3D视线可视化绘制 ==========
void MainWindow::draw3DGazeVisualization(cv::Mat& eyeImage, const cv::Point2f& pupilCenter, const cv::RotatedRect& pupilEllipse) {
//    qDebug() << "[draw3DGazeVisualization] 函数调用 - 瞳孔位置:" << pupilCenter.x << "," << pupilCenter.y;
//    qDebug() << "[draw3DGazeVisualization] 眼睛图像尺寸:" << eyeImage.cols << "x" << eyeImage.rows;
//    qDebug() << "[draw3DGazeVisualization] 瞳孔中心:" << pupilCenter.x << "," << pupilCenter.y;
//    qDebug() << "[draw3DGazeVisualization] 眼球中心:" << m_currentEyeSphereCenter.x << "," << m_currentEyeSphereCenter.y;

    if (!m_gaze3DCalculator) {
//        qDebug() << "[draw3DGazeVisualization] 错误：计算器为空";
        return;
    }

    if (pupilCenter.x < 0 || pupilCenter.y < 0) {
//        qDebug() << "[draw3DGazeVisualization] 错误：瞳孔中心无效";
        return;
    }

    try {
//        qDebug() << "[draw3DGazeVisualization] 开始计算3D注视向量";

        // 【关键修改】假3D模式使用真3D眼球球心
        cv::Point2f eyeCenter;
        bool useReal3DCenter = false;

        // 【关键修改】尝试获取真3D眼球球心，不检查真3D模式是否启用
        if (worker && worker->isReal3DModelBuilt()) {
            cv::Point2f real3DCenter = worker->getReal3DEyeCenter();
            if (real3DCenter.x > 0 && real3DCenter.y > 0) {
                eyeCenter = real3DCenter;
                useReal3DCenter = true;
//                qDebug() << "[draw3DGazeVisualization] ✅ 假3D使用真3D眼球球心:" << eyeCenter.x << "," << eyeCenter.y;
            } else {
//                qDebug() << "[draw3DGazeVisualization] ⚠️ 真3D球心无效:" << real3DCenter.x << "," << real3DCenter.y;
            }
        } else {
//            qDebug() << "[draw3DGazeVisualization] ⚠️ Worker不存在或3D模型未构建";
        }

        // 如果无法获取真3D球心，回退到假3D计算的球心
        if (!useReal3DCenter) {
            eyeCenter = m_currentEyeSphereCenter;

            // 如果假3D球心也无效，使用图像中心作为默认值
            if (eyeCenter.x <= 0 || eyeCenter.y <= 0) {
                eyeCenter = cv::Point2f(eyeImage.cols / 2.0f, eyeImage.rows / 2.0f);
//                qDebug() << "[draw3DGazeVisualization] 使用默认眼球中心:" << eyeCenter.x << "," << eyeCenter.y;
            } else {
//                qDebug() << "[draw3DGazeVisualization] 使用假3D眼球中心:" << eyeCenter.x << "," << eyeCenter.y;
            }
        }

        // 【修正】使用真正的3D计算结果，不再使用简单的2D方法
        GazeVector3D gaze3D;

        // 直接使用computeGazeVector计算真正的3D方向向量
        gaze3D = computeGazeVector(pupilCenter, eyeCenter, eyeImage.cols, eyeImage.rows);

//        qDebug() << QString("[draw3DGazeVisualization] 3D计算结果 - 有效:%1, 方向:(%2,%3,%4)")
//                    .arg(gaze3D.valid)
//                    .arg(gaze3D.direction.x).arg(gaze3D.direction.y).arg(gaze3D.direction.z);

        // 【新增】假3D角度计算 - 使用真3D眼球模型几何信息
//        qDebug() << QString("[draw3DGazeVisualization] 条件检查: gaze3D.valid=%1, worker=%2, real3DModelBuilt=%3")
//                    .arg(gaze3D.valid ? "true" : "false")
//                    .arg(worker ? "true" : "false")
//                    .arg(worker && worker->isReal3DModelBuilt() ? "true" : "false");

        if (gaze3D.valid && worker && worker->isReal3DModelBuilt()) {
            // 获取真3D眼球模型的几何信息
            auto eyeGeometry = worker->getReal3DEyeballGeometry();

            if (eyeGeometry.valid) {
                // 基于真3D眼球几何重建瞳孔圆的法线向量

                // 1. 将2D瞳孔中心投影到3D空间
                double focal_length = eyeGeometry.focal_length;
                // 动态获取图像中心，而不是硬编码
                double image_center_x = eyeImage.cols / 2.0;
                double image_center_y = eyeImage.rows / 2.0;

                // 计算射线方向（与多点标定完全一致，不进行归一化）
                cv::Point3f ray_direction(
                    (pupilCenter.x - image_center_x) / focal_length,
                    (pupilCenter.y - image_center_y) / focal_length,
                    1.0
                );

                // 2. 计算射线与眼球表面的交点
                cv::Point3f eye_center = eyeGeometry.center_3d;
                double eye_radius = eyeGeometry.radius;

                // 射线方程: P = ray_origin + t * ray_direction
                // 射线起点（相机位置）
                cv::Point3f ray_origin(0, 0, 0);  // 相机坐标系原点

                // 球面方程: |P - eye_center|^2 = radius^2
                // 展开得到二次方程的系数
                cv::Point3f oc = ray_origin - eye_center;  // 从球心到射线起点的向量

                double a = ray_direction.dot(ray_direction);
                double b = 2.0 * oc.dot(ray_direction);
                double c = oc.dot(oc) - eye_radius * eye_radius;

                double discriminant = b*b - 4*a*c;

                if (discriminant >= 0 && a != 0) {
                    // 使用与多点标定完全一致的交点计算方法
                    double t = (-b + sqrt(discriminant)) / (2 * a);

                    if (t > 0) {
                        // 计算3D瞳孔位置
                        cv::Point3f pupil_3d = ray_origin + static_cast<float>(t) * ray_direction;

                        // 3. 计算瞳孔圆的法线向量（从眼球中心指向瞳孔的方向）
                        cv::Point3f normal_vector = pupil_3d - eye_center;
                        double normal_length = sqrt(normal_vector.x*normal_vector.x +
                                                   normal_vector.y*normal_vector.y +
                                                   normal_vector.z*normal_vector.z);

                        if (normal_length > 1e-6) {
                            // 归一化法线向量
                            double nx = normal_vector.x / normal_length;
                            double ny = normal_vector.y / normal_length;
                            double nz = normal_vector.z / normal_length;

                            // 4. 修复角度计算公式 - 交换pitch和yaw的计算
                            // 修正：Pitch应该控制上下，Yaw应该控制左右
                            double alpha = std::atan2(nz, ny) * 180.0 / CV_PI;  // Pitch: 上下方向 (ny控制上下)

                            double beta = std::atan2(nz, nx) * 180.0 / CV_PI;   // Yaw: 左右方向 (nx控制左右)

                            // 【解耦】计算屏幕坐标映射（不受G键控制）
                            cv::Point2f screenCoords(-1, -1);
                            std::string screenText = "Screen: NOT CALIBRATED";

                            // 获取屏幕尺寸
                            QScreen* screen = QGuiApplication::primaryScreen();
                            QRect screenGeometry = screen->geometry();
                            int screenWidth = screenGeometry.width();
                            int screenHeight = screenGeometry.height();

                            // 【解耦修复】优先检查多点标定，然后是单点标定，最后是原始角度
                            if (m_fake3D_multiPointCalibrated) {
                                // 多点标定模式：使用纯多点映射（不含单点校正）
//                                qDebug() << QString("【多点推理调用】准备调用applyFake3DMultiPointMapping, 输入角度: alpha=%.3f°, beta=%.3f°").arg(alpha).arg(beta);
                                screenCoords = applyFake3DMultiPointMapping(alpha, beta);
                                screenText = "Screen: (" + std::to_string((int)screenCoords.x) + "," + std::to_string((int)screenCoords.y) + ") [Multi-Point]";
//                                qDebug() << QString("[解耦显示] 使用多点映射，原始角度(%.1f°,%.1f°) -> 屏幕(%.0f,%.0f)")
//                                            .arg(alpha).arg(beta).arg(screenCoords.x).arg(screenCoords.y);
                            } else if (m_fake3D_isCalibrated) {
                                // G键单点标定模式：使用G键校正后的单点映射
                                double calibrated_alpha = alpha - m_fake3D_zeroPitchOffset;
                                double calibrated_beta = beta - m_fake3D_zeroYawOffset;
                                screenCoords = mapFake3DAnglesToScreen(calibrated_alpha, calibrated_beta, screenWidth, screenHeight);
                                screenText = "Screen: (" + std::to_string((int)screenCoords.x) + "," + std::to_string((int)screenCoords.y) + ") [G-key]";
//                                qDebug() << QString("[解耦显示] 使用G键校正，原始角度(%.1f°,%.1f°) -> 校正后(%.1f°,%.1f°) -> 屏幕(%.0f,%.0f)")
//                                            .arg(alpha).arg(beta).arg(calibrated_alpha).arg(calibrated_beta).arg(screenCoords.x).arg(screenCoords.y);
                            } else {
                                // 未标定模式：使用原始角度进行粗略映射
                                screenCoords = mapFake3DAnglesToScreen(alpha, beta, screenWidth, screenHeight);
                                screenText = "Screen: (" + std::to_string((int)screenCoords.x) + "," + std::to_string((int)screenCoords.y) + ") [Raw]";
//                                qDebug() << QString("[解耦显示] 使用原始角度，角度(%.1f°,%.1f°) -> 屏幕(%.0f,%.0f)")
//                                            .arg(alpha).arg(beta).arg(screenCoords.x).arg(screenCoords.y);
                            }

                            // 缓存坐标供其他模块使用
                            m_lastFake3DScreenCoords = screenCoords;

                            // 显示角度和屏幕坐标
                            std::string angleText = "Fake3D - Pitch: " + std::to_string(alpha) + ", Yaw: " + std::to_string(beta);
                            cv::putText(eyeImage, angleText, cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
                            cv::putText(eyeImage, screenText, cv::Point(10, 90), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);

//                            qDebug() << QString("[假3D优化] 图像中心:(%.1f,%.1f), t值:%.3f, 瞳孔3D:(%.3f,%.3f,%.3f)")
//                                        .arg(image_center_x).arg(image_center_y).arg(t)
//                                        .arg(pupil_3d.x).arg(pupil_3d.y).arg(pupil_3d.z);
//                            qDebug() << QString("[假3D优化] Pitch: %.2f°, Yaw: %.2f°, 法线:(%.3f,%.3f,%.3f)")
//                                        .arg(alpha).arg(beta).arg(nx).arg(ny).arg(nz);
                        } else {
//                            qDebug() << "[假3D角度] ⚠️ 法线向量长度过小";
                        }
                    } else {
//                        qDebug() << "[假3D角度] ⚠️ 无有效交点，t值:" << t;
                    }
                } else {
//                    qDebug() << "[假3D角度] ⚠️ 射线与眼球不相交";
                }
            } else {
//                qDebug() << "[假3D角度] ⚠️ 无法获取真3D眼球几何信息";
            }
        }

        // 1. 先绘制瞳孔椭圆（保持与原有显示一致）
        if (pupilEllipse.size.width > 0 && pupilEllipse.size.height > 0) {
            cv::ellipse(eyeImage, pupilEllipse, cv::Scalar(0, 0, 255), 2); // 红色瞳孔椭圆
            cv::circle(eyeImage, pupilCenter, 2, cv::Scalar(255, 255, 255), -1); // 白色瞳孔中心点
        }

        // 计算合适的球体半径（相对于图像尺寸）
        int sphereRadius = min(eyeImage.cols, eyeImage.rows) / 3;  // 动态半径
//        qDebug() << "[draw3DGazeVisualization] 球体半径:" << sphereRadius;

        // 2. 绘制眼球球体圆圈 (使用实际眼球中心位置)
//        cv::circle(eyeImage, eyeCenter, sphereRadius, cv::Scalar(50, 50, 255), 2);  // 红色球体边界

        // 3. 绘制眼球中心点 (使用实际眼球中心位置)
        cv::circle(eyeImage, eyeCenter, 8, cv::Scalar(0, 255, 255), -1);  // 黄色眼球中心

        // 4. 绘制从眼球中心到瞳孔中心的连线
        cv::line(eyeImage, eyeCenter, pupilCenter, cv::Scalar(50, 150, 255), 2);  // 橙色连线

        // 5. 【按照temp05 Python代码第393-404行】绘制延伸的注视射线
        // Python代码：
        //   dx = center_x - model_center_average[0]
        //   dy = center_y - model_center_average[1]
        //   extended_x = int(model_center_average[0] + 2 * dx)
        //   extended_y = int(model_center_average[1] + 2 * dy)
        //   cv2.line(frame, (center_x, center_y), (extended_x, extended_y), (200, 255, 0), 3)

        float dx = pupilCenter.x - eyeCenter.x;  // center_x - model_center_average[0]
        float dy = pupilCenter.y - eyeCenter.y;  // center_y - model_center_average[1]
        cv::Point2f pythonExtended(
            eyeCenter.x + 2.0f * dx,  // model_center_average[0] + 2 * dx
            eyeCenter.y + 2.0f * dy   // model_center_average[1] + 2 * dy
        );

        // 绘制Python风格的射线：从瞳孔中心到延伸点，BGR(0,255,200)，粗细3
        cv::line(eyeImage, pupilCenter, pythonExtended, cv::Scalar(0, 255, 200), 3);

//        qDebug() << QString("[draw3DGazeVisualization] Python风格射线 - 眼球:(%1,%2) 瞳孔:(%3,%4) 向量:(%5,%6) 延伸:(%7,%8)")
//                    .arg(eyeCenter.x).arg(eyeCenter.y)
//                    .arg(pupilCenter.x).arg(pupilCenter.y)
//                    .arg(dx).arg(dy)
//                    .arg(pythonExtended.x).arg(pythonExtended.y);

        // 6. 【旧的复杂3D计算】现在注释掉了
        if (gaze3D.valid) {
            // 将3D方向向量投影到2D图像平面上
            // 注意：gaze3D.direction 已经是归一化的3D方向向量
            cv::Point2f gaze2D_direction(gaze3D.direction.x, -gaze3D.direction.y); // Y轴翻转以匹配图像坐标系

            // 归一化2D方向（确保稳定性）
            float length2D = sqrt(gaze2D_direction.x * gaze2D_direction.x + gaze2D_direction.y * gaze2D_direction.y);
            if (length2D > 1e-6f) {
                gaze2D_direction.x /= length2D;
                gaze2D_direction.y /= length2D;

                // 延伸射线到图像边界
                float extensionLength = 150.0f;  // 延伸长度
                cv::Point2f extendedPoint(
                    pupilCenter.x + gaze2D_direction.x * extensionLength,
                    pupilCenter.y + gaze2D_direction.y * extensionLength
                );

                // 确保延伸点在图像范围内
                extendedPoint.x = max(0.0f, min((float)(eyeImage.cols-1), extendedPoint.x));
                extendedPoint.y = max(0.0f, min((float)(eyeImage.rows-1), extendedPoint.y));

                // 【临时】先注释掉复杂的3D计算，用Python的简单方法
                // cv::line(eyeImage, pupilCenter, extendedPoint, cv::Scalar(0, 255, 200), 3);

                // 【删除重复代码】

//                qDebug() << QString("[draw3DGazeVisualization] 3D射线绘制成功 - 从:(%1,%2) 到:(%3,%4)")
//                            .arg(pupilCenter.x).arg(pupilCenter.y)
//                            .arg(extendedPoint.x).arg(extendedPoint.y);
            } else {
//                qDebug() << "[draw3DGazeVisualization] ⚠️ 3D方向向量长度过小，跳过绘制";
            }
        } else {
//            qDebug() << "[draw3DGazeVisualization] ⚠️ 3D计算无效，跳过射线绘制";
        }

        // 原先的3D坐标信息文本显示已去除，现在只显示角度信息


        // 7. 在右上角显示模式标识
        std::string modeText = "3D GAZE MODE";
        cv::Point modeTextPos(eyeImage.cols - 150, 25);
        cv::putText(eyeImage, modeText, modeTextPos + cv::Point(1,1),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 2);
        cv::putText(eyeImage, modeText, modeTextPos,
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 2);



    } catch (const std::exception& e) {
//        qDebug() << "[3D可视化] 绘制异常:" << e.what();
    }
}


void MainWindow::moveEvent(QMoveEvent *event)
{
    QMainWindow::moveEvent(event);
    if (m_gazeOverlay) {
        // 主窗口移动时，覆盖窗口也移动到相同位置
        m_gazeOverlay->move(this->pos());
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    // 处理注视覆盖窗口
    if (m_gazeOverlay) {
        m_gazeOverlay->resize(this->size());
    }

    // 【简化修复】只在主窗口过小时设置保护性最小尺寸，其他时候不干预
    static bool preventRecursion = false;
    if (!preventRecursion && (this->width() < 800 || this->height() < 600)) {
        preventRecursion = true;

        QTimer::singleShot(100, [this]() {
            QList<QDockWidget*> allDocks = {m_controlsDock, m_dataDock, m_graphTabDock, m_cameraDock, m_aiStatusDock, m_sceneDock};

            // 只在窗口过小时设置最小尺寸保护
            for (QDockWidget* dock : allDocks) {
                if (dock) {
                    int minWidth = qMax(100, this->width() * 10 / 100);
                    int minHeight = qMax(50, this->height() * 10 / 100);
                    dock->setMinimumSize(minWidth, minHeight);
                }
            }

            preventRecursion = false;
        });
    }

    // 【关键修复】主窗口恢复到正常尺寸时，延迟强制恢复dock尺寸
    if (this->width() >= 800 && this->height() >= 600) {
        QTimer::singleShot(200, this, [this]() {
            restoreDockSizesFromSettings();
        });
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    // 【关键】在窗口显示后重新设置图标，确保系统能正确识别
    static bool firstShow = true;
    if (firstShow) {
        firstShow = false;
//        qDebug() << "=== showEvent: 窗口首次显示，重新设置图标 ===";

        // 延迟100ms再设置，确保窗口完全初始化
        QTimer::singleShot(100, this, [this]() {
            // 优先使用ICO格式（Windows原生支持最好）
            QString icoPath = ":/icons/eye.ico";
            QIcon icoIcon(icoPath);

            if (!icoIcon.isNull()) {
                this->setWindowIcon(icoIcon);
                QApplication::setWindowIcon(icoIcon);

                // 强制窗口更新
                this->update();
                this->repaint();

//                qDebug() << "showEvent延迟设置ICO图标完成";

                // 验证设置结果
                QIcon currentIcon = this->windowIcon();
//                qDebug() << "验证: 窗口图标是否为空:" << currentIcon.isNull();
                if (!currentIcon.isNull()) {
//                    qDebug() << "验证: 图标可用尺寸数量:" << currentIcon.availableSizes().size();
                }
            } else {
//                qDebug() << "showEvent: ICO图标加载失败，尝试PNG";
                QIcon pngIcon(":/icons/eye.png");
                if (!pngIcon.isNull()) {
                    this->setWindowIcon(pngIcon);
                    QApplication::setWindowIcon(pngIcon);
                    this->update();
//                    qDebug() << "showEvent延迟设置PNG图标完成";
                }
            }
        });
    }
}

// mainwindow.cpp (在文件末尾添加)
void MainWindow::closeEvent(QCloseEvent *event)
{
    // 【改进】使用增强的窗口状态保存机制
    saveWindowPositions();
    saveSessionState();

    // 为了兼容性，保留原有的保存逻辑
    QSettings settings("EyeTracker", "MainWindow");

//    qDebug() << "【保存】设置文件位置:" << settings.fileName();
//    qDebug() << "【保存前】窗口位置:" << pos() << "尺寸:" << size();

    QByteArray geometry = saveGeometry();
    QByteArray windowState = saveState();

    settings.setValue("geometry", geometry);
    settings.setValue("windowState", windowState);

    // 【新增】保存各个dock的具体宽度和高度
    if (m_dataDock) {
        int width = m_dataDock->width();
        int height = m_dataDock->height();  // 添加高度保存
        settings.setValue("dataDockWidth", width);
        settings.setValue("dataDockHeight", height);  // 保存高度
//        qDebug() << "【保存】数据分析窗口宽度:" << width;
    }

    if (m_controlsDock) {
        int width = m_controlsDock->width();
        int height = m_controlsDock->height();  // 添加高度保存
        settings.setValue("controlsDockWidth", width);
        settings.setValue("controlsDockHeight", height);  // 保存高度
//        qDebug() << "【保存】控件窗口宽度:" << width;
    }

    if (m_cameraDock) {
        int width = m_cameraDock->width();
        int height = m_cameraDock->height();  // 添加高度保存
        settings.setValue("cameraDockWidth", width);
        settings.setValue("cameraDockHeight", height);  // 保存高度
//        qDebug() << "【保存】摄像头控制窗口宽度:" << width;
    }

    if (m_aiStatusDock) {
        int width = m_aiStatusDock->width();
        int height = m_aiStatusDock->height();
        settings.setValue("aiStatusDockWidth", width);
        settings.setValue("aiStatusDockHeight", height);
//        qDebug() << "【保存】AI状态窗口宽度:" << width;
    }

    if (m_graphTabDock) {
        int width = m_graphTabDock->width();
        int height = m_graphTabDock->height();  // 添加高度保存
        settings.setValue("graphTabDockWidth", width);
        settings.setValue("graphTabDockHeight", height);  // 保存高度
//        qDebug() << "【保存】图表分析窗口宽度:" << width;
    }

    settings.sync();

//    qDebug() << "【保存】几何数据大小:" << geometry.size() << "字节";
//    qDebug() << "【保存】窗口状态数据大小:" << windowState.size() << "字节";
//    qDebug() << "【保存完成】";

    QMainWindow::closeEvent(event);
}

// 【新增】专门用于强制恢复dock尺寸的函数
void MainWindow::restoreDockSizesFromSettings()
{
    static bool isRestoring = false;
    if (isRestoring) return; // 防止重复调用

    isRestoring = true;

    QSettings dockSettings("EyeTracker", "MainWindow");
    QList<QDockWidget*> allDocks = {m_controlsDock, m_dataDock, m_graphTabDock, m_cameraDock, m_aiStatusDock, m_sceneDock};

    // 先重置所有dock的最小尺寸约束
    for (QDockWidget* dock : allDocks) {
        if (dock) {
            dock->setMinimumSize(100, 50);
            dock->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        }
    }

    // 延迟50ms后恢复具体尺寸
    QTimer::singleShot(50, [this]() {
        QSettings dockSettings("EyeTracker", "MainWindow");
        // 恢复数据分析窗口
        if (m_dataDock && dockSettings.contains("dataDockWidth")) {
            int width = dockSettings.value("dataDockWidth").toInt();
            int height = dockSettings.value("dataDockHeight", 600).toInt();
            if (width > 100 && height > 100) {
                resizeDocks({m_dataDock}, {width}, Qt::Horizontal);
                resizeDocks({m_dataDock}, {height}, Qt::Vertical);
            }
        }

        // 恢复控件窗口
        if (m_controlsDock && dockSettings.contains("controlsDockWidth")) {
            int width = dockSettings.value("controlsDockWidth").toInt();
            int height = dockSettings.value("controlsDockHeight", 400).toInt();
            if (width > 100 && height > 100) {
                resizeDocks({m_controlsDock}, {width}, Qt::Horizontal);
                resizeDocks({m_controlsDock}, {height}, Qt::Vertical);
            }
        }

        // 恢复摄像头控制窗口
        if (m_cameraDock && dockSettings.contains("cameraDockWidth")) {
            int width = dockSettings.value("cameraDockWidth").toInt();
            int height = dockSettings.value("cameraDockHeight", 300).toInt();
            if (width > 100 && height > 100) {
                resizeDocks({m_cameraDock}, {width}, Qt::Horizontal);
                resizeDocks({m_cameraDock}, {height}, Qt::Vertical);
            }
        }

        // 恢复AI状态窗口
        if (m_aiStatusDock && dockSettings.contains("aiStatusDockWidth")) {
            int width = dockSettings.value("aiStatusDockWidth").toInt();
            int height = dockSettings.value("aiStatusDockHeight", 400).toInt();
            if (width > 100 && height > 100) {
                resizeDocks({m_aiStatusDock}, {width}, Qt::Horizontal);
                resizeDocks({m_aiStatusDock}, {height}, Qt::Vertical);
            }
        }

        // 恢复图表分析窗口
        if (m_graphTabDock && dockSettings.contains("graphTabDockWidth")) {
            int width = dockSettings.value("graphTabDockWidth").toInt();
            int height = dockSettings.value("graphTabDockHeight", 400).toInt();
            if (width > 100 && height > 100) {
                resizeDocks({m_graphTabDock}, {width}, Qt::Horizontal);
                resizeDocks({m_graphTabDock}, {height}, Qt::Vertical);
            }
        }

        isRestoring = false;
    });
}

// 确保重要的dock窗口在启动后正确显示
void MainWindow::ensureDocksVisible()
{
    // 只显示应该默认显示的dock，不强制显示所有dock
    if (m_dataDock && !m_dataDock->isVisible()) {
        m_dataDock->setVisible(true);
    }

    if (m_controlsDock && !m_controlsDock->isVisible()) {
        m_controlsDock->setVisible(true);
    }

    // 摄像头dock和图表dock通常默认是隐藏的，除非用户之前设置为显示
    // 这里不强制显示它们
}


// MainWindow.cpp

// 将此函数实现添加到您的 .cpp 文件中
void MainWindow::initializeGazeKalmanFilter()
{
    // 状态向量: [x, y, vx, vy] (位置和速度)
    int stateSize = 4;
    // 测量向量: [x, y] (我们只测量位置)
    int measurementSize = 2;
    int controlSize = 0;

    m_gazeKalmanFilter.init(stateSize, measurementSize, controlSize, CV_32F);

    // 状态转移矩阵 A: 描述状态如何随时间演变
    // 我们将在每次更新时动态设置dt，这里先初始化为单位阵
    cv::setIdentity(m_gazeKalmanFilter.transitionMatrix);

    // 测量矩阵 H: 描述如何从状态中获取测量值
    m_gazeKalmanFilter.measurementMatrix = cv::Mat::zeros(measurementSize, stateSize, CV_32F);
    m_gazeKalmanFilter.measurementMatrix.at<float>(0, 0) = 1.0f;
    m_gazeKalmanFilter.measurementMatrix.at<float>(1, 1) = 1.0f;

    // --- 关键调优参数 ---
    // 过程噪声协方差 Q: 模型预测的不确定性。值越小，轨迹越平滑。
    cv::setIdentity(m_gazeKalmanFilter.processNoiseCov, cv::Scalar::all(1e-2));

    // 测量噪声协方差 R: 测量值(原始gaze)的不确定性。值越大，对测量的信任度越低，轨迹也越平滑。
    cv::setIdentity(m_gazeKalmanFilter.measurementNoiseCov, cv::Scalar::all(5e-2)); // 初始值设为5e-2，比之前稍大，平滑效果更明显

    // 初始误差协方差 P
    cv::setIdentity(m_gazeKalmanFilter.errorCovPost, cv::Scalar::all(1));

    // 初始化测量值矩阵
    m_gazeKalmanMeasurement = cv::Mat(measurementSize, 1, CV_32F);
    m_gazeKalmanMeasurement.setTo(cv::Scalar(0));

    m_isKalmanFilterInitialized = false;
    m_lastValidGaze = cv::Point2f(-1, -1);
}



// MainWindow.cpp

// 将此函数实现添加到您的 .cpp 文件中
cv::Point2f MainWindow::applyGazeKalmanFilter(const cv::Point2f& gaze, float pupilConfidence)
{
    // --- 动态更新dt，提高预测精度 ---
    float dt = m_frameTimer.elapsed() / 1000.0f; // 获取自上一帧以来的秒数
    if (dt < 1e-6) dt = 1.0f / 60.0f; // 防止dt为0
    m_frameTimer.restart(); // 重置计时器
    m_gazeKalmanFilter.transitionMatrix.at<float>(0, 2) = dt;
    m_gazeKalmanFilter.transitionMatrix.at<float>(1, 3) = dt;

    // --- 根据瞳孔置信度动态调整测量噪声协方差 ---
    // 置信度越小，测量噪声越大，滤波越平滑
    // 置信度范围 [0, 1]，噪声系数范围 [0.05, 5.0]
    float baseNoise = 0.05f;  // 基础噪声（高置信度时）- 更小，更灵敏
    float maxNoise = 5.0f;    // 最大噪声（低置信度时）- 更大，更平滑

    // 使用指数函数增强低置信度时的滤波效果
    float confidenceInverse = 1.0f - pupilConfidence;
    float exponentialFactor = confidenceInverse * confidenceInverse; // 平方增强
    float adaptiveNoise = baseNoise + (maxNoise - baseNoise) * exponentialFactor;

    // 应用自适应测量噪声协方差
    cv::setIdentity(m_gazeKalmanFilter.measurementNoiseCov, cv::Scalar::all(adaptiveNoise));

    // --- 预测阶段 ---
    // 无论输入是否有效，都先进行预测。这是实现"实时性"的关键。
    cv::Mat prediction = m_gazeKalmanFilter.predict();
    cv::Point2f predictedPoint(prediction.at<float>(0), prediction.at<float>(1));

    // --- 校正阶段 ---
    // 判断输入的gaze是否有效
    bool isGazeValid = (gaze.x >= 0 && gaze.y >= 0);

    if (isGazeValid) {
        // 如果gaze有效，用它来校正模型
        m_lastValidGaze = gaze; // 记录最后一个有效的原始gaze

        // 首次初始化
        if (!m_isKalmanFilterInitialized) {
            m_gazeKalmanFilter.statePost.at<float>(0) = gaze.x;
            m_gazeKalmanFilter.statePost.at<float>(1) = gaze.y;
            m_gazeKalmanFilter.statePost.at<float>(2) = 0;
            m_gazeKalmanFilter.statePost.at<float>(3) = 0;
            m_isKalmanFilterInitialized = true;
            return gaze;
        }

        // 用当前测量值更新并校正
        m_gazeKalmanMeasurement.at<float>(0) = gaze.x;
        m_gazeKalmanMeasurement.at<float>(1) = gaze.y;
        cv::Mat estimated = m_gazeKalmanFilter.correct(m_gazeKalmanMeasurement);

        // 调试信息：显示置信度和自适应噪声
        static int debugCounter = 0;
        if (++debugCounter % 30 == 0) { // 每30帧输出一次
//            qDebug() << "[自适应卡尔曼] 置信度:" << pupilConfidence
//                     << " 噪声系数:" << adaptiveNoise
//                     << " 滤波强度:" << (adaptiveNoise > 0.2f ? "强" : "弱");
        }

        return cv::Point2f(estimated.at<float>(0), estimated.at<float>(1));

    } else {
        // 瞳孔无效时使用预测值
        if (m_isKalmanFilterInitialized) {
            return predictedPoint;
        }
        return cv::Point2f(-1, -1);
    }
}



// 新增函数：保存精确标定模式的参数
bool MainWindow::saveJingqueCalibrationParams()
{
    // 检查系数是否有效
    if (rcx.empty() || rcy.empty() || rcx.rows != unknowns || rcy.rows != unknowns) {
//        qDebug() << "错误: 标定系数无效或数量不正确，无法保存。";
        return false;
    }

    FILE* fp = fopen(".//jingque_matrix.txt", "w");
    if (fp == NULL) {
        return false; // 文件打开失败
    }

    // 写入文件头，标明参数数量和多项式类型，便于将来扩展
    fprintf(fp, " unknowns: %d\n", unknowns);
    fprintf(fp, " plType: %d\n", plType);

    // 逐行写入 rcx 和 rcy 的系数，用空格隔开
    for (int i = 0; i < unknowns; ++i) {
        fprintf(fp, "%lf %lf\n", rcx.at<double>(i), rcy.at<double>(i));
    }

    fclose(fp);
    return true;
}


// 文件: mainwindow.cpp (可以添加到文件的末尾)

// 在 mainwindow.cpp 文件末尾添加这个新函数
void MainWindow::performSinglePointCorrection()
{
    // 1. 前置条件检查：确保处于正确的模式且基础标定已完成
    if (flag_calibration_status != 0 || !calibration_success) {
//        qDebug() << "单点校正失败：必须在精确标定模式下，并已完成一次基础标定。";
        QMessageBox::warning(this, "操作无效", "请先完成一次精确标定，再使用单点误差校正功能。");
        return;
    }

    // 2. 检查是否能获取到有效的瞳孔位置
    if (eyeVector.x < 0 || eyeVector.y < 0) {
//        qDebug() << "单点校正失败：无法获取有效的瞳孔位置。";
        QMessageBox::warning(this, "校正失败", "无法检测到瞳孔，请确保眼睛在摄像头视野内。");
        return;
    }

//    qDebug() << "开始单点误差校正... 用户应注视当前鼠标位置。";

    // 3. 获取当前鼠标在屏幕上的位置作为“真实目标点”
    QPoint mousePos = QCursor::pos();
    cv::Point2f targetPoint(mousePos.x(), mousePos.y());

    // 4. 计算当前瞳孔位置对应的、未经补偿的屏幕坐标
    // 我们直接调用 evaluate，它内部会使用已有的标定系数
    cv::Point2f rawGazePoint = evaluate(eyeVector);

    // 5. 计算当前帧的误差向量 (目标点 - 实际注视点)
    cv::Point2f currentError = targetPoint - rawGazePoint;

    // 6. 更新全局误差补偿向量
    // 为了更稳定，可以使用低通滤波（EMA）来平滑地更新全局误差，而不是直接覆盖。
    // 这可以防止单次不准确的注视对整体补偿造成太大影响。
    const float alpha = 1.0f; // 平滑系数，值越小，更新越平滑
    m_globalErrorOffset.x = currentError.x;
    m_globalErrorOffset.y = currentError.y ;

//    qDebug() << "单点误差校正成功！";
//    qDebug() << "  鼠标目标点: (" << targetPoint.x << ", " << targetPoint.y << ")";
//    qDebug() << "  原始注视点: (" << rawGazePoint.x << ", " << rawGazePoint.y << ")";
//    qDebug() << "  计算出的误差: (" << currentError.x << ", " << currentError.y << ")";
//    qDebug() << "  更新后全局补偿向量: (" << m_globalErrorOffset.x << ", " << m_globalErrorOffset.y << ")";

    QMessageBox::information(this, "校正成功", QString("误差补偿已更新！\n当前补偿量: (X: %1, Y: %2)")
                             .arg(m_globalErrorOffset.x, 0, 'f', 2)
                             .arg(m_globalErrorOffset.y, 0, 'f', 2));
}

// ========== Worker 线程相关函数实现 ==========

void MainWindow::setupWorkerThread()
{
//    qDebug() << "[Worker] 开始初始化 Worker 线程系统";

        workerThread = new QThread(this); // 父对象设为 this 依然是好的实践
        worker = new Worker();
        worker->moveToThread(workerThread);

        // 设置默认摄像头FPS（将在摄像头实际配置时更新）
        worker->setCameraFps(480.0f);  // 默认480fps

        // 注册元类型
        qRegisterMetaType<ProcessingResult>("ProcessingResult");
        qRegisterMetaType<cv::Mat>("cv::Mat");

        // 连接信号槽
        connect(workerThread, &QThread::started, worker, &Worker::initialize);
        connect(this, &MainWindow::frameProcessingRequested, worker, &Worker::processFrames, Qt::QueuedConnection);
        connect(worker, &Worker::resultReady, this, &MainWindow::handleProcessingResult, Qt::QueuedConnection);
        connect(worker, &Worker::errorOccurred, this, &MainWindow::handleError, Qt::QueuedConnection);

        // 添加线程异常处理
        connect(workerThread, &QThread::finished, this, [this]() {
//            qDebug() << "Worker线程已结束";
        });

        // 【关键修改】确保 Worker 在线程结束后被删除
        connect(workerThread, &QThread::finished, worker, &Worker::deleteLater);

        // 【新增的关键连接】当 MainWindow 销毁时，触发线程的退出
        // 这比在析构函数中手动调用 quit() 更早、更可靠
        connect(this, &MainWindow::destroyed, workerThread, &QThread::quit);

        // 【可选但推荐】如果 worker 的析构很慢，也可以在线程结束后再销毁线程对象
        // connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
        // 但通常 workerThread 作为 MainWindow 的子对象，会自动被销毁，所以这行不是必须的。

        workerThread->start();

        // 暂时禁用多线程处理，使用稳定的单线程模式
        QTimer::singleShot(50, this, [this]() {
            if (worker) {
                // 先使用单线程模式保证稳定性
                worker->enableParallelProcessing(false, 1);
//                qDebug() << "使用单线程模式确保稳定性";
//                qDebug() << "多线程状态:" << worker->isParallelProcessingEnabled();
            }
        });

//        qDebug() << "[Worker] Worker 线程系统初始化完成";
}

void MainWindow::handleProcessingResult(const ProcessingResult& result)
{
    // 添加线程安全检查
    if (!this || !ui || !ui->label_eye || !ui->label_scene) {
//        qDebug() << "UI对象无效，跳过处理";
        return;
    }

    // 检查图像数据有效性
    if (result.processedEyeImage.empty() || result.originalSceneImage.empty()) {
//        qDebug() << "接收到空图像数据，跳过显示";
        return;
    }

    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter % 30 == 1) {
//        qDebug() << "处理帧" << frameCounter << "瞳孔检测:" << result.pupilFound;
    }

    // 单线程模式下确保每帧都更新UI
    static auto lastUIUpdate = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    lastUIUpdate = now;

    m_processingStartTime = std::chrono::high_resolution_clock::now();
    // ===================================================================
    // 阶段一：处理特殊的“演示标定”图像采集帧
    // ===================================================================
    if (result.isForCalibration && flag_calibration_status == 1) {
//        qDebug() << "[演示标定] 接收到标定帧结果，点:" << num_yanshi + 1;
        if (result.pupilFound) {
            QDir().mkpath("./yanshiCalib");
            char eyeName[128], sceneName[128];
            sprintf(eyeName, ".//yanshiCalib//eye_%d.png", num_yanshi);
            sprintf(sceneName, ".//yanshiCalib//scene_%d.png", num_yanshi);

            cv::Mat sceneToSave = result.originalSceneImage.clone();
            cv::cvtColor(sceneToSave, sceneToSave, CV_RGB2BGR);
            putText(sceneToSave, std::to_string(num_yanshi + 1), cv::Point(50, 60),
                    cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(255, 255, 255), 4, 8);

            imwrite(eyeName, result.originalEyeGray);
            imwrite(sceneName, sceneToSave);
//            qDebug() << "已保存标定图像:" << eyeName << "和" << sceneName;

            int totalPoints = (CALIBRATIONPOINTS9 == 10) ? 12 : CALIBRATIONPOINTS9;
            if (num_yanshi >= totalPoints - 1) {
                QMessageBox::information(this, "采集完成", "所有演示标定图像采集完毕，请点击“校准”菜单中的“演示标定”按钮开始手动标定。");
                num_yanshi = 0;
            } else {
                num_yanshi++;
            }
        } else {
            QMessageBox::warning(this, "采集失败", QString("在标定点 %1 未能检测到瞳孔，请重试。").arg(num_yanshi + 1));
        }
        QImage eyeQImage(result.processedEyeImage.data, result.processedEyeImage.cols, result.processedEyeImage.rows, result.processedEyeImage.step, QImage::Format_RGB888);
        ui->label_eye->setPixmap(QPixmap::fromImage(eyeQImage.rgbSwapped()));
        return;
    }

    // ===================================================================
    // 阶段二：常规帧处理 - 更新主线程状态和帧数统计
    // ===================================================================
    m_frameCount++;
    m_processingFrameCount++;
    this->pupilCenter = result.pupilFound ? result.pupilData.center : cv::Point2f(-1, -1);
    this->eyeVector = this->pupilCenter;
    this->glintsDetected = result.glintsFound;
    this->glint1_x = result.glint1.x;
    this->glint1_y = result.glint1.y;
    this->glint2_x = result.glint2.x;
    this->glint2_y = result.glint2.y;

    // ===================================================================
    // 阶段三：常规帧处理 - 数据采集逻辑 (精确模式和3D模式)
    // ===================================================================
    if (result.pupilFound && isCollecting && flag_calibration_status == 0) {
        // 2D精确标定：采集瞳孔位置数据用于多项式拟合
        currentSamplePoints.push_back(result.pupilData.center);
        collectedCount++;
        if (collectedCount >= 10) {
            cv::Point2f finalPoint = processCalibrationSamples(currentSamplePoints);
            if (finalPoint.x > 0) {
                rightPupil.insert(finalPoint);
                Markgaze.insert(predefinedScreenPoints[calibrationIndex]);
                allPupilPoints.push_back(finalPoint);
                allScreenPoints.push_back(predefinedScreenPoints[calibrationIndex]);
            }
            isCollecting = false;
            if (calibrationIndex >= CALIBRATIONPOINTS9 - 1) {
                calibrate();
                calibrationIndex = 0;
            } else {
                calibrationIndex++;
            }
        }
    } else if (result.pupilFound && isCollecting && flag_calibration_status == 2) {
        // 3D模式：采集瞳孔位置数据用于3D几何标定
        currentSamplePoints.push_back(result.pupilData.center);
        collectedCount++;
        if (collectedCount >= 10) {
            cv::Point2f finalPoint = processCalibrationSamples(currentSamplePoints);
            if (finalPoint.x > 0) {
                // 3D模式：收集3D标定数据
                m_3DCalibrationPupilPoints.push_back(finalPoint);
                m_3DCalibrationScreenPoints.push_back(predefinedScreenPoints[calibrationIndex]);

                // 【新增】假3D模式：收集用户实际观看角度数据用于多点标定
                bool shouldCollectFake3D = (worker && !worker->isReal3DModeEnabled());
                qDebug() << QString("[数据收集] worker存在:%1, 假3D模式:%2, 应该收集:%3")
                            .arg(worker != nullptr)
                            .arg(worker ? !worker->isReal3DModeEnabled() : false)
                            .arg(shouldCollectFake3D);

                if (shouldCollectFake3D) {
                    // 使用与实时推理完全相同的角度计算方法
                    auto eyeGeometry = worker->getReal3DEyeballGeometry();
                    if (eyeGeometry.valid) {
                        // 使用与实时推理相同的3D光线相交计算
                        double image_center_x = eyeImage.cols / 2.0;
                        double image_center_y = eyeImage.rows / 2.0;
                        double focal_length = eyeGeometry.focal_length;

                        // 计算射线方向（与标定逻辑一致）
                        cv::Point3f ray_direction(
                            (finalPoint.x - image_center_x) / focal_length,
                            (finalPoint.y - image_center_y) / focal_length,
                            1.0  // 修复：与标定时保持一致
                        );
                        cv::Point3f ray_origin(0, 0, 0);  // 原点

                        cv::Point3f eye_center = eyeGeometry.center_3d;
                        double eye_radius = eyeGeometry.radius;

                        // 光线与眼球相交计算（与实时推理完全相同）
                        cv::Point3f oc = ray_origin - eye_center;
                        double a = ray_direction.dot(ray_direction);
                        double b = 2.0 * oc.dot(ray_direction);
                        double c = oc.dot(oc) - eye_radius * eye_radius;
                        double discriminant = b * b - 4 * a * c;

                        if (discriminant >= 0 && a != 0) {
                            double t = (-b + sqrt(discriminant)) / (2 * a);
                            if (t > 0) {
                                // 计算3D瞳孔位置
                                cv::Point3f pupil_3d = ray_origin + static_cast<float>(t) * ray_direction;

                                // 计算法线向量（与实时推理完全相同）
                                cv::Point3f normal_vector = pupil_3d - eye_center;
                                double normal_length = sqrt(normal_vector.x*normal_vector.x +
                                                           normal_vector.y*normal_vector.y +
                                                           normal_vector.z*normal_vector.z);

                                if (normal_length > 1e-6) {
                                    // 归一化法线向量
                                    double nx = normal_vector.x / normal_length;
                                    double ny = normal_vector.y / normal_length;
                                    double nz = normal_vector.z / normal_length;

                                    // 使用与实时推理完全相同的角度计算公式
                                    double alpha = std::atan2(nz, ny) * 180.0 / CV_PI;  // Pitch
                                    double beta = std::atan2(nz, nx) * 180.0 / CV_PI;   // Yaw

                                    cv::Point2f measuredAngles(alpha, beta);
                                    m_fake3D_calibAngles.push_back(measuredAngles);  // (pitch, yaw)
                                    m_fake3D_calibScreens.push_back(predefinedScreenPoints[calibrationIndex]);

                                    qDebug() << QString("[假3D数据收集] 点%1: 屏幕坐标(%.0f,%.0f) -> 瞳孔(%.1f,%.1f) -> 实际测量角度(P:%.2f°,Y:%.2f°)")
                                                .arg(calibrationIndex + 1)
                                                .arg(predefinedScreenPoints[calibrationIndex].x)
                                                .arg(predefinedScreenPoints[calibrationIndex].y)
                                                .arg(finalPoint.x).arg(finalPoint.y)
                                                .arg(alpha).arg(beta);
                                    qDebug() << QString("[假3D数据收集] 3D几何: 眼球中心(%.1f,%.1f,%.1f) 半径%.1f 焦距%.1f")
                                                .arg(eye_center.x).arg(eye_center.y).arg(eye_center.z)
                                                .arg(eye_radius).arg(focal_length);
                                } else {
                                    qDebug() << "[假3D数据收集] 警告：法线向量长度为0";
                                }
                            } else {
                                qDebug() << "[假3D数据收集] 警告：射线参数t <= 0";
                            }
                        } else {
                            qDebug() << "[假3D数据收集] 警告：射线与眼球不相交";
                        }
                    } else {
                        qDebug() << "[假3D数据收集] 警告：眼球几何信息无效";
                    }
                }

//                qDebug() << "3D标定采集点" << calibrationIndex + 1 << ": 瞳孔("
//                         << finalPoint.x << "," << finalPoint.y << ") -> 屏幕("
//                         << predefinedScreenPoints[calibrationIndex].x << ","
//                         << predefinedScreenPoints[calibrationIndex].y << ")";
            }
            isCollecting = false;
            if (calibrationIndex >= CALIBRATIONPOINTS9 - 1) {
                // 3D模式：执行3D几何标定
                qDebug() << "[3D标定] 多点标定完成，调用perform3DCalibration()";
                perform3DCalibration();

                // 【新增】假3D模式：计算多点映射矩阵
                bool isFake3D = (worker && !worker->isReal3DModeEnabled());
                qDebug() << QString("[标定完成] worker存在:%1, 假3D模式:%2").arg(worker != nullptr).arg(isFake3D);

                if (isFake3D) {
                    qDebug() << "[假3D多点标定] 开始调用computeFake3DMultiPointMapping()";
                    computeFake3DMultiPointMapping();
                    qDebug() << QString("[假3D多点标定] 完成%1点标定").arg(CALIBRATIONPOINTS9);

                    // 【恢复】显示假3D多点标定误差图
                    qDebug() << "[假3D多点标定] 计算并显示误差图";
                    double avgError = calculateFake3DAverageError();
                    qDebug() << QString("[假3D多点标定] 平均误差: %.1f").arg(avgError);
                    if (avgError > 0) {
                        qDebug() << "[假3D多点标定] 调用showFake3DCalibrationResult显示误差图";
                        showFake3DCalibrationResult(avgError);
                    } else {
                        qDebug() << "[假3D多点标定] 误差为0，使用默认值显示误差图";
                        showFake3DCalibrationResult(50.0); // 使用默认误差值
                    }
                } else {
                    qDebug() << "[标定完成] 跳过假3D多点标定，当前为真3D模式";
                }

                calibrationIndex = 0;
            } else {
                calibrationIndex++;
            }
        }
    }

    // ===================================================================
    // 阶段四：常规帧处理 - 核心计算（视线映射、滤波、补偿）
    // ===================================================================
    cv::Point2f gaze_raw(-1, -1);
    if (result.pupilFound) {
        if (flag_calibration_status == 1) {
            if (yanshiCalibrated) {
                gaze_raw = Homography_map_point1(eyeVector, map_yanshi);
            } else if (calibration_success) {
                // 演示模式中也支持CalibMe的evaluate算法
                gaze_raw = evaluate(eyeVector);
            }
        } else if (flag_calibration_status == 0) {
            if (calibration_success) {
                gaze_raw = evaluate(eyeVector);
            }
        } else if (flag_calibration_status == 2) {
            // 3D模式分支：区分真3D和假3D
            if (worker && worker->isReal3DModeEnabled()) {
                // 真3D模式：使用gaze3DCalculator
                if (m_gaze3DCalculator) {
                    gaze_raw = calculateGaze3D(eyeVector);
                }
            } else {
                // 假3D模式：使用缓存的假3D屏幕坐标
                if (m_lastFake3DScreenCoords.x >= 0 && m_lastFake3DScreenCoords.y >= 0) {
                    gaze_raw = m_lastFake3DScreenCoords;
//                    qDebug() << QString("[假3D gaze_raw] 使用缓存坐标: (%.1f,%.1f)")
//                                .arg(gaze_raw.x).arg(gaze_raw.y);
                }
            }
        }
    }

    m_rawGazeBeforeCorrection = gaze_raw;
    cv::Point2f gaze_filtered = applyGazeKalmanFilter(gaze_raw, result.pupilData.confidence);
    cv::Point2f final_gaze = applyGazeCorrection(gaze_filtered);

    // 共享内存更新已移至imshow位置实时执行

    // ===================================================================
    // 【修复】回填视线数据到ProcessingResult中，解决数据传递断裂问题
    // ===================================================================
    // 创建result的副本，因为原参数是const引用
    ProcessingResult resultCopy = result;

    // 【简化修复】直接填入视线数据，不做复杂判断
    resultCopy.gazeScreenCoords = final_gaze;
    resultCopy.gazeValid = (final_gaze.x >= 0 && final_gaze.y >= 0);

    // 简单的视线速度计算
    static cv::Point2f previousGaze(-1, -1);
    static qint64 previousGazeTime = 0;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (previousGaze.x >= 0 && previousGazeTime > 0 && resultCopy.gazeValid) {
        qint64 timeDiff = currentTime - previousGazeTime;
        if (timeDiff > 0 && timeDiff < 200) {
            float timeInSeconds = timeDiff / 1000.0f;
            cv::Point2f displacement = final_gaze - previousGaze;
            resultCopy.gazeVelocity = displacement / timeInSeconds;
            resultCopy.gazeSpeed = cv::norm(resultCopy.gazeVelocity);
        } else {
            resultCopy.gazeVelocity = cv::Point2f(0, 0);
            resultCopy.gazeSpeed = 0.0f;
        }
    } else {
        resultCopy.gazeVelocity = cv::Point2f(0, 0);
        resultCopy.gazeSpeed = 0.0f;
    }

    // 更新历史
    if (resultCopy.gazeValid) {
        previousGaze = final_gaze;
        previousGazeTime = currentTime;
    }

    // 立即调试输出
//    qDebug() << "【MainWindow数据回填】final_gaze:" << final_gaze.x << "," << final_gaze.y
//             << " gazeValid:" << resultCopy.gazeValid
//             << " gazeVelocity:" << resultCopy.gazeVelocity.x << "," << resultCopy.gazeVelocity.y
//             << " pupilVelocity:" << resultCopy.eyeMetrics.pupilVelocity.x << "," << resultCopy.eyeMetrics.pupilVelocity.y;

    // ===================================================================
    // 阶段五：【发射信号】通知DataTable和GraphPlot更新数据
    // ===================================================================
    auto signalNow = std::chrono::steady_clock::now();

    // 发射瞳孔数据信号（仅在瞳孔被检测到时）
    if (result.pupilFound) {
        quint64 timestamp = QDateTime::currentMSecsSinceEpoch();
        emit pupilDataUpdated(timestamp, result.pupilData, "");
    }

    // 【修正】无论瞳孔是否被检测到，都发射眼动指标信号
    // 这样眨眼时的眼动指标也能正常传递到DataTable

    // 调试：确认信号发射
    static int emitCount = 0;
    emitCount++;

    // 更新最新处理结果（供3D计算等功能使用）
    {
        QMutexLocker locker(&m_resultMutex);
        m_lastProcessingResult = resultCopy;
    }

    emit processingResultUpdated(resultCopy);

    // 2. 发射独立的全局帧数信号
        emit framecountUpdated(m_frameCount);

    // 发射总帧数信号
    emit framecountUpdated(m_frameCount);

    // 每秒计算并发射一次FPS信号
    auto timeSinceLastFPSUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(signalNow - m_lastFPSTime).count();
    if (timeSinceLastFPSUpdate > 500) {
        // 此处假设相机帧率与处理帧率相似，实际应从相机回调中获取真实相机帧率
        double cameraFps = m_frameCount * 1000.0 / timeSinceLastFPSUpdate;
        emit cameraFPSUpdated(cameraFps);

        double processingFps = m_processingFrameCount * 1000.0 / timeSinceLastFPSUpdate;
        emit processingFPSUpdated(processingFps);

        // 重置计数器和计时器
        m_lastFPSTime = signalNow;
        m_frameCount = 0;
        m_processingFrameCount = 0;
    }

    // ===================================================================
    // 阶段六：常规帧处理 - UI 更新
    // ===================================================================
    // 1. 更新眼部图像（添加安全检查）
    cv::Mat eyeDisplay = result.processedEyeImage.clone();  // 使用clone确保数据安全
    if (!eyeDisplay.empty() && eyeDisplay.channels() == 3) {
        // 【调试】检查所有条件
//        qDebug() << QString("[调试] flag_calibration_status=%1, pupilFound=%2, worker=%3, real3D=%4")
//                    .arg(flag_calibration_status)
//                    .arg(result.pupilFound ? "true" : "false")
//                    .arg(worker ? "true" : "false")
//                    .arg(worker ? (worker->isReal3DModeEnabled() ? "true" : "false") : "N/A");

        if (flag_calibration_status == 2 && result.pupilFound) {
            // 【修复】只有在假3D模式时才调用draw3DGazeVisualization，避免真3D和假3D同时运行
            if (worker && !worker->isReal3DModeEnabled()) {
//                qDebug() << "[handleProcessingResult] ✅ 假3D模式：调用draw3DGazeVisualization";
                draw3DGazeVisualization(eyeDisplay, result.pupilData.center, result.pupilData);
            } else {
//                qDebug() << "[handleProcessingResult] ❌ 真3D模式：跳过draw3DGazeVisualization，避免冲突";
            }
        } else {
//            qDebug() << "[handleProcessingResult] ❌ 条件不满足：无法调用draw3DGazeVisualization";
        }

        // 原有的eyeImage已经有坐标显示，无需重复添加

        try {
            QImage eyeQImage(eyeDisplay.data, static_cast<int>(eyeDisplay.cols), static_cast<int>(eyeDisplay.rows), static_cast<int>(eyeDisplay.step), QImage::Format_RGB888);
            if (!eyeQImage.isNull()) {
                ui->label_eye->setPixmap(QPixmap::fromImage(eyeQImage.rgbSwapped()));
            }
        } catch (const std::exception& e) {
//            qDebug() << "眼部图像显示异常:" << e.what();
        }
    }

    // 2. 更新场景图像
    cv::Mat sceneDisplay = result.originalSceneImage.clone();

    // CalibMe ArUco标记检测和数据采集
    if (gmark == 1 && result.pupilFound && CALIBRATIONPOINTS9 == 1) {
        vector<int> ids;
        vector<vector<Point2f>> corners;
        cv::aruco::detectMarkers(sceneDisplay, dict, corners, ids, detectorParameters);

        if (!ids.empty()) {
            // 估计ArUco标记位姿
            cameraMatrix = getCameraMatrix();
            distCoeffs = getDistortionCoefficients();
            cv::aruco::estimatePoseSingleMarkers(corners, 0.10f, cameraMatrix, distCoeffs, rvecs, tvecs);

            for (unsigned int i = 0; i < ids.size(); i++) {
                Marker marker(corners[i], ids[i]);
                // 计算标记中心
                cv::Point2f center = cv::Point2f(0, 0);
                for (const auto& corner : corners[i]) {
                    center += corner;
                }
                center /= 4.0f;
                marker.center = cv::Point3f(center.x, center.y, 0);

                // 在场景图像上绘制标记中心
                cv::circle(sceneDisplay, cv::Point(center.x, center.y), 5, cv::Scalar(255, 0, 0), -1);

                // 采集数据：瞳孔中心 -> 标记中心
                rightPupil.insert(result.pupilData.center);
                Markgaze.insert(center);
                markers.push_back(marker);
            }
        }
    }

    if (final_gaze.x >= 0) {

//        if (flag_calibration_status != 1) {
//            cv::circle(sceneDisplay, Point(final_gaze.x-579,final_gaze.y-181), 15, cv::Scalar(0, 255, 0), 2);
//            cv::line(sceneDisplay, cv::Point(final_gaze.x - 559, final_gaze.y - 181), cv::Point(final_gaze.x -539, final_gaze.y -181), cv::Scalar(0, 255, 0), 2);
//            cv::line(sceneDisplay, cv::Point(final_gaze.x - 579, final_gaze.y - 191), cv::Point(final_gaze.x -579, final_gaze.y -171), cv::Scalar(0, 255, 0), 2);
//        }
//        else
        if (flag_calibration_status == 1)
        {
            cv::circle(sceneDisplay, final_gaze, 15, cv::Scalar(0, 255, 0), 2);
            cv::line(sceneDisplay, cv::Point(final_gaze.x - 10, final_gaze.y), cv::Point(final_gaze.x + 10, final_gaze.y), cv::Scalar(0, 255, 0), 2);
            cv::line(sceneDisplay, cv::Point(final_gaze.x, final_gaze.y - 10), cv::Point(final_gaze.x, final_gaze.y + 10), cv::Scalar(0, 255, 0), 2);
        }

    }
    // 安全地显示场景图像
    if (!sceneDisplay.empty() && sceneDisplay.channels() == 3) {
        try {
            // 绘制AI检测框（只在演示模式下绘制，因为目标检测只在演示模式下有效）
            if (m_aiResultsConnected && m_currentAIResults.detection_count > 0 && flag_calibration_status == 1) {
                for (uint32_t i = 0; i < m_currentAIResults.detection_count; i++) {
                    const DetectionBox& box = m_currentAIResults.detection_boxes[i];

                    // 坐标补偿：将局部坐标转换为对应显示图像的坐标
                    // main05.py传回的是gazeRegion上的局部坐标，需要加上cropOffset
                    int compensated_x = static_cast<int>(box.x) + m_currentCropOffset.x;
                    int compensated_y = static_cast<int>(box.y) + m_currentCropOffset.y;
                    int compensated_w = static_cast<int>(box.w);
                    int compensated_h = static_cast<int>(box.h);

                    // 检查补偿后的坐标是否在显示图像范围内
                    if (compensated_x < 0 || compensated_y < 0 ||
                        compensated_x + compensated_w >= sceneDisplay.cols ||
                        compensated_y + compensated_h >= sceneDisplay.rows) {
                        // 跳过超出显示图像范围的检测框
                        continue;
                    }

                    // 根据置信度选择颜色
                    cv::Scalar color;
                    if (box.confidence > 0.8f) {
                        color = cv::Scalar(0, 255, 0);    // 高置信度：绿色
                    } else if (box.confidence > 0.5f) {
                        color = cv::Scalar(0, 255, 255);  // 中等置信度：黄色
                    } else {
                        color = cv::Scalar(255, 165, 0);  // 低置信度：橙色
                    }

                    // 绘制检测框 - 使用补偿后的坐标
                    cv::Point2i topLeft(compensated_x, compensated_y);
                    cv::Point2i bottomRight(compensated_x + compensated_w, compensated_y + compensated_h);
                    cv::rectangle(sceneDisplay, topLeft, bottomRight, color, 2);

                    // 绘制类别标签（格式化置信度为2位小数）
                    char confStr[16];
                    snprintf(confStr, sizeof(confStr), "%.2f", box.confidence);
                    std::string label = std::string(box.class_name) + ": " + confStr;

                    cv::putText(sceneDisplay, label,
                               cv::Point(topLeft.x, topLeft.y - 8),
                               cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);

                    // 绘制目标中心点 - 使用补偿后的坐标
                    cv::Point2i center(compensated_x + compensated_w/2, compensated_y + compensated_h/2);
                    cv::circle(sceneDisplay, center, 3, cv::Scalar(255, 0, 0), -1);
                }
                // 只在检测框数量变化时输出调试信息
                static uint32_t last_count = 0;
                if (m_currentAIResults.detection_count != last_count) {
                    qDebug() << "✅ 已在场景图像上绘制" << m_currentAIResults.detection_count << "个检测框";
                    last_count = m_currentAIResults.detection_count;
                }
            }

            QImage sceneQImage(sceneDisplay.data, static_cast<int>(sceneDisplay.cols), static_cast<int>(sceneDisplay.rows), static_cast<int>(sceneDisplay.step), QImage::Format_RGB888);
            if (!sceneQImage.isNull()) {
                ui->label_scene->setPixmap(QPixmap::fromImage(sceneQImage.rgbSwapped()));
            }
        } catch (const std::exception& e) {
            qDebug() << "场景图像显示异常:" << e.what();
        }
    }

    // 3. 更新文本标签
    ui->xZuobiao->setText(QString::number(final_gaze.x, 'f', 2));
    ui->yZuobiao->setText(QString::number(final_gaze.y, 'f', 2));
    if (flag_calibration_status != 0 && flag_calibration_status != 2) {
        compute_angle(final_gaze);
        ui->pitch_qs->setText(QString::number(pitch, 'f', 6));
        ui->yaw_qs->setText(QString::number(yaw, 'f', 6));
    }

    // 4. 更新T模式
//    if (m_freeGazeMode) {
//        qDebug() << "准备调用updateFreeGaze，final_gaze坐标:" << final_gaze.x << "," << final_gaze.y;
//    }
    updateFreeGaze(final_gaze);

    // 🔍 视线区域截取和显示（根据模式选择不同图像源）
    if (final_gaze.x >= 0) {
        cv::Mat sourceImage;
        QString windowTitle;

        if (flag_calibration_status == 1) {  // 演示模式 - 使用场景图像
            if (!sceneImage.empty()) {
                sourceImage = sceneImage;
                windowTitle = "视线区域 - 场景图像 (演示模式)";

                int cropWidth = m_currentImageWidth;    // 使用动态图像宽度
                int cropHeight = m_currentImageHeight;  // 使用动态图像高度

                cv::Mat gazeRegion;
                int x1 = 0, y1 = 0;  // 默认偏移量

                // 检查是否为全图像模式（1280x720）
                if (cropWidth >= sourceImage.cols && cropHeight >= sourceImage.rows) {
                    // 全图像模式：直接发送完整场景图像
                    gazeRegion = sourceImage.clone();
                    windowTitle = "完整场景图像 (演示模式)";
                    x1 = 0;
                    y1 = 0;
                } else {
                    // 裁剪模式：按视线位置裁剪
                    int halfWidth = cropWidth / 2;
                    int halfHeight = cropHeight / 2;

                    // 确保截取区域在场景图像范围内
                    x1 = ((int)final_gaze.x - halfWidth) > 0 ? ((int)final_gaze.x - halfWidth) : 0;
                    y1 = ((int)final_gaze.y - halfHeight) > 0 ? ((int)final_gaze.y - halfHeight) : 0;
                    int x2 = ((int)final_gaze.x + halfWidth) < sourceImage.cols ? ((int)final_gaze.x + halfWidth) : sourceImage.cols;
                    int y2 = ((int)final_gaze.y + halfHeight) < sourceImage.rows ? ((int)final_gaze.y + halfHeight) : sourceImage.rows;

                    if (x2 > x1 && y2 > y1) {
                        cv::Rect gazeROI(x1, y1, x2 - x1, y2 - y1);
                        gazeRegion = sourceImage(gazeROI).clone();
                    }
                }

                if (!gazeRegion.empty()) {

                    // 在截取区域中心画红色十字，标记视线精确位置
                    // int centerX = (int)final_gaze.x - x1;
                    // int centerY = (int)final_gaze.y - y1;
                    // cv::line(gazeRegion, cv::Point(centerX - 5, centerY), cv::Point(centerX + 5, centerY), cv::Scalar(0, 0, 255), 2);
                    // cv::line(gazeRegion, cv::Point(centerX, centerY - 5), cv::Point(centerX, centerY + 5), cv::Scalar(0, 0, 255), 2);

                    // 原尺寸显示
                    // cv::imshow(windowTitle.toStdString(), gazeRegion);

                    // 限制共享内存写入频率到3FPS，保持视线坐标精度
                    if (m_sharedMemoryEnabled) {
                        static qint64 lastSharedMemoryTime = 0;
                        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
                        if (currentTime - lastSharedMemoryTime >= 333) { // 333ms = 3FPS
                            // 保存当前的截图偏移量，用于检测框坐标补偿
                            m_currentCropOffset = cv::Point2i(x1, y1);
                            // 【修改】所有模式都发送gazeRegion（视线区域图像）
                            updateSharedMemory(gazeRegion, final_gaze, m_currentCropOffset);
                            lastSharedMemoryTime = currentTime;
                        }
                        // 调试：输出写入的坐标
//                        static int debug_counter = 0;
//                        debug_counter++;
//                        if (debug_counter % 1 == 0) {
//                            qDebug() << "📤 [演示模式] 写入共享内存坐标:" << final_gaze.x << "," << final_gaze.y;
//                        }
                    }
                }
            }
        } else if (flag_calibration_status == 0 || flag_calibration_status == 2) {  // 精确标定模式和3D模式 - 截取屏幕图像
            // 精确模式和3D模式都是将视线映射到屏幕，需要截取屏幕图像
            // 【优化】限制屏幕截取频率到3FPS，避免性能问题
            static qint64 lastScreenCaptureTime = 0;
            static cv::Mat cachedScreenImage;
            qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

            QScreen* screen = QGuiApplication::primaryScreen();
            if (screen && (currentTime - lastScreenCaptureTime >= 333 || cachedScreenImage.empty())) { // 333ms = 3FPS
                QPixmap screenPixmap = screen->grabWindow(0);  // 截取整个屏幕
                QImage screenQImage = screenPixmap.toImage().convertToFormat(QImage::Format_RGB888);

                // 将QImage转换为cv::Mat
                cv::Mat screenImage(screenQImage.height(), screenQImage.width(), CV_8UC3,
                                  (void*)screenQImage.constBits(), screenQImage.bytesPerLine());
                cv::cvtColor(screenImage, screenImage, cv::COLOR_RGB2BGR);  // QImage是RGB，OpenCV是BGR
                cachedScreenImage = screenImage.clone();
                lastScreenCaptureTime = currentTime;
            }

            // 使用缓存的屏幕图像或刚截取的图像
            if (!cachedScreenImage.empty()) {
                sourceImage = cachedScreenImage;

                if (flag_calibration_status == 0) {
                    windowTitle = "视线区域 - 屏幕截图 (精确模式)";
                } else {
                    windowTitle = "视线区域 - 屏幕截图 (3D模式)";
                }

                int cropWidth = m_currentImageWidth;    // 使用动态图像宽度
                int cropHeight = m_currentImageHeight;  // 使用动态图像高度
                int halfWidth = cropWidth / 2;
                int halfHeight = cropHeight / 2;

                // 确保截取区域在屏幕范围内
                int x1 = ((int)final_gaze.x - halfWidth) > 0 ? ((int)final_gaze.x - halfWidth) : 0;
                int y1 = ((int)final_gaze.y - halfHeight) > 0 ? ((int)final_gaze.y - halfHeight) : 0;
                int x2 = ((int)final_gaze.x + halfWidth) < sourceImage.cols ? ((int)final_gaze.x + halfWidth) : sourceImage.cols;
                int y2 = ((int)final_gaze.y + halfHeight) < sourceImage.rows ? ((int)final_gaze.y + halfHeight) : sourceImage.rows;

                if (x2 > x1 && y2 > y1) {
                    cv::Rect gazeROI(x1, y1, x2 - x1, y2 - y1);
                    cv::Mat gazeRegion = sourceImage(gazeROI).clone();

                    // 在截取区域中心标记视线位置
                    // int centerX = (int)final_gaze.x - x1;
                    // int centerY = (int)final_gaze.y - y1;

                    // if (flag_calibration_status == 0) {
                    //     // 精确模式：绿色十字和圆圈
                    //     cv::circle(gazeRegion, cv::Point(centerX, centerY), 5, cv::Scalar(0, 255, 0), 2);
                    //     cv::line(gazeRegion, cv::Point(centerX - 8, centerY), cv::Point(centerX + 8, centerY), cv::Scalar(0, 255, 0), 2);
                    //     cv::line(gazeRegion, cv::Point(centerX, centerY - 8), cv::Point(centerX, centerY + 8), cv::Scalar(0, 255, 0), 2);
                    // } else {
                    //     // 3D模式：蓝色十字和圆圈
                    //     cv::circle(gazeRegion, cv::Point(centerX, centerY), 5, cv::Scalar(255, 0, 0), 2);
                    //     cv::line(gazeRegion, cv::Point(centerX - 8, centerY), cv::Point(centerX + 8, centerY), cv::Scalar(255, 0, 0), 2);
                    //     cv::line(gazeRegion, cv::Point(centerX, centerY - 8), cv::Point(centerX, centerY + 8), cv::Scalar(255, 0, 0), 2);
                    // }

                    // 原尺寸显示
                    // cv::imshow(windowTitle.toStdString(), gazeRegion);

                    // 限制共享内存写入频率到3FPS，保持视线坐标精度
                    if (m_sharedMemoryEnabled) {
                        static qint64 lastSharedMemoryTime2 = 0;
                        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
                        if (currentTime - lastSharedMemoryTime2 >= 333) { // 333ms = 3FPS
                            // 保存当前的截图偏移量，用于检测框坐标补偿
                            m_currentCropOffset = cv::Point2i(x1, y1);
                            // 【修改】所有模式都发送gazeRegion（视线区域图像）
                            updateSharedMemory(gazeRegion, final_gaze, m_currentCropOffset);
                            lastSharedMemoryTime2 = currentTime;
                        }
                        // 调试：输出写入的坐标

                    }
                }
            }
        }
    }

    // 5. 传递视线坐标给EyeTrackingDialog（修复：视线坐标无法触发按钮的问题）
    if (m_dialog && final_gaze.x >= 0) {
        m_dialog->updateGazePoint(final_gaze);
    }

    // 5. 更新误差测试窗口数据采集
    if (m_samplingActive && final_gaze.x >= 0) {
        if (m_gazePoints.size() > m_currentPoint) {
            m_gazePoints[m_currentPoint].push_back(final_gaze);
            m_originalGazePoints[m_currentPoint].push_back(gaze_raw);
            m_gazeCount++;
            if (m_gazeCount >= m_maxGazePerPoint) {
                m_samplingActive = false;
                cv::Point2f target_cv = m_calibrationPoints[m_currentPoint];
                QPointF target_qt(target_cv.x, target_cv.y);
                QPointF gaze_qt(final_gaze.x, final_gaze.y);

                // 【修复】标定过程中不调用误差校正，只记录纯净数据
                // onPointCompleted(m_currentPoint, target_qt, gaze_qt);

                m_currentPoint++;
                if (m_currentPoint >= m_calibrationPoints.size()) {
                    onAllPointsCompleted();
                    m_currentPoint = 0;
                    m_waitingForSpace = true;
                } else {
                    m_waitingForSpace = true;
                }
            }
        }
    }

    // 6. 计算并打印总耗时
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_processingStartTime);
    static int logCounter = 0;
    if (++logCounter % 30 == 0) {
//        qDebug() << "Total frame pipeline time:" << duration.count() / 1000.0 << "ms";
    }
}

// ==================== 【新增】摄像头管理系统实现 ====================

void MainWindow::createCameraSelectionUIInControlsDock()
{
//    qDebug() << "=== 开始创建摄像头选择UI组件（独立创建，不依赖控件窗口） ===";

    // 如果组框已存在，直接返回
    if (m_cameraGroupBox) {
//        qDebug() << "摄像头组框已存在，跳过创建";
        return;
    }

    // 创建摄像头选择分组框（专业样式）
    m_cameraGroupBox = new QGroupBox("");
    m_cameraGroupBox->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
    m_cameraGroupBox->setStyleSheet(
        "QGroupBox { "
        "    font-weight: bold; "
        "    border: 2px solid #CCCCCC; "
        "    border-radius: 5px; "
        "    margin-top: 10px; "
        "    padding-top: 15px; "
        "    background-color: #FAFAFA; "
        "} "
        "QGroupBox::title { "
        "    subcontrol-origin: margin; "
        "    left: 10px; "
        "    padding: 0 8px 0 8px; "
        "    background-color: white; "
        "}");

    // 使用QFormLayout实现专业的表单对齐
    QFormLayout* formLayout = new QFormLayout(m_cameraGroupBox);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setHorizontalSpacing(12);
    formLayout->setVerticalSpacing(10);

    // === 眼球摄像头选择区域 ===
    QWidget* eyeCameraWidget = new QWidget();
    QHBoxLayout* eyeCameraLayout = new QHBoxLayout(eyeCameraWidget);
    eyeCameraLayout->setContentsMargins(0, 0, 0, 0);
    eyeCameraLayout->setSpacing(10);

    m_eyeCameraComboBox = new QComboBox();
    m_eyeCameraComboBox->setFont(QFont("Microsoft YaHei", 9));
    m_eyeCameraComboBox->setMinimumWidth(200);
    m_eyeCameraComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);



    m_eyeCameraComboBox->setStyleSheet(
        "QComboBox {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    background-color: #FFFFFF;"
        "    min-width: 80px;"
        "}"
        "QComboBox:hover {"
        "    border-color: #2196F3;"
        "    background-color: #F3F9FF;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #FFFFFF;"
        "    border: 1px solid #E0E0E0;"
        "    selection-background-color: #E3F2FD;"
        "    selection-color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "    padding: 6px 8px;"
        "    border: none;"
        "    background-color: #FFFFFF;"
        "    color: #212121;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "    background-color: #E3F2FD;"
        "    color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "    background-color: #BBDEFB;"
        "    color: #1976D2;"
        "}"
    );


    m_eyeCameraPreviewBtn = new QPushButton("预览");
    m_eyeCameraPreviewBtn->setFont(QFont("Microsoft YaHei", 9));
    m_eyeCameraPreviewBtn->setFixedSize(70, 32);
    QIcon previewIcon(":/icons/iconSet/cameraList.png");
    if (!previewIcon.isNull()) {
        m_eyeCameraPreviewBtn->setIcon(previewIcon);
        m_eyeCameraPreviewBtn->setIconSize(QSize(18, 18));
//        qDebug() << "眼球摄像头预览图标加载成功";
    } else {
//        qDebug() << "眼球摄像头预览图标加载失败: :/icons/iconSet/cameraList.png";
    }
    m_eyeCameraPreviewBtn->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #42A5F5, stop:1 #2196F3);"
        "    color: white;"
        "    border: 1px solid #1976D2;"
        "    border-radius: 6px;"
        "    padding: 4px 8px;"
        "    text-align: center;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #64B5F6, stop:1 #42A5F5);"
        "    border: 1px solid #2196F3;"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1976D2, stop:1 #0D47A1);"
        "    border: 1px solid #0D47A1;"
        "}");

    eyeCameraLayout->addWidget(m_eyeCameraComboBox);
    eyeCameraLayout->addWidget(m_eyeCameraPreviewBtn);

    QLabel* eyeCameraLabel = new QLabel("眼球摄像头:");
    eyeCameraLabel->setFont(QFont("Microsoft YaHei", 9));
    eyeCameraLabel->setMinimumWidth(80);
    formLayout->addRow(eyeCameraLabel, eyeCameraWidget);

    // 眼球摄像头参数
    QWidget* eyeParamsWidget = new QWidget();
    QHBoxLayout* eyeParamsLayout = new QHBoxLayout(eyeParamsWidget);
    eyeParamsLayout->setContentsMargins(0, 0, 0, 0);
    eyeParamsLayout->setSpacing(15);

    QLabel* eyeResLabel = new QLabel("分辨率:");
    eyeResLabel->setFont(QFont("Microsoft YaHei", 8));
    m_eyeCameraResComboBox = new QComboBox();
    m_eyeCameraResComboBox->setFont(QFont("Microsoft YaHei", 8));
    m_eyeCameraResComboBox->addItems({"320x240", "640x480"});
    m_eyeCameraResComboBox->setCurrentText("320x240"); // 默认选择320x240对应480fps
    m_eyeCameraResComboBox->setFixedWidth(100);


    m_eyeCameraResComboBox->setStyleSheet(
        "QComboBox {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    background-color: #FFFFFF;"
        "    min-width: 80px;"
        "}"
        "QComboBox:hover {"
        "    border-color: #2196F3;"
        "    background-color: #F3F9FF;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #FFFFFF;"
        "    border: 1px solid #E0E0E0;"
        "    selection-background-color: #E3F2FD;"
        "    selection-color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "    padding: 6px 8px;"
        "    border: none;"
        "    background-color: #FFFFFF;"
        "    color: #212121;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "    background-color: #E3F2FD;"
        "    color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "    background-color: #BBDEFB;"
        "    color: #1976D2;"
        "}"
    );




    QLabel* eyeFpsLabel = new QLabel("采集帧率:");
    eyeFpsLabel->setFont(QFont("Microsoft YaHei", 8));
    m_eyeCameraFpsComboBox = new QComboBox();
    m_eyeCameraFpsComboBox->setFont(QFont("Microsoft YaHei", 8));
    m_eyeCameraFpsComboBox->addItems({"240", "480"});
    m_eyeCameraFpsComboBox->setCurrentText("480");
    m_eyeCameraFpsComboBox->setFixedWidth(70);


    m_eyeCameraFpsComboBox->setStyleSheet(
        "QComboBox {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    background-color: #FFFFFF;"
        "    min-width: 80px;"
        "}"
        "QComboBox:hover {"
        "    border-color: #2196F3;"
        "    background-color: #F3F9FF;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #FFFFFF;"
        "    border: 1px solid #E0E0E0;"
        "    selection-background-color: #E3F2FD;"
        "    selection-color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "    padding: 6px 8px;"
        "    border: none;"
        "    background-color: #FFFFFF;"
        "    color: #212121;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "    background-color: #E3F2FD;"
        "    color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "    background-color: #BBDEFB;"
        "    color: #1976D2;"
        "}"
    );




    eyeParamsLayout->addWidget(eyeResLabel);
    eyeParamsLayout->addWidget(m_eyeCameraResComboBox);
    eyeParamsLayout->addWidget(eyeFpsLabel);
    eyeParamsLayout->addWidget(m_eyeCameraFpsComboBox);
    eyeParamsLayout->addStretch();

    formLayout->addRow("", eyeParamsWidget);

    // 添加分隔线
    QFrame* separator1 = new QFrame();
    separator1->setFrameShape(QFrame::HLine);
    separator1->setFrameShadow(QFrame::Sunken);
    separator1->setStyleSheet("QFrame { color: #CCCCCC; margin-top: 5px; margin-bottom: 5px; }");
    formLayout->addRow(separator1);

    // === 场景摄像头选择区域 ===
    QWidget* sceneCameraWidget = new QWidget();
    QHBoxLayout* sceneCameraLayout = new QHBoxLayout(sceneCameraWidget);
    sceneCameraLayout->setContentsMargins(0, 0, 0, 0);
    sceneCameraLayout->setSpacing(10);

    m_sceneCameraComboBox = new QComboBox();
    m_sceneCameraComboBox->setFont(QFont("Microsoft YaHei", 9));
    m_sceneCameraComboBox->setMinimumWidth(200);
    m_sceneCameraComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);


    m_sceneCameraComboBox->setStyleSheet(
        "QComboBox {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    background-color: #FFFFFF;"
        "    min-width: 80px;"
        "}"
        "QComboBox:hover {"
        "    border-color: #2196F3;"
        "    background-color: #F3F9FF;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #FFFFFF;"
        "    border: 1px solid #E0E0E0;"
        "    selection-background-color: #E3F2FD;"
        "    selection-color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "    padding: 6px 8px;"
        "    border: none;"
        "    background-color: #FFFFFF;"
        "    color: #212121;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "    background-color: #E3F2FD;"
        "    color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "    background-color: #BBDEFB;"
        "    color: #1976D2;"
        "}"
    );





    m_sceneCameraPreviewBtn = new QPushButton("预览");
    m_sceneCameraPreviewBtn->setFont(QFont("Microsoft YaHei", 9));
    m_sceneCameraPreviewBtn->setFixedSize(70, 32);
    QIcon scenePreviewIcon(":/icons/iconSet/cameraList.png");
    if (!scenePreviewIcon.isNull()) {
        m_sceneCameraPreviewBtn->setIcon(scenePreviewIcon);
        m_sceneCameraPreviewBtn->setIconSize(QSize(18, 18));
//        qDebug() << "场景摄像头预览图标加载成功";
    } else {
//        qDebug() << "场景摄像头预览图标加载失败: :/icons/iconSet/cameraList.png";
    }
    m_sceneCameraPreviewBtn->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #42A5F5, stop:1 #2196F3);"
        "    color: white;"
        "    border: 1px solid #1976D2;"
        "    border-radius: 6px;"
        "    padding: 4px 8px;"
        "    text-align: center;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #64B5F6, stop:1 #42A5F5);"
        "    border: 1px solid #2196F3;"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1976D2, stop:1 #0D47A1);"
        "    border: 1px solid #0D47A1;"
        "}");

    sceneCameraLayout->addWidget(m_sceneCameraComboBox);
    sceneCameraLayout->addWidget(m_sceneCameraPreviewBtn);

    QLabel* sceneCameraLabel = new QLabel("场景摄像头:");
    sceneCameraLabel->setFont(QFont("Microsoft YaHei", 9));
    sceneCameraLabel->setMinimumWidth(80);
    formLayout->addRow(sceneCameraLabel, sceneCameraWidget);

    // 场景摄像头参数
    QWidget* sceneParamsWidget = new QWidget();
    QHBoxLayout* sceneParamsLayout = new QHBoxLayout(sceneParamsWidget);
    sceneParamsLayout->setContentsMargins(0, 0, 0, 0);
    sceneParamsLayout->setSpacing(15);

    QLabel* sceneResLabel = new QLabel("分辨率:");
    sceneResLabel->setFont(QFont("Microsoft YaHei", 8));
    m_sceneCameraResComboBox = new QComboBox();
    m_sceneCameraResComboBox->setFont(QFont("Microsoft YaHei", 8));
    m_sceneCameraResComboBox->addItems({"640x480", "1280x720", "1920x1080"});
    m_sceneCameraResComboBox->setCurrentText("1280x720");
    m_sceneCameraResComboBox->setFixedWidth(100);


    m_sceneCameraResComboBox->setStyleSheet(
        "QComboBox {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    background-color: #FFFFFF;"
        "    min-width: 80px;"
        "}"
        "QComboBox:hover {"
        "    border-color: #2196F3;"
        "    background-color: #F3F9FF;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #FFFFFF;"
        "    border: 1px solid #E0E0E0;"
        "    selection-background-color: #E3F2FD;"
        "    selection-color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "    padding: 6px 8px;"
        "    border: none;"
        "    background-color: #FFFFFF;"
        "    color: #212121;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "    background-color: #E3F2FD;"
        "    color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "    background-color: #BBDEFB;"
        "    color: #1976D2;"
        "}"
    );



    QLabel* sceneFpsLabel = new QLabel("采集帧率:");
    sceneFpsLabel->setFont(QFont("Microsoft YaHei", 8));
    m_sceneCameraFpsComboBox = new QComboBox();
    m_sceneCameraFpsComboBox->setFont(QFont("Microsoft YaHei", 8));
    m_sceneCameraFpsComboBox->addItems({"30", "60", "120"});
    m_sceneCameraFpsComboBox->setCurrentText("60");
    m_sceneCameraFpsComboBox->setFixedWidth(70);



    m_sceneCameraFpsComboBox->setStyleSheet(
        "QComboBox {"
        "    border: 1px solid #CCCCCC;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    background-color: #FFFFFF;"
        "    min-width: 80px;"
        "}"
        "QComboBox:hover {"
        "    border-color: #2196F3;"
        "    background-color: #F3F9FF;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #FFFFFF;"
        "    border: 1px solid #E0E0E0;"
        "    selection-background-color: #E3F2FD;"
        "    selection-color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "    padding: 6px 8px;"
        "    border: none;"
        "    background-color: #FFFFFF;"
        "    color: #212121;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "    background-color: #E3F2FD;"
        "    color: #1976D2;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "    background-color: #BBDEFB;"
        "    color: #1976D2;"
        "}"
    );




    sceneParamsLayout->addWidget(sceneResLabel);
    sceneParamsLayout->addWidget(m_sceneCameraResComboBox);
    sceneParamsLayout->addWidget(sceneFpsLabel);
    sceneParamsLayout->addWidget(m_sceneCameraFpsComboBox);
    sceneParamsLayout->addStretch();

    formLayout->addRow("", sceneParamsWidget);

    // 添加分隔线
    QFrame* separator2 = new QFrame();
    separator2->setFrameShape(QFrame::HLine);
    separator2->setFrameShadow(QFrame::Sunken);
    separator2->setStyleSheet("QFrame { color: #CCCCCC; margin-top: 5px; margin-bottom: 5px; }");
    formLayout->addRow(separator2);

    // === 控制按钮区域 ===
    QWidget* buttonWidget = new QWidget();
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(20); // 增加按钮间距，避免拥挤

    m_refreshCamerasBtn = new QPushButton("刷新设备");
    m_refreshCamerasBtn->setFont(QFont("Microsoft YaHei", 9));
    m_refreshCamerasBtn->setFixedSize(120, 36); // 增加按钮宽度，避免文字被遮挡
    QIcon refreshIcon(":/icons/iconSet/settings.png");
    if (!refreshIcon.isNull()) {
        m_refreshCamerasBtn->setIcon(refreshIcon);
        m_refreshCamerasBtn->setIconSize(QSize(20, 20));
//        qDebug() << "刷新设备图标加载成功";
    } else {
//        qDebug() << "刷新设备图标加载失败: :/icons/iconSet/settings.png";
    }
    m_refreshCamerasBtn->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #8a8a8a, stop:1 #6a6a6a);"
        "    color: white;"
        "    border: 1px solid #555555;"
        "    border-radius: 8px;"
        "    padding: 6px 12px;"
        "    text-align: center;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #9a9a9a, stop:1 #7a7a7a);"
        "    border: 1px solid #666666;"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #5a5a5a, stop:1 #4a4a4a);"
        "    border: 1px solid #444444;"
        "}");

    m_applyCameraSettingsBtn = new QPushButton("应用设置");
    m_applyCameraSettingsBtn->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
    m_applyCameraSettingsBtn->setFixedSize(120, 36); // 增加按钮宽度，避免文字被遮挡
    QIcon applyIcon(":/icons/iconSet/settings.png");
    if (!applyIcon.isNull()) {
        m_applyCameraSettingsBtn->setIcon(applyIcon);
        m_applyCameraSettingsBtn->setIconSize(QSize(20, 20));
//        qDebug() << "应用设置图标加载成功";
    } else {
//        qDebug() << "应用设置图标加载失败: :/icons/iconSet/settings.png";
    }
    m_applyCameraSettingsBtn->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #66BB6A, stop:1 #4CAF50);"
        "    color: white;"
        "    border: 1px solid #45a049;"
        "    border-radius: 8px;"
        "    padding: 6px 12px;"
        "    text-align: center;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #76d275, stop:1 #5cbf60);"
        "    border: 1px solid #4caf50;"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4caf50, stop:1 #45a049);"
        "    border: 1px solid #3d8b40;"
        "}");

    buttonLayout->addWidget(m_refreshCamerasBtn);
    buttonLayout->addWidget(m_applyCameraSettingsBtn);
    buttonLayout->addStretch();

    formLayout->addRow("", buttonWidget);

    // === 状态显示区域 ===
    m_cameraStatusLabel = new QLabel("状态: 未连接摄像头");
    m_cameraStatusLabel->setFont(QFont("Microsoft YaHei", 9));
    m_cameraStatusLabel->setStyleSheet(
        "QLabel { color: #FF6B6B; padding: 5px; background-color: #FFF5F5; border-radius: 3px; }");
    formLayout->addRow("", m_cameraStatusLabel);

    // 连接信号槽
    connect(m_refreshCamerasBtn, &QPushButton::clicked, this, &MainWindow::on_refreshCamerasBtn_clicked);
    connect(m_applyCameraSettingsBtn, &QPushButton::clicked, this, &MainWindow::on_applyCameraSettingsBtn_clicked);
    connect(m_eyeCameraPreviewBtn, &QPushButton::clicked, this, &MainWindow::on_eyeCameraPreviewBtn_clicked);
    connect(m_sceneCameraPreviewBtn, &QPushButton::clicked, this, &MainWindow::on_sceneCameraPreviewBtn_clicked);
    connect(m_eyeCameraComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::on_eyeCameraComboBox_currentIndexChanged);

    // 眼球摄像头分辨率和帧率约束：320x240对应480fps，640x480对应120fps
    connect(m_eyeCameraResComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                if (index == 0) { // 320x240
                    m_eyeCameraFpsComboBox->setCurrentText("480");
                } else if (index == 1) { // 640x480
                    m_eyeCameraFpsComboBox->setCurrentText("120");
                }
//                qDebug() << "眼球摄像头分辨率变更为:" << m_eyeCameraResComboBox->currentText()
//                         << "对应帧率:" << m_eyeCameraFpsComboBox->currentText();
            });

    connect(m_eyeCameraFpsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                if (index == 0) { // 120fps
                    m_eyeCameraResComboBox->setCurrentText("640x480");
                } else if (index == 1) { // 480fps
                    m_eyeCameraResComboBox->setCurrentText("320x240");
                }
//                qDebug() << "眼球摄像头帧率变更为:" << m_eyeCameraFpsComboBox->currentText()
//                         << "对应分辨率:" << m_eyeCameraResComboBox->currentText();
            });
    connect(m_sceneCameraComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::on_sceneCameraComboBox_currentIndexChanged);

//    qDebug() << "摄像头选择UI组件已创建完成，准备添加到独立窗口中";
}

void MainWindow::createCameraSelectionUI()
{
    // 这个函数现在不再使用，改为使用createCameraSelectionUIInControlsDock
//    qDebug() << "createCameraSelectionUI: 此函数已废弃，请使用createCameraSelectionUIInControlsDock";
}

// ==================== 【以下是正确的摄像头管理函数实现】 ====================


void MainWindow::enumerateAvailableCameras()
{
    m_availableCameras.clear();

//    qDebug() << "开始枚举可用摄像头设备...";

#ifdef _WIN32
    // 使用DirectShow API枚举摄像头设备（仅获取设备列表，不测试打开）
    CoInitialize(nullptr);

    // 创建System Device Enumerator
    ICreateDevEnum* pCreateDevEnum = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum, (void**)&pCreateDevEnum);

    if (SUCCEEDED(hr)) {
        // 创建枚举器
        IEnumMoniker* pEnumMoniker = nullptr;
        hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);

        if (SUCCEEDED(hr) && pEnumMoniker) {
            IMoniker* pMoniker = nullptr;
            int deviceIndex = 0;

            // 枚举设备
            while (pEnumMoniker->Next(1, &pMoniker, nullptr) == S_OK) {
                // 获取设备属性
                IPropertyBag* pPropertyBag = nullptr;
                hr = pMoniker->BindToStorage(nullptr, nullptr, IID_IPropertyBag, (void**)&pPropertyBag);

                if (SUCCEEDED(hr)) {
                    VARIANT varName;
                    VariantInit(&varName);

                    QString friendlyName = QString("Camera %1").arg(deviceIndex);
                    QString devicePath = QString("Device Index: %1").arg(deviceIndex);

                    // 获取FriendlyName（设备管理器中显示的真实名称）
                    hr = pPropertyBag->Read(L"FriendlyName", &varName, nullptr);
                    if (SUCCEEDED(hr) && varName.vt == VT_BSTR) {
                        friendlyName = QString::fromWCharArray(varName.bstrVal);
                    }

                    // 添加到可用摄像头列表
                    m_availableCameras.push_back(CameraDeviceInfo(deviceIndex, friendlyName, devicePath, true));
//                    qDebug() << QString("发现摄像头设备 %1: %2").arg(deviceIndex).arg(friendlyName);

                    VariantClear(&varName);
                    pPropertyBag->Release();
                }

                pMoniker->Release();
                deviceIndex++;
            }

            pEnumMoniker->Release();
        }

        pCreateDevEnum->Release();
    }

    CoUninitialize();
#else
    // 非Windows平台简单枚举
    for (int i = 0; i < 5; ++i) {
        QString deviceName = QString("Camera %1").arg(i);
        QString devicePath = QString("Device Index: %1").arg(i);
        m_availableCameras.push_back(CameraDeviceInfo(i, deviceName, devicePath, true));
//        qDebug() << QString("添加摄像头设备 %1: %2").arg(i).arg(deviceName);
    }
#endif

//    qDebug() << QString("摄像头枚举完成，共发现 %1 个设备").arg(m_availableCameras.size());

    // 如果没有发现任何摄像头，添加默认选项
    if (m_availableCameras.empty()) {
        m_availableCameras.push_back(CameraDeviceInfo(0, "默认摄像头 0", "Device Index: 0", false));
        m_availableCameras.push_back(CameraDeviceInfo(1, "默认摄像头 1", "Device Index: 1", false));
//        qDebug() << "未发现可用摄像头，添加默认设备选项";
    }
}

void MainWindow::populateCameraComboBoxes()
{
    // 清空现有选项
    m_eyeCameraComboBox->clear();
    m_sceneCameraComboBox->clear();

    // 添加"未选择"选项
    m_eyeCameraComboBox->addItem("请选择眼球摄像头", -1);
    m_sceneCameraComboBox->addItem("请选择场景摄像头", -1);

    // 填充可用摄像头
    for (const auto& camera : m_availableCameras) {
        QString itemText = camera.friendlyName;
        if (!camera.isAvailable) {
            itemText += " (不可用)";
        }

        m_eyeCameraComboBox->addItem(itemText, camera.deviceIndex);
        m_sceneCameraComboBox->addItem(itemText, camera.deviceIndex);
    }

//    qDebug() << QString("摄像头下拉框已填充，共 %1 个选项").arg(m_availableCameras.size());
}

bool MainWindow::initializeCamerasWithSelection()
{
    if (m_selectedEyeCameraIndex < 0 || m_selectedSceneCameraIndex < 0) {
        QMessageBox::warning(this, "摄像头选择", "请先选择眼球摄像头和场景摄像头");
        return false;
    }

    // 停止现有摄像头
    if (camera1) {
        camera1->Stop();
        camera1.reset();
    }
    if (camera2) {
        camera2->Stop();
        camera2.reset();
    }

    try {
        // 获取摄像头参数
        CameraParams eyeParams = getEyeCameraParams();
        CameraParams sceneParams = getSceneCameraParams();

        // 配置眼球摄像头 (camera1)
        CameraConfig camera1_config;
        camera1_config.deviceIndex = m_selectedEyeCameraIndex;
        camera1_config.width = eyeParams.width;
        camera1_config.height = eyeParams.height;
        camera1_config.fps = eyeParams.fps;

        // 配置场景摄像头 (camera2)
        CameraConfig camera2_config;
        camera2_config.deviceIndex = m_selectedSceneCameraIndex;
        camera2_config.width = sceneParams.width;
        camera2_config.height = sceneParams.height;
        camera2_config.fps = sceneParams.fps;

        // 创建摄像头对象
        camera1 = std::make_unique<DirectShowOpenCVCamera>(camera1_config.deviceIndex, camera1_config);
        camera2 = std::make_unique<DirectShowOpenCVCamera>(camera2_config.deviceIndex, camera2_config);

        // 初始化摄像头
        bool camera1_ok = camera1->Initialize();
        bool camera2_ok = camera2->Initialize();

        if (!camera1_ok || !camera2_ok) {
            QString errorMsg = "摄像头初始化失败:\n";
            if (!camera1_ok) errorMsg += QString("眼球摄像头 (设备 %1) 初始化失败\n").arg(m_selectedEyeCameraIndex);
            if (!camera2_ok) errorMsg += QString("场景摄像头 (设备 %1) 初始化失败\n").arg(m_selectedSceneCameraIndex);

            QMessageBox::critical(this, "摄像头错误", errorMsg);

            // 清理失败的摄像头对象
            camera1.reset();
            camera2.reset();
            return false;
        }

        // 连接信号和槽
        connect(camera1.get(), &DirectShowOpenCVCamera::newFrameAvailable, this, &MainWindow::updateCamera1Label);
        connect(camera2.get(), &DirectShowOpenCVCamera::newFrameAvailable, this, &MainWindow::updateCamera2Label);

        // 启动摄像头
        camera1->Start();
        camera2->Start();

        m_camerasInitialized = true;
        updateCameraStatus();

        // 设置Worker的摄像头采集帧率
        if (worker) {
            worker->setCameraFps(static_cast<float>(eyeParams.fps));
//            qDebug() << "设置Worker摄像头采集帧率为:" << eyeParams.fps;
        }

//        qDebug() << QString("摄像头初始化成功: 眼球摄像头(设备%1, %2x%3@%4fps), 场景摄像头(设备%5, %6x%7@%8fps)")
//                    .arg(m_selectedEyeCameraIndex).arg(eyeParams.width).arg(eyeParams.height).arg(eyeParams.fps)
//                    .arg(m_selectedSceneCameraIndex).arg(sceneParams.width).arg(sceneParams.height).arg(sceneParams.fps);

        // 保存设置
        saveCameraSettings();

        return true;

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "摄像头错误", QString("摄像头初始化异常: %1").arg(e.what()));
        camera1.reset();
        camera2.reset();
        return false;
    }
}

void MainWindow::updateCameraStatus()
{
    QString statusText;
    QString styleSheet;

    if (m_camerasInitialized && camera1 && camera2) {
        statusText = QString("状态: 已连接 (眼球:%1, 场景:%2)")
                        .arg(m_selectedEyeCameraIndex)
                        .arg(m_selectedSceneCameraIndex);
        styleSheet = "QLabel { color: #4CAF50; }"; // 绿色
    } else if (m_selectedEyeCameraIndex >= 0 && m_selectedSceneCameraIndex >= 0) {
        statusText = "状态: 已选择设备，未初始化";
        styleSheet = "QLabel { color: #FF9800; }"; // 橙色
    } else {
        statusText = "状态: 未选择摄像头";
        styleSheet = "QLabel { color: #FF6B6B; }"; // 红色
    }

    m_cameraStatusLabel->setText(statusText);
    m_cameraStatusLabel->setStyleSheet(styleSheet);
}

QString MainWindow::getCameraConfigString()
{
    CameraParams eyeParams = getEyeCameraParams();
    CameraParams sceneParams = getSceneCameraParams();

    return QString("Eye: Dev%1 %2x%3@%4fps | Scene: Dev%5 %6x%7@%8fps")
            .arg(m_selectedEyeCameraIndex).arg(eyeParams.width).arg(eyeParams.height).arg(eyeParams.fps)
            .arg(m_selectedSceneCameraIndex).arg(sceneParams.width).arg(sceneParams.height).arg(sceneParams.fps);
}

void MainWindow::saveCameraSettings()
{
    // 保存摄像头设置到exe上一层目录
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir dir(exeDir);
    dir.cdUp(); // 回到上一层目录
    QString settingsPath = dir.absolutePath() + "/EyeTracker_CameraConfig.ini";
    QDir().mkpath(QFileInfo(settingsPath).absolutePath());
    QSettings settings(settingsPath, QSettings::IniFormat);
    settings.setValue("eyeCameraIndex", m_selectedEyeCameraIndex);
    settings.setValue("sceneCameraIndex", m_selectedSceneCameraIndex);
    settings.setValue("eyeCameraRes", m_eyeCameraResComboBox->currentText());
    settings.setValue("eyeCameraFps", m_eyeCameraFpsComboBox->currentText().toInt());
    settings.setValue("sceneCameraRes", m_sceneCameraResComboBox->currentText());
    settings.setValue("sceneCameraFps", m_sceneCameraFpsComboBox->currentText().toInt());

//    qDebug() << "摄像头设置已保存:" << getCameraConfigString();
}

void MainWindow::loadCameraSettings()
{
    // 从exe上一层目录加载摄像头设置
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir dir(exeDir);
    dir.cdUp(); // 回到上一层目录
    QString settingsPath = dir.absolutePath() + "/EyeTracker_CameraConfig.ini";
    QSettings settings(settingsPath, QSettings::IniFormat);

    // 加载摄像头索引
    m_selectedEyeCameraIndex = settings.value("eyeCameraIndex", -1).toInt();
    m_selectedSceneCameraIndex = settings.value("sceneCameraIndex", -1).toInt();

    // 加载分辨率和帧率设置
    QString eyeRes = settings.value("eyeCameraRes", "640x480").toString();
    int eyeFps = settings.value("eyeCameraFps", 240).toInt();
    QString sceneRes = settings.value("sceneCameraRes", "1280x720").toString();
    int sceneFps = settings.value("sceneCameraFps", 60).toInt();

    // 设置UI控件
    int eyeResIndex = m_eyeCameraResComboBox->findText(eyeRes);
    if (eyeResIndex >= 0) m_eyeCameraResComboBox->setCurrentIndex(eyeResIndex);

    int eyeFpsIndex = m_eyeCameraFpsComboBox->findText(QString::number(eyeFps));
    if (eyeFpsIndex >= 0) m_eyeCameraFpsComboBox->setCurrentIndex(eyeFpsIndex);

    int sceneResIndex = m_sceneCameraResComboBox->findText(sceneRes);
    if (sceneResIndex >= 0) m_sceneCameraResComboBox->setCurrentIndex(sceneResIndex);

    int sceneFpsIndex = m_sceneCameraFpsComboBox->findText(QString::number(sceneFps));
    if (sceneFpsIndex >= 0) m_sceneCameraFpsComboBox->setCurrentIndex(sceneFpsIndex);

    // 设置摄像头选择下拉框
    for (int i = 0; i < m_eyeCameraComboBox->count(); ++i) {
        if (m_eyeCameraComboBox->itemData(i).toInt() == m_selectedEyeCameraIndex) {
            m_eyeCameraComboBox->setCurrentIndex(i);
            break;
        }
    }

    for (int i = 0; i < m_sceneCameraComboBox->count(); ++i) {
        if (m_sceneCameraComboBox->itemData(i).toInt() == m_selectedSceneCameraIndex) {
            m_sceneCameraComboBox->setCurrentIndex(i);
            break;
        }
    }

//    qDebug() << "摄像头设置已加载:" << getCameraConfigString();
}

MainWindow::CameraParams MainWindow::getEyeCameraParams() const
{
    QString resStr = m_eyeCameraResComboBox->currentText();
    int fps = m_eyeCameraFpsComboBox->currentText().toInt();
    return CameraParams::fromString(resStr, fps);
}

MainWindow::CameraParams MainWindow::getSceneCameraParams() const
{
    QString resStr = m_sceneCameraResComboBox->currentText();
    int fps = m_sceneCameraFpsComboBox->currentText().toInt();
    return CameraParams::fromString(resStr, fps);
}

// ==================== 槽函数实现 ====================

void MainWindow::on_refreshCamerasBtn_clicked()
{
//    qDebug() << "刷新摄像头设备列表...";

    // 重新枚举设备
    enumerateAvailableCameras();
    populateCameraComboBoxes();

    // 重置选择
    m_selectedEyeCameraIndex = -1;
    m_selectedSceneCameraIndex = -1;

    updateCameraStatus();

    QMessageBox::information(this, "设备刷新",
                           QString("摄像头设备刷新完成，发现 %1 个可用设备").arg(m_availableCameras.size()));
}

void MainWindow::on_applyCameraSettingsBtn_clicked()
{
//    qDebug() << "应用摄像头设置...";

    if (initializeCamerasWithSelection()) {
//        QMessageBox::information(this, "摄像头设置",
//                               QString("摄像头初始化成功!\n\n%1").arg(getCameraConfigString()));
    }

    process = new QProcess(this);  // parent=this 可自动管理内存
//    QString pythonPath = "F:/LY/PycharmProjects/PY39/venv1/Scripts/python.exe";
    QString pythonPath = "F:/LY/PycharmProjects/Anaconda3/envs/EyeVision2.0/venv_clean/Scripts/python.exe";
    process->setProgram(pythonPath);

    // 2. 设置脚本路径 (建议用绝对路径)
    QString scriptPath = "F:/LY/PycharmProjects/Anaconda3/envs/EyeVision2.0/clean/main05.py";
    process->setArguments({scriptPath});


    // 3. 设置工作目录 (影响脚本中的相对路径读取)
    process->setWorkingDirectory("F:/LY/PycharmProjects/Anaconda3/envs/EyeVision2.0/clean");


    // 4. 设置环境变量 (继承系统环境，避免丢失依赖)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // 如果需要，可以在这里插入特定的 PYTHONPATH
    // env.insert("PYTHONPATH", "D:/Projects/MyProjectRoot");
    process->setProcessEnvironment(env);

    // 5. 连接信号处理输出
    // ✅ 1. 标准输出信号 - 用 [this] 替代 [process]
    QObject::connect(process, &QProcess::readyReadStandardOutput, [this]() {
        qDebug() << "Output:" << process->readAllStandardOutput();
    });

    // ✅ 2. 标准错误信号 - 用 [this] 替代 [process]
    QObject::connect(process, &QProcess::readyReadStandardError, [this]() {
        qDebug() << "Error:" << process->readAllStandardError();
    });

    // ✅ 3. 完成信号 - 用 [this] 替代 [process]，并删除 deleteLater()
    QObject::connect(process,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [this](int exitCode, QProcess::ExitStatus exitStatus) {
            qDebug() << "Finished with code:" << exitCode;

            // ❌ 删掉 process->deleteLater();
            // ✅ 因为 process 是成员变量，且构造函数中 new 时传了 parent=this，
            //    Qt 会在 MainWindow 析构时自动清理，手动删反而会导致崩溃

            // 可选：在这里更新 UI 状态，比如恢复按钮
            // ui->btnRun->setEnabled(true);
            // ui->btnStop->setEnabled(false);
        });

    // 6. 启动
    process->start();
    // 错误信息在initializeCamerasWithSelection中已经显示
}

void MainWindow::on_eyeCameraPreviewBtn_clicked()
{
    if (m_selectedEyeCameraIndex >= 0) {
        showCameraPreview(m_selectedEyeCameraIndex, "眼球摄像头预览");
    } else {
        QMessageBox::warning(this, "预览", "请先选择眼球摄像头");
    }
}

void MainWindow::on_sceneCameraPreviewBtn_clicked()
{
    if (m_selectedSceneCameraIndex >= 0) {
        showCameraPreview(m_selectedSceneCameraIndex, "场景摄像头预览");
    } else {
        QMessageBox::warning(this, "预览", "请先选择场景摄像头");
    }
}

void MainWindow::on_eyeCameraComboBox_currentIndexChanged(int index)
{
    if (index > 0) { // 跳过"请选择"选项
        m_selectedEyeCameraIndex = m_eyeCameraComboBox->itemData(index).toInt();
//        qDebug() << "眼球摄像头选择:" << m_selectedEyeCameraIndex;
    } else {
        m_selectedEyeCameraIndex = -1;
    }
    updateCameraStatus();
}

void MainWindow::on_sceneCameraComboBox_currentIndexChanged(int index)
{
    if (index > 0) { // 跳过"请选择"选项
        m_selectedSceneCameraIndex = m_sceneCameraComboBox->itemData(index).toInt();
//        qDebug() << "场景摄像头选择:" << m_selectedSceneCameraIndex;
    } else {
        m_selectedSceneCameraIndex = -1;
    }
    updateCameraStatus();
}

void MainWindow::showCameraPreview(int cameraIndex, const QString& windowTitle)
{
    // 关闭现有预览
    closeCameraPreview();

    // 获取当前选择的摄像头参数
    CameraParams params;
    if (windowTitle.contains("眼球")) {
        params = getEyeCameraParams();  // 获取240fps设置
    } else {
        params = getSceneCameraParams(); // 获取场景摄像头设置
    }

    // 创建DirectShow摄像头配置
    CameraConfig config;
    config.deviceIndex = cameraIndex;
    config.width = params.width;
    config.height = params.height;
    config.fps = params.fps;

    // 创建DirectShow预览摄像头
    m_previewCamera = std::make_unique<DirectShowOpenCVCamera>(cameraIndex, config);

    if (!m_previewCamera->Initialize()) {
        QMessageBox::warning(this, "预览失败", QString("无法初始化摄像头设备 %1\n分辨率: %2x%3\n帧率: %4fps")
                           .arg(cameraIndex).arg(params.width).arg(params.height).arg(params.fps));
        m_previewCamera.reset();
        return;
    }

    if (!m_previewCamera->Start()) {
        QMessageBox::warning(this, "预览失败", QString("无法启动摄像头设备 %1").arg(cameraIndex));
        m_previewCamera.reset();
        return;
    }

    m_previewWindowName = windowTitle;
    cv::namedWindow(m_previewWindowName.toStdString(), cv::WINDOW_AUTOSIZE);

    // 连接新帧信号
    connect(m_previewCamera.get(), &DirectShowOpenCVCamera::newFrameAvailable,
            this, &MainWindow::updateCameraPreview, Qt::QueuedConnection);

    // 启动预览定时器 - 适应高帧率显示
    int displayInterval = max(16, 1000 / min(params.fps, 60)); // 最高60fps显示
    m_previewTimer->start(displayInterval);

//    qDebug() << QString("开始预览摄像头 %1: %2 (%3x%4@%5fps)")
//               .arg(cameraIndex).arg(windowTitle).arg(params.width).arg(params.height).arg(params.fps);
}

void MainWindow::closeCameraPreview()
{
    if (m_previewTimer && m_previewTimer->isActive()) {
        m_previewTimer->stop();
    }

    if (m_previewCamera) {
        // 断开信号连接
        disconnect(m_previewCamera.get(), &DirectShowOpenCVCamera::newFrameAvailable,
                   this, &MainWindow::updateCameraPreview);

        m_previewCamera->Stop();
        m_previewCamera.reset();
    }

    if (!m_previewWindowName.isEmpty()) {
        cv::destroyWindow(m_previewWindowName.toStdString());
        m_previewWindowName.clear();
    }
}

void MainWindow::updateCameraPreview()
{
    if (!m_previewCamera || m_previewCamera->GetStatus() != CameraStatus::RUNNING) {
        closeCameraPreview();
        return;
    }

    cv::Mat frame;
    if (m_previewCamera->opencv_callback_->GetLatestFrame(frame) && !frame.empty()) {
        // 获取摄像头配置信息
        const auto& config = m_previewCamera->config_;

        // 在图像上添加信息文本
        cv::putText(frame, "DirectShow Preview - Press ESC to close",
                   cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        cv::putText(frame, QString("Device: %1").arg(m_previewWindowName).toStdString(),
                   cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);
        cv::putText(frame, QString("Config: %1x%2@%3fps").arg(config.width).arg(config.height).arg(config.fps).toStdString(),
                   cv::Point(10, 90), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);

        cv::imshow(m_previewWindowName.toStdString(), frame);

        // 检查是否按下ESC键
        int key = cv::waitKey(1) & 0xFF;
        if (key == 27) { // ESC键
            closeCameraPreview();
        }
    }
}

// ==================== 专业化UI样式函数实现 ====================

void MainWindow::loadProfessionalStyle()
{
    // 先尝试加载外部QSS文件
    QFile styleFile(":/styles/professional_style.qss");
    if (styleFile.open(QIODevice::ReadOnly)) {
        QString professionalStyle = QLatin1String(styleFile.readAll());
        this->setStyleSheet(professionalStyle);
//        qDebug() << "专业样式表已从QSS文件加载";
        return;
    } else {
//        qDebug() << "无法加载QSS文件，使用内置样式";
    }

    // 备用：内置样式
    QString professionalStyle = R"(
/* 简洁明亮专业样式 */

QMainWindow {
    background-color: #f5f6fa;
    color: #2c3e50;
    font-family: "Microsoft YaHei UI", "Segoe UI", Arial;
    font-size: 14px;
}

/* 简洁菜单栏 - 大字体 */
QMenuBar {
    background-color: #ffffff;
    border-bottom: 2px solid #e9ecef;
    color: #2c3e50;
    font-size: 16px;
    font-weight: 600;
    padding: 12px 0px;
    min-height: 45px;
}

QMenuBar::item {
    background: transparent;
    padding: 12px 24px;
    border-radius: 6px;
    margin: 0px 4px;
}

QMenuBar::item:selected {
    background-color: #e8f4fd;
    color: #1976d2;
}

QMenuBar::item:pressed {
    background-color: #c8e6f5;
}

/* 下拉菜单 */
QMenu {
    background-color: #ffffff;
    border: 1px solid #ddd;
    border-radius: 6px;
    padding: 4px;
    font-size: 13px;
}

QMenu::item {
    background: transparent;
    padding: 8px 20px;
    border-radius: 4px;
    margin: 1px;
    color: #2c3e50;
}

QMenu::item:selected {
    background-color: #e8f4fd;
    color: #1976d2;
}

QMenu::separator {
    height: 1px;
    background-color: #eee;
    margin: 4px 8px;
}

/* 分组框样式 - 大字体 */
QGroupBox {
    background-color: #ffffff;
    border: 2px solid #e1e8ed;
    border-radius: 8px;
    margin: 12px 4px 4px 4px;
    padding-top: 20px;
    font-weight: 600;
    font-size: 18px;
    color: #2c3e50;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 16px;
    padding: 0 12px 0 12px;
    color: #2c3e50;
    font-weight: 700;
    font-size: 18px;
    background: transparent;
}

/* 摄像头控制组特殊样式 */
QGroupBox#cameraGroupBox {
    background-color: #f8f9ff;
    border: 2px solid #6c5ce7;
    color: #2c3e50;
}

QGroupBox#cameraGroupBox::title {
    color: #6c5ce7;
    font-size: 20px;
    font-weight: 700;
}

/* 按钮样式 - 大字体和足够宽度 */
QPushButton {
    background-color: #6c5ce7;
    border: none;
    border-radius: 6px;
    padding: 10px 24px;
    color: #ffffff;
    font-weight: 600;
    font-size: 15px;
    min-height: 35px;
    min-width: 100px;
}

QPushButton:hover {
    background-color: #5f39e8;
}

QPushButton:pressed {
    background-color: #4834d4;
}

QPushButton:disabled {
    background-color: #b8b8b8;
    color: #7a7a7a;
}

/* 下拉框样式 - 大字体 */
QComboBox {
    background-color: #ffffff;
    border: 2px solid #e1e8ed;
    border-radius: 6px;
    padding: 8px 16px;
    color: #2c3e50;
    font-size: 15px;
    font-weight: normal;
    min-height: 25px;
}

QComboBox:hover {
    border-color: #6c5ce7;
}

QComboBox:focus {
    border-color: #6c5ce7;
}

QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 20px;
    border-left: 1px solid #e1e8ed;
    border-top-right-radius: 6px;
    border-bottom-right-radius: 6px;
    background-color: #f8f9fa;
}

QComboBox QAbstractItemView {
    background-color: #ffffff;
    border: 1px solid #e1e8ed;
    border-radius: 4px;
    color: #2c3e50;
    selection-background-color: #e8f4fd;
    selection-color: #1976d2;
    font-size: 15px;
}

/* 标签样式 - 大字体 */
QLabel {
    color: #2c3e50;
    font-size: 16px;
    font-weight: normal;
    background: transparent;
}

QLabel#eyeCameraLabel, QLabel#sceneCameraLabel {
    color: #2c3e50;
    font-weight: 600;
    font-size: 17px;
}

QLabel#cameraStatusLabel {
    background-color: #e8f5e8;
    border: 1px solid #c3e6c3;
    border-radius: 6px;
    padding: 8px 12px;
    color: #155724;
    font-weight: 600;
    font-size: 15px;
}

/* 状态栏样式 - 大字体 */
QStatusBar {
    background-color: #ffffff;
    color: #6c757d;
    border-top: 2px solid #e9ecef;
    font-size: 14px;
    font-weight: normal;
    padding: 8px;
}

/* 按钮特殊样式 - 更大字体和足够宽度 */
QPushButton#eyeCameraPreviewBtn, QPushButton#sceneCameraPreviewBtn {
    background-color: #00b894;
    font-size: 16px;
    padding: 12px 30px;
    min-height: 38px;
    min-width: 120px;
    border-bottom: 3px solid #00a085;
}

QPushButton#eyeCameraPreviewBtn:hover, QPushButton#sceneCameraPreviewBtn:hover {
    background-color: #00a085;
    border-bottom: 2px solid #008f7a;
}

QPushButton#applyCameraSettingsBtn {
    background-color: #e17055;
    font-size: 17px;
    font-weight: 700;
    padding: 14px 32px;
    min-height: 42px;
    min-width: 150px;
    border-bottom: 3px solid #d63031;
}

QPushButton#applyCameraSettingsBtn:hover {
    background-color: #d63031;
    border-bottom: 2px solid #c92a2a;
}

QPushButton#refreshCamerasBtn {
    background-color: #fdcb6e;
    color: #2d3436;
    font-size: 16px;
    font-weight: 600;
    padding: 12px 30px;
    min-height: 38px;
    min-width: 140px;
    border-bottom: 3px solid #f39c12;
}

QPushButton#refreshCamerasBtn:hover {
    background-color: #f39c12;
    border-bottom: 2px solid #e67e22;
}

/* DockWidget 标题栏美化 - 区域分隔 */
QDockWidget {
    background-color: #ffffff;
    border: 2px solid #e9ecef;
    border-radius: 8px;
    font-size: 16px;
    font-weight: 600;
    color: #2c3e50;
    margin: 4px;
}

QDockWidget::title {
    background-color: #f8f9fa;
    border-bottom: 2px solid #e9ecef;
    padding: 18px;
    font-size: 16px;
    font-weight: 700;
    font-family: "Microsoft YaHei", "Segoe UI Emoji", Arial, sans-serif;
    color: #495057;
    text-align: center;
}

QDockWidget::close-button, QDockWidget::float-button {
    background-color: #6c757d;
    border: none;
    border-radius: 4px;
    padding: 4px;
    margin: 2px;
}

QDockWidget::close-button:hover, QDockWidget::float-button:hover {
    background-color: #495057;
}

/* 分隔线和区域美化 */
QSplitter::handle {
    background-color: #dee2e6;
    border: 1px solid #adb5bd;
    border-radius: 2px;
}

QSplitter::handle:horizontal {
    width: 6px;
    margin: 2px 0px;
}

QSplitter::handle:vertical {
    height: 6px;
    margin: 0px 2px;
}

QSplitter::handle:hover {
    background-color: #6c5ce7;
}
)";

    this->setStyleSheet(professionalStyle);
}

// ==================== 专业相机控制系统实现 ====================

void MainWindow::createAdvancedCameraControls()
{
//    qDebug() << "=== 开始创建专业相机控制组件（独立创建，不依赖控件窗口） ===";

    // 如果组框已存在，直接返回
    if (m_advancedCameraGroupBox) {
//        qDebug() << "专业相机控制组框已存在，跳过创建";
        return;
    }

//    qDebug() << "开始创建专业相机控制组件";

    // 创建专业相机控制分组框
    m_advancedCameraGroupBox = new QGroupBox("");
    m_advancedCameraGroupBox->setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
    m_advancedCameraGroupBox->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #cccccc;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px 0 5px;"
        "    color: #2c3e50;"
        "}"
    );

    // 创建布局
    QVBoxLayout* advancedLayout = new QVBoxLayout(m_advancedCameraGroupBox);
    advancedLayout->setContentsMargins(5, 15, 5, 5);
    advancedLayout->setSpacing(5);

    // 创建树形控件
    m_advancedCameraTree = new QTreeWidget(m_advancedCameraGroupBox);
    m_advancedCameraTree->setHeaderHidden(true);
    m_advancedCameraTree->setIndentation(0);
    m_advancedCameraTree->setAnimated(true);
    m_advancedCameraTree->setMinimumHeight(800);
    m_advancedCameraTree->setMaximumHeight(800);


    advancedLayout->addWidget(m_advancedCameraTree);

//    qDebug() << "专业相机控制分组框已创建，现在设置布局";

    // 设置专业相机控制的布局
    setupAdvancedCameraLayout();

//    qDebug() << "布局设置完成，现在添加到主界面";

//    qDebug() << "专业相机控制组件已创建完成，准备添加到独立窗口中";

    // 初始化相机适配器
    m_cameraAdapter = new CameraSettingsAdapter(this);

    // 连接信号
    connectAdvancedCameraSignals();

//    qDebug() << "专业相机控制界面已创建";
}

void MainWindow::setupTabifiedDockWidgets()
{
//    qDebug() << "=== 恢复Qt原生dock widget完全自由停靠功能 ===";

    // ========== 【恢复】Qt原生灵活停靠系统 ==========

    // 启用dock widget嵌套功能
    setDockNestingEnabled(true);

    // 收集所有dock widgets
    QList<QDockWidget*> allDocks;
    if (m_controlsDock) allDocks.append(m_controlsDock);
    if (m_dataDock) allDocks.append(m_dataDock);
    if (m_graphTabDock) allDocks.append(m_graphTabDock);
    if (m_sceneDock) allDocks.append(m_sceneDock);
    if (m_eyeDock) allDocks.append(m_eyeDock);
    if (m_cameraDock) allDocks.append(m_cameraDock);

    // 为所有dock widgets设置完全自由的停靠属性
    for (QDockWidget* dock : allDocks) {
        if (dock) {
            // 启用Qt原生的完全自由停靠功能
            dock->setAllowedAreas(Qt::AllDockWidgetAreas);

            // 启用所有dock widget功能
            dock->setFeatures(QDockWidget::DockWidgetMovable |
                             QDockWidget::DockWidgetFloatable |
                             QDockWidget::DockWidgetClosable);

            // 设置合理的最小尺寸，但不限制布局
            dock->setMinimumSize(200, 150);
            dock->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

//            qDebug() << "已启用完全自由停靠:" << dock->windowTitle();
        }
    }

    // 不强制任何特定布局，让Qt自然处理dock widget位置
    // 用户可以自由拖拽到任何位置：左、右、上、下、中间、浮动、标签页

//    qDebug() << "=== Qt原生完全自由停靠功能已恢复 ===";

    // 设置响应式布局
    setupResponsiveLayout();
}

// 响应式布局支持
void MainWindow::setupResponsiveLayout() {
    // 设置初始尺寸约束
    setDockSizeConstraints();

    // 调整初始尺寸
    adjustDockSizesForMainWindow();
}

void MainWindow::setDockSizeConstraints() {
    int mainWidth = this->width();
    int mainHeight = this->height();

    // 【优化】只有在主窗口非常小的时候才调整约束，保护用户的布局设置
    if (mainWidth < 600 || mainHeight < 400) {
        QList<QDockWidget*> allDocks = {m_controlsDock, m_dataDock, m_graphTabDock, m_cameraDock, m_aiStatusDock, m_sceneDock};

        for (QDockWidget* dock : allDocks) {
            if (dock) {
                // 只在主窗口过小时设置保护性的最小尺寸
                int minWidth = qMax(150, mainWidth * 10 / 100);
                int minHeight = qMax(100, mainHeight * 10 / 100);

                // 检查当前最小尺寸，避免不必要的设置
                QSize currentMinSize = dock->minimumSize();
                if (currentMinSize.width() != minWidth || currentMinSize.height() != minHeight) {
                    dock->setMinimumSize(minWidth, minHeight);
                }

                // 确保最大尺寸不受限制
                dock->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            }
        }
    }
    // 【重要】在主窗口尺寸正常的情况下，完全不调整dock约束，让用户自由布局
}

void MainWindow::adjustDockSizesForMainWindow() {
    // 【优化】减少对用户布局的干扰，只在初次创建或明确需要时调整
    static bool initialAdjustmentDone = false;

    // 只在特定情况下进行调整：
    // 1. 初次创建时
    // 2. 主窗口宽度小于800像素时（需要保护性调整）
    bool needsAdjustment = !initialAdjustmentDone || this->width() < 800;

    if (!needsAdjustment || !m_controlsDock || !m_dataDock) {
        return;
    }

    int mainWidth = this->width();

    // 只在窗口较小时进行保护性的比例调整
    if (mainWidth < 800) {
        // 计算合理的宽度，确保dock不会太小
        int controlsWidth = qMax(200, mainWidth * 20 / 100);  // 至少200px，最多20%
        int dataWidth = qMax(250, mainWidth * 25 / 100);      // 至少250px，最多25%

        // 只调整宽度，让用户保持高度设置
        resizeDocks({m_controlsDock}, {controlsWidth}, Qt::Horizontal);
        resizeDocks({m_dataDock}, {dataWidth}, Qt::Horizontal);
    }

    initialAdjustmentDone = true;
}

void MainWindow::createCameraDockWindow()
{
//    qDebug() << "=== 开始创建独立的摄像头控制窗口 ===";

    // 创建摄像头控制dock widget
    m_cameraDock = new QDockWidget(tr("📷 摄像头控制"), this);
    m_cameraDock->setStyleSheet(
        "QDockWidget::title {"
        "    text-align: left;"
        "    font-size: 16px !important;" // <--- 修改：从 20px 改为 16px
        "    font-weight: bold !important;"
        "    padding: 18px !important;"
        "    color: #2c3e50 !important;"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f8f9fa, stop:1 #e9ecef) !important;"
        "    border: 1px solid #dee2e6 !important;"
        "    min-height: 35px !important;"
        "}"
        "QDockWidget {"
        "    font-size: 16px !important;" // <--- 修改：从 20px 改为 16px
        "}");
    // 强制设置标题字体
    QFont cameraTitleFont("Microsoft YaHei", 14, QFont::Bold); // <--- 修改：从 20 改为 16
    m_cameraDock->setFont(cameraTitleFont);
    // 样式由全局CSS统一管理

    // 字体已在CSS中设置
    m_cameraDock->setObjectName("CameraDock");
    m_cameraDock->setAttribute(Qt::WA_DeleteOnClose, false);
    m_cameraDock->setFeatures(QDockWidget::DockWidgetMovable |
                             QDockWidget::DockWidgetFloatable |
                             QDockWidget::DockWidgetClosable);

    // 移除手动位置保存连接，统一使用Qt原生状态管理

    // 设置窗口图标
    QIcon cameraIcon(":/icons/iconSet/cameraList.png");
    if (cameraIcon.isNull()) {
        // 如果图标不存在，创建一个简单的文本图标
        QPixmap pixmap(16, 16);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setPen(Qt::black);
        painter.drawText(pixmap.rect(), Qt::AlignCenter, "📷");
        cameraIcon = QIcon(pixmap);
    }
    m_cameraDock->setWindowIcon(cameraIcon);

    // 创建摄像头控制主widget
    QWidget* cameraMainWidget = new QWidget();
    QVBoxLayout* cameraMainLayout = new QVBoxLayout(cameraMainWidget);
    cameraMainLayout->setContentsMargins(5, 5, 5, 5);
    cameraMainLayout->setSpacing(10);

    // 创建tab widget来组织摄像头功能
    QTabWidget* cameraTabWidget = new QTabWidget(cameraMainWidget);
    cameraTabWidget->setTabPosition(QTabWidget::North);

    // === Tab 1: 相机选择 ===
    QWidget* cameraSelectionTab = new QWidget();
    QVBoxLayout* selectionLayout = new QVBoxLayout(cameraSelectionTab);
    selectionLayout->setContentsMargins(10, 15, 10, 10); // 恢复正常边距，无标题不会遮挡

    // 创建相机选择组框
    createCameraSelectionUIInControlsDock();
    if (m_cameraGroupBox) {
        m_cameraGroupBox->setParent(cameraSelectionTab);
        selectionLayout->addWidget(m_cameraGroupBox);
        selectionLayout->addStretch();
        m_cameraGroupBox->show();

        // 枚举可用摄像头并填充下拉框
        enumerateAvailableCameras();
        populateCameraComboBoxes();
        loadCameraSettings();
    }

    cameraTabWidget->addTab(cameraSelectionTab, "📷 设备选择");

    // === Tab 2: 专业设置 ===
    QWidget* advancedSettingsTab = new QWidget();
    QVBoxLayout* advancedLayout = new QVBoxLayout(advancedSettingsTab);
    advancedLayout->setContentsMargins(10, 15, 10, 10); // 恢复正常边距，无标题不会遮挡

    // 创建专业相机控制组框
    createAdvancedCameraControls();
    if (m_advancedCameraGroupBox) {
        m_advancedCameraGroupBox->setParent(advancedSettingsTab);
        advancedLayout->addWidget(m_advancedCameraGroupBox);
        advancedLayout->addStretch();
        m_advancedCameraGroupBox->show();
    }

    cameraTabWidget->addTab(advancedSettingsTab, "⚙️ 专业设置");

    // 将tab widget添加到主布局
    cameraMainLayout->addWidget(cameraTabWidget);

    // 创建滚动区域
    QScrollArea* cameraScrollArea = new QScrollArea();
    cameraScrollArea->setWidgetResizable(true);
    cameraScrollArea->setFrameShape(QFrame::NoFrame);
    cameraScrollArea->setWidget(cameraMainWidget);

    // 设置dock widget的内容
    m_cameraDock->setWidget(cameraScrollArea);

    // 设置dock widget的尺寸 - 放宽限制以支持更灵活的布局
    m_cameraDock->setMinimumSize(300, 250);
    // 移除最大宽度限制，允许更灵活的调整
    // m_cameraDock->setMaximumWidth(500);

    // 将dock widget添加到主窗口 - 初始放在右侧，但允许用户移动到任何区域
    addDockWidget(Qt::RightDockWidgetArea, m_cameraDock);
    m_dockWidgets.append(m_cameraDock);

    // 为动态创建的dock窗口恢复保存的尺寸和状态
    QTimer::singleShot(500, this, [this]() {
        QString exeDir = QCoreApplication::applicationDirPath();
        QDir dir(exeDir);
        dir.cdUp();
        QString settingsPath = dir.absolutePath() + "/EyeTracker_WindowConfig.ini";
        QSettings settings(settingsPath, QSettings::IniFormat);

        if (m_cameraDock && settings.contains("docks/cameraDock/geometry")) {
            QByteArray dockGeometry = settings.value("docks/cameraDock/geometry").toByteArray();
            bool floating = settings.value("docks/cameraDock/floating", false).toBool();
            // 动态创建时强制显示摄像头控制窗口
            bool visible = true;  // 始终显示
            int width = settings.value("docks/cameraDock/width", 300).toInt();
            int height = settings.value("docks/cameraDock/height", 300).toInt();

            if (!dockGeometry.isEmpty()) {
                m_cameraDock->restoreGeometry(dockGeometry);
            }
            m_cameraDock->setFloating(floating);
            m_cameraDock->setVisible(visible);
            m_cameraDock->show();  // 强制显示
            m_cameraDock->raise(); // 提升到前面
            // 使用resizeDocks方法恢复尺寸
            if (!floating && width > 0 && height > 0) {
                QTimer::singleShot(100, this, [this, width, height]() {
                    resizeDocks({m_cameraDock}, {width}, Qt::Horizontal);
                    QTimer::singleShot(50, this, [this, height]() {
                        resizeDocks({m_cameraDock}, {height}, Qt::Vertical);
                    });
                });
            }
        }
    });

    // 设置摄像头dock的属性以支持灵活布局
    m_cameraDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_cameraDock->setFeatures(QDockWidget::DockWidgetMovable |
                             QDockWidget::DockWidgetFloatable |
                             QDockWidget::DockWidgetClosable);

    // 减少强制tabify关系，只在必要时进行tabify
    // 这样用户可以更自由地将摄像头窗口拖拽到左侧与控件窗口并排
    if (m_dataDock && m_graphTabDock) {
        tabifyDockWidget(m_dataDock, m_graphTabDock);
        m_graphTabDock->raise(); // 激活图表分析窗口以便记住大小
//        qDebug() << "已将数据分析和图表分析窗口设置为tab模式，图表分析窗口已激活";
    }

    // 强制显示摄像头控制窗口
    m_cameraDock->show();
    m_cameraDock->raise();
    m_cameraDock->setVisible(true);

    // 设置摄像头控制窗口的尺寸
    resizeDocks({m_cameraDock}, {450}, Qt::Horizontal);

//    qDebug() << "独立的摄像头控制窗口已创建，包含设备选择和专业设置两个tab";
//    qDebug() << "摄像头控制窗口已添加到右侧dock区域并强制显示";
}

void MainWindow::createAIStatusDockWindow()
{
    qDebug() << "=== 开始创建独立的AI状态窗口 ===";

    // 创建AI状态dock widget
    m_aiStatusDock = new QDockWidget(tr("🤖 AI分析状态"), this);
    m_aiStatusDock->setStyleSheet(
        "QDockWidget::title {"
        "    text-align: left;"
        "    font-size: 16px !important;"
        "    font-weight: bold;"
        "    padding: 8px;"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f0f8ff, stop:1 #e8f4f8);"
        "    border: 1px solid #b0c4de;"
        "    border-radius: 5px;"
        "}");

    // 强制设置标题字体
    QFont aiTitleFont("Microsoft YaHei", 14, QFont::Bold);
    m_aiStatusDock->setFont(aiTitleFont);

    m_aiStatusDock->setObjectName("AIStatusDock");
    m_aiStatusDock->setAttribute(Qt::WA_DeleteOnClose, false);
    m_aiStatusDock->setFeatures(QDockWidget::DockWidgetMovable |
                               QDockWidget::DockWidgetFloatable |
                               QDockWidget::DockWidgetClosable);

    // 创建窗口图标
    QIcon aiIcon;
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(0, 150, 255), 2));
    painter.setFont(QFont("Arial", 12, QFont::Bold));
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "🤖");
    aiIcon = QIcon(pixmap);
    m_aiStatusDock->setWindowIcon(aiIcon);

    // 创建AI状态主widget
    QWidget* aiMainWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(aiMainWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // 语音识别文字和编辑框
    QLabel* voiceLabel = new QLabel("语音识别结果:");
    voiceLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #2196F3; margin-bottom: 5px;");
    m_voiceDisplayEdit = new QTextEdit();
    m_voiceDisplayEdit->setFixedHeight(60);
    m_voiceDisplayEdit->setPlaceholderText("等待语音识别结果...");
    mainLayout->addWidget(voiceLabel);
    mainLayout->addWidget(m_voiceDisplayEdit);
    mainLayout->addSpacing(20);

    // AI分析文字和编辑框
    QLabel* aiLabel = new QLabel("AI分析响应:");
    aiLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #4CAF50; margin-bottom: 5px;");
    m_aiResponseDisplayEdit = new QTextEdit();
    m_aiResponseDisplayEdit->setFixedHeight(200);  // 增加高度从100到200
    m_aiResponseDisplayEdit->setPlaceholderText("等待AI分析结果...");
    mainLayout->addWidget(aiLabel);
    mainLayout->addWidget(m_aiResponseDisplayEdit);

    // ASDF模式按钮组
    QLabel* modeLabel = new QLabel("模式选择:");
    modeLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #9C27B0; margin-top: 10px; margin-bottom: 5px;");
    mainLayout->addWidget(modeLabel);

    QHBoxLayout* modeButtonLayout = new QHBoxLayout();
    modeButtonLayout->setSpacing(8);

    // A模式按钮
    m_modeAButton = new QPushButton("A");
    m_modeAButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "    padding: 8px 16px;"
        "    border: none;"
        "    border-radius: 6px;"
        "    margin: 5px 0px;"
        "    min-width: 40px;"
        "    max-width: 40px;"
        "    min-height: 35px;"
        "    max-height: 40px;"
        "}"
        "QPushButton:hover { background-color: #45a049; }"
    );
    connect(m_modeAButton, &QPushButton::clicked, this, &MainWindow::onModeAClicked);
    modeButtonLayout->addWidget(m_modeAButton);

    // S模式按钮
    m_modeSButton = new QPushButton("S");
    m_modeSButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "    padding: 8px 16px;"
        "    border: none;"
        "    border-radius: 6px;"
        "    margin: 5px 0px;"
        "    min-width: 40px;"
        "    max-width: 40px;"
        "    min-height: 35px;"
        "    max-height: 40px;"
        "}"
        "QPushButton:hover { background-color: #1976D2; }"
    );
    connect(m_modeSButton, &QPushButton::clicked, this, &MainWindow::onModeSClicked);
    modeButtonLayout->addWidget(m_modeSButton);

    // D模式按钮
    m_modeDButton = new QPushButton("D");
    m_modeDButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #FF5722;"
        "    color: white;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "    padding: 8px 16px;"
        "    border: none;"
        "    border-radius: 6px;"
        "    margin: 5px 0px;"
        "    min-width: 40px;"
        "    max-width: 40px;"
        "    min-height: 35px;"
        "    max-height: 40px;"
        "}"
        "QPushButton:hover { background-color: #E64A19; }"
    );
    connect(m_modeDButton, &QPushButton::clicked, this, &MainWindow::onModeDClicked);
    modeButtonLayout->addWidget(m_modeDButton);

    // F模式按钮
    m_modeFButton = new QPushButton("F");
    m_modeFButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #9C27B0;"
        "    color: white;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "    padding: 8px 16px;"
        "    border: none;"
        "    border-radius: 6px;"
        "    margin: 5px 0px;"
        "    min-width: 40px;"
        "    max-width: 40px;"
        "    min-height: 35px;"
        "    max-height: 40px;"
        "}"
        "QPushButton:hover { background-color: #7B1FA2; }"
    );
    connect(m_modeFButton, &QPushButton::clicked, this, &MainWindow::onModeFClicked);
    modeButtonLayout->addWidget(m_modeFButton);

    modeButtonLayout->addStretch(); // 添加弹性空间使按钮左对齐
    mainLayout->addLayout(modeButtonLayout);

    // ASDF模式状态显示标签
    m_modeStatusLabel = new QLabel("等待模式选择...");
    m_modeStatusLabel->setFixedHeight(30);
    m_modeStatusLabel->setWordWrap(true);
    m_modeStatusLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_modeStatusLabel->setStyleSheet(
        "font-family: 'Microsoft YaHei'; "
        "font-size: 12px; "
        "color: #333333; "
        "padding: 5px 0px;"
    );
    mainLayout->addWidget(m_modeStatusLabel);
    mainLayout->addSpacing(20);

    // 目标检测文字和显示框
    QLabel* detectionTitleLabel = new QLabel("目标检测信息:");
    detectionTitleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #FF9800; margin-bottom: 5px;");
    m_detectionInfoLabel = new QLabel("⏳ 等待目标检测结果...\n💡 点击上方'开始目标检测'按钮启动检测");
    m_detectionInfoLabel->setFixedHeight(180);  // 增加高度从120到180
    m_detectionInfoLabel->setWordWrap(true);
    m_detectionInfoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_detectionInfoLabel->setStyleSheet(
        "border: 1px solid #FF9800; "
        "padding: 8px; "
        "background-color: #FFF8E1; "
        "font-family: 'Consolas', 'Monaco', monospace; "  // 等宽字体便于坐标对齐
        "font-size: 14px; "  // 增大字体从11px到14px
        "font-weight: bold;"
    );
    mainLayout->addWidget(detectionTitleLabel);
    mainLayout->addWidget(m_detectionInfoLabel);

    // 目标检测按钮（放在目标检测信息下面）
    m_targetDetectionButton = new QPushButton("🎯 开始目标检测");
    m_targetDetectionButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #FF9800;"
        "    color: white;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "    padding: 8px 16px;"
        "    border: none;"
        "    border-radius: 6px;"
        "    margin: 5px 0px;"
        "    min-width: 120px;"
        "    max-width: 150px;"
        "    min-height: 35px;"
        "    max-height: 40px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #E65100;"
        "}"
    );
    connect(m_targetDetectionButton, &QPushButton::clicked, this, &MainWindow::onTargetDetectionClicked);
    mainLayout->addWidget(m_targetDetectionButton);

    mainLayout->addStretch(); // 添加弹性空间

    // 创建滚动区域
    QScrollArea* aiScrollArea = new QScrollArea();
    aiScrollArea->setWidget(aiMainWidget);
    aiScrollArea->setWidgetResizable(true);
    aiScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    aiScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 设置dock widget的内容
    m_aiStatusDock->setWidget(aiScrollArea);

    // 设置dock widget的尺寸
    m_aiStatusDock->setMinimumSize(320, 400);
    m_aiStatusDock->setMaximumWidth(600);

    // 将dock widget添加到主窗口 - 初始放在右侧
    addDockWidget(Qt::RightDockWidgetArea, m_aiStatusDock);
    m_dockWidgets.append(m_aiStatusDock);

    // 为动态创建的dock窗口恢复保存的尺寸和状态
    QTimer::singleShot(500, this, [this]() {
        QString exeDir = QCoreApplication::applicationDirPath();
        QDir dir(exeDir);
        dir.cdUp();
        QString settingsPath = dir.absolutePath() + "/EyeTracker_WindowConfig.ini";
        QSettings settings(settingsPath, QSettings::IniFormat);

        if (m_aiStatusDock && settings.contains("docks/aiStatusDock/geometry")) {
            QByteArray dockGeometry = settings.value("docks/aiStatusDock/geometry").toByteArray();
            bool floating = settings.value("docks/aiStatusDock/floating", false).toBool();
            // 动态创建时强制显示AI状态窗口
            bool visible = true;  // 始终显示
            int width = settings.value("docks/aiStatusDock/width", 400).toInt();
            int height = settings.value("docks/aiStatusDock/height", 400).toInt();

            if (!dockGeometry.isEmpty()) {
                m_aiStatusDock->restoreGeometry(dockGeometry);
            }
            m_aiStatusDock->setFloating(floating);
            m_aiStatusDock->setVisible(visible);
            m_aiStatusDock->show();  // 强制显示
            m_aiStatusDock->raise(); // 提升到前面
            // 使用resizeDocks方法恢复尺寸
            if (!floating && width > 0 && height > 0) {
                QTimer::singleShot(100, this, [this, width, height]() {
                    resizeDocks({m_aiStatusDock}, {width}, Qt::Horizontal);
                    QTimer::singleShot(50, this, [this, height]() {
                        resizeDocks({m_aiStatusDock}, {height}, Qt::Vertical);
                    });
                });
            }
        }
    });

    // 设置AI状态dock的属性以支持灵活布局
    m_aiStatusDock->setAllowedAreas(Qt::AllDockWidgetAreas);

    // 连接信号（窗口状态保存）
    if (m_aiStatusDock) {
        connect(m_aiStatusDock, &QDockWidget::dockLocationChanged, this, [this]() {
            QTimer::singleShot(1000, this, &MainWindow::saveWindowPositions);
        });
        connect(m_aiStatusDock, &QDockWidget::topLevelChanged, this, [this]() {
            QTimer::singleShot(1000, this, &MainWindow::saveWindowPositions);
        });
        connect(m_aiStatusDock, &QDockWidget::visibilityChanged, this, [this]() {
            QTimer::singleShot(1000, this, &MainWindow::saveWindowPositions);
        });
    }

    // 强制显示AI状态窗口
    m_aiStatusDock->show();
    m_aiStatusDock->raise();
    m_aiStatusDock->setVisible(true);

    // 设置AI状态窗口的尺寸
    resizeDocks({m_aiStatusDock}, {400}, Qt::Horizontal);

    qDebug() << "独立的AI状态窗口已创建并添加到右侧dock区域";
}

void MainWindow::setupUIButtonIcons()
{
//    qDebug() << "[INFO] 开始设置专业级UI按钮图标";

    // 创建增强图标的辅助函数
    auto createEnhancedIcon = [](const QString& iconPath, const QColor& backgroundColor = QColor(255, 255, 255, 180)) -> QIcon {
        QIcon originalIcon(iconPath);
        if (originalIcon.isNull()) {
//            qDebug() << "[WARNING] 图标加载失败:" << iconPath;
            return QIcon();
        }

        // 创建增强版本的图标
        QPixmap originalPixmap = originalIcon.pixmap(24, 24);
        QPixmap enhancedPixmap(28, 28);
        enhancedPixmap.fill(Qt::transparent);

        QPainter painter(&enhancedPixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        // 绘制圆形背景
        painter.setBrush(QBrush(backgroundColor));
        painter.setPen(QPen(QColor(200, 200, 200, 150), 1));
        painter.drawEllipse(2, 2, 24, 24);

        // 绘制图标
        painter.drawPixmap(2, 2, originalPixmap);

        return QIcon(enhancedPixmap);
    };

    // 设置相机控制按钮图标（这些是可见的）- 使用蓝色主题
    if (ui->eyeCameraPreviewBtn) {
        ui->eyeCameraPreviewBtn->setIcon(createEnhancedIcon(":/icons/iconSet/sampling.png", QColor(74, 144, 226, 180)));
        ui->eyeCameraPreviewBtn->setText("预览");
        ui->eyeCameraPreviewBtn->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4A90E2, stop:1 #357ABD);"
            "    color: white; font-weight: bold; border-radius: 6px;"
            "    border: 1px solid #357ABD;"
            "    font-family: 'Microsoft YaHei';"
            "    font-size: 9px;"
            "    padding: 4px 8px;"
            "}"
            "QPushButton:hover {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #5BA0F2, stop:1 #4A8ACD);"
            "}"
        );
//        qDebug() << "[SUCCESS] 眼球摄像头预览按钮专业图标已设置";
    }

    if (ui->sceneCameraPreviewBtn) {
        ui->sceneCameraPreviewBtn->setIcon(createEnhancedIcon(":/icons/iconSet/cameraList.png", QColor(74, 144, 226, 180)));
        ui->sceneCameraPreviewBtn->setText("预览");
        ui->sceneCameraPreviewBtn->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4A90E2, stop:1 #357ABD);"
            "    color: white; font-weight: bold; border-radius: 6px;"
            "    border: 1px solid #357ABD;"
            "    font-family: 'Microsoft YaHei';"
            "    font-size: 9px;"
            "    padding: 4px 8px;"
            "}"
            "QPushButton:hover {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #5BA0F2, stop:1 #4A8ACD);"
            "}"
        );
//        qDebug() << "[SUCCESS] 场景摄像头预览按钮专业图标已设置";
    }

    if (ui->refreshCamerasBtn) {
        ui->refreshCamerasBtn->setIcon(createEnhancedIcon(":/icons/iconSet/settings.png", QColor(255, 193, 7, 180)));
        ui->refreshCamerasBtn->setText("刷新设备");
        ui->refreshCamerasBtn->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FFC107, stop:1 #FF8F00);"
            "    color: white; font-weight: bold; border-radius: 6px;"
            "    border: 1px solid #FF8F00;"
            "    font-family: 'Microsoft YaHei';"
            "    font-size: 9px;"
            "    padding: 4px 8px;"
            "    min-width: 120px;"
            "}"
            "QPushButton:hover {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FFD54F, stop:1 #FFA000);"
            "}"
        );
//        qDebug() << "[SUCCESS] 刷新摄像头按钮专业图标已设置";
    }

    if (ui->applyCameraSettingsBtn) {
        ui->applyCameraSettingsBtn->setIcon(createEnhancedIcon(":/icons/iconSet/settings.png", QColor(76, 175, 80, 180)));
        ui->applyCameraSettingsBtn->setText("应用设置");
        ui->applyCameraSettingsBtn->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4CAF50, stop:1 #388E3C);"
            "    color: white; font-weight: bold; border-radius: 6px;"
            "    border: 1px solid #388E3C;"
            "    font-family: 'Microsoft YaHei';"
            "    font-size: 9px;"
            "    padding: 4px 8px;"
            "    min-width: 120px;"
            "}"
            "QPushButton:hover {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #66BB6A, stop:1 #4CAF50);"
            "}"
        );
//        qDebug() << "[SUCCESS] 应用设置按钮专业图标已设置";
    }

    // 为其他按钮设置专业图标（即使隐藏也设置，以防将来显示）- 使用不同颜色主题
    if (ui->calibPointBtn) {
        ui->calibPointBtn->setIcon(createEnhancedIcon(":/icons/iconSet/ROI.png", QColor(156, 39, 176, 180)));
        ui->calibPointBtn->setText("🎯 精确标定点输入");
        ui->calibPointBtn->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #9C27B0, stop:1 #7B1FA2);"
            "    color: white; font-weight: bold; border-radius: 6px; padding: 6px 12px;"
            "}"
        );
    }

    if (ui->setOrignalBtn) {
        ui->setOrignalBtn->setIcon(createEnhancedIcon(":/icons/iconSet/settings.png", QColor(255, 87, 34, 180)));
        ui->setOrignalBtn->setText("🔧 参数初始化");
        ui->setOrignalBtn->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FF5722, stop:1 #D84315);"
            "    color: white; font-weight: bold; border-radius: 6px; padding: 6px 12px;"
            "}"
        );
    }

    if (ui->calibImgCollectBtn) {
        ui->calibImgCollectBtn->setIcon(createEnhancedIcon(":/icons/iconSet/sampling.png", QColor(0, 188, 212, 180)));
        ui->calibImgCollectBtn->setText("📸 标定图像采集");
        ui->calibImgCollectBtn->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00BCD4, stop:1 #0097A7);"
            "    color: white; font-weight: bold; border-radius: 6px; padding: 6px 12px;"
            "}"
        );
    }

    if (ui->TestButton) {
        ui->TestButton->setIcon(createEnhancedIcon(":/icons/iconSet/misc.png", QColor(255, 152, 0, 180)));
        ui->TestButton->setText("🧪 精确测试");
        ui->TestButton->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FF9800, stop:1 #F57C00);"
            "    color: white; font-weight: bold; border-radius: 6px; padding: 6px 12px;"
            "}"
        );
    }

    if (ui->TestResultButton) {
        ui->TestResultButton->setIcon(createEnhancedIcon(":/icons/iconSet/histogram.png", QColor(103, 58, 183, 180)));
        ui->TestResultButton->setText("📊 测试结果");
        ui->TestResultButton->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #673AB7, stop:1 #512DA8);"
            "    color: white; font-weight: bold; border-radius: 6px; padding: 6px 12px;"
            "}"
        );
    }

    if (ui->pushButton) {
        ui->pushButton->setIcon(createEnhancedIcon(":/icons/iconSet/misc.png", QColor(76, 175, 80, 180)));
        ui->pushButton->setText("🎯 标定");
        ui->pushButton->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4CAF50, stop:1 #388E3C);"
            "    color: white; font-weight: bold; border-radius: 6px; padding: 8px 16px;"
            "    font-size: 16px;"
            "}"
        );
    }

    if (ui->ReloadButton) {
        ui->ReloadButton->setIcon(createEnhancedIcon(":/icons/iconSet/settings.png", QColor(33, 150, 243, 180)));
        ui->ReloadButton->setText("🔄 重载");
        ui->ReloadButton->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2196F3, stop:1 #1976D2);"
            "    color: white; font-weight: bold; border-radius: 6px; padding: 8px 16px;"
            "    font-size: 16px;"
            "}"
        );
    }

    if (ui->openButton) {
        ui->openButton->setIcon(createEnhancedIcon(":/icons/iconSet/cameraList.png", QColor(76, 175, 80, 180)));
        ui->openButton->setText("📷 打开摄像头");
        ui->openButton->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4CAF50, stop:1 #388E3C);"
            "    color: white; font-weight: bold; border-radius: 6px; padding: 6px 12px;"
            "}"
        );
    }

    if (ui->interact01) {
        ui->interact01->setIcon(createEnhancedIcon(":/icons/iconSet/misc.png", QColor(255, 87, 34, 180)));
        ui->interact01->setText("🤝 交互");
        ui->interact01->setStyleSheet(
            "QPushButton {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FF5722, stop:1 #D84315);"
            "    color: white; font-weight: bold; border-radius: 6px; padding: 6px 12px;"
            "}"
        );
    }

    // 其他隐藏按钮也设置图标
    if (ui->Markcalib) {
        ui->Markcalib->setIcon(createEnhancedIcon(":/icons/iconSet/misc.png", QColor(121, 85, 72, 180)));
        ui->Markcalib->setText("🔧 Markcalib");
    }

    if (ui->pointcalibButton_2) {
        ui->pointcalibButton_2->setIcon(createEnhancedIcon(":/icons/iconSet/misc.png", QColor(96, 125, 139, 180)));
        ui->pointcalibButton_2->setText("🎨 点可视化");
    }

    if (ui->MarkerBtn) {
        ui->MarkerBtn->setIcon(createEnhancedIcon(":/icons/iconSet/misc.png", QColor(158, 158, 158, 180)));
        ui->MarkerBtn->setText("📌 Markcollect");
    }

//    qDebug() << "[SUCCESS] 所有UI按钮专业图标设置完成！";
}

void MainWindow::setupAdvancedCameraLayout()
{
//    qDebug() << "开始设置专业相机控制布局";

    if (!m_advancedCameraTree) {
//        qDebug() << "错误：专业相机树形控件不存在";
        return;
    }

//    qDebug() << "创建曝光控制分类";

    // 创建曝光控制分类
    {
        QTreeWidgetItem* pCategory = new QTreeWidgetItem();
        m_advancedCameraTree->addTopLevelItem(pCategory);

//        qDebug() << "创建分类按钮";
        m_exposureButton = new QtCategoryButton1(tr("Exposure & Gain"),
                                                ":/icons/iconSet/exposureValue.png",
                                                m_advancedCameraTree,
                                                pCategory);
        m_advancedCameraTree->setItemWidget(pCategory, 0, m_exposureButton);
//        qDebug() << "分类按钮已创建并设置";

//        qDebug() << "创建内容容器";
        QFrame* pFrame = new QFrame(m_advancedCameraTree);
        QVBoxLayout* pLayout = new QVBoxLayout(pFrame);
        pLayout->setMargin(0);

//        qDebug() << "创建曝光控制表单";
        m_exposureForm = new FormExposure(pFrame);
        pLayout->addWidget(m_exposureForm);
//        qDebug() << "曝光控制表单已创建并添加到布局";

//        qDebug() << "添加到树形控件";
        QTreeWidgetItem* pContainer = new QTreeWidgetItem();
        pContainer->setDisabled(true);
        pCategory->addChild(pContainer);
        m_advancedCameraTree->setItemWidget(pContainer, 0, pFrame);

        // 默认展开
        pCategory->setExpanded(true);
//        qDebug() << "曝光控制分类已完成设置";
    }

    // 添加白平衡控制分类
    {
        QTreeWidgetItem* pCategory = new QTreeWidgetItem();
        m_advancedCameraTree->addTopLevelItem(pCategory);

        QtCategoryButton1* whiteBalanceButton = new QtCategoryButton1(tr("White Balance"),
                                                ":/icons/iconSet/whiteBalance.png",
                                                m_advancedCameraTree,
                                                pCategory);
        m_advancedCameraTree->setItemWidget(pCategory, 0, whiteBalanceButton);

        QFrame* pFrame = new QFrame(m_advancedCameraTree);
        QVBoxLayout* pLayout = new QVBoxLayout(pFrame);
        pLayout->setMargin(5);

        // 自动白平衡开关
        QCheckBox* autoWBCheckBox = new QCheckBox("Auto White Balance", pFrame);
        autoWBCheckBox->setChecked(true);
        pLayout->addWidget(autoWBCheckBox);

        // 色温调整
        QLabel* tempLabel = new QLabel("Temperature:", pFrame);
        QSlider* tempSlider = new QSlider(Qt::Horizontal, pFrame);
        tempSlider->setRange(2500, 10000);
        tempSlider->setValue(6500);
        QSpinBox* tempSpinBox = new QSpinBox(pFrame);
        tempSpinBox->setRange(2500, 10000);
        tempSpinBox->setValue(6500);
        tempSpinBox->setSuffix("K");

        QHBoxLayout* tempLayout = new QHBoxLayout();
        tempLayout->addWidget(tempSlider);
        tempLayout->addWidget(tempSpinBox);

        pLayout->addWidget(tempLabel);
        pLayout->addLayout(tempLayout);

        // 连接信号槽
        connect(tempSlider, &QSlider::valueChanged, tempSpinBox, &QSpinBox::setValue);
        connect(tempSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), tempSlider, &QSlider::setValue);
        connect(autoWBCheckBox, &QCheckBox::toggled, [tempSlider, tempSpinBox](bool enabled) {
            tempSlider->setEnabled(!enabled);
            tempSpinBox->setEnabled(!enabled);
        });

        QTreeWidgetItem* pContainer = new QTreeWidgetItem();
        pContainer->setDisabled(true);
        pCategory->addChild(pContainer);
        m_advancedCameraTree->setItemWidget(pContainer, 0, pFrame);

        pCategory->setExpanded(false);
//        qDebug() << "白平衡控制分类已完成设置";
    }

    // 添加图像质量控制分类
    {
        QTreeWidgetItem* pCategory = new QTreeWidgetItem();
        m_advancedCameraTree->addTopLevelItem(pCategory);

        QtCategoryButton1* imageQualityButton = new QtCategoryButton1(tr("Image Quality"),
                                                ":/icons/iconSet/colorAdjust.png",
                                                m_advancedCameraTree,
                                                pCategory);
        m_advancedCameraTree->setItemWidget(pCategory, 0, imageQualityButton);

        QFrame* pFrame = new QFrame(m_advancedCameraTree);
        QVBoxLayout* pLayout = new QVBoxLayout(pFrame);
        pLayout->setMargin(5);

        // 清晰度控制
        QLabel* sharpnessLabel = new QLabel("Sharpness:", pFrame);
        QSlider* sharpnessSlider = new QSlider(Qt::Horizontal, pFrame);
        sharpnessSlider->setRange(0, 255);
        sharpnessSlider->setValue(128);
        QSpinBox* sharpnessSpinBox = new QSpinBox(pFrame);
        sharpnessSpinBox->setRange(0, 255);
        sharpnessSpinBox->setValue(128);

        QHBoxLayout* sharpnessLayout = new QHBoxLayout();
        sharpnessLayout->addWidget(sharpnessSlider);
        sharpnessLayout->addWidget(sharpnessSpinBox);

        pLayout->addWidget(sharpnessLabel);
        pLayout->addLayout(sharpnessLayout);

        // 亮度控制
        QLabel* brightnessLabel = new QLabel("Brightness:", pFrame);
        QSlider* brightnessSlider = new QSlider(Qt::Horizontal, pFrame);
        brightnessSlider->setRange(0, 255);
        brightnessSlider->setValue(128);
        QSpinBox* brightnessSpinBox = new QSpinBox(pFrame);
        brightnessSpinBox->setRange(0, 255);
        brightnessSpinBox->setValue(128);

        QHBoxLayout* brightnessLayout = new QHBoxLayout();
        brightnessLayout->addWidget(brightnessSlider);
        brightnessLayout->addWidget(brightnessSpinBox);

        pLayout->addWidget(brightnessLabel);
        pLayout->addLayout(brightnessLayout);

        // 对比度控制
        QLabel* contrastLabel = new QLabel("Contrast:", pFrame);
        QSlider* contrastSlider = new QSlider(Qt::Horizontal, pFrame);
        contrastSlider->setRange(0, 255);
        contrastSlider->setValue(128);
        QSpinBox* contrastSpinBox = new QSpinBox(pFrame);
        contrastSpinBox->setRange(0, 255);
        contrastSpinBox->setValue(128);

        QHBoxLayout* contrastLayout = new QHBoxLayout();
        contrastLayout->addWidget(contrastSlider);
        contrastLayout->addWidget(contrastSpinBox);

        pLayout->addWidget(contrastLabel);
        pLayout->addLayout(contrastLayout);

        // 饱和度控制
        QLabel* saturationLabel = new QLabel("Saturation:", pFrame);
        QSlider* saturationSlider = new QSlider(Qt::Horizontal, pFrame);
        saturationSlider->setRange(0, 255);
        saturationSlider->setValue(128);
        QSpinBox* saturationSpinBox = new QSpinBox(pFrame);
        saturationSpinBox->setRange(0, 255);
        saturationSpinBox->setValue(128);

        QHBoxLayout* saturationLayout = new QHBoxLayout();
        saturationLayout->addWidget(saturationSlider);
        saturationLayout->addWidget(saturationSpinBox);

        pLayout->addWidget(saturationLabel);
        pLayout->addLayout(saturationLayout);

        // 连接信号槽
        connect(sharpnessSlider, &QSlider::valueChanged, sharpnessSpinBox, &QSpinBox::setValue);
        connect(sharpnessSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), sharpnessSlider, &QSlider::setValue);
        connect(brightnessSlider, &QSlider::valueChanged, brightnessSpinBox, &QSpinBox::setValue);
        connect(brightnessSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), brightnessSlider, &QSlider::setValue);
        connect(contrastSlider, &QSlider::valueChanged, contrastSpinBox, &QSpinBox::setValue);
        connect(contrastSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), contrastSlider, &QSlider::setValue);
        connect(saturationSlider, &QSlider::valueChanged, saturationSpinBox, &QSpinBox::setValue);
        connect(saturationSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), saturationSlider, &QSlider::setValue);

        QTreeWidgetItem* pContainer = new QTreeWidgetItem();
        pContainer->setDisabled(true);
        pCategory->addChild(pContainer);
        m_advancedCameraTree->setItemWidget(pContainer, 0, pFrame);

        pCategory->setExpanded(false);
//        qDebug() << "图像质量控制分类已完成设置";
    }

//    qDebug() << "专业相机控制布局设置完成";
}

void MainWindow::connectAdvancedCameraSignals()
{
    if (m_cameraAdapter) {
        // 连接相机适配器信号
        connect(m_cameraAdapter, &CameraSettingsAdapter::settingsChanged,
                this, &MainWindow::updateAdvancedCameraUI);
        connect(m_cameraAdapter, &CameraSettingsAdapter::errorOccurred,
                this, [this](const QString& error) {
//                    qDebug() << "相机设置错误:" << error;
                    QMessageBox::warning(this, "相机设置错误", error);
                });

//        qDebug() << "已连接专业相机控制信号";
    } else {
//        qDebug() << "警告：相机适配器不存在，无法连接信号";
    }
}

void MainWindow::updateAdvancedCameraUI()
{
    // 当相机设置发生变化时更新UI
    if (m_exposureForm && camera1) {  // camera1是现有的DirectShow相机
        // 设置相机到适配器
        m_cameraAdapter->setCamera(camera1.get());

        // 初始化表单UI
        m_exposureForm->initUI(camera1.get());
    }
}

// ========== 3D多点标定实现 ==========
// 完全复用现有CalibError系统，只在数据处理环节添加3D支持

void MainWindow::complete3DMultiPointCalibration()
{
//    qDebug() << "[3D多点标定] 开始计算区域校正，标定点数：" << m_calibrationPoints.size();

    // 清空之前的数据
    m_3DCalibrationDirections.clear();
    m_3DCalibrationOffsets.clear();

    // 确保有标定数据
    if (m_calibrationPoints.empty() || m_gazePoints.empty()) {
//        qDebug() << "[3D多点标定] 错误：没有标定数据";
        return;
    }

    // 为每个标定点计算校正偏移
    for (int i = 0; i < m_calibrationPoints.size() && i < m_gazePoints.size(); ++i) {
        if (m_gazePoints[i].empty()) continue;

        // 计算该点的平均注视位置
        cv::Point2f avgGaze(0, 0);
        for (const auto& gaze : m_gazePoints[i]) {
            avgGaze.x += gaze.x;
            avgGaze.y += gaze.y;
        }
        avgGaze.x /= m_gazePoints[i].size();
        avgGaze.y /= m_gazePoints[i].size();

        // 计算目标点和实际注视点的偏移
        cv::Point2f target = m_calibrationPoints[i];
        cv::Point2f offset = target - avgGaze;

        m_3DCalibrationOffsets.push_back(offset);

//        qDebug() << QString("[3D多点标定] 点%1: 目标(%2,%3) 注视(%4,%5) 偏移(%6,%7)")
//                    .arg(i+1).arg(target.x).arg(target.y)
//                    .arg(avgGaze.x).arg(avgGaze.y)
//                    .arg(offset.x).arg(offset.y);
    }

    // 启用3D区域校正
    m_3DRegionalCorrectionEnabled = true;

    // 计算总体质量指标
    double totalErrorMagnitude = 0.0;
    for (const auto& offset : m_3DCalibrationOffsets) {
        totalErrorMagnitude += cv::norm(offset);
    }
    double avgErrorMagnitude = totalErrorMagnitude / m_3DCalibrationOffsets.size();
//    qDebug() << "3D标定完成，平均误差幅度: " << avgErrorMagnitude << " 像素";

    // 模仿精确标定：保存3D标定参数
//    qDebug() << "3D标定成功，正在保存标定参数...";
    if (save3DCalibrationParams()) {
        QMessageBox::information(this, "3D标定保存成功", "3D标定参数已成功保存到 3d_calibration.txt");
    } else {
        QMessageBox::warning(this, "3D标定保存失败", "无法保存3D标定参数，请检查文件权限。");
    }

    // 注意：误差图显示逻辑移到了calibrate()函数中统一处理

//    qDebug() << QString("[3D多点标定] 完成，生成了%1个校正点").arg(m_3DCalibrationOffsets.size());
}

cv::Point2f MainWindow::apply3DRegionalCorrection(const cv::Point2f& rawScreenPoint)
{
    if (!m_3DRegionalCorrectionEnabled || m_3DCalibrationOffsets.empty() || m_calibrationPoints.empty()) {
        return rawScreenPoint;
    }

    // 找到最近的标定点
    float minDistance = FLT_MAX;
    int nearestIndex = -1;

    for (int i = 0; i < m_calibrationPoints.size(); ++i) {
        float distance = cv::norm(rawScreenPoint - m_calibrationPoints[i]);
        if (distance < minDistance) {
            minDistance = distance;
            nearestIndex = i;
        }
    }

    // 如果找到最近点且有对应的校正数据，应用校正
    if (nearestIndex >= 0 && nearestIndex < m_3DCalibrationOffsets.size()) {
        cv::Point2f correctedPoint = rawScreenPoint + m_3DCalibrationOffsets[nearestIndex];

        // 使用距离加权来平滑校正
        float maxDistance = 200.0f; // 校正影响的最大距离（像素）
        if (minDistance < maxDistance) {
            float weight = 1.0f - (minDistance / maxDistance);
            cv::Point2f interpolatedOffset = m_3DCalibrationOffsets[nearestIndex] * weight;
            return rawScreenPoint + interpolatedOffset;
        }
    }

    return rawScreenPoint;
}

cv::Point2f MainWindow::interpolate3DCorrection(const cv::Point2f& screenPoint) const
{
    if (!m_3DRegionalCorrectionEnabled || m_3DCalibrationOffsets.empty() || m_calibrationPoints.empty()) {
        return cv::Point2f(0, 0);
    }

    // 使用反距离权重插值算法
    float totalWeight = 0.0f;
    cv::Point2f totalOffset(0, 0);

    const float maxInfluenceDistance = 300.0f; // 最大影响距离

    for (int i = 0; i < m_calibrationPoints.size() && i < m_3DCalibrationOffsets.size(); ++i) {
        float distance = cv::norm(screenPoint - m_calibrationPoints[i]);

        if (distance < maxInfluenceDistance) {
            // 使用反距离权重，避免除零
            float weight = 1.0f / (distance + 1.0f);
            totalWeight += weight;
            totalOffset += m_3DCalibrationOffsets[i] * weight;
        }
    }

    if (totalWeight > 0) {
        return totalOffset / totalWeight;
    }

    return cv::Point2f(0, 0);
}

void MainWindow::show3DCalibrationResult(const CalibrationQuality& quality)
{
    QString title = quality.isAcceptable ? "3D注视标定成功" : "3D注视标定失败";
    QString icon = quality.isAcceptable ? "🎉" : "⚠️";

    QString message = QString("%1 3D标定质量评估\n\n"
                             "🎯 质量等级：%2\n"
                             "📊 平均误差：%3 像素\n"
                             "🔺 最大误差：%4 像素\n"
                             "🔻 最小误差：%5 像素\n"
                             "🎯 有效点数：%6/%7\n")
                     .arg(icon)
                     .arg(quality.qualityLevel)
                     .arg(quality.averageError, 0, 'f', 1)
                     .arg(quality.maxError, 0, 'f', 1)
                     .arg(quality.minError, 0, 'f', 1)
                     .arg(quality.validPoints)
                     .arg(m_calibrationPoints.size());

    // 添加3D模式特有信息
    if (m_3DRegionalCorrectionEnabled && !m_3DCalibrationOffsets.empty()) {
        message += QString("🌐 区域校正：已生成%1个校正点\n").arg(m_3DCalibrationOffsets.size());

        // 计算平均校正偏移
        cv::Point2f avgOffset(0, 0);
        for (const auto& offset : m_3DCalibrationOffsets) {
            avgOffset += offset;
        }
        avgOffset /= static_cast<float>(m_3DCalibrationOffsets.size());

        message += QString("📐 平均校正偏移：(%.1f, %.1f) 像素\n").arg(avgOffset.x).arg(avgOffset.y);
    }

    if (m_is3DZeroCalibrated) {
        message += QString("🔧 零点校准：方位角%.2f°，俯仰角%.2f°\n")
                          .arg(m_zeroAzimuthOffset * 180.0 / M_PI)
                          .arg(m_zeroElevationOffset * 180.0 / M_PI);
    }

    message += "\n";

    if (quality.isAcceptable) {
        message += "✅ 3D注视标定质量合格，可以开始使用系统。\n";
        message += "💡 系统已启用区域校正，注视精度将得到改善。";
    } else {
        message += "❌ 3D注视标定质量不合格，建议重新标定。\n\n";
        message += "💡 3D标定改进建议：\n";
        message += "  • 确保头部保持稳定，避免大幅移动\n";
        message += "  • 先进行E键零点校准，再进行多点标定\n";
        message += "  • 确保眼部准确对准每个标定点\n";
        message += "  • 检查环境光线和摄像头角度\n";
        message += "  • 考虑增加标定点数量（选择12点或16点）";
    }

    // 使用QMessageBox显示结果
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(quality.isAcceptable ? QMessageBox::Information : QMessageBox::Warning);

    if (quality.isAcceptable) {
        msgBox.setStandardButtons(QMessageBox::Ok);
    } else {
        msgBox.setStandardButtons(QMessageBox::Retry | QMessageBox::Ignore);
        msgBox.setDefaultButton(QMessageBox::Retry);
    }

    int result = msgBox.exec();

    // 处理用户选择
    if (!quality.isAcceptable && result == QMessageBox::Retry) {
//        qDebug() << "🔄 用户选择重新进行3D标定";
        // 重置3D标定相关数据
        m_3DCalibrationOffsets.clear();
        m_3DCalibrationDirections.clear();
        m_3DRegionalCorrectionEnabled = false;

        // 重置通用标定数据
        manualResetCumulativeErrorCorrection();

        // 可以重新开始标定流程
        if (m_calibError) {
            m_calibError->resetForNewTest();
        }
    } else if (quality.isAcceptable) {
//        qDebug() << "🎉 3D注视标定成功完成";
    }
}

// 模仿精确标定：保存3D标定参数到文件
// 执行3D几何标定（正确的3D标定方法）
void MainWindow::perform3DCalibration()
{
    qDebug() << "🎯 [perform3DCalibration] 开始执行3D几何标定...";
    bool isFake3D = (worker && !worker->isReal3DModeEnabled());
    qDebug() << QString("[perform3DCalibration] worker存在:%1, 假3D模式:%2").arg(worker != nullptr).arg(isFake3D);

    if (m_3DCalibrationPupilPoints.size() != m_3DCalibrationScreenPoints.size() ||
        m_3DCalibrationPupilPoints.size() < 5) {
//        qDebug() << "❌ 3D标定数据不足，需要至少5个标定点";
        QMessageBox::warning(this, "3D标定失败", "标定数据不足，请重新采集标定点。");
        return;
    }

    // 清空之前的3D标定结果
    m_3DCalibrationOffsets.clear();
    m_3DRegionalCorrectionEnabled = false;
    m_is3DCalibrated = false;

    // 简化的3D标定：使用现有的Gaze3DCalculator
    if (m_gaze3DCalculator) {
        // 使用标定点优化眼球中心位置和相机参数
        for (size_t i = 0; i < m_3DCalibrationPupilPoints.size(); i++) {
            cv::Point2f pupilCenter = m_3DCalibrationPupilPoints[i];
            cv::Point2f screenTarget = m_3DCalibrationScreenPoints[i];

            // 【修正】使用统一的calculateGaze3D函数，受F键切换控制
            cv::Point2f gazeResult = calculateGaze3D(pupilCenter);
            MainWindow::Gaze3DCalculator::GazeVector3D gaze3D;
            gaze3D.valid = (gazeResult.x != pupilCenter.x || gazeResult.y != pupilCenter.y);
            gaze3D.direction = cv::Point3f(gazeResult.x - 320, 240 - gazeResult.y, 1.0f);
            cv::Point2f predictedGaze = mapGaze3DToScreen(gaze3D.direction, m_screenWidth, m_screenHeight);

            // 计算误差并存储用于后续优化
            cv::Point2f error = screenTarget - predictedGaze;
            m_3DCalibrationOffsets.push_back(error);

//            qDebug() << "标定点" << i+1 << ": 目标(" << screenTarget.x << "," << screenTarget.y
//                     << ") 预测(" << predictedGaze.x << "," << predictedGaze.y
//                     << ") 误差(" << error.x << "," << error.y << ")";
        }

        // 启用区域校正
        m_3DRegionalCorrectionEnabled = true;
        m_is3DCalibrated = true;

//        qDebug() << "✅ 3D几何标定完成";
//        qDebug() << "   - 标定点数:" << m_3DCalibrationPupilPoints.size();
//        qDebug() << "   - 区域校正已启用，平均偏移量:" << m_3DCalibrationOffsets.size();

        // 保存3D标定参数
        if (save3DCalibrationParams()) {
            QMessageBox::information(this, "3D标定成功", "3D几何标定已完成并保存参数");
        } else {
            QMessageBox::warning(this, "保存失败", "3D标定成功但无法保存参数文件");
        }

        // 根据3D模式显示对应的误差分析图
        bool isReal3D = (worker && worker->isReal3DModeEnabled());
        qDebug() << QString("[3D标定完成2] worker存在:%1, 真3D模式:%2").arg(worker != nullptr).arg(isReal3D);

        if (worker && !worker->isReal3DModeEnabled()) {
            // 假3D模式：收集假3D数据并设置标定状态，然后显示误差图
            qDebug() << "[3D标定完成] 假3D模式，收集数据并设置标定状态";
            collectFake3DCalibrationData(); // 收集假3D角度数据

            // 移除调试消息框，避免干扰用户操作

            if (!m_fake3D_calibAngles.empty() && !m_fake3D_calibScreens.empty()) {
                computeFake3DMultiPointMapping(); // 计算映射矩阵
                qDebug() << "[3D标定完成] 假3D多点映射计算完成";

                // 移除调试消息框，避免干扰用户操作
            } else {
                qDebug() << "[3D标定完成] 假3D数据收集失败，回退到简单模式";
                m_fake3D_multiPointCalibrated = true; // 至少标记为已标定
            }
            // 假3D模式不需要show3DErrorPlot，已经在上面调用了showFake3DCalibrationResult
        } else {
            // 真3D模式：直接显示误差图
            qDebug() << "[3D标定完成] 真3D模式，显示误差图";
            show3DErrorPlot();
        }

    } else {
//        qDebug() << "❌ Gaze3DCalculator未初始化";
        QMessageBox::warning(this, "3D标定失败", "3D注视计算器未初始化。");
    }
}

bool MainWindow::save3DCalibrationParams()
{
    // 检查3D标定数据是否有效
    if (m_3DCalibrationPupilPoints.empty() || m_3DCalibrationScreenPoints.empty()) {
//        qDebug() << "错误: 3D标定数据无效，无法保存。";
        return false;
    }

    FILE* fp = fopen(".//3d_calibration.txt", "w");
    if (fp == NULL) {
//        qDebug() << "错误: 无法创建3d_calibration.txt文件";
        return false;
    }

    // 写入文件头，说明3D标定参数格式
    fprintf(fp, "# 3D Gaze Calibration Parameters\n");
    fprintf(fp, "# Generated by 3D Multi-Point Calibration\n");
    fprintf(fp, "calibration_points: %d\n", (int)m_calibrationPoints.size());
    fprintf(fp, "correction_points: %d\n", (int)m_3DCalibrationOffsets.size());

    // 保存零点校准参数
    if (m_is3DZeroCalibrated) {
        fprintf(fp, "zero_calibrated: true\n");
        fprintf(fp, "zero_azimuth_offset: %.6f\n", m_zeroAzimuthOffset);
        fprintf(fp, "zero_elevation_offset: %.6f\n", m_zeroElevationOffset);
    } else {
        fprintf(fp, "zero_calibrated: false\n");
    }

    fprintf(fp, "\n# Calibration Points and Correction Offsets\n");
    fprintf(fp, "# Format: target_x target_y offset_x offset_y\n");

    // 保存每个标定点的目标坐标和校正偏移
    for (int i = 0; i < m_calibrationPoints.size() && i < m_3DCalibrationOffsets.size(); ++i) {
        fprintf(fp, "%.2f %.2f %.6f %.6f\n",
                m_calibrationPoints[i].x, m_calibrationPoints[i].y,
                m_3DCalibrationOffsets[i].x, m_3DCalibrationOffsets[i].y);
    }

    // 计算并保存质量指标
    double totalError = 0.0;
    double maxError = 0.0;
    for (const auto& offset : m_3DCalibrationOffsets) {
        double error = cv::norm(offset);
        totalError += error;
        if (error > maxError) maxError = error;
    }
    double avgError = totalError / m_3DCalibrationOffsets.size();

    fprintf(fp, "\n# Quality Metrics\n");
    fprintf(fp, "average_error_pixels: %.2f\n", avgError);
    fprintf(fp, "max_error_pixels: %.2f\n", maxError);
    fprintf(fp, "regional_correction_enabled: %s\n", m_3DRegionalCorrectionEnabled ? "true" : "false");

    fclose(fp);
//    qDebug() << "3D标定参数已保存到 3d_calibration.txt";
    return true;
}

// 模仿精确标定：显示3D误差分析图
void MainWindow::show3DErrorPlot()
{
    qDebug() << "🟢 [调用追踪] show3DErrorPlot() 被调用 - 真3D误差显示";
    cv::Mat errorPlot = generate3DErrorPlot();

    if (!errorPlot.empty()) {
        cv::cvtColor(errorPlot, errorPlot, cv::COLOR_BGR2RGB);
        cv::imshow("errorPlot", errorPlot);
        cv::waitKey(1);
    }
}

// 模仿精确标定：生成3D误差分析图
cv::Mat MainWindow::generate3DErrorPlot()
{
    // 检查数据有效性
    if (m_3DCalibrationScreenPoints.empty() || m_3DCalibrationOffsets.empty()) {
//        qDebug() << "没有3D标定数据可用于生成误差图";
        return cv::Mat();
    }

    // 获取屏幕分辨率创建全屏画布
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    int screenWidth = screenRect.width();
    int screenHeight = screenRect.height();

    // 创建画布 - 使用屏幕尺寸，避免截断（与精确标定一致）
    cv::Mat plot(screenHeight, screenWidth, CV_8UC3, cv::Scalar(255, 255, 255));

    // 计算绘图区域和缩放比例
    float margin = 50.0f;
    float plotWidth = screenWidth - 2 * margin;
    float plotHeight = screenHeight - 2 * margin;

    // 找到标定点的边界
    float minX = FLT_MAX, maxX = -FLT_MAX;
    float minY = FLT_MAX, maxY = -FLT_MAX;
    for (const auto& point : m_3DCalibrationScreenPoints) {
        if (point.x < minX) minX = point.x;
        if (point.x > maxX) maxX = point.x;
        if (point.y < minY) minY = point.y;
        if (point.y > maxY) maxY = point.y;
    }

    // 添加边距
    float rangeX = maxX - minX;
    float rangeY = maxY - minY;
    minX -= rangeX * 0.1f;
    maxX += rangeX * 0.1f;
    minY -= rangeY * 0.1f;
    maxY += rangeY * 0.1f;

    // 计算缩放比例
    float scaleX = plotWidth / (maxX - minX);
    float scaleY = plotHeight / (maxY - minY);
    float scale = min(scaleX, scaleY);

    // 转换坐标函数
    auto toPlotCoord = [&](const cv::Point2f& point) -> cv::Point2f {
        return cv::Point2f(
            margin + (point.x - minX) * scale,
            margin + (point.y - minY) * scale
        );
    };

    // 计算误差统计
    double totalError = 0.0;
    double maxError = 0.0;
    cv::Point2f avgOffset(0, 0);

    for (const auto& offset : m_3DCalibrationOffsets) {
        double error = cv::norm(offset);
        totalError += error;
        if (error > maxError) maxError = error;
        avgOffset += offset;
    }
    double avgError = totalError / m_3DCalibrationOffsets.size();
    avgOffset /= static_cast<float>(m_3DCalibrationOffsets.size());

    // 绘制每个标定点和其误差
    for (int i = 0; i < m_3DCalibrationScreenPoints.size() && i < m_3DCalibrationOffsets.size(); ++i) {
        cv::Point2f target = toPlotCoord(m_3DCalibrationScreenPoints[i]);
        cv::Point2f predictedPoint = m_3DCalibrationScreenPoints[i] - m_3DCalibrationOffsets[i]; // 预测点 = 目标点 - 偏移
        cv::Point2f predicted = toPlotCoord(predictedPoint);

        double errorMagnitude = cv::norm(m_3DCalibrationOffsets[i]);

        // 绘制误差圆圈 (橙色半透明)
        float errorRadius = errorMagnitude * scale * 0.5f;
        cv::circle(plot, target, static_cast<int>(errorRadius), cv::Scalar(100, 150, 255), -1); // 橙色填充

        // 绘制目标点 (红色圆圈)
        cv::circle(plot, target, 8, cv::Scalar(0, 0, 255), 2); // 红色边框
        cv::circle(plot, target, 3, cv::Scalar(0, 0, 255), -1); // 红色填充

        // 绘制预测点 (绿色圆圈)
        cv::circle(plot, predicted, 6, cv::Scalar(0, 255, 0), 2); // 绿色边框
        cv::circle(plot, predicted, 2, cv::Scalar(0, 255, 0), -1); // 绿色填充

        // 绘制误差向量 (蓝色箭头，从预测点指向目标点)
        cv::arrowedLine(plot, predicted, target, cv::Scalar(255, 0, 0), 2, 8, 0, 0.3);

        // 添加点标签和误差值
        std::string label = "P" + std::to_string(i + 1) + ": " +
                           std::to_string(static_cast<int>(errorMagnitude)) + "px";
        cv::putText(plot, label, cv::Point(target.x + 10, target.y - 10),
                   cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 0), 1);
    }

    // 绘制平均误差向量 (紫色箭头)
    if (cv::norm(avgOffset) > 1.0f) {
        cv::Point2f center(600, 400); // 图像中心
        cv::Point2f avgVector = center + cv::Point2f(avgOffset.x * scale, avgOffset.y * scale);
        cv::arrowedLine(plot, center, avgVector, cv::Scalar(255, 0, 255), 3, 8, 0, 0.4);

        std::string avgLabel = "Avg Offset: (" +
                              std::to_string(static_cast<int>(avgOffset.x)) + "," +
                              std::to_string(static_cast<int>(avgOffset.y)) + ")px";
        cv::putText(plot, avgLabel, cv::Point(center.x + 10, center.y + 30),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 255), 1);
    }

    // 添加标题和统计信息
    std::string title = "3D Gaze Calibration Error Analysis";
    cv::putText(plot, title, cv::Point(50, 30), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);

    // 统计信息
    std::vector<std::string> stats = {
        "Calibration Points: " + std::to_string(m_calibrationPoints.size()),
        "Average Error: " + std::to_string(static_cast<int>(avgError)) + " pixels",
        "Max Error: " + std::to_string(static_cast<int>(maxError)) + " pixels",
        "Regional Correction: " + std::string(m_3DRegionalCorrectionEnabled ? "Enabled" : "Disabled"),
        "Zero Point Calibrated: " + std::string(m_is3DZeroCalibrated ? "Yes" : "No")
    };

    for (int i = 0; i < stats.size(); ++i) {
        cv::putText(plot, stats[i], cv::Point(50, 70 + i * 25),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    }

    // 添加图例
    int legendY = 600;
    cv::putText(plot, "Legend:", cv::Point(50, legendY), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 2);

    // 目标点图例
    cv::circle(plot, cv::Point(70, legendY + 25), 8, cv::Scalar(0, 0, 255), 2);
    cv::putText(plot, "Target Points", cv::Point(85, legendY + 30), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 0), 1);

    // 预测点图例
    cv::circle(plot, cv::Point(70, legendY + 50), 6, cv::Scalar(0, 255, 0), 2);
    cv::putText(plot, "Predicted Points", cv::Point(85, legendY + 55), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 0), 1);

    // 误差向量图例
    cv::arrowedLine(plot, cv::Point(65, legendY + 75), cv::Point(75, legendY + 75), cv::Scalar(255, 0, 0), 2);
    cv::putText(plot, "Error Vectors", cv::Point(85, legendY + 80), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 0), 1);

    return plot;
}

// 实验范式相关函数实现已全部删除




// ========================================================================
// 【Shared Memory Functions - Inter-process communication with main05.py】
// ========================================================================

bool MainWindow::initializeSharedMemory()
{
    // Image shared memory: 1280x720x3 bytes = 2,764,800 bytes
    const size_t imageSize = 1280 * 720 * 3;
    // Gaze shared memory: [gaze_x, gaze_y, valid, timestamp] = 4 floats = 16 bytes
    const size_t gazeSize = 4 * sizeof(float);

    try {
        qDebug() << "开始初始化共享内存...";

#ifdef _WIN32
        // Windows implementation using CreateFileMapping
        // 先尝试清理可能残留的共享内存
        HANDLE hExisting = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "eyetracking_image");
        if (hExisting) {
            qDebug() << "发现已存在的共享内存，正在清理...";
            CloseHandle(hExisting);
        }

        hImageMapping = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            imageSize,
            "eyetracking_image"
        );

        if (hImageMapping == NULL) {
            qDebug() << "Failed to create image file mapping:" << GetLastError();
            return false;
        }

        shm_image_ptr = (uint8_t*)MapViewOfFile(
            hImageMapping,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            imageSize
        );

        if (shm_image_ptr == NULL) {
            qDebug() << "Failed to map image view of file:" << GetLastError();
            CloseHandle(hImageMapping);
            hImageMapping = NULL;
            return false;
        }

        hGazeMapping = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            gazeSize,
            "eyetracking_gaze"
        );

        if (hGazeMapping == NULL) {
            qDebug() << "Failed to create gaze file mapping:" << GetLastError();
            UnmapViewOfFile(shm_image_ptr);
            CloseHandle(hImageMapping);
            shm_image_ptr = nullptr;
            hImageMapping = NULL;
            return false;
        }

        shm_gaze_ptr = (float*)MapViewOfFile(
            hGazeMapping,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            gazeSize
        );

        if (shm_gaze_ptr == NULL) {
            qDebug() << "Failed to map gaze view of file:" << GetLastError();
            UnmapViewOfFile(shm_image_ptr);
            CloseHandle(hImageMapping);
            CloseHandle(hGazeMapping);
            shm_image_ptr = nullptr;
            hImageMapping = NULL;
            hGazeMapping = NULL;
            return false;
        }

#else
        // Linux implementation using shm_open
        // 先尝试清理可能残留的共享内存
        shm_unlink("eyetracking_image");
        shm_unlink("eyetracking_gaze");

        shm_image_fd = shm_open("eyetracking_image", O_CREAT | O_RDWR, 0666);
        if (shm_image_fd == -1) {
            qDebug() << "Failed to create image shared memory:" << strerror(errno);
            return false;
        }

        if (ftruncate(shm_image_fd, imageSize) == -1) {
            qDebug() << "Failed to set image shared memory size:" << strerror(errno);
            close(shm_image_fd);
            shm_unlink("eyetracking_image");
            return false;
        }

        shm_image_ptr = (uint8_t*)mmap(NULL, imageSize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_image_fd, 0);
        if (shm_image_ptr == MAP_FAILED) {
            qDebug() << "Failed to map image shared memory:" << strerror(errno);
            close(shm_image_fd);
            shm_unlink("eyetracking_image");
            return false;
        }

        shm_gaze_fd = shm_open("eyetracking_gaze", O_CREAT | O_RDWR, 0666);
        if (shm_gaze_fd == -1) {
            qDebug() << "Failed to create gaze shared memory:" << strerror(errno);
            munmap(shm_image_ptr, imageSize);
            close(shm_image_fd);
            shm_unlink("eyetracking_image");
            return false;
        }

        if (ftruncate(shm_gaze_fd, gazeSize) == -1) {
            qDebug() << "Failed to set gaze shared memory size:" << strerror(errno);
            close(shm_gaze_fd);
            close(shm_image_fd);
            munmap(shm_image_ptr, imageSize);
            shm_unlink("eyetracking_image");
            shm_unlink("eyetracking_gaze");
            return false;
        }

        shm_gaze_ptr = (float*)mmap(NULL, gazeSize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_gaze_fd, 0);
        if (shm_gaze_ptr == MAP_FAILED) {
            qDebug() << "Failed to map gaze shared memory:" << strerror(errno);
            close(shm_gaze_fd);
            close(shm_image_fd);
            munmap(shm_image_ptr, imageSize);
            shm_unlink("eyetracking_image");
            shm_unlink("eyetracking_gaze");
            return false;
        }
#endif

        // Initialize shared memory with default values
        std::lock_guard<std::mutex> lock(m_sharedMemoryMutex);

        // Clear image memory
        memset(shm_image_ptr, 0, imageSize);

        // Initialize gaze data: [gaze_x, gaze_y, valid, timestamp]
        shm_gaze_ptr[0] = 0.0f;  // gaze_x
        shm_gaze_ptr[1] = 0.0f;  // gaze_y
        shm_gaze_ptr[2] = 0.0f;  // valid (0 = invalid, 1 = valid)
        shm_gaze_ptr[3] = 0.0f;  // timestamp

        m_sharedMemoryEnabled = true;
        qDebug() << "✅ 共享内存初始化成功，可与main05.py通信";
        qDebug() << "   - 图像共享内存: eyetracking_image (" << imageSize << " bytes)";
        qDebug() << "   - 视线共享内存: eyetracking_gaze (" << gazeSize << " bytes)";

        // 创建就绪标志文件，告知main05.py可以安全连接
        // 延迟创建标志文件，确保共享内存完全就绪且有数据写入后再创建
        QTimer::singleShot(2000, this, [this]() {
            QString flagFile = QCoreApplication::applicationDirPath() + "/shared_memory_ready.flag";
            QFile file(flagFile);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QString content = QString("shared_memory_ready\ntimestamp:%1\n").arg(QDateTime::currentMSecsSinceEpoch());
                file.write(content.toUtf8());
                file.close();
//                qDebug() << "📄 已创建共享内存就绪标志文件:" << flagFile;
//                qDebug() << "💡 main05.py现在可以安全连接到共享内存";
            }
        });

        return true;

    } catch (const std::exception& e) {
        qDebug() << "Exception during shared memory initialization:" << e.what();
        cleanupSharedMemory();
        return false;
    }
}

void MainWindow::updateSharedMemory(const cv::Mat& image, const cv::Point2f& gazePoint, const cv::Point2i& cropOffset)
{
    if (!m_sharedMemoryEnabled || shm_image_ptr == nullptr || shm_gaze_ptr == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_sharedMemoryMutex);

    try {
        // 静态变量记录是否已经开始写入数据
        static bool hasStartedWriting = false;
        static qint64 firstWriteTime = 0;

        // Update image data if image is valid
        if (!image.empty() && image.channels() == 3) {
            // 【修改】传输原始尺寸图像，不进行强制缩放
            cv::Mat imageToSend = image;

            // 检查图像尺寸是否超过共享内存最大支持尺寸（1280x720）
            int maxWidth = 1280;
            int maxHeight = 720;
            int maxDataSize = maxWidth * maxHeight * 3;

            bool needResize = false;
            if (image.cols > maxWidth || image.rows > maxHeight) {
                // 如果图像超过最大尺寸，等比例缩放以适配
                float scaleX = static_cast<float>(maxWidth) / image.cols;
                float scaleY = static_cast<float>(maxHeight) / image.rows;
                float scale = min(scaleX, scaleY);

                int newWidth = static_cast<int>(image.cols * scale);
                int newHeight = static_cast<int>(image.rows * scale);

                cv::resize(image, imageToSend, cv::Size(newWidth, newHeight));
                needResize = true;

                static int oversizeCounter = 0;
                if (++oversizeCounter % 30 == 1) { // 每30帧输出一次
                    qDebug() << QString("📤 图像过大，缩放传输: %1x%2 -> %3x%4")
                                .arg(image.cols).arg(image.rows).arg(newWidth).arg(newHeight);
                }
            }

            // 传输实际尺寸的图像数据
            int actualDataSize = imageToSend.cols * imageToSend.rows * 3;
            if (actualDataSize <= maxDataSize) {
                memcpy(shm_image_ptr, imageToSend.data, actualDataSize);

                static int sizeCounter = 0;
                if (++sizeCounter % 60 == 1) { // 每60帧输出一次调试信息
                    qDebug() << QString("📤 传输图像: %1x%2 (%3 bytes)")
                                .arg(imageToSend.cols).arg(imageToSend.rows).arg(actualDataSize);
                }
            }

            // 将图像尺寸信息写入mode_control共享内存
            if (this->shm_mode_control_ptr) {
                memcpy(this->shm_mode_control_ptr + 12, &imageToSend.cols, 4);  // 宽度
                memcpy(this->shm_mode_control_ptr + 16, &imageToSend.rows, 4);  // 高度
                // 添加gazeRegion的初始坐标信息（用于目标检测框坐标转换）
                memcpy(this->shm_mode_control_ptr + 20, &cropOffset.x, 4);  // 截图初始X坐标
                memcpy(this->shm_mode_control_ptr + 24, &cropOffset.y, 4);  // 截图初始Y坐标
            }

            // 记录首次成功写入时间
            if (!hasStartedWriting) {
                hasStartedWriting = true;
                firstWriteTime = QDateTime::currentMSecsSinceEpoch();
//                qDebug() << "🎬 开始向共享内存写入gazeRegion数据，时间戳:" << firstWriteTime;
            }
        }

        // Update gaze data
        shm_gaze_ptr[0] = gazePoint.x;
        shm_gaze_ptr[1] = gazePoint.y;
        shm_gaze_ptr[2] = (gazePoint.x >= 0 && gazePoint.y >= 0) ? 1.0f : 0.0f;  // valid flag
        shm_gaze_ptr[3] = static_cast<float>(QDateTime::currentMSecsSinceEpoch());  // timestamp

    } catch (const std::exception& e) {
        qDebug() << "Exception during shared memory update:" << e.what();
    }
}

void MainWindow::cleanupSharedMemory()
{
    if (!m_sharedMemoryEnabled) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_sharedMemoryMutex);

    try {
#ifdef _WIN32
        if (shm_image_ptr) {
            UnmapViewOfFile(shm_image_ptr);
            shm_image_ptr = nullptr;
        }
        if (hImageMapping) {
            CloseHandle(hImageMapping);
            hImageMapping = nullptr;
        }

        if (shm_gaze_ptr) {
            UnmapViewOfFile(shm_gaze_ptr);
            shm_gaze_ptr = nullptr;
        }
        if (hGazeMapping) {
            CloseHandle(hGazeMapping);
            hGazeMapping = nullptr;
        }
#else
        if (shm_image_ptr != nullptr && shm_image_ptr != MAP_FAILED) {
            munmap(shm_image_ptr, 1280 * 720 * 3);
            shm_image_ptr = nullptr;
        }
        if (shm_image_fd != -1) {
            close(shm_image_fd);
            shm_unlink("eyetracking_image");
            shm_image_fd = -1;
        }

        if (shm_gaze_ptr != nullptr && shm_gaze_ptr != MAP_FAILED) {
            munmap(shm_gaze_ptr, 4 * sizeof(float));
            shm_gaze_ptr = nullptr;
        }
        if (shm_gaze_fd != -1) {
            close(shm_gaze_fd);
            shm_unlink("eyetracking_gaze");
            shm_gaze_fd = -1;
        }
#endif

        m_sharedMemoryEnabled = false;

        // 删除就绪标志文件，通知其他程序共享内存不再可用
        QString flagFile = QCoreApplication::applicationDirPath() + "/shared_memory_ready.flag";
        if (QFile::exists(flagFile)) {
            QFile::remove(flagFile);
//            qDebug() << "📄 已删除共享内存就绪标志文件";
        }

//        qDebug() << "Shared memory cleaned up";

    } catch (const std::exception& e) {
        qDebug() << "Exception during shared memory cleanup:" << e.what();
    }
}

// ========================================================================
// 【AI Results Shared Memory Functions】
// ========================================================================

void MainWindow::setupAIResultsUI()
{
    // 状态栏中的语音和AI标签已删除，只保留AI分析状态窗口
    qDebug() << "✅ AI结果UI组件已创建（仅AI状态窗口）";
}

bool MainWindow::connectToAIResultsSharedMemory()
{
    try {
#ifdef _WIN32
        this->hResultsMapping = OpenFileMappingA(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            "ai_analysis_results"
        );

        if (this->hResultsMapping == NULL) {
//            qDebug() << "无法打开AI分析结果共享内存，将定期重试...";
            // 设置定时器定期重试连接
            if (!this->m_aiResultsTimer) {
                this->m_aiResultsTimer = new QTimer(this);
                connect(this->m_aiResultsTimer, &QTimer::timeout, this, &MainWindow::updateAIResultsSlot);
                this->m_aiResultsTimer->start(1000); // 每秒重试连接
            }
            return false;
        }

        this->shm_results_ptr = (uint8_t*)MapViewOfFile(
            this->hResultsMapping,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            4096
        );

        if (this->shm_results_ptr == NULL) {
            qDebug() << "无法映射AI分析结果共享内存视图:" << GetLastError();
            CloseHandle(this->hResultsMapping);
            this->hResultsMapping = NULL;
            return false;
        }
#else
        this->shm_results_fd = shm_open("ai_analysis_results", O_RDONLY, 0666);
        if (this->shm_results_fd == -1) {
//            qDebug() << "无法打开AI分析结果共享内存:" << strerror(errno);
            // 设置定时器定期重试连接
            if (!this->m_aiResultsTimer) {
                this->m_aiResultsTimer = new QTimer(this);
                connect(this->m_aiResultsTimer, &QTimer::timeout, this, &MainWindow::updateAIResultsSlot);
                this->m_aiResultsTimer->start(1000); // 每秒重试连接
            }
            return false;
        }

        this->shm_results_ptr = (uint8_t*)mmap(NULL, 4096, PROT_READ, MAP_SHARED, this->shm_results_fd, 0);
        if (this->shm_results_ptr == MAP_FAILED) {
            qDebug() << "无法映射AI分析结果共享内存:" << strerror(errno);
            close(this->shm_results_fd);
            this->shm_results_fd = -1;
            return false;
        }
#endif

        this->m_aiResultsConnected = true;
        qDebug() << "✅ 成功连接到AI分析结果共享内存";

        // 设置定时器定期读取AI结果
        if (!this->m_aiResultsTimer) {
            this->m_aiResultsTimer = new QTimer(this);
            connect(this->m_aiResultsTimer, &QTimer::timeout, this, &MainWindow::updateAIResultsSlot);
        }
        this->m_aiResultsTimer->start(100); // 每100ms更新一次

        return true;

    } catch (const std::exception& e) {
        qDebug() << "AI结果共享内存连接异常:" << e.what();
        return false;
    }
}

void MainWindow::updateAIResultsSlot()
{
    if (!this->m_aiResultsConnected) {
        // 尝试重新连接
        if (this->connectToAIResultsSharedMemory()) {
            return; // 连接成功，下次定时器触发时会读取数据
        }
        return; // 连接失败，等待下次重试
    }

    this->updateAIResults();
}

void MainWindow::updateAIResults()
{
    if (!this->m_aiResultsConnected || this->shm_results_ptr == nullptr) {
        return;
    }

    try {
        AIAnalysisResults results;
        if (this->readAIAnalysisResults(results)) {
            this->m_currentAIResults = results;
            this->displayAIResults();
        }
    } catch (const std::exception& e) {
        qDebug() << "读取AI结果时发生异常:" << e.what();
    }
}

bool MainWindow::readAIAnalysisResults(AIAnalysisResults& results)
{
    if (!this->m_aiResultsConnected || this->shm_results_ptr == nullptr) {
        return false;
    }

    // 使用互斥锁保护共享内存读取
    std::lock_guard<std::mutex> lock(this->m_aiResultsMutex);

    try {
        // 先清零结果结构体
        memset(&results, 0, sizeof(AIAnalysisResults));

        // 读取头部信息
        memcpy(&results.magic_number, this->shm_results_ptr, 32);

//        qDebug() << "✅ Magic number:" << QString::number(results.magic_number, 16);
//        qDebug() << "📝 Voice length:" << results.voice_text_length;
//        qDebug() << "🎯 Detection count:" << results.detection_count;
//        qDebug() << "🤖 AI response length:" << results.ai_response_length;

    // 验证魔数
    if (results.magic_number != 0x41495254) {
        return false; // 无效数据或尚未写入
    }

    // 读取语音识别文本
    if (results.voice_text_length > 0 && results.voice_text_length <= 512) {
        memcpy(results.voice_text, this->shm_results_ptr + 32, results.voice_text_length);
        results.voice_text[results.voice_text_length] = '\0';
    } else {
        results.voice_text[0] = '\0';
    }

    // 读取检测框数据
    if (results.detection_count > 0 && results.detection_count <= 28) {
        memcpy(results.detection_boxes, this->shm_results_ptr + 544, results.detection_count * sizeof(DetectionBox));
        qDebug() << "🔍 读取了" << results.detection_count << "个检测框";

        // 调试：显示第一个检测框的信息
        if (results.detection_count > 0) {
            const DetectionBox& first_box = results.detection_boxes[0];
            qDebug() << "   第一个目标：" << QString::fromUtf8(first_box.class_name)
                     << " x=" << first_box.x << " y=" << first_box.y
                     << " w=" << first_box.w << " h=" << first_box.h
                     << " conf=" << first_box.confidence;
        }
    }

    // 读取AI响应文本
    if (results.ai_response_length > 0 && results.ai_response_length <= 2336) {
        memcpy(results.ai_response, this->shm_results_ptr + 1664, results.ai_response_length);
        results.ai_response[results.ai_response_length] = '\0';
    } else {
        results.ai_response[0] = '\0';
    }

    return true;

    } catch (const std::exception& e) {
        qDebug() << "readAIAnalysisResults异常:" << e.what();
        return false;
    } catch (...) {
        qDebug() << "readAIAnalysisResults发生未知异常";
        return false;
    }
}

void MainWindow::displayAIResults()
{
    try {
        // 确保字符串以null结尾（防止strlen访问越界）
        this->m_currentAIResults.voice_text[511] = '\0';
        this->m_currentAIResults.ai_response[2335] = '\0';

        // 状态栏显示已删除，只更新AI分析状态窗口

        // ========== 更新独立AI状态窗口（如果存在） ==========
        if (this->m_voiceDisplayEdit) {
            if (strlen(this->m_currentAIResults.voice_text) > 0) {
                QString voiceText = QString::fromUtf8(this->m_currentAIResults.voice_text);
                this->m_voiceDisplayEdit->setPlainText(voiceText);
            } else {
                this->m_voiceDisplayEdit->setPlainText("等待语音识别结果...");
            }
        }

        // 分离不同类型的消息到不同的显示区域
        QString aiResponse = QString::fromUtf8(this->m_currentAIResults.ai_response);

        if (this->m_aiResponseDisplayEdit) {
            // AI分析响应编辑框：只显示非检测类型的消息
            if (strlen(this->m_currentAIResults.ai_response) > 0) {
                if (!aiResponse.contains("全场景检测模式") && !aiResponse.contains("[READY]") && !aiResponse.contains("[PAUSE]")) {
                    // 这是正常的AI分析结果，显示在AI响应编辑框
                    this->m_aiResponseDisplayEdit->setPlainText(aiResponse);
                } else {
                    // 这是检测相关的状态消息，AI响应编辑框保持原有内容或显示等待消息
                    if (this->m_aiResponseDisplayEdit->toPlainText().isEmpty() ||
                        this->m_aiResponseDisplayEdit->toPlainText() == "等待AI分析结果...") {
                        this->m_aiResponseDisplayEdit->setPlainText("等待AI分析结果...");
                    }
                }
            } else {
                this->m_aiResponseDisplayEdit->setPlainText("等待AI分析结果...");
            }
        }

        if (this->m_detectionInfoLabel) {
            QString detectionInfo;

            // 目标检测信息框：显示检测状态和检测结果
            if (aiResponse.contains("全场景检测模式") || aiResponse.contains("[READY]") || aiResponse.contains("[PAUSE]")) {
                detectionInfo = aiResponse + "\n\n";
            }

            if (this->m_currentAIResults.detection_count > 0) {
                detectionInfo += QString("检测到的目标详情：");

                // 显示前3个目标的详细信息
                uint32_t showCount = qMin(this->m_currentAIResults.detection_count, 3u);
                for (uint32_t i = 0; i < showCount; i++) {
                    const DetectionBox& box = this->m_currentAIResults.detection_boxes[i];
                    QString className = QString::fromUtf8(box.class_name);
                    if (className.isEmpty()) className = "未知对象";

                    // 显示目标类别、坐标(x,y,w,h)和置信度
                    detectionInfo += QString("\n📦 %1\n   位置: x=%2, y=%3, w=%4, h=%5\n   置信度: %6")
                        .arg(className)
                        .arg((int)box.x)
                        .arg((int)box.y)
                        .arg((int)box.w)
                        .arg((int)box.h)
                        .arg(box.confidence, 0, 'f', 2);
                }

                // 如果有更多目标，显示统计信息
                if (this->m_currentAIResults.detection_count > 3) {
                    detectionInfo += QString("\n... 还有 %1 个目标，总计 %2 个目标被检测到")
                        .arg(this->m_currentAIResults.detection_count - 3)
                        .arg(this->m_currentAIResults.detection_count);
                }
            } else {
                detectionInfo = "⏳ 等待目标检测结果...\n💡 点击上方'开始目标检测'按钮启动全场景检测";
            }
            this->m_detectionInfoLabel->setText(detectionInfo);
        }

//        qDebug() << "AI结果显示更新完成";

    } catch (const std::exception& e) {
        qDebug() << "displayAIResults异常:" << e.what();
    } catch (...) {
        qDebug() << "displayAIResults发生未知异常";
    }
}

void MainWindow::drawDetectionBoxes(cv::Mat& frame)
{
    if (!this->m_aiResultsConnected) {
        // qDebug() << "AI结果未连接，跳过绘制检测框";
        return;
    }

    if (this->m_currentAIResults.detection_count == 0) {
        // qDebug() << "没有检测框数据，跳过绘制";
        return;
    }

    qDebug() << "开始绘制" << this->m_currentAIResults.detection_count << "个检测框";

    for (uint32_t i = 0; i < this->m_currentAIResults.detection_count; i++) {
        const DetectionBox& box = this->m_currentAIResults.detection_boxes[i];

        // 根据置信度选择颜色
        cv::Scalar color;
        int thickness;
        if (box.confidence > 0.8f) {
            color = cv::Scalar(0, 255, 0);    // 高置信度：绿色
            thickness = 3;
        } else if (box.confidence > 0.5f) {
            color = cv::Scalar(0, 255, 255);  // 中等置信度：黄色
            thickness = 2;
        } else {
            color = cv::Scalar(255, 165, 0);  // 低置信度：橙色
            thickness = 2;
        }

        // 绘制检测框
        cv::Point2i topLeft(static_cast<int>(box.x), static_cast<int>(box.y));
        cv::Point2i bottomRight(static_cast<int>(box.x + box.w), static_cast<int>(box.y + box.h));
        cv::rectangle(frame, topLeft, bottomRight, color, thickness);

        // 绘制目标中心点
        cv::Point2i center(static_cast<int>(box.x + box.w/2), static_cast<int>(box.y + box.h/2));
        cv::circle(frame, center, 5, color, -1);

        // 绘制类别标签
        std::string label = std::string(box.class_name) + ": " +
                          std::to_string(box.confidence).substr(0, 4);

        int baseline;
        cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.7, 2, &baseline);

        // 标签背景
        cv::rectangle(frame,
                     cv::Point(topLeft.x, topLeft.y - textSize.height - 12),
                     cv::Point(topLeft.x + textSize.width + 8, topLeft.y),
                     color, -1);

        // 标签文字
        cv::putText(frame, label,
                   cv::Point(topLeft.x + 4, topLeft.y - 6),
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 2);
    }
}

void MainWindow::disconnectAIResultsSharedMemory()
{
    try {
        if (this->m_aiResultsTimer) {
            this->m_aiResultsTimer->stop();
        }

#ifdef _WIN32
        if (this->shm_results_ptr) {
            UnmapViewOfFile(this->shm_results_ptr);
            this->shm_results_ptr = nullptr;
        }
        if (this->hResultsMapping) {
            CloseHandle(this->hResultsMapping);
            this->hResultsMapping = NULL;
        }
#else
        if (this->shm_results_ptr != nullptr && this->shm_results_ptr != MAP_FAILED) {
            munmap(this->shm_results_ptr, 4096);
            this->shm_results_ptr = nullptr;
        }
        if (this->shm_results_fd != -1) {
            close(this->shm_results_fd);
            this->shm_results_fd = -1;
        }
#endif

        this->m_aiResultsConnected = false;
        qDebug() << "AI结果共享内存已断开连接";

    } catch (const std::exception& e) {
        qDebug() << "断开AI结果共享内存时发生异常:" << e.what();
    }
}

// ========================================================================
// 【新增按钮槽函数实现】
// ========================================================================

void MainWindow::onTargetDetectionClicked()
{
    qDebug() << "目标检测按钮被点击";

    // 更新按钮状态
    if (m_targetDetectionButton->text().contains("开始")) {
        // 启动目标检测
        m_targetDetectionButton->setText("🛑 停止目标检测");
        m_targetDetectionButton->setStyleSheet(
            "QPushButton {"
            "    background-color: #F44336;"
            "    color: white;"
            "    font-weight: bold;"
            "    font-size: 12px;"
            "    padding: 8px 16px;"
            "    border: none;"
            "    border-radius: 6px;"
            "    margin: 5px 0px;"
            "    min-width: 120px;"
            "    max-width: 150px;"
            "    min-height: 35px;"
            "    max-height: 40px;"
            "}"
            "QPushButton:hover { background-color: #D32F2F; }"
        );

        // 发送检测启动指令给main05.py (通过模式控制共享内存扩展字段)
        if (m_modeControlConnected && shm_mode_control_ptr) {
            // 使用保留字段发送检测控制指令
            // 字节6: 检测控制 (1=启动, 0=停止)
            shm_mode_control_ptr[6] = 1;  // 启动检测
            uint32_t timestamp = QDateTime::currentSecsSinceEpoch();
            memcpy(shm_mode_control_ptr + 8, &timestamp, 4); // 更新时间戳
            qDebug() << "✅ 已发送启动目标检测指令";

            if (m_aiResponseDisplayEdit) {
                m_aiResponseDisplayEdit->append("🎯 目标检测启动指令已发送，等待main05.py响应...");
            }
        } else {
            qDebug() << "❌ 模式控制共享内存未连接，无法启动检测";
            if (m_aiResponseDisplayEdit) {
                m_aiResponseDisplayEdit->append("❌ 无法启动检测：与main05.py的连接未建立");
            }
            // 恢复按钮状态
            m_targetDetectionButton->setText("🎯 开始目标检测");
            m_targetDetectionButton->setStyleSheet(
                "QPushButton {"
                "    background-color: #FF9800;"
                "    color: white;"
                "    font-weight: bold;"
                "    font-size: 12px;"
                "    padding: 8px 16px;"
                "    border: none;"
                "    border-radius: 6px;"
                "    margin: 5px 0px;"
                "    min-width: 120px;"
                "    max-width: 150px;"
                "    min-height: 35px;"
                "    max-height: 40px;"
                "}"
                "QPushButton:hover { background-color: #E65100; }"
            );
        }

        // 确保AI结果共享内存已连接
        if (!m_aiResultsConnected) {
            qDebug() << "🔄 尝试重新连接AI结果共享内存...";
            connectToAIResultsSharedMemory();
        }


    } else {
        // 停止目标检测
        m_targetDetectionButton->setText("🎯 开始目标检测");
        m_targetDetectionButton->setStyleSheet(
            "QPushButton {"
            "    background-color: #FF9800;"
            "    color: white;"
            "    font-weight: bold;"
            "    font-size: 12px;"
            "    padding: 8px 16px;"
            "    border: none;"
            "    border-radius: 6px;"
            "    margin: 5px 0px;"
            "    min-width: 120px;"
            "    max-width: 150px;"
            "    min-height: 35px;"
            "    max-height: 40px;"
            "}"
            "QPushButton:hover { background-color: #E65100; }"
        );

        // 发送检测停止指令给main05.py
        if (m_modeControlConnected && shm_mode_control_ptr) {
            shm_mode_control_ptr[6] = 0;  // 停止检测
            uint32_t timestamp = QDateTime::currentSecsSinceEpoch();
            memcpy(shm_mode_control_ptr + 8, &timestamp, 4); // 更新时间戳
            qDebug() << "✅ 已发送停止目标检测指令";

            if (m_aiResponseDisplayEdit) {
                m_aiResponseDisplayEdit->append("🛑 目标检测已停止");
            }
        }


        // 清除检测结果数据，避免检测框残留
        m_currentAIResults.detection_count = 0;
        memset(m_currentAIResults.detection_boxes, 0, sizeof(m_currentAIResults.detection_boxes));

        // 清除目标信息显示
        if (m_aiResponseDisplayEdit) {
            m_aiResponseDisplayEdit->append("🧹 已清除检测框显示");
        }
    }
}

void MainWindow::onModeAClicked()
{
    qDebug() << "A模式按钮被点击";

    // 发送模式切换指令到main05.py
    sendModeCommand('A');

    // A模式：焦点分析模式
    if (m_modeStatusLabel) {
        m_modeStatusLabel->setText("已切换到A模式 - 焦点分析模式");
    }

    // ASDF按钮样式保持不变，不做任何高亮处理
}

void MainWindow::onModeSClicked()
{
    qDebug() << "S模式按钮被点击";

    // 发送模式切换指令到main05.py
    sendModeCommand('S');

    // S模式：危险检测模式
    if (m_modeStatusLabel) {
        m_modeStatusLabel->setText("已切换到S模式 - 危险检测模式");
    }

    // ASDF按钮样式保持不变，不做任何高亮处理
}

void MainWindow::onModeDClicked()
{
    qDebug() << "D模式按钮被点击";

    // 发送模式切换指令到main05.py
    sendModeCommand('D');

    // D模式：意图推理模式
    if (m_modeStatusLabel) {
        m_modeStatusLabel->setText("已切换到D模式 - 意图推理模式");
    }

    // ASDF按钮样式保持不变，不做任何高亮处理
}

void MainWindow::onModeFClicked()
{
    qDebug() << "F模式按钮被点击";

    // 发送模式切换指令到main05.py
    sendModeCommand('F');

    // F模式：详细描述模式
    if (m_modeStatusLabel) {
        m_modeStatusLabel->setText("已切换到F模式 - 详细描述模式");
    }

    // ASDF按钮样式保持不变，不做任何高亮处理
}

// 初始化模式控制共享内存
void MainWindow::initializeModeControlSharedMemory()
{
#ifdef _WIN32
    // Windows实现
    this->hModeControlMapping = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        256,  // 256字节足够存储模式控制信息
        "ai_mode_control"
    );

    if (this->hModeControlMapping == NULL) {
        qDebug() << "创建模式控制共享内存失败:" << GetLastError();
        this->m_modeControlConnected = false;
        return;
    }

    this->shm_mode_control_ptr = (uint8_t*)MapViewOfFile(
        this->hModeControlMapping,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        256
    );

    if (this->shm_mode_control_ptr == NULL) {
        qDebug() << "映射模式控制共享内存视图失败:" << GetLastError();
        CloseHandle(this->hModeControlMapping);
        this->hModeControlMapping = NULL;
        this->m_modeControlConnected = false;
        return;
    }
#else
    // Linux实现
    this->shm_mode_control_fd = shm_open("ai_mode_control", O_CREAT | O_RDWR, 0666);
    if (this->shm_mode_control_fd == -1) {
        qDebug() << "创建模式控制共享内存失败:" << strerror(errno);
        this->m_modeControlConnected = false;
        return;
    }

    if (ftruncate(this->shm_mode_control_fd, 256) == -1) {
        qDebug() << "设置模式控制共享内存大小失败:" << strerror(errno);
        close(this->shm_mode_control_fd);
        this->shm_mode_control_fd = -1;
        this->m_modeControlConnected = false;
        return;
    }

    this->shm_mode_control_ptr = (uint8_t*)mmap(NULL, 256, PROT_READ | PROT_WRITE, MAP_SHARED, this->shm_mode_control_fd, 0);
    if (this->shm_mode_control_ptr == MAP_FAILED) {
        qDebug() << "映射模式控制共享内存失败:" << strerror(errno);
        close(this->shm_mode_control_fd);
        this->shm_mode_control_fd = -1;
        this->m_modeControlConnected = false;
        return;
    }
#endif

    this->m_modeControlConnected = true;
    qDebug() << "✅ 模式控制共享内存初始化成功";

    // 初始化共享内存内容
    memset(this->shm_mode_control_ptr, 0, 256);

    // 写入魔数和初始模式
    uint32_t magic = 0x4D4F4445; // "MODE"
    memcpy(this->shm_mode_control_ptr, &magic, 4);
    this->shm_mode_control_ptr[4] = 'A'; // 默认A模式
    this->shm_mode_control_ptr[5] = 1;   // 模式更新标志
}

// 发送模式切换指令
void MainWindow::sendModeCommand(char mode)
{
    if (!this->m_modeControlConnected || this->shm_mode_control_ptr == nullptr) {
        qDebug() << "模式控制共享内存未连接，无法发送模式指令";
        return;
    }

    // 更新模式和标志位
    this->shm_mode_control_ptr[4] = mode;         // 模式字符 (A/S/D/F)
    this->shm_mode_control_ptr[5] = 1;            // 更新标志位
    uint32_t timestamp = QDateTime::currentSecsSinceEpoch();
    memcpy(this->shm_mode_control_ptr + 8, &timestamp, 4); // 时间戳

    qDebug() << "✅ 已发送模式切换指令:" << mode << "（将自动触发分析）";
}

// ==================== 【新增】图像尺寸控制槽函数实现 ====================
void MainWindow::onImageSizeSliderChanged(int value)
{
    // 将滑块值(0-100)映射到图像尺寸
    // 0: 400x300 (最小)
    // 100: 1280x720 (最大)

    float ratio = value / 100.0f;

    // 宽度从400插值到1280
    m_currentImageWidth = static_cast<int>(400 + (1280 - 400) * ratio);

    // 高度从300插值到720
    m_currentImageHeight = static_cast<int>(300 + (720 - 300) * ratio);

    // 确保尺寸为偶数（某些图像处理需要）
    m_currentImageWidth = (m_currentImageWidth + 1) & ~1;
    m_currentImageHeight = (m_currentImageHeight + 1) & ~1;

    // 更新标签显示
    if (m_imageSizeLabel) {
        m_imageSizeLabel->setText(QString("%1x%2").arg(m_currentImageWidth).arg(m_currentImageHeight));
    }

    qDebug() << "图像传输尺寸已更新为:" << m_currentImageWidth << "x" << m_currentImageHeight;
}


