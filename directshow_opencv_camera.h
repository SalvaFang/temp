#ifndef DIRECTSHOW_OPENCV_CAMERA_H
#define DIRECTSHOW_OPENCV_CAMERA_H

#include <windows.h>
#include <dshow.h>
#include <comdef.h>
#include <opencv2/opencv.hpp>
#include <QImage>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <iostream>

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "user32.lib")

// Manual GUID definitions
DEFINE_GUID(CLSID_SampleGrabber, 0xc1f400a0, 0x3f08, 0x11d3, 0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37);
DEFINE_GUID(IID_ISampleGrabber, 0x6b652fff, 0x11fe, 0x4fce, 0x92, 0xad, 0x02, 0x66, 0xb5, 0xd7, 0xc7, 0x8f);
DEFINE_GUID(IID_ISampleGrabberCB, 0x0579154a, 0x2b53, 0x4994, 0xb0, 0xd0, 0xe7, 0x73, 0x14, 0x8e, 0xff, 0x85);
DEFINE_GUID(CLSID_NullRenderer, 0xc1f400a4, 0x3f08, 0x11d3, 0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37);

// Manual interface definitions
interface ISampleGrabberCB : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE SampleCB(double SampleTime, IMediaSample* pSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE BufferCB(double SampleTime, BYTE* pBuffer, long BufferLen) = 0;
};

interface ISampleGrabber : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE SetOneShot(BOOL OneShot) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMediaType(const AM_MEDIA_TYPE* pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType( AM_MEDIA_TYPE* pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL BufferThem) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(long* pBufferSize, long* pBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(IMediaSample** ppSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCallback(ISampleGrabberCB* pCallback, long WhichMethodToCallback) = 0;
};

// CameraConfig and CameraStatus
struct CameraConfig {
    int width = 640;
    int height = 480;
    int fps = 240;
    int deviceIndex = 0;
    std::string deviceName;
};

enum class CameraStatus {
    UNINITIALIZED, INITIALIZING, RUNNING, STOPPED
};

class OpenCVImageCallback : public ISampleGrabberCB {
private:
    ULONG ref_count_;
    cv::Mat current_frame_;
    std::mutex frame_mutex_;
    std::atomic<bool> new_frame_available_;
    std::atomic<int> frame_count_;
    int camera_id_;
    ISampleGrabber* sample_grabber_;

public:
    OpenCVImageCallback(int camera_id) : ref_count_(1), new_frame_available_(false), frame_count_(0), camera_id_(camera_id), sample_grabber_(nullptr) {}

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) {
        if (riid == IID_IUnknown || riid == IID_ISampleGrabberCB) {
            *ppv = static_cast<ISampleGrabberCB*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&ref_count_); }
    STDMETHODIMP_(ULONG) Release() {
        ULONG count = InterlockedDecrement(&ref_count_);
        if (count == 0) { delete this; }
        return count;
    }

    STDMETHODIMP SampleCB(double SampleTime, IMediaSample* pSample) { return S_OK; }

    STDMETHODIMP BufferCB(double SampleTime, BYTE* pBuffer, long BufferLen) {
        if (!pBuffer || BufferLen <= 0 || !sample_grabber_) return S_OK;

        try {
            AM_MEDIA_TYPE mt;
            HRESULT hr = sample_grabber_->GetConnectedMediaType(&mt);
            if (FAILED(hr)) return S_OK;

            if (mt.majortype == MEDIATYPE_Video && mt.formattype == FORMAT_VideoInfo) {
                VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.pbFormat;
                int width = vih->bmiHeader.biWidth;
                int height = abs(vih->bmiHeader.biHeight);
                int expected_size = width * height * 3;

                if (BufferLen >= expected_size && mt.subtype == MEDIASUBTYPE_RGB24) {
                    std::lock_guard<std::mutex> lock(frame_mutex_);
                    cv::Mat temp_frame(height, width, CV_8UC3, pBuffer);
                    cv::cvtColor(temp_frame, current_frame_, cv::COLOR_BGR2RGB);
                    new_frame_available_ = true;
                    frame_count_.fetch_add(1);
                }
            }
            DeleteMediaType(&mt);
        } catch (const std::exception& e) {
            std::cerr << "OpenCV error: " << e.what() << std::endl;
        }
        return S_OK;
    }

    bool GetLatestFrame(cv::Mat& frame) {
        if (!new_frame_available_) return false;
        std::lock_guard<std::mutex> lock(frame_mutex_);
        if (!current_frame_.empty()) {
            frame = current_frame_.clone();
            new_frame_available_ = false;
            return true;
        }
        return false;
    }

    int GetFrameCount() const { return frame_count_.load(); }

    QImage getLatestQImage() {
        cv::Mat frame;
        if (GetLatestFrame(frame)) {
            QImage img(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
            return img.copy();
        }
        return QImage();
    }

    void setSampleGrabber(ISampleGrabber* grabber) { sample_grabber_ = grabber; }

private:
    void DeleteMediaType(AM_MEDIA_TYPE* mt) {
        if (mt->cbFormat != 0) {
            CoTaskMemFree(mt->pbFormat);
            mt->cbFormat = 0;
            mt->pbFormat = nullptr;
        }
        if (mt->pUnk) {
            mt->pUnk->Release();
            mt->pUnk = nullptr;
        }
        CoTaskMemFree(mt);
    }
};

class DirectShowOpenCVCamera {
private:
    IGraphBuilder* graph_builder_;
    ICaptureGraphBuilder2* capture_builder_;
    IMediaControl* media_control_;
    IMediaEvent* media_event_;
    IBaseFilter* source_filter_;
    IBaseFilter* sample_grabber_filter_;
    ISampleGrabber* sample_grabber_;
    CameraConfig config_;
    CameraStatus status_;
    std::atomic<double> actual_fps_;
    std::atomic<uint64_t> frame_count_;
    LARGE_INTEGER frequency_;
    LARGE_INTEGER last_time_;
    std::unique_ptr<OpenCVImageCallback> opencv_callback_;

public:
    DirectShowOpenCVCamera(int deviceIndex, const CameraConfig& config, HWND parent_hwnd = nullptr)
        : graph_builder_(nullptr), capture_builder_(nullptr), media_control_(nullptr),
          media_event_(nullptr), source_filter_(nullptr), sample_grabber_filter_(nullptr),
          sample_grabber_(nullptr), config_(config), status_(CameraStatus::UNINITIALIZED),
          actual_fps_(0.0), frame_count_(0) {
        config_.deviceIndex = deviceIndex;
        QueryPerformanceFrequency(&frequency_);
        QueryPerformanceCounter(&last_time_);
        opencv_callback_ = std::make_unique<OpenCVImageCallback>(deviceIndex);
    }

    ~DirectShowOpenCVCamera() {
        Stop();
        ReleaseInterfaces();
    }

    bool Initialize() {
        status_ = CameraStatus::INITIALIZING;
        HRESULT hr;

        hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&graph_builder_);
        if (FAILED(hr)) return false;

        hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&capture_builder_);
        if (FAILED(hr)) return false;

        hr = capture_builder_->SetFiltergraph(graph_builder_);
        if (FAILED(hr)) return false;

        hr = graph_builder_->QueryInterface(IID_IMediaControl, (void**)&media_control_);
        if (FAILED(hr)) return false;

        hr = graph_builder_->QueryInterface(IID_IMediaEvent, (void**)&media_event_);
        if (FAILED(hr)) return false;

        if (!CreateVideoSource()) return false;
        if (!CreateSampleGrabber()) return false;
        if (!ConnectFilters()) return false;

        status_ = CameraStatus::STOPPED;
        std::cout << "Camera " << config_.deviceIndex << " initialized" << std::endl;
        return true;
    }

    bool Start() {
        if (status_ != CameraStatus::STOPPED) return false;

        HRESULT hr = media_control_->Run();
        if (FAILED(hr)) return false;

        status_ = CameraStatus::RUNNING;
        std::cout << "Camera " << config_.deviceIndex << " started" << std::endl;
        return true;
    }

    bool Stop() {
        if (status_ == CameraStatus::RUNNING && media_control_) {
            media_control_->Stop();
        }
        status_ = CameraStatus::STOPPED;
        return true;
    }

    double GetActualFPS() const { return actual_fps_.load(); }
    uint64_t GetFrameCount() const { return frame_count_.load(); }
    CameraStatus GetStatus() const { return status_; }
    int GetOpenCVFrameCount() const { return opencv_callback_ ? opencv_callback_->GetFrameCount() : 0; }
    QImage GetLatestQImage() { return opencv_callback_ ? opencv_callback_->getLatestQImage() : QImage(); }

    void UpdateFPSStats() {
        frame_count_.fetch_add(1);
        if (frame_count_.load() % 60 == 0) {
            LARGE_INTEGER current_time;
            QueryPerformanceCounter(&current_time);
            double elapsed = static_cast<double>(current_time.QuadPart - last_time_.QuadPart) / frequency_.QuadPart;
            if (elapsed > 0) actual_fps_.store(60.0 / elapsed);
            last_time_ = current_time;
        }
    }

private:
    bool CreateVideoSource() {
        ICreateDevEnum* dev_enum = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&dev_enum);
        if (FAILED(hr)) return false;

        IEnumMoniker* enum_moniker = nullptr;
        hr = dev_enum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &enum_moniker, 0);
        if (hr != S_OK) {
            dev_enum->Release();
            return false;
        }

        IMoniker* moniker = nullptr;
        int current_index = 0;
        bool found = false;

        while (enum_moniker->Next(1, &moniker, nullptr) == S_OK) {
            if (current_index == config_.deviceIndex) {
                hr = moniker->BindToObject(nullptr, nullptr, IID_IBaseFilter, (void**)&source_filter_);
                if (SUCCEEDED(hr)) {
                    hr = graph_builder_->AddFilter(source_filter_, L"Video Capture");
                    if (SUCCEEDED(hr)) {
                        ConfigureCameraFormat();
                        found = true;
                    }
                }
                moniker->Release();
                break;
            }
            moniker->Release();
            current_index++;
        }

        enum_moniker->Release();
        dev_enum->Release();
        return found;
    }

    bool CreateSampleGrabber() {
        HRESULT hr = CoCreateInstance(CLSID_SampleGrabber, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&sample_grabber_filter_);
        if (FAILED(hr)) return false;

        hr = graph_builder_->AddFilter(sample_grabber_filter_, L"Sample Grabber");
        if (FAILED(hr)) return false;

        hr = sample_grabber_filter_->QueryInterface(IID_ISampleGrabber, (void**)&sample_grabber_);
        if (FAILED(hr)) return false;

        opencv_callback_->setSampleGrabber(sample_grabber_);

        AM_MEDIA_TYPE mt;
        ZeroMemory(&mt, sizeof(mt));
        mt.majortype = MEDIATYPE_Video;
        mt.subtype = MEDIASUBTYPE_RGB24;
        hr = sample_grabber_->SetMediaType(&mt);
        if (FAILED(hr)) return false;

        hr = sample_grabber_->SetCallback(opencv_callback_.get(), 1);
        if (FAILED(hr)) return false;

        hr = sample_grabber_->SetBufferSamples(FALSE);
        if (FAILED(hr)) return false;

        return true;
    }

    bool ConnectFilters() {
        HRESULT hr;

        // Connect Source -> Sample Grabber -> Null Renderer
        IBaseFilter* null_renderer = nullptr;
        hr = CoCreateInstance(CLSID_NullRenderer, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&null_renderer);
        if (FAILED(hr)) return false;

        hr = graph_builder_->AddFilter(null_renderer, L"Null Renderer");
        if (FAILED(hr)) {
            null_renderer->Release();
            return false;
        }

        hr = capture_builder_->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, source_filter_, sample_grabber_filter_, null_renderer);
        if (FAILED(hr)) {
            hr = capture_builder_->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, source_filter_, sample_grabber_filter_, null_renderer);
        }

        null_renderer->Release();
        return SUCCEEDED(hr);
    }

    void ConfigureCameraFormat() {
        IPin* output_pin = nullptr;
        if (GetPin(source_filter_, PINDIR_OUTPUT, 0, &output_pin)) {
            IAMStreamConfig* stream_config = nullptr;
            HRESULT hr = output_pin->QueryInterface(IID_IAMStreamConfig, (void**)&stream_config);
            if (SUCCEEDED(hr)) {
                int count, size;
                hr = stream_config->GetNumberOfCapabilities(&count, &size);
                if (SUCCEEDED(hr)) {
                    for (int i = 0; i < count; i++) {
                        AM_MEDIA_TYPE* mt;
                        VIDEO_STREAM_CONFIG_CAPS caps;
                        hr = stream_config->GetStreamCaps(i, &mt, (BYTE*)&caps);
                        if (SUCCEEDED(hr)) {
                            if (mt->majortype == MEDIATYPE_Video) {
                                VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt->pbFormat;
                                vih->bmiHeader.biWidth = config_.width;
                                vih->bmiHeader.biHeight = config_.height;
                                vih->AvgTimePerFrame = 10000000LL / config_.fps;
                                hr = stream_config->SetFormat(mt);
                                if (SUCCEEDED(hr)) {
                                    std::cout << "Camera " << config_.deviceIndex << " format set" << std::endl;
                                    DeleteMediaType(mt);
                                    break;
                                }
                            }
                            DeleteMediaType(mt);
                        }
                    }
                }
                stream_config->Release();
            }
            output_pin->Release();
        }
    }

    bool GetPin(IBaseFilter* filter, PIN_DIRECTION dir, int index, IPin** pin) {
        IEnumPins* enum_pins = nullptr;
        HRESULT hr = filter->EnumPins(&enum_pins);
        if (FAILED(hr)) return false;

        IPin* current_pin = nullptr;
        int current_index = 0;

        while (enum_pins->Next(1, &current_pin, nullptr) == S_OK) {
            PIN_DIRECTION pin_dir;
            hr = current_pin->QueryDirection(&pin_dir);
            if (SUCCEEDED(hr) && pin_dir == dir) {
                if (current_index == index) {
                    *pin = current_pin;
                    enum_pins->Release();
                    return true;
                }
                current_index++;
            }
            current_pin->Release();
        }

        enum_pins->Release();
        return false;
    }

    void DeleteMediaType(AM_MEDIA_TYPE* mt) {
        if (mt->cbFormat != 0) {
            CoTaskMemFree(mt->pbFormat);
            mt->cbFormat = 0;
            mt->pbFormat = nullptr;
        }
        if (mt->pUnk) {
            mt->pUnk->Release();
            mt->pUnk = nullptr;
        }
        CoTaskMemFree(mt);
    }

    void ReleaseInterfaces() {
        if (sample_grabber_) { sample_grabber_->Release(); sample_grabber_ = nullptr; }
        if (sample_grabber_filter_) { sample_grabber_filter_->Release(); sample_grabber_filter_ = nullptr; }
        if (source_filter_) { source_filter_->Release(); source_filter_ = nullptr; }
        if (media_event_) { media_event_->Release(); media_event_ = nullptr; }
        if (media_control_) { media_control_->Release(); media_control_ = nullptr; }
        if (capture_builder_) { capture_builder_->Release(); capture_builder_ = nullptr; }
        if (graph_builder_) { graph_builder_->Release(); graph_builder_ = nullptr; }
    }
};

#endif // DIRECTSHOW_OPENCV_CAMERA_H
