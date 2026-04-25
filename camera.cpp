#include "camera.h"
#include <QDebug>
#include <QApplication>
#include <chrono>

Camera::Camera(int deviceIndex, const CameraConfig& config, QObject* parent)
    : QObject(parent)
    , config_(config)
    , status_(CameraStatus::UNINITIALIZED)
    , camera_id_(deviceIndex)
    , actual_fps_(0.0)
    , frame_count_(0)
    , new_frame_available_(false)
    , capture_timer_(nullptr)
{
    config_.deviceIndex = deviceIndex;
    last_time_ = std::chrono::high_resolution_clock::now();

    // 创建捕获定时器
    capture_timer_ = new QTimer(this);
    connect(capture_timer_, &QTimer::timeout, this, &Camera::captureFrame);
}

Camera::~Camera() {
    stop();
}

bool Camera::initialize() {
    setStatus(CameraStatus::INITIALIZING);

    try {
        // 在Windows上使用DirectShow后端
        capture_.open(config_.deviceIndex, cv::CAP_DSHOW);

        if (!capture_.isOpened()) {
            qDebug() << "Failed to open camera" << config_.deviceIndex;
            setStatus(CameraStatus::ERROR_STATE);
            emit errorOccurred(camera_id_, QString("Failed to open camera %1").arg(config_.deviceIndex));
            return false;
        }

        // 设置摄像头参数
        capture_.set(cv::CAP_PROP_FRAME_WIDTH, config_.width);
        capture_.set(cv::CAP_PROP_FRAME_HEIGHT, config_.height);
        capture_.set(cv::CAP_PROP_FPS, config_.fps);

        // Windows特定优化
        capture_.set(cv::CAP_PROP_BUFFERSIZE, 1); // 减少缓冲延迟

        // 验证设置是否成功
        int actual_width = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH));
        int actual_height = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT));
        double actual_fps = capture_.get(cv::CAP_PROP_FPS);

        qDebug() << "Camera" << camera_id_ << "initialized:";
        qDebug() << "  Resolution:" << actual_width << "x" << actual_height;
        qDebug() << "  FPS:" << actual_fps;

        setStatus(CameraStatus::STOPPED);
        return true;

    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV exception in camera initialization:" << e.what();
        setStatus(CameraStatus::ERROR_STATE);
        emit errorOccurred(camera_id_, QString("OpenCV error: %1").arg(e.what()));
        return false;
    }
}

bool Camera::start() {
    if (status_ != CameraStatus::STOPPED) {
        return false;
    }

    if (!capture_.isOpened()) {
        qDebug() << "Camera not initialized";
        return false;
    }

    // 启动定时器，根据FPS计算间隔
    int interval = 1000 / config_.fps;  // 毫秒
    capture_timer_->start(interval);

    setStatus(CameraStatus::RUNNING);
    qDebug() << "Camera" << camera_id_ << "started with" << config_.fps << "FPS";
    return true;
}

bool Camera::stop() {
    if (capture_timer_) {
        capture_timer_->stop();
    }

    if (capture_.isOpened()) {
        capture_.release();
    }

    setStatus(CameraStatus::STOPPED);
    qDebug() << "Camera" << camera_id_ << "stopped";
    return true;
}

void Camera::captureFrame() {
    if (status_ != CameraStatus::RUNNING || !capture_.isOpened()) {
        return;
    }

    try {
        cv::Mat frame;
        bool success = capture_.read(frame);

        if (success && !frame.empty()) {
            // 线程安全地更新当前帧
            QMutexLocker locker(&frame_mutex_);
            current_frame_ = frame.clone();
            new_frame_available_ = true;

            // 更新统计信息
            updateFPSStats();

            // 发射信号
            emit frameReady(camera_id_, frame);
        } else {
            qDebug() << "Failed to capture frame from camera" << camera_id_;
        }

    } catch (const cv::Exception& e) {
        qDebug() << "OpenCV exception in frame capture:" << e.what();
        emit errorOccurred(camera_id_, QString("Frame capture error: %1").arg(e.what()));
    }
}

bool Camera::getLatestFrame(cv::Mat& frame) {
    QMutexLocker locker(&frame_mutex_);

    if (!new_frame_available_ || current_frame_.empty()) {
        return false;
    }

    frame = current_frame_.clone();
    new_frame_available_ = false;
    return true;
}

void Camera::updateFPSStats() {
    frame_count_.fetch_add(1);

    // 每30帧计算一次实际FPS
    if (frame_count_.load() % 30 == 0) {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time_);

        if (elapsed.count() > 0) {
            double fps = 30000.0 / elapsed.count();  // 30帧 * 1000ms
            actual_fps_.store(fps);
        }

        last_time_ = current_time;
    }
}

void Camera::setStatus(CameraStatus status) {
    if (status_ != status) {
        status_ = status;
        emit statusChanged(camera_id_, status);
    }
}

// DualCameraManager 实现

DualCameraManager::DualCameraManager(QObject* parent)
    : QObject(parent)
{
    // 设置默认配置
    default_config_.width = 640;
    default_config_.height = 480;
    default_config_.fps = 30;
}

DualCameraManager::~DualCameraManager() {
    stopCameras();
}

bool DualCameraManager::initializeCameras(int camera1Index, int camera2Index) {
    // 创建摄像头实例
    camera1_ = std::make_unique<Camera>(camera1Index, default_config_, this);
    camera2_ = std::make_unique<Camera>(camera2Index, default_config_, this);

    // 连接信号
    connect(camera1_.get(), &Camera::frameReady, this, &DualCameraManager::onFrameReady);
    connect(camera1_.get(), &Camera::statusChanged, this, &DualCameraManager::onStatusChanged);
    connect(camera1_.get(), &Camera::errorOccurred, this, &DualCameraManager::onErrorOccurred);

    connect(camera2_.get(), &Camera::frameReady, this, &DualCameraManager::onFrameReady);
    connect(camera2_.get(), &Camera::statusChanged, this, &DualCameraManager::onStatusChanged);
    connect(camera2_.get(), &Camera::errorOccurred, this, &DualCameraManager::onErrorOccurred);

    // 初始化摄像头
    bool success1 = camera1_->initialize();
    bool success2 = camera2_->initialize();

    qDebug() << "Camera initialization results:";
    qDebug() << "  Camera 1 (index" << camera1Index << "):" << (success1 ? "SUCCESS" : "FAILED");
    qDebug() << "  Camera 2 (index" << camera2Index << "):" << (success2 ? "SUCCESS" : "FAILED");

    return success1 && success2;
}

bool DualCameraManager::startCameras() {
    bool success1 = camera1_ ? camera1_->start() : false;
    bool success2 = camera2_ ? camera2_->start() : false;

    qDebug() << "Camera start results:";
    qDebug() << "  Camera 1:" << (success1 ? "STARTED" : "FAILED");
    qDebug() << "  Camera 2:" << (success2 ? "STARTED" : "FAILED");

    return success1 && success2;
}

bool DualCameraManager::stopCameras() {
    bool success1 = camera1_ ? camera1_->stop() : true;
    bool success2 = camera2_ ? camera2_->stop() : true;

    return success1 && success2;
}

Camera* DualCameraManager::getCamera(int index) const {
    if (index == 0) {
        return camera1_.get();
    } else if (index == 1) {
        return camera2_.get();
    }
    return nullptr;
}

bool DualCameraManager::getLatestFrame(int cameraIndex, cv::Mat& frame) {
    Camera* camera = getCamera(cameraIndex);
    return camera ? camera->getLatestFrame(frame) : false;
}

void DualCameraManager::onFrameReady(int cameraId, const cv::Mat& frame) {
    emit frameReady(cameraId, frame);
}

void DualCameraManager::onStatusChanged(int cameraId, CameraStatus status) {
    emit cameraStatusChanged(cameraId, status);
}

void DualCameraManager::onErrorOccurred(int cameraId, const QString& error) {
    emit cameraError(cameraId, error);
}

#include "camera.moc"

