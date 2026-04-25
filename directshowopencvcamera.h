#ifndef DIRECTSHOWOPENCVCAMERA_H
#define DIRECTSHOWOPENCVCAMERA_H

#include <windows.h>
#include <dshow.h>
#include <comdef.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <QObject>

// 前向声明
class OpenCVImageCallback;
interface ISampleGrabber;

struct CameraConfig {
    int width = 640;
    int height = 480;
    int fps = 240;
    int deviceIndex = 0;
    std::string deviceName;
};

enum class CameraStatus {
    UNINITIALIZED, INITIALIZING, RUNNING, STOPPED,
};

class DirectShowOpenCVCamera : public QObject {
    Q_OBJECT
private:
    IGraphBuilder* graph_builder_;
    ICaptureGraphBuilder2* capture_builder_;
    IMediaControl* media_control_;
    IMediaEvent* media_event_;
    IBaseFilter* source_filter_;
    IBaseFilter* video_renderer_;
    IBaseFilter* sample_grabber_filter_;
    ISampleGrabber* sample_grabber_; // 第37行可能的错误位置


public:
    CameraConfig config_;
    CameraStatus status_;
    std::unique_ptr<OpenCVImageCallback> opencv_callback_;

    DirectShowOpenCVCamera(int deviceIndex, const CameraConfig& config);
    ~DirectShowOpenCVCamera();

    bool Initialize();
    bool Start();
    bool Stop();
    CameraStatus GetStatus() const { return status_; }

signals:
    void newFrameAvailable();

private:
    bool CreateVideoSource();
    bool CreateSampleGrabber();
    bool CreateVideoRenderer();
    bool ConnectFilters();
    void ConfigureCameraFormat();
    bool GetPin(IBaseFilter* filter, PIN_DIRECTION dir, int index, IPin** pin);
    void DeleteMediaType(AM_MEDIA_TYPE* mt);
    void ReleaseInterfaces();
};

#endif // DIRECTSHOWOPENCVCAMERA_H
