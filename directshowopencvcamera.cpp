#include "directshowopencvcamera.h"
#include "opencvimagecallback.h"

DirectShowOpenCVCamera::DirectShowOpenCVCamera(int deviceIndex, const CameraConfig& config)
    : QObject(nullptr), graph_builder_(nullptr), capture_builder_(nullptr),
      media_control_(nullptr), media_event_(nullptr), source_filter_(nullptr),
      video_renderer_(nullptr), sample_grabber_filter_(nullptr), sample_grabber_(nullptr),
      config_(config), status_(CameraStatus::UNINITIALIZED) {
    config_.deviceIndex = deviceIndex;
    opencv_callback_ = std::make_unique<OpenCVImageCallback>(deviceIndex, this);
}

DirectShowOpenCVCamera::~DirectShowOpenCVCamera() {
    Stop();
    ReleaseInterfaces();
}

bool DirectShowOpenCVCamera::Initialize() {
    status_ = CameraStatus::INITIALIZING;

    HRESULT hr;

    hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                          IID_IGraphBuilder, (void**)&graph_builder_);
    if (FAILED(hr)) return false;

    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER,
                          IID_ICaptureGraphBuilder2, (void**)&capture_builder_);
    if (FAILED(hr)) return false;

    hr = capture_builder_->SetFiltergraph(graph_builder_);
    if (FAILED(hr)) return false;

    hr = graph_builder_->QueryInterface(IID_IMediaControl, (void**)&media_control_);
    if (FAILED(hr)) return false;

    hr = graph_builder_->QueryInterface(IID_IMediaEvent, (void**)&media_event_);
    if (FAILED(hr)) return false;

    if (!CreateVideoSource()) return false;
    if (!CreateSampleGrabber()) return false;
    if (!CreateVideoRenderer()) return false;
    if (!ConnectFilters()) return false;

    status_ = CameraStatus::STOPPED;
    std::cout << "Camera " << config_.deviceIndex << " initialized" << std::endl;
    return true;
}

bool DirectShowOpenCVCamera::Start() {
    if (status_ != CameraStatus::STOPPED) return false;

    HRESULT hr = media_control_->Run();
    if (FAILED(hr)) return false;

    status_ = CameraStatus::RUNNING;
    std::cout << "Camera " << config_.deviceIndex << " started" << std::endl;
    return true;
}

bool DirectShowOpenCVCamera::Stop() {
    if (status_ == CameraStatus::RUNNING && media_control_) {
        media_control_->Stop();
    }
    status_ = CameraStatus::STOPPED;
    return true;
}

bool DirectShowOpenCVCamera::CreateVideoSource() {
    ICreateDevEnum* dev_enum = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum, (void**)&dev_enum);
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

bool DirectShowOpenCVCamera::CreateSampleGrabber() {
    HRESULT hr = CoCreateInstance(CLSID_SampleGrabber, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_IBaseFilter, (void**)&sample_grabber_filter_);
    if (FAILED(hr)) return false;

    hr = graph_builder_->AddFilter(sample_grabber_filter_, L"Sample Grabber");
    if (FAILED(hr)) return false;

    hr = sample_grabber_filter_->QueryInterface(IID_ISampleGrabber, (void**)&sample_grabber_);
    if (FAILED(hr)) return false;

    AM_MEDIA_TYPE mt;
    ZeroMemory(&mt, sizeof(mt));
    mt.majortype = MEDIATYPE_Video;
    mt.subtype = MEDIASUBTYPE_RGB24;
    hr = sample_grabber_->SetMediaType(&mt);
    if (FAILED(hr)) return false;

    hr = sample_grabber_->SetCallback(opencv_callback_.get(), 1); // BufferCB
    if (FAILED(hr)) return false;

    hr = sample_grabber_->SetBufferSamples(FALSE);
    if (FAILED(hr)) return false;

    return true;
}

bool DirectShowOpenCVCamera::CreateVideoRenderer() {
    HRESULT hr = CoCreateInstance(CLSID_NullRenderer, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_IBaseFilter, (void**)&video_renderer_);
    if (FAILED(hr)) {
        std::cerr << "Failed to create Null Renderer" << std::endl;
        return false;
    }
    hr = graph_builder_->AddFilter(video_renderer_, L"Null Renderer");
    if (FAILED(hr)) return false;

    return true;
}

bool DirectShowOpenCVCamera::ConnectFilters() {
    HRESULT hr;

    hr = capture_builder_->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
                                        source_filter_, sample_grabber_filter_, video_renderer_);
    if (FAILED(hr)) {
        hr = capture_builder_->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
                                            source_filter_, sample_grabber_filter_, video_renderer_);
    }
    return SUCCEEDED(hr);
}

void DirectShowOpenCVCamera::ConfigureCameraFormat() {
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
                                std::cout << "Camera " << config_.deviceIndex
                                          << " format set: " << config_.width << "x" << config_.height
                                          << " @ " << config_.fps << "fps" << std::endl;
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

bool DirectShowOpenCVCamera::GetPin(IBaseFilter* filter, PIN_DIRECTION dir, int index, IPin** pin) {
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

void DirectShowOpenCVCamera::DeleteMediaType(AM_MEDIA_TYPE* mt) {
    if (mt->cbFormat != 0) {
        CoTaskMemFree((PVOID)mt->pbFormat);
        mt->cbFormat = 0;
        mt->pbFormat = nullptr;
    }
    if (mt->pUnk != nullptr) {
        mt->pUnk->Release();
        mt->pUnk = nullptr;
    }
    CoTaskMemFree(mt);
}

void DirectShowOpenCVCamera::ReleaseInterfaces() {
    if (sample_grabber_) { sample_grabber_->Release(); sample_grabber_ = nullptr; }
    if (sample_grabber_filter_) { sample_grabber_filter_->Release(); sample_grabber_filter_ = nullptr; }
    if (video_renderer_) { video_renderer_->Release(); video_renderer_ = nullptr; }
    if (source_filter_) { source_filter_->Release(); source_filter_ = nullptr; }
    if (media_event_) { media_event_->Release(); media_event_ = nullptr; }
    if (media_control_) { media_control_->Release(); media_control_ = nullptr; }
    if (capture_builder_) { capture_builder_->Release(); capture_builder_ = nullptr; }
    if (graph_builder_) { graph_builder_->Release(); graph_builder_ = nullptr; }
}
