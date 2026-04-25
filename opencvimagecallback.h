#ifndef OPENCVIMAGECALLBACK_H
#define OPENCVIMAGECALLBACK_H

#include <windows.h>
#include <dshow.h>
#include <comdef.h>
#include <iostream>
#include <atomic>
#include <mutex>
#include <opencv2/opencv.hpp>

// 前向声明
class DirectShowOpenCVCamera;

// 定义DirectShow GUID
DEFINE_GUID(CLSID_SampleGrabber, 0xc1f400a0, 0x3f08, 0x11d3, 0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37);
DEFINE_GUID(IID_ISampleGrabber, 0x6b652fff, 0x11fe, 0x4fce, 0x92, 0xad, 0x02, 0x66, 0xb5, 0xd7, 0xc7, 0x8f);
DEFINE_GUID(CLSID_NullRenderer, 0xC1F400A4, 0x3F08, 0x11D3, 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37);
DEFINE_GUID(IID_ISampleGrabberCB, 0x0579154A, 0x2B53, 0x4994, 0xB0, 0xD0, 0xE7, 0x73, 0x14, 0x8E, 0xFF, 0x85);

interface ISampleGrabberCB : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE SampleCB(double SampleTime, IMediaSample* pSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE BufferCB(double SampleTime, BYTE* pBuffer, long BufferLen) = 0;
};

interface ISampleGrabber : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE SetOneShot(BOOL OneShot) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMediaType(const AM_MEDIA_TYPE* pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(AM_MEDIA_TYPE* pType) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL BufferThem) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(long* pBufferSize, long* pBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(IMediaSample** ppSample) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCallback(ISampleGrabberCB* pCallback, long WhichMethodToCallback) = 0;
};

class OpenCVImageCallback : public ISampleGrabberCB {
private:
    ULONG ref_count_;
    cv::Mat current_frame_;
    std::mutex frame_mutex_;
    std::atomic<bool> new_frame_available_;
    std::atomic<int> frame_count_;
    int camera_id_;
    DirectShowOpenCVCamera* camera_;


public:
    OpenCVImageCallback(int camera_id, DirectShowOpenCVCamera* camera);

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP SampleCB(double SampleTime, IMediaSample* pSample);
    STDMETHODIMP BufferCB(double SampleTime, BYTE* pBuffer, long BufferLen);

    bool GetLatestFrame(cv::Mat& frame);
};

#endif // OPENCVIMAGECALLBACK_H
