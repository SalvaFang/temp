#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QColor>

// 为QColor提供qHash函数，以支持QSet<QColor>等容器
inline uint qHash(const QColor &color, uint seed = 0) noexcept {
    return qHash(color.rgba(), seed);
}
#include <QPaintEvent>
#include <QTimer>
#include <QPainter>
#include <QPixmap>
#include <QLabel>
#include <QImage>
#include <QThread>
#include <QPropertyAnimation>
#include <QGraphicsEffect>
#include <QProgressBar>
#include <QPainter>
#include <QStyleOption>
#include <QtConcurrent/QtConcurrent>
#include <opencv2/opencv.hpp>
#include <ui_mainwindow.h>
#include <random>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include "showcal.h"
#include "worker.h"
#include "eyetrackingdialog.h"
#include "showpoint.h"
#include <QButtonGroup>
#include "PupilDetectionMethod.h"
#include "PupilTracker.h"
#include "PuReST.h"
#include "PupilTrackingMethod.h"
#include <opencv/cv.h>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include "FrameGrabber.h"
#include "caliberror.h"
#include <QMenuBar>
#include <QMenu>
#include <QDockWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QSettings>
#include <QGroupBox>
#include <QComboBox>
#include <QPushButton>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QRandomGenerator>
#include <QColor>
#include <windows.h>
#include <dshow.h>
#include <comdef.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include "gazeoverlaywidget.h" // 1. 包含新创建的头文件
#include <memory>
#include <chrono>
#include <string>
#include <mutex>
// Shared memory support
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif
// Temporarily commented out to debug crashes
#include <Eigen/Dense>
#include "eyetrackingcalibrator.h"
 // OpenCV头文件
#include <opencv2/opencv.hpp>
#include "directshowopencvcamera.h"
#include "aroverlay.h"
#include "Pupil.h"
#include "dataTable.h"
#include "graphPlot.h"
#include <QDateTime>

// 专业相机控制模块
#include "camctrl/QtCategoryButton1.h"
#include "camctrl/CameraSettingsAdapter.h"
#include "camctrl/FormExposure.h"
#include <QProcess>
#include <QScopedPointer>



#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "user32.lib")

// using namespace cv; // Removed to avoid namespace conflicts with cv::Point2f in STL containers


using std::vector;
using std::map;
using std::string;


namespace Ui {
class MainWindow;
}

struct HeadPose {
    float pitch_rad;
    float yaw_rad;
    float roll_rad;
    bool valid;

    HeadPose() : pitch_rad(0), yaw_rad(0), roll_rad(0), valid(false) {}
    HeadPose(float p, float y, float r) : pitch_rad(p), yaw_rad(y), roll_rad(r), valid(true) {}

    HeadPose operator-(const HeadPose& other) const {
        return HeadPose(pitch_rad - other.pitch_rad,
                       yaw_rad - other.yaw_rad,
                       roll_rad - other.roll_rad);
    }

    // 从Vec3d转换（适配01.cpp中现有的m_headEulerAngles）
    static HeadPose fromVec3d(const cv::Vec3d& eulerAngles) {
        return HeadPose(eulerAngles[1], eulerAngles[2], eulerAngles[0]); // pitch, yaw, roll
    }

    // 转换为Vec3d
    cv::Vec3d toVec3d() const {
        return cv::Vec3d(roll_rad, pitch_rad, yaw_rad);
    }
};

class Marker {
public:
    explicit Marker() :
        corners(std::vector<cv::Point2f>()),
        center(cv::Point3f(0,0,0)),
        id(-1),
        rv(cv::Mat()),
        tv(cv::Mat()) { }
    explicit Marker(std::vector<cv::Point2f> corners, int id) :
        corners(corners),
        center( cv::Point3f(0,0,0) ),
        id(id),
        rv(cv::Mat()),
        tv(cv::Mat()) { }
    std::vector<cv::Point2f> corners;
    cv::Point3f center;
    // Not exported atm
    int id;
    cv::Mat rv;
    cv::Mat tv;
};

struct greaterThanPtr {
    bool operator () (const float * a, const float * b) const
    { return (*a > *b) ? true : (*a < *b) ? false : (a > b); }
};

class PointVector
{
public:
    void insert(cv::Point2f p) { x.push_back(p.x); y.push_back(p.y); }
    void insert(cv::Point3f p) {
        x.push_back(p.x);
        y.push_back(p.y);
        z.push_back(p.z);



    }
    unsigned int size() { return x.rows; }
    cv::Mat x, y, z;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
public:
    // AI Analysis Results structures (必须在public区域以便函数参数使用)
    struct DetectionBox {
        float x, y, w, h;
        float confidence;
        uint32_t class_id;
        char class_name[16];
    };

    struct AIAnalysisResults {
        uint32_t magic_number;      // 0x41495254 ("AIRT")
        uint32_t timestamp;         // 时间戳
        uint32_t voice_text_length; // 语音识别文本长度
        uint32_t detection_count;   // 检测框数量
        uint32_t ai_response_length;// AI响应文本长度
        uint32_t reserved[3];       // 保留字段

        char voice_text[512];       // 语音识别文本
        DetectionBox detection_boxes[28]; // 检测框数据 (减少到28个以适配4KB)
        char ai_response[2336];     // AI响应文本 (调整大小以适配4KB)
    };

    explicit MainWindow(QWidget *parent = 0);
    //explicit MainWindow(FrameGrabber *grabber, QWidget *parent = 0);
    ~MainWindow();
    CvPoint2D32f  Homography_map_point(CvPoint2D32f p, double map_affine[2][3]);
    CvPoint2D32f Homography_map_point_homography(CvPoint2D32f p, double H[3][3]);
    CvPoint2D32f  Zuixiaoercheng_map_point(CvPoint2D32f  p,double map_matrix[6][2]);
    CvPoint2D32f  Homography_map_point1(CvPoint2D32f  p,double map_matrix[][3]);


    void compute_angle(cv::Point2f gaze);


public slots:
    void updateImage();
    void clickCalibButton();
    void clickReloadButton();

    // 吸附坐标接收槽函数
    void receiveAttractedGaze(const cv::Point2f& attractedGaze, bool isAttracting);

    // 误差校正触发槽函数
    void onErrorCorrectionTriggered();

    // CalibError相关槽函数
    void onFirstPointCompleted();
    void onAllPointsCompleted();
    void onErrorCalculationRequest();
    void onPointCompleted(int pointIndex, const QPointF& targetPoint, const QPointF& gazePoint);
    void onCycleReset();

    // 实验流程控制槽函数
    void showInteractionDialog();
    void hideInteractionDialog();
    void getSelectionCount();


//protected:
//    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    QAction *yanshiAction;  // “演示”模式的 QAction
    QAction *jingqueAction; // “精确”模式的 QAction
    QAction *gaze3DAction;  // "3D注视"模式的 QAction
    // 其他现有成员变量...

private:
    // 现有虚拟屏幕相关变量（01.cpp中已有，确保声明存在）
    std::vector<cv::Point2f> m_originalScreenCorners;
    bool m_headPoseCalibrated;
    HeadPose m_referenceHeadPose;
    cv::Point3f m_referenceHeadPosition;    // 标定的参考头部位置


protected:
    // 4. 声明需要重写的事件处理器
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override; // 添加窗口显示事件处理
    void paintEvent(QPaintEvent *e) override; // 确保paintEvent声明还在

    // 窗口尺寸恢复函数
    void restoreDockSizesFromSettings();

private:
    // 响应式布局支持
    void setupResponsiveLayout();
    void adjustDockSizesForMainWindow();
    void setDockSizeConstraints();


private:
    bool m_freeGazeMode;
    cv::Point2f m_currentFreeGaze;
    // QQueue<cv::Point2f> m_gazeTrail; // 旧的轨迹队列
    QQueue<QPointF> m_gazeTrailQt;     // 2. 新增：使用QPointF版本的轨迹队列，避免重复转换
    QTimer* m_gazeTrailTimer;
    qint64 m_lastGazeTime;

    GazeOverlayWidget *m_gazeOverlay; // 3. 添加覆盖窗口的指针

    // T模式相关函数
    void setFreeGazeMode(bool enabled);
    bool isFreeGazeMode() const { return m_freeGazeMode; }
    void updateFreeGaze(const cv::Point2f& gazePoint);
    void drawFreeGazeTrail(QPainter& painter);
    void addGazeTrailPoint(const cv::Point2f& point);


    cv::KalmanFilter m_gazeKalmanFilter;      // 卡尔曼滤波器对象
    cv::Mat m_gazeKalmanMeasurement;          // 存储测量值的矩阵
    bool m_isKalmanFilterInitialized;         // 标志位，判断是否已用第一个有效点初始化
    cv::Point2f m_lastValidGaze;              // 存储上一个有效的原始gaze点
    QElapsedTimer m_frameTimer;               // 用于计算动态的帧间隔时间(dt)

private:
    // ▼▼▼ 添加函数声明 ▼▼▼
    void initializeGazeKalmanFilter();
    cv::Point2f applyGazeKalmanFilter(const cv::Point2f& gaze, float pupilConfidence);

    // Shared memory functions
    bool initializeSharedMemory();
    void updateSharedMemory(const cv::Mat& image, const cv::Point2f& gazePoint, const cv::Point2i& cropOffset = cv::Point2i(0, 0));
    void cleanupSharedMemory();

    // AI Results shared memory functions
    bool connectToAIResultsSharedMemory();
    void updateAIResults();
    void displayAIResults();
    void drawDetectionBoxes(cv::Mat& frame);
    bool readAIAnalysisResults(AIAnalysisResults& results);
    void setupAIResultsUI();
    void createAIStatusDockWindow();
    void disconnectAIResultsSharedMemory();
    QVector<cv::Point2f> m_calibrationPoints;       // 标定点（绿色）
    QVector<QVector<cv::Point2f>> m_gazePoints;     // 每个标定点的多组预测点（红色）- 用于显示，已校正
    QVector<QVector<cv::Point2f>> m_originalGazePoints; // 每个标定点的原始预测点 - 用于训练累积误差校正
    QVector<QVector<double>> m_errors;              // 每个标定点的多组误差
    int m_screenWidth;
    int m_screenHeight;
    int m_currentPoint;
    bool m_sampling = false;

    bool m_recording;   // 是否正在校准流程中;
    bool m_samplingActive; // 是否处于一次采样序列中
    bool m_waitingForSpace;   // 初始等待用户启动第一个点
    ShowCal* calWin;


    int m_gazeCount;
    int m_maxGazePerPoint;
    //QImage m_calibrationImage;                      // 用于绘制的图像
    bool isSamplingGaze = false;
    int gazeSampleCounter = 0;
    std::vector<cv::Point2f> gazeBuffer;

    std::vector<cv::Point2f> generateCalibrationPoints(int numberOfPoints);
    double calculateError(const cv::Point2f &gt, const cv::Point2f &gaze); // 修复语法错误
    double calculateErrorWithAttraction(const cv::Point2f &gt, const cv::Point2f &gaze); // 考虑吸附的误差计算

    // 系统性误差分析和修复功能
    cv::Point2f calculateSystematicError();                           // 计算系统性误差
    void applySystematicErrorCorrection();                            // 应用系统性误差校正
    void analyzeCalibrationErrors();                                  // 分析标定误差模式
    cv::Point2f getRegionalErrorOffset(const cv::Point2f& point);     // 获取区域误差偏移

    // 视线校正相关函数
    void addGazeCorrectionSample(const cv::Point2f& originalGaze, const cv::Point2f& correctedGaze);
    void updateAverageOffset();
    cv::Point2f applyGazeCorrection(const cv::Point2f& rawGaze);
    void resetGazeCorrection();

    // 累积误差校正相关函数
    void updateCumulativeErrorOffset();                            // 更新累积误差偏移
    void resetCumulativeErrorCorrection();                         // 重置累积误差校正
    void manualResetCumulativeErrorCorrection();                   // 手动重置累积误差校正（用户主动调用）
    void forceResetAllStaticVariables();                           // 强制重置所有静态变量
    cv::Point2f calculateFilteredAverage(const QVector<cv::Point2f>& points);  // 计算过滤离散点后的平均值

    // 标定质量评估相关函数
    struct CalibrationQuality {
        float averageError;      // 平均误差
        float maxError;          // 最大误差
        float minError;          // 最小误差
        int validPoints;         // 有效点数
        bool isAcceptable;       // 是否可接受
        QString qualityLevel;    // 质量等级
    };

    CalibrationQuality evaluateCalibrationQuality();              // 评估标定质量
    void showCalibrationResult(const CalibrationQuality& quality); // 显示标定结果
    bool isCalibrationAcceptable(const CalibrationQuality& quality); // 判断标定是否可接受

    // 多项式拟合标定误差评估
    float evaluatePolynomialCalibrationError();                    // 评估多项式标定的初始误差
    bool checkPolynomialCalibrationQuality(float averageError);    // 检查多项式标定质量
    //void drawCalibrationImage();
    void calculateErrors();
    CalibError *m_calibError;                       // 修改为 CalibError

    bool calibration_success=false;
    // 在 MainWindow 类中添加成员变量：
    bool isCollecting = false;
    int collectedCount = 0;
    std::vector<cv::Point2f> currentSamplePoints;
    std::vector<cv::Point2f> allPupilPoints;
    std::vector<cv::Point2f> allScreenPoints;
    std::vector<cv::Point2f> predefinedScreenPoints;
     int calibrationIndex = 0;                         // 当前标定点下标
     // 多项式拟合系数
     cv::Mat coeffsX, coeffsY;
    cv::Mat polyFit3(const std::vector<cv::Point2f>& inputPts, const std::vector<float>& outputVals);
    void computePolynomialMapping();
    cv::Point2f mapToScreenUsingPolynomial(const cv::Point2f& pupilPoint);
    void cal_calibration(const std::vector<cv::Point2f>& scenePoints,
                        const std::vector<cv::Point2f>& vectorPoints);
    // 🚀 混合EPNP+单ArUco策略函数声明
    bool tryEPNPMethod(const std::map<int, std::vector<cv::Point2f>>& markerMap, cv::Mat& displayFrame);
    bool trySingleArUcoMethod(const std::map<int, std::vector<cv::Point2f>>& markerMap, cv::Mat& displayFrame);
    bool fuseSingleArUcoPoses(const std::map<int, cv::Mat>& validRvecs, const std::map<int, cv::Mat>& validTvecs, cv::Mat& finalRvec, cv::Mat& finalTvec);
    void processEPNPResult(const cv::Mat& rvec, const cv::Mat& tvec, cv::Mat& displayFrame, const std::vector<int>& usedIds);
    void processSingleArUcoResult(const cv::Mat& rvec, const cv::Mat& tvec, cv::Mat& displayFrame, const std::vector<int>& markerIds);
    bool saveJingqueCalibrationParams(); // 新增的函数声明

public:
    // Temporarily commented out to debug crashes - Eigen matrices
     Eigen::MatrixXd A1 = Eigen::MatrixXd(12,6);
     Eigen::MatrixXd B = Eigen::MatrixXd(12,6);
    // MatrixXd A = MatrixXd(12,5);
    // MatrixXd B = MatrixXd(12,7);

     Eigen::VectorXd b1 = Eigen::VectorXd(12);
     Eigen::VectorXd b2 = Eigen::VectorXd(12);

     Eigen::VectorXd x1 = Eigen::VectorXd(6);
     Eigen::VectorXd x2 = Eigen::VectorXd(6);
     cv::Rect glintROI;



public slots:

    void showCalibrationWindow();



private:
    QMutex cfgMutex;
    int num = 0;                //number of saved images for caliberation
    int num_yanshi = 0;
    int num_test = 0;
    int pressCount = 0;
    int scNum = 0;
    bool isAffine = false;  // 新增：记录当前使用的映射类型
    int flag = 0;
    int test_flag = 0;
    cv::Point2f m_globalErrorOffset;


    int m_dKeyPressCount = 0;        // D键按压次数
    int m_validCollectionCount = 0;   // 有效采集次数
    int m_failedCollectionCount = 0;  // 失败采集次数

    quint32 CALIBRATIONPOINTS9;
    cv::Ptr<cv::aruco::Dictionary> dict;
    cv::Ptr<cv::aruco::DetectorParameters> detectorParameters;
    cv::Ptr<cv::aruco::Dictionary> dictionary;
    cv::Ptr<cv::aruco::DetectorParameters> parameters;


    int eyecount=0;
    QTimer theTimer,DTimer;        // timer for main loop
    Mat sceneImage, eyeImage;//stand for scene and eye
    Mat orig_eye;
    cv::VideoCapture sceneCap;
    cv::VideoCapture eyeCap;

    // Shared memory for inter-process communication with main05.py
#ifdef _WIN32
    HANDLE hImageMapping;    // Windows file mapping handle for image
    HANDLE hGazeMapping;     // Windows file mapping handle for gaze data
    HANDLE hResultsMapping;  // Windows file mapping handle for AI analysis results
#else
    int shm_image_fd;        // Linux shared memory file descriptor for image
    int shm_gaze_fd;         // Linux shared memory file descriptor for gaze data
    int shm_results_fd;      // Linux shared memory file descriptor for AI results
#endif
    uint8_t* shm_image_ptr;  // Pointer to shared memory for image data
    float* shm_gaze_ptr;     // Pointer to shared memory for gaze data
    uint8_t* shm_results_ptr; // Pointer to shared memory for AI analysis results
    bool m_sharedMemoryEnabled; // Flag to enable/disable shared memory
    std::mutex m_sharedMemoryMutex; // Mutex for thread-safe shared memory access
    std::mutex m_aiResultsMutex;    // Mutex for AI results shared memory access


    // AI results management
    AIAnalysisResults m_currentAIResults;
    QTimer* m_aiResultsTimer;

    // 状态栏显示组件已删除

    // 独立AI状态窗口
    QDockWidget* m_aiStatusDock;
    QTextEdit* m_voiceDisplayEdit;
    QTextEdit* m_aiResponseDisplayEdit;
    QLabel* m_detectionInfoLabel;
    QPushButton* m_targetDetectionButton;
    QLabel* m_modeStatusLabel;              // ASDF模式状态显示标签
    QPushButton* m_modeAButton;
    QPushButton* m_modeSButton;
    QPushButton* m_modeDButton;
    QPushButton* m_modeFButton;
    bool m_aiResultsConnected;

    // 模式控制共享内存相关
#ifdef _WIN32
    HANDLE hModeControlMapping;      // Windows file mapping handle for mode control
    uint8_t* shm_mode_control_ptr;   // Pointer to mode control shared memory
#else
    int shm_mode_control_fd;         // Linux shared memory file descriptor for mode control
    uint8_t* shm_mode_control_ptr;   // Pointer to mode control shared memory
#endif
    bool m_modeControlConnected;
    //capture pictures
    QLabel *sceneLabel;     //point to scene label child widget
    QLabel *eyeLabel;
    QLabel* labelm;
    double in[5];
   /* double     map_matrix[3][3] = {{5.340092, -1.893519, -458.326169},
                                   {1.324743, -4.774534, -378.113115},
                                   {0.003026, -0.001204, -0.820234}};*/

    double map_matrix[6][2] = {0};
    //double map_yanshi[2][3] = {0};
    double map_yanshi[3][3] = {0};

    double     map_yanshi1[3][3] = {{5.340092, -1.893519, -458.326169},
                                   {1.324743, -4.774534, -378.113115},
                                   {0.003026, -0.001204, -0.820234}};
    cv::Mat cameraMatrix, newCameraMatrix, distCoeffs;
    cv::Mat rvecs, tvecs;

    CvPoint2D32f eyeVector;
    CvPoint2D32f gaze;
    QString pitch_s;
    QString yaw_s;
    double pitch;
    double yaw;
    double z_length;
    double l_perPix;
    Ui::MainWindow *ui;
    ShowPoint *showPoint;
    PupilTrackingMethod *pupilTrackingMethod = nullptr;
    PupilDetectionMethod *pupilDetectionMethod = nullptr;
    TrackedPupil params;
    Timestamp timestamp;
    cv::Point2f testPoint;
    // Initialize parameters
    float angle = 0.0;  // Angle for rotation
    float radius = 100.0;  // Radius of the circular path
    QButtonGroup *groupButton;

    int flag_calibration_status = 1;//0:jingque,1:yanshi,2:3D_gaze
    FrameGrabber *m_grabber;
    FrameGrabber *m_grabber1;
    cv::Mat getCameraMatrix() {
            return (cv::Mat_<double>(3,3) <<
                        640, 0, 320,
                        0, 640, 240,
                        0, 0 ,1 );
    }
    cv::Mat getDistortionCoefficients() {
            return (cv::Mat_<double>(4,1) << 0, 0, 0, 0);
    }
    PointVector rightPupil;
    PointVector leftPupil;     // 用于存储第二个亮斑的瞳孔角膜矢量
    PointVector Markgaze;
    std::vector<Marker> markers;


    // AR Overlay related variables
    bool m_arOverlayEnabled;
    QLabel* m_arOverlayWidget;
    cv::Mat m_arOverlayBackground;
    cv::Point3f m_lastHeadPosition;
    cv::Vec3f m_lastHeadRotation;
    double m_arOpacity;
    QTimer* m_arUpdateTimer;

signals:
    void newFrameAvailable(const cv::Mat &frame); // 信号通知新帧
    void gazePointUpdated(const QPointF& gazePoint); // 注视点更新信号
    void frameProcessingRequested(const cv::Mat& eyeFrame, const cv::Mat& sceneFrame, bool isForCalibration); // <-- 确保是这个3参数版本
protected:

    void keyPressEvent(QKeyEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void onCalibrationReceived();  // 接收通知后处理



private slots:
    //void on_pushButton_2_clicked();
    void on_TestButton_clicked();
    void slots_calibration_status(QAction *action);
    void on_TestResultButton_clicked();
    void displayResult(const Mat &img_matches, double yaw, double pitch, double roll);
    void handleError(const QString &msg);
    void updateFlashColor();

    void on_calibPointBtn_clicked();
    void on_calibImgCollectBtn_clicked();
    void on_setOrignalBtn_clicked();
    //void on_pushButton_clicked();

    void on_Markcalib_clicked();
    void on_pointcalibButton_2_clicked();
//    void on_pushButton_clicked();
    void on_MarkerBtn_clicked();
    void handleCamera1Image(Timestamp t, cv::Mat frame);
    void handleCamera2Image(Timestamp t, cv::Mat frame);
    void on_comboBox_activated(int index);

    void on_comboBox_2_currentIndexChanged(int index);

    // DataTable and GraphPlot related slots
    void onCreateDataTable();
    void onCreateGraphPlot(const QString &value);
    void onCloseGraphPlot(GraphPlot* graphPlot); // 新增：关闭GraphPlot的槽函数
    void onToggleDataTable();
    void onShowAllGraphPlots();
    void onHideAllGraphPlots();

    // GraphPlot基本管理函数
    void onCreateGraphPlotAsTab(const QString &value);        // 创建Tab模式的GraphPlot
    void onCreateGraphPlotAsWindow(const QString &value);     // 创建独立窗口的GraphPlot
    void onRestoreAllWindows(); // 恢复所有窗口
    void onToggleSceneWindow(); // 切换场景图像窗口
    void onToggleEyeWindow();   // 切换眼睛图像窗口
    void onToggleControlsWindow(); // 切换控件窗口

    // 确保dock窗口正确显示
    void ensureDocksVisible();
    // GraphPlot close handling removed to prevent crashes

private:
    QWidget* calibrationOverlay;
    bool yanshiCalibrated = false; // 新增标志位

    // 专业化UI样式函数
    void loadProfessionalStyle();
    void setupProfessionalIcons();

    // 清空实验范式相关变量（等待重新设计）

    // 视线数据存储（被其他功能使用，需要保留）
    ProcessingResult m_lastProcessingResult;
    QMutex m_resultMutex;




private:
    QThread *workerThread;
    Worker *worker;
    int gvar;
    int gmark;
//    QCamera *camera1 = nullptr; //QT camera
//    QCamera *camera0;

    // B键冻结功能相关变量
    bool m_imageProcessingPaused = false;
    cv::Mat m_frozenEyeImage;
    cv::Mat m_frozenSceneImage;
    bool m_hasFrozenImages = false;

    // C键标定功能相关变量
    bool m_calibrationActive = false;
    bool m_singlePointCalibrationMode = false;

    bool toggleState = false;
    bool togglemarkState = false;
   // QThread m_wokerThread;

public:
    enum PlType {
        POLY_1_X_Y_XY,
        POLY_1_X_Y_XY_XX_YY,
        POLY_1_X_Y_XY_XX_YY_XXYY,
        POLY_1_X_Y_XY_XX_YY_XYY_YXX,
        POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY,
        POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXX_YYY
    };
    enum InputType{
        BINOCULAR   = 0,
        BINOCULAR_MEAN_POR   = 1,
        MONO_LEFT   = 2,
        MONO_RIGHT  = 3,
        UNKNOWN = 4
    };
    bool calibrate();
    cv::Point2f evaluate(cv::Point2f rightPupil);
//    cv::Point3f estimateGaze();
    bool allowCenterAndScale() { return true; }
    std::string description();


public:
    vector<KeyPoint> run1(Mat input, int maxCorners);
    Mat CornersShow1(Mat src, vector<KeyPoint> corners);
    void DistanceChoice1(float minDistance, int maxCorners, vector<KeyPoint> &corners);
    vector<KeyPoint> ANMS1(vector<KeyPoint> kpts, int maxCorners);
    vector<KeyPoint> ssc1(vector<KeyPoint> keyPoints, int numRetPoints, float tolerance, int cols, int rows);

private:
    void GetGradient1(Mat src, Mat &Ixx, Mat &Ixy, Mat &Iyy);
    Mat CalcMinEigenVal1(Mat Ixx, Mat Ixy, Mat Iyy);

    vector<float*> CornersChoice1(Mat eig, float thresh);
    void GetKeyPoint1(Mat eig, vector<float*> tmpCorners, int maxCorners, vector<KeyPoint> &corners);

    template <typename T> vector<size_t> sort_indexes1(const vector<T> &v);
    double computeR1(Point2i x1, Point2i x2);


private:
    std::chrono::steady_clock::time_point m_processingStartTime;
    Mat eig_;
    vector<float*> tmpCorners_;
    // MainWindow.h 中添加：
    QTimer *m_flashTimer;
    QColor m_flashColor;
    int m_flashState = 0; // 用于控制颜色轮换
    QTimer *timer;


private:
    int cframe=0;
    PlType plType;

    cv::Mat1d rcx;
    cv::Mat1d rcy;
    cv::Mat1d lcx;  // 第二个亮斑标定矩阵X
    cv::Mat1d lcy;  // 第二个亮斑标定矩阵Y

    cv::Mat depthMap;
    unsigned int unknowns;
    InputType calibrationInputType;
    bool binocularCalibrated;
    bool monoLeftCalibrated;
    bool monoRightCalibrated;

    // 新增成员变量
       cv::Point2f m_averageError;
       std::vector<cv::Point2f> m_predictedPoints;
       std::vector<cv::Point2f> m_errorVectors;

       // 新增函数
       cv::Mat generateErrorPlot();
       void showErrorPlot();

    bool calibrate(cv::Mat &x, cv::Mat &y, cv::Mat &z, cv::Mat1d &c);
    bool calibrateWithValidData(cv::Mat &x, cv::Mat &y, cv::Mat &z, cv::Mat1d &c);
    float evaluateCalibrationError(cv::Mat &x, cv::Mat &y, cv::Mat &targetX, cv::Mat &targetY, cv::Mat1d &cx, cv::Mat1d &cy);
    float evaluate(float x, float y, cv::Mat1d &c);
    cv::Point2f calculateGazeFromCornealVector(cv::Point2f cornealVector, cv::Mat1d &cx, cv::Mat1d &cy);



private:
    std::unique_ptr<DirectShowOpenCVCamera> camera1; // 第一个摄像头对象
    std::unique_ptr<DirectShowOpenCVCamera> camera2; // 第二个摄像头对象

    // ==================== 【新增】摄像头选择和管理 ====================
    struct CameraDeviceInfo {
        int deviceIndex;
        QString friendlyName;
        QString devicePath;
        bool isAvailable;

        CameraDeviceInfo() : deviceIndex(-1), isAvailable(false) {}
        CameraDeviceInfo(int idx, const QString& name, const QString& path, bool available = true)
            : deviceIndex(idx), friendlyName(name), devicePath(path), isAvailable(available) {}
    };

    std::vector<CameraDeviceInfo> m_availableCameras;  // 可用摄像头列表
    int m_selectedEyeCameraIndex;     // 选择的眼球摄像头索引
    int m_selectedSceneCameraIndex;   // 选择的场景摄像头索引
    bool m_camerasInitialized;        // 摄像头是否已初始化

    // 摄像头管理函数
    void createCameraSelectionUI();             // 动态创建摄像头选择UI
    void createCameraSelectionUIInControlsDock(); // 在控件窗口中创建摄像头选择UI
    void enumerateAvailableCameras();           // 枚举可用摄像头
    void populateCameraComboBoxes();            // 填充摄像头选择下拉框
    bool initializeCamerasWithSelection();      // 使用选择的摄像头初始化
    void updateCameraStatus();                  // 更新摄像头状态显示
    QString getCameraConfigString();            // 获取当前摄像头配置信息
    void saveCameraSettings();                  // 保存摄像头设置
    void loadCameraSettings();                  // 加载摄像头设置

    // 摄像头预览功能
    void showCameraPreview(int cameraIndex, const QString& windowTitle);
    void closeCameraPreview();
    std::unique_ptr<DirectShowOpenCVCamera> m_previewCamera;  // DirectShow预览摄像头对象
    QTimer* m_previewTimer;                     // 预览定时器
    QString m_previewWindowName;                // 预览窗口名称

    // 摄像头参数结构
    struct CameraParams {
        int width;
        int height;
        int fps;

        CameraParams(int w = 640, int h = 480, int f = 30) : width(w), height(h), fps(f) {}

        static CameraParams fromString(const QString& resStr, int fps) {
            QStringList parts = resStr.split('x');
            if (parts.size() == 2) {
                return CameraParams(parts[0].toInt(), parts[1].toInt(), fps);
            }
            return CameraParams();
        }
    };

    CameraParams getEyeCameraParams() const;    // 获取眼球摄像头参数
    CameraParams getSceneCameraParams() const;  // 获取场景摄像头参数

    // 动态创建的UI控件指针
    QGroupBox* m_cameraGroupBox;
    QComboBox* m_eyeCameraComboBox;
    QComboBox* m_sceneCameraComboBox;
    QComboBox* m_eyeCameraResComboBox;
    QComboBox* m_eyeCameraFpsComboBox;
    QComboBox* m_sceneCameraResComboBox;
    QComboBox* m_sceneCameraFpsComboBox;
    QPushButton* m_eyeCameraPreviewBtn;
    QPushButton* m_sceneCameraPreviewBtn;
    QPushButton* m_refreshCamerasBtn;
    QPushButton* m_applyCameraSettingsBtn;
    QLabel* m_cameraStatusLabel;

    // ==================== 【新增】图像传输尺寸控制 ====================
    QSlider* m_imageSizeSlider;          // 图像尺寸调节滑块
    QLabel* m_imageSizeLabel;            // 图像尺寸显示标签
    int m_currentImageWidth;             // 当前图像宽度
    int m_currentImageHeight;            // 当前图像高度
    cv::Point2i m_currentCropOffset;     // 当前截图偏移量（用于检测框坐标补偿）
    // ================================================================

    // ==================== 【新增】专业相机控制系统 ====================
    QTreeWidget* m_advancedCameraTree;         // 专业相机控制树形控件
    QGroupBox* m_advancedCameraGroupBox;       // 专业相机控制分组框

    // 专业相机控制组件
    QtCategoryButton1* m_exposureButton;
    FormExposure* m_exposureForm;
    CameraSettingsAdapter* m_cameraAdapter;

    // 专业相机控制函数
    void createAdvancedCameraControls();       // 创建专业相机控制界面
    void setupAdvancedCameraLayout();          // 设置专业相机控制布局
    void connectAdvancedCameraSignals();       // 连接专业相机控制信号
    void updateAdvancedCameraUI();             // 更新专业相机控制UI
    void setupUIButtonIcons();                 // 设置UI按钮图标
    void setupTabifiedDockWidgets();           // 设置所有dock widget为tabified模式
    void createCameraDockWindow();             // 创建独立的摄像头控制窗口
    // ================================================================

    QImage cvMatToQImage(const cv::Mat& mat); // 将OpenCV Mat转换为QImage

private:
    // UI 相关
    void showCalibrationPoint(const cv::Point2f& point);
    void showCalibrationInstruction(const QString& instruction);
    void hideCalibrationUI();
    void showMessage(const QString& message);

    // B键和C键功能相关函数
    void startSinglePointCalibration();
    void finishSinglePointCalibration();
    void showCalibrationMarker(const cv::Mat& calibImage);
    void hideCalibrationMarker();
    void updateStatusIndicators();

    void MainWindow::performSinglePointCorrection();
    cv::Point2f processCalibrationSamples(const std::vector<cv::Point2f>& samples);

    // 成员变量
    QLabel* calibrationPointLabel;
    QLabel* instructionLabel;




private:
    EyeTrackingDialog* m_dialog; // 定义在这里

    // 吸附坐标相关变量
    cv::Point2f m_lastAttractedGaze;    // 最后接收到的吸附坐标
    bool m_useAttractedGaze;            // 是否使用吸附坐标
    bool m_attractionActive;            // 吸附是否激活
    cv::Point2f m_rawGazeBeforeCorrection;  // 校正前的原始视线坐标

    // 系统性视线校正相关变量
    std::vector<cv::Point2f> m_gazeOffsets;            // 视线偏差样本(校正坐标-原始坐标)
    cv::Point2f m_averageOffset;                       // 平均偏差
    bool m_gazeCorrectionEnabled;                      // 是否启用视线校正
    int m_correctionSampleCount;                       // 校正样本数量
    static const int MAX_CORRECTION_SAMPLES = 3;     // 最大样本数量

    // 标定质量阈值常量
    static constexpr float EXCELLENT_THRESHOLD = 30.0f;   // 优秀标定阈值(像素)
    static constexpr float GOOD_THRESHOLD = 50.0f;        // 良好标定阈值(像素)
    static constexpr float ACCEPTABLE_THRESHOLD = 80.0f;   // 可接受标定阈值(像素)
    static constexpr float MAX_ACCEPTABLE_ERROR = 120.0f;  // 最大可接受误差(像素)
    static constexpr int MIN_VALID_POINTS = 7;             // 最少有效点数

    // 多项式拟合标定误差阈值
    static constexpr float POLYNOMIAL_GOOD_THRESHOLD = 40.0f;      // 多项式标定良好阈值(像素)
    static constexpr float POLYNOMIAL_ACCEPTABLE_THRESHOLD = 70.0f; // 多项式标定可接受阈值(像素)
    static constexpr float POLYNOMIAL_MAX_ERROR = 100.0f;          // 多项式标定最大可接受误差(像素)

    // 系统性误差分析变量
    cv::Point2f m_systematicErrorOffset;               // 全局系统性误差偏移
    std::vector<cv::Point2f> m_regionalErrorOffsets;   // 区域性误差偏移(9宫格)
    bool m_systematicErrorCorrectionEnabled;           // 是否启用系统性误差校正

    // 累积误差校正相关变量
    std::vector<cv::Point2f> m_accumulatedErrors;      // 累积的误差向量(已完成点的误差)
    std::vector<cv::Point2f> m_accumulatedTargets;     // 累积的目标点(已完成点的目标)
    cv::Point2f m_cumulativeErrorOffset;               // 基于所有已完成点的累积误差偏移
    bool m_cumulativeErrorCorrectionEnabled;           // 是否启用累积误差校正

    // CalibError轮次管理变量
    cv::Point2f m_finalErrorOffset;                    // 第一轮完成后的最终误差偏置
    bool m_useFixedErrorOffset;                        // 是否使用固定误差偏置（第二轮开始）
    bool m_firstCalibErrorCycleCompleted;              // 第一轮CalibError是否已完成

    std::unique_ptr<EyeTrackingCalibrator> m_calibrator;

    // 瞳孔-角膜标定相关函数
    void loadLatestCalibration();
    void saveCurrentCalibration();

    // Worker 线程相关函数
    void setupWorkerThread();
    void handleProcessingResult(const ProcessingResult& result);

    // 在 MainWindow.h 中添加：
    // 在MainWindow类的private或protected部分添加：
    cv::Mat m_headRotationMatrix;
    cv::Mat m_headTranslationVector;
    cv::Mat m_headRotationVector;
    cv::Mat m_perspectiveMatrix;
    // 头部位姿相关变量
        cv::Vec3d m_headEulerAngles;

 private:
     cv::Vec3d m_cachedEulerAngles;
     cv::Mat m_lastRotationMatrix;
     bool m_eulerAnglesValid = false;


    // 添加辅助函数声明：
    cv::Vec3d rotationMatrixToEulerAngles(const cv::Mat& R);
    void calculatePerspectiveMatrix(const std::vector<cv::Point2f>& imagePoints);
    cv::Vec3d rotationMatrixToEulerAnglesDegrees(const cv::Mat& R);

    void processDetectedMarkers(const std::map<int, std::vector<cv::Point2f>>& markerMap,cv::Mat& displayFrame);
    void drawPoseInfo(cv::Mat& frame, const cv::Mat& headTranslation,
                     const cv::Vec3d& eulerAngles);

    cv::Mat m_cameraMatrix;
    cv::Mat m_distCoeffs;


    static const int FILTER_WINDOW_SIZE = 5;


        // 视线-头部位姿融合函数
        cv::Point2f fuseGazeWithHeadPose(const cv::Point2f& rawGaze,
                                         const cv::Vec3d& eulerAngles,
                                         const cv::Mat& translationVector);
    cv::Vec3d getCachedEulerAngles(const cv::Mat& rotationMatrix);
        // 时间滤波函数
        cv::Point2f temporalFilter(const cv::Point2f& currentGaze);



private slots:
    void updateCamera1Label();                // 更新第一个摄像头的图像
    void updateCamera2Label();                // 更新第二个摄像头
    void on_openButton_clicked();
    void on_interact01_clicked();
    void updateGazeTrail();                   // T模式轨迹更新槽函数
    void updateAIResultsSlot();               // AI结果更新槽函数

    // 新增的按钮槽函数
    void onTargetDetectionClicked();          // 目标检测按钮槽函数
    void onModeAClicked();                    // A模式按钮槽函数
    void onModeSClicked();                    // S模式按钮槽函数
    void onModeDClicked();                    // D模式按钮槽函数
    void onModeFClicked();                    // F模式按钮槽函数

    // 模式控制相关函数
    void initializeModeControlSharedMemory(); // 初始化模式控制共享内存
    void sendModeCommand(char mode);          // 发送模式切换指令

    // ==================== 【新增】摄像头选择控制槽函数 ====================
    void on_refreshCamerasBtn_clicked();      // 刷新摄像头设备列表
    void on_applyCameraSettingsBtn_clicked(); // 应用摄像头设置
    void on_eyeCameraPreviewBtn_clicked();    // 眼球摄像头预览
    void on_sceneCameraPreviewBtn_clicked();  // 场景摄像头预览
    void on_eyeCameraComboBox_currentIndexChanged(int index);    // 眼球摄像头选择改变
    void on_sceneCameraComboBox_currentIndexChanged(int index);  // 场景摄像头选择改变
    void updateCameraPreview();               // 更新摄像头预览图像

    // ==================== 【新增】图像尺寸控制槽函数 ====================
    void onImageSizeSliderChanged(int value); // 图像尺寸滑块值改变
    // ================================================================

private:
    // DataTable and GraphPlot management
    bool m_isDestroying;  // ✅ 析构标志位
    int scaleValue(int value) const;  // 根据 DPI 缩放数值
    qreal getDevicePixelRatio() const;
    DataTable* m_dataTable;
    QList<GraphPlot*> m_graphPlots;
    QList<QDockWidget*> m_dockWidgets; // Track all dock widgets for position saving
    QSplitter* m_dataSplitter;   // 数据分析Split窗口
    QDockWidget* m_dataDock;     // 数据分析停靠窗口
    QTabWidget* m_dataTabWidget; // GraphPlot的Tab容器
    QDockWidget* m_graphTabDock; // Tab容器的Dock窗口
    // ✅ 改为 QPointer（Qt 安全弱指针，对象销毁时自动置空）

    QPointer<QProcess> process;

    // Image window dock widgets
    QDockWidget* m_sceneDock;
    QDockWidget* m_eyeDock;
    QDockWidget* m_controlsDock;
    QDockWidget* m_cameraDock;  // 新增：专门的摄像头控制窗口

    // Data emission functions for table and plots
    void emitPupilData(quint64 timestamp, const PupilData &pupil);
    void emitCameraFPS(double fps);
    void emitProcessingFPS(double fps);
    void emitFramecount(int framecount);

    // Frame counting and FPS calculation variables
    int m_frameCount;
    std::chrono::steady_clock::time_point m_lastFPSTime;
    std::chrono::steady_clock::time_point m_lastProcessingFPSTime;
    std::chrono::steady_clock::time_point m_startTime;
    int m_fpsFrameCount;
    int m_processingFrameCount;

    // Window position save/restore functions
    void saveWindowPositions();
    void restoreWindowPositions();
    void restoreDockDetailedPositions(); // 恢复dock精确位置和尺寸
    void restoreDockSizes(); // 专门恢复dock尺寸
    void restoreDockWidgetPositions();
    void restoreCompleteWindowState(); // Complete restoration in proper order
    void debugWindowState(); // Debug current dock widget state

    // Session state save/restore functions
    void saveSessionState();
    void restoreSessionState();

    // View menu management
    void updateViewMenu();
    QMenu* m_viewMenu;

    // 原始亮斑检测函数声明
    bool detectGlints(const cv::Mat &eyeImage, const cv::Rect &coarseROI, const cv::Point2f &pupilCenter, float pupilRadius, float &x1, float &y1, float &x2, float &y2, cv::Mat &debugImg);

    // Glint detection variables
    bool glintsDetected;
    float glint1_x;
    float glint1_y;
    float glint2_x;
    float glint2_y;

    // Pupil detection variable for calibration
    cv::Point2f pupilCenter;

    // 🚀 Glint tracking system - 直接复用PupilTrackingMethod
    struct GlintAsCircle {
        cv::Point2f center;
        float radius;
        float confidence;
        bool valid;

        GlintAsCircle() : center(0,0), radius(3.0f), confidence(0), valid(false) {}
        GlintAsCircle(cv::Point2f c, float r, float conf) : center(c), radius(r), confidence(conf), valid(true) {}

        // 转换为Pupil对象供PupilTrackingMethod使用
        Pupil toPupil() const {
            Pupil p;
            p.center = center;
            p.size = cv::Size2f(radius*2, radius*2);
            p.angle = 0;
            p.confidence = confidence;
            return p;
        }

        // 从Pupil对象转换回来
        static GlintAsCircle fromPupil(const Pupil& p) {
            return GlintAsCircle(p.center, p.size.width/2.0f, p.confidence);
        }
    };

    // 亮斑跟踪器状态
    GlintAsCircle m_lastGlint1;            // 上一帧左亮斑
    GlintAsCircle m_lastGlint2;            // 上一帧右亮斑
    bool m_glint1TrackingActive;           // 左亮斑跟踪激活
    bool m_glint2TrackingActive;           // 右亮斑跟踪激活
    int m_glintTrackingLostFrames1;        // 左亮斑丢失帧数
    int m_glintTrackingLostFrames2;        // 右亮斑丢失帧数

    // 复用现有的跟踪算法
    PupilTrackingMethod* glintTrackingMethod; // 复用瞳孔跟踪算法跟踪亮斑

    // 集成式亮斑跟踪函数（仿照瞳孔跟踪模式）
    bool integratedGlintTracking(const cv::Mat &eyeImage, const cv::Rect &coarseROI,
                                const cv::Point2f &pupilCenter, float pupilRadius,
                                float &x1, float &y1, float &x2, float &y2, cv::Mat &debugImg);

    // 混合检测+跟踪函数（保留作为备用）
    bool hybridGlintDetectionAndTracking(const cv::Mat &eyeImage, const cv::Rect &coarseROI,
                                        const cv::Point2f &pupilCenter, float pupilRadius,
                                        float &x1, float &y1, float &x2, float &y2, cv::Mat &debugImg);

    // 亮斑跟踪函数（复用PupilTrackingMethod）
    bool trackGlintUsingPupilTracker(const cv::Mat &eyeROI, const GlintAsCircle &lastGlint,
                                    GlintAsCircle &currentGlint, const cv::Rect &searchROI);

    // 亮斑检测简化版（只在跟踪失败时使用）
    bool detectGlintsSimplified(const cv::Mat &eyeImage, const cv::Rect &coarseROI,
                               const cv::Point2f &pupilCenter, float pupilRadius,
                               std::vector<cv::Point2f> &glintCenters);

    // 双亮斑角膜反射视线跟踪系统
    struct DualGlintMapping {
        cv::Mat mappingMatrix_Glint1;     // 瞳孔->亮斑1矢量的映射矩阵
        cv::Mat mappingMatrix_Glint2;     // 瞳孔->亮斑2矢量的映射矩阵
        bool glint1MappingValid;          // 亮斑1映射是否有效
        bool glint2MappingValid;          // 亮斑2映射是否有效
        float glint1Accuracy;             // 亮斑1映射精度
        float glint2Accuracy;             // 亮斑2映射精度

        DualGlintMapping() : glint1MappingValid(false), glint2MappingValid(false),
                           glint1Accuracy(0), glint2Accuracy(0) {}
    };

    DualGlintMapping m_dualGlintMapping;

    // 双亮斑标定数据存储
    std::vector<cv::Point2f> m_dualGlint_pupilCenters;    // 标定时的瞳孔中心
    std::vector<cv::Point2f> m_dualGlint_glint1Positions; // 标定时的亮斑1位置
    std::vector<cv::Point2f> m_dualGlint_glint2Positions; // 标定时的亮斑2位置
    std::vector<cv::Point2f> m_dualGlint_screenTargets;   // 标定时的屏幕目标点
    std::vector<bool> m_dualGlint_hasGlint1;              // 是否检测到亮斑1
    std::vector<bool> m_dualGlint_hasGlint2;              // 是否检测到亮斑2

    // 兼容性配置
    bool m_useDualGlintMapping;                           // 是否启用双亮斑映射（默认启用）

    // 双亮斑标定和计算函数
    void addDualGlintCalibrationSample(const cv::Point2f& pupilCenter,
                                     const cv::Point2f& glint1, bool hasGlint1,
                                     const cv::Point2f& glint2, bool hasGlint2,
                                     const cv::Point2f& screenTarget);
    void computeDualGlintMappings();
    cv::Point2f calculateDualGlintGaze(const cv::Point2f& pupilCenter,
                                     const cv::Point2f& glint1, bool hasGlint1,
                                     const cv::Point2f& glint2, bool hasGlint2);
    cv::Point2f pupilToGlintVector(const cv::Point2f& pupil, const cv::Point2f& glint);
    cv::Point2f mapVectorToScreen(const cv::Point2f& vector, const cv::Mat& mappingMatrix);

    // 辅助函数声明
    bool computeMappingMatrix(const std::vector<cv::Point2f>& inputVectors,
                            const std::vector<cv::Point2f>& outputTargets,
                            cv::Mat& mappingMatrix);
    float evaluateMappingAccuracy(const std::vector<cv::Point2f>& inputVectors,
                                const std::vector<cv::Point2f>& outputTargets,
                                const cv::Mat& mappingMatrix);

    void KeyPoint1(cv::Mat eig, vector<float*> tmpCorners, int maxCorners, std::vector<cv::KeyPoint> &corners);

    // 3D注视计算相关类
    class Gaze3DCalculator {
    public:
        struct GazeVector3D {
            cv::Point3f origin;      // 眼球中心世界坐标
            cv::Point3f direction;   // 注视方向向量
            bool valid;

            GazeVector3D() : origin(0,0,0), direction(0,0,0), valid(false) {}
            GazeVector3D(const cv::Point3f& orig, const cv::Point3f& dir)
                : origin(orig), direction(dir), valid(true) {}
        };

        // 视线角度信息结构
        struct GazeAngles {
            float azimuth_deg;       // 方位角（水平角度）
            float elevation_deg;     // 仰角（垂直角度）
            float magnitude;         // 向量长度
            bool valid;

            GazeAngles() : azimuth_deg(0), elevation_deg(0), magnitude(0), valid(false) {}
            GazeAngles(float az, float el, float mag)
                : azimuth_deg(az), elevation_deg(el), magnitude(mag), valid(true) {}
        };

        Gaze3DCalculator();
        GazeVector3D computeGaze3D(const cv::Point2f& pupilCenter,
                                  const cv::Point2f& sphereCenter,
                                  int screenWidth = 640,
                                  int screenHeight = 480);

        // 新增：角度计算功能
        GazeAngles computeGazeAngles(const GazeVector3D& gaze3D);

        // 新增：改进的屏幕映射功能（类似Python实现）
        cv::Point2f mapToScreen(const GazeVector3D& gaze3D,
                               int screenWidth = 1920,
                               int screenHeight = 1080);

        void updateEyeSphereHistory(const cv::RotatedRect& ellipse);
        cv::Point2f computeAverageEyeCenter(const std::vector<cv::RotatedRect>& rayHistory);
        cv::Point2f project3DToScreen(const GazeVector3D& gaze3D, int screenWidth = 640, int screenHeight = 480,
                                     float maxAzimuthDeg = 120.0f, float maxElevationDeg = 100.0f);

        // 新增：数据输出功能
        void writeGazeData(const GazeVector3D& gaze3D, const GazeAngles& angles,
                          const cv::Point2f& screenPoint);

    private:
        static const int MAX_RAY_HISTORY = 100;
        cv::Point3f computeSphereIntersection(const cv::Point3f& rayOrigin,
                                            const cv::Point3f& rayDirection,
                                            const cv::Point3f& sphereCenter,
                                            float sphereRadius = 1.0f/1.05f);
        cv::Mat computeRotationMatrix(const cv::Point3f& targetDirection);
    };

    // 3D注视计算相关成员变量
    std::unique_ptr<Gaze3DCalculator> m_gaze3DCalculator;
    cv::Point2f m_currentEyeSphereCenter;

    // 【新增】假3D算法所需的数据成员
    std::vector<cv::Point2f> m_storedIntersections;  // 存储射线交点（模拟Python的stored_intersections）
    std::vector<cv::Point2f> m_eyeCenterHistory;     // 眼球中心历史点（模拟Python的point_list）
    std::vector<cv::RotatedRect> m_rayHistory;       // 射线椭圆历史（从Gaze3DCalculator移动到此处）
    static const int MAX_INTERSECTIONS = 50;         // 最大交点存储数（模拟Python的M参数）
    static const int MAX_CENTER_HISTORY = 20;        // 最大中心历史数（模拟Python的N参数）

    QTimer* m_gazeTimer; // 【新增】视线检测定时器
    QDateTime m_gazeFixationStart; // 【新增】注视开始时间
    bool m_is3DZeroCalibrated;
    float m_zeroAzimuthOffset;     // 真3D用的偏移（保留，但现在不用于假3D）
    float m_zeroElevationOffset;   // 真3D用的偏移（保留，但现在不用于假3D）

    // 【新增】假3D独立的标定偏移量
    float m_fake3D_zeroPitchOffset;     // 假3D的Pitch零点偏移
    float m_fake3D_zeroYawOffset;       // 假3D的Yaw零点偏移
    bool m_fake3D_isCalibrated;         // 假3D是否已标定
    cv::Point2f m_lastFake3DScreenCoords; // 【修复】缓存假3D计算的屏幕坐标

    // 【新增】假3D多点标定支持
    std::vector<cv::Point2f> m_fake3D_calibAngles;    // 收集的标定角度数据 (pitch, yaw)
    std::vector<cv::Point2f> m_fake3D_calibScreens;   // 对应的屏幕坐标
    cv::Mat m_fake3D_transformMatrix;                 // 角度到屏幕的映射矩阵
    bool m_fake3D_multiPointCalibrated;               // 是否完成多点标定

    // 类似3DTracker的眼球中心跟踪变量 (已在上面定义)
    static const int MAX_EYE_CENTER_HISTORY = 200;  // 最大历史记录数

    // 3D屏幕映射标定参数（可调整以适应不同用户）
    float m_maxAzimuthDeg;    // 最大水平视角范围（度）
    float m_maxElevationDeg;  // 最大垂直视角范围（度）

    bool m_isFirstShow = true; // 添加一个标志位
    // 3D标定系统
    bool m_3DCalibrated;                    // 是否已完成3D标定
    cv::Point3f m_referenceGazeDirection;   // 参考视线方向（对应屏幕中心）
    cv::Point3f m_referenceSphereCenter;    // 参考球心位置
    std::vector<cv::Point3f> m_calibrationDirections;  // 标定点的3D方向
    std::vector<cv::Point2f> m_calibrationScreenPoints; // 对应的屏幕坐标

    // 3D多点标定系统（复用现有CalibError系统）
    std::vector<cv::Point3f> m_3DCalibrationDirections; // 3D标定采集的方向向量（与m_gazePoints对应）
    std::vector<cv::Point2f> m_3DCalibrationOffsets;    // 各标定点的校正偏移量
    bool m_3DRegionalCorrectionEnabled;    // 是否启用3D区域校正

    // 3D标定的专用数据采集
    std::vector<cv::Point2f> m_3DCalibrationPupilPoints;  // 3D标定采集的瞳孔点
    std::vector<cv::Point2f> m_3DCalibrationScreenPoints; // 3D标定对应的屏幕点
    bool m_is3DCalibrated; // 3D标定状态标志

    // 3D注视相关函数
    cv::Point2f calculateGaze3D(const cv::Point2f& pupilCenter);
    bool is3DGazeModeActive() const { return flag_calibration_status == 2; }

    // 【新增】假3D算法辅助函数（基于temp05 Python实现）
    void updateEllipseHistory(const PupilData& pupilData);
    cv::Point2f computeAverageIntersection();
    cv::Point2f updateAndAveragePoint(const cv::Point2f& newPoint);
    struct GazeVector3D { cv::Point3f direction; bool valid; };
    GazeVector3D computeGazeVector(const cv::Point2f& pupilCenter, const cv::Point2f& sphereCenter, int screenWidth, int screenHeight);
    cv::Point2f findLineIntersection(const cv::RotatedRect& ellipse1, const cv::RotatedRect& ellipse2);
    void perform3DCalibration(); // 执行3D几何标定
    void draw3DGazeVisualization(cv::Mat& eyeImage, const cv::Point2f& pupilCenter, const cv::RotatedRect& pupilEllipse);
    void updateEyeSphereCenter(const cv::Point2f& pupilCenter);  // 动态更新眼球中心
    cv::Point2f mapGaze3DToScreen(const cv::Point3f& gazeDirection, int screenWidth, int screenHeight);  // 使用参考点映射

    // 【新增】假3D独立标定系统
    void performFake3DCalibration();  // 假3D单点零点校正
    cv::Point2f mapFake3DAnglesToScreen(double pitch, double yaw, int screenWidth, int screenHeight);  // 假3D角度映射到屏幕坐标
    cv::Point2f calculateFake3DAnglesForScreenPoint(const cv::Point2f& screenPoint);  // 计算屏幕坐标对应的假3D角度
    void computeFake3DMultiPointMapping();  // 计算假3D多点映射矩阵
    cv::Point2f applyFake3DMultiPointMapping(double pitch, double yaw);  // 应用多点映射矩阵
    void showFake3DCalibrationResult(double avgError);  // 显示假3D标定结果
    cv::Mat generateFake3DErrorPlot();  // 生成假3D误差分析图
    double calculateFake3DAverageError();  // 计算假3D平均误差
    void collectFake3DCalibrationData();  // 收集假3D标定数据

    // 【新增】假3D多项式拟合辅助函数
    void fillFake3DPolynomialCoefficients(cv::Mat& A, int row, float pitch, float yaw, PlType plType, int unknowns);
    QString getPolynomialMethodName(PlType plType);
    float applyFake3DPolynomial(float pitch, float yaw, PlType plType, int unknowns, bool isXCoord);

    // 【修复】3D状态重置
    void reset3DStates();  // 重置3D相关状态

    // 3D多点标定函数（复用现有CalibError系统）
    void complete3DMultiPointCalibration();                     // 完成3D多点标定，计算区域校正
    cv::Point2f apply3DRegionalCorrection(const cv::Point2f& rawScreenPoint); // 应用3D区域校正
    cv::Point2f interpolate3DCorrection(const cv::Point2f& screenPoint) const; // 插值计算3D校正
    void show3DCalibrationResult(const CalibrationQuality& quality);          // 显示3D标定结果
    bool save3DCalibrationParams();                              // 保存3D标定参数（模仿精确标定）
    void show3DErrorPlot();                                      // 显示3D误差分析图（模仿精确标定）
    cv::Mat generate3DErrorPlot();                               // 生成3D误差分析图

signals:
    void pupilDataUpdated(quint64 timestamp, const PupilData &pupil, const QString &filename);
    void processingResultUpdated(const ProcessingResult &result);  // 新增眼动指标信号
    void cameraFPSUpdated(double fps);
    void processingFPSUpdated(double fps);
    void framecountUpdated(int framecount);
};




#endif // MAINWINDOW_H
