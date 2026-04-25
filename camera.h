#ifndef CAMERA_H
#define CAMERA_H

#include <opencv2/opencv.hpp>
#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QThread>
#include <atomic>
#include <memory>


struct CameraConfig {
    int width = 640;
    int height = 480;
    int fps = 30;
    int deviceIndex = 0;
    QString deviceName;
};


enum class CameraStatus {
    UNINITIALIZED,
    INITIALIZING,
    RUNNING,
    STOPPED,
    ERROR_STATE
};


class Camera : public QObject {
    Q_OBJECT

public:
    explicit Camera(int deviceIndex, const CameraConfig& config, QObject* parent = nullptr);
    ~Camera();

    bool initialize();
    bool start();
    bool stop();

    // 获取状态信息
    CameraStatus getStatus() const { return status_; }
    double getActualFPS() const { return actual_fps_.load(); }
    uint64_t getFrameCount() const { return frame_count_.load(); }
    int getCameraId() const { return camera_id_; }

    // 获取最新帧
    bool getLatestFrame(cv::Mat& frame);

    // 配置相关
    void setConfig(const CameraConfig& config) { config_ = config; }
    CameraConfig getConfig() const { return config_; }

signals:
    void frameReady(int cameraId, const cv::Mat& frame);
    void statusChanged(int cameraId, CameraStatus status);
    void errorOccurred(int cameraId, const QString& error);

private slots:
    void captureFrame();

private:
    void updateFPSStats();
    void setStatus(CameraStatus status);

private:
    cv::VideoCapture capture_;
    CameraConfig config_;
    CameraStatus status_;
    int camera_id_;

    // 帧率统计
    std::atomic<double> actual_fps_;
    std::atomic<uint64_t> frame_count_;
    std::chrono::high_resolution_clock::time_point last_time_;

    // 线程安全
    QMutex frame_mutex_;
    cv::Mat current_frame_;
    std::atomic<bool> new_frame_available_;

    // 定时器用于帧捕获
    QTimer* capture_timer_;
};

/**
 * @brief 双摄像头管理器
 */
class DualCameraManager : public QObject {
    Q_OBJECT

public:
    explicit DualCameraManager(QObject* parent = nullptr);
    ~DualCameraManager();

    bool initializeCameras(int camera1Index = 0, int camera2Index = 1);
    bool startCameras();
    bool stopCameras();

    // 获取摄像头实例
    Camera* getCamera(int index) const;

    // 获取最新帧
    bool getLatestFrame(int cameraIndex, cv::Mat& frame);

signals:
    void frameReady(int cameraId, const cv::Mat& frame);
    void cameraStatusChanged(int cameraId, CameraStatus status);
    void cameraError(int cameraId, const QString& error);

private slots:
    void onFrameReady(int cameraId, const cv::Mat& frame);
    void onStatusChanged(int cameraId, CameraStatus status);
    void onErrorOccurred(int cameraId, const QString& error);

private:
    std::unique_ptr<Camera> camera1_;
    std::unique_ptr<Camera> camera2_;
    CameraConfig default_config_;
};

#endif // CAMERA_H

