#include <initguid.h>
#include "opencvimagecallback.h"
#include "directshowopencvcamera.h"
#include <emmintrin.h>  // 确保包含SSE2的头文件


OpenCVImageCallback::OpenCVImageCallback(int camera_id, DirectShowOpenCVCamera* camera)
    : ref_count_(1), new_frame_available_(false), frame_count_(0),
      camera_id_(camera_id), camera_(camera) {


    std::cout << "time complexity: " << camera->config_.width<< " ms" << std::endl;

}

STDMETHODIMP OpenCVImageCallback::QueryInterface(REFIID riid, void** ppv) {
    if (riid == IID_IUnknown || riid == IID_ISampleGrabberCB) {
        *ppv = static_cast<ISampleGrabberCB*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) OpenCVImageCallback::AddRef() {
    return InterlockedIncrement(&ref_count_);
}

STDMETHODIMP_(ULONG) OpenCVImageCallback::Release() {
    ULONG count = InterlockedDecrement(&ref_count_);
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP OpenCVImageCallback::SampleCB(double SampleTime, IMediaSample* pSample) {
    return S_OK;
}


STDMETHODIMP OpenCVImageCallback::BufferCB(double SampleTime, BYTE* pBuffer, long BufferLen) {
    if (!pBuffer || BufferLen <= 0) return S_OK;

    try {
        int width = camera_->config_.width;
        int height = camera_->config_.height;

        int expected_size = width * height * 3;

        if (BufferLen >= expected_size) {
                std::lock_guard<std::mutex> lock(frame_mutex_);
                // 直接将RGB数据包装成Mat，不进行任何转换
                current_frame_ = cv::Mat(height, width, CV_8UC3, pBuffer);
                new_frame_available_ = true;
                frame_count_.fetch_add(1);
            if (camera_) {
                emit camera_->newFrameAvailable();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "OpenCV处理错误: " << e.what() << std::endl;
    }
    return S_OK;
}

bool OpenCVImageCallback::GetLatestFrame(cv::Mat& frame) {
    if (!new_frame_available_) return false;

    std::lock_guard<std::mutex> lock(frame_mutex_);
    if (!current_frame_.empty()) {
        frame = current_frame_.clone();
        new_frame_available_ = false;
        return true;
    }
    return false;
}
