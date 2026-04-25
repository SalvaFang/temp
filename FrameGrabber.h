#ifndef FRAMEGRABBER_H
#define FRAMEGRABBER_H

#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <QElapsedTimer>
#include <QThread>
#include <QTimer>
#include <QDebug>

#include <opencv/cv.hpp>

#ifdef TURBOJPEG
#include "turbojpeg.h"
#endif

#include "utils.h"

class FrameGrabber : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    explicit FrameGrabber(QString id, int code, QObject *parent = 0);
    ~FrameGrabber();
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const;
    cv::Mat getCurrentFrame() const; // 新增方法，用于获取当前帧
signals:
    void newFrame(Timestamp t, cv::Mat frame);
    //add by zl
    //void newFrameEx(const cv::Mat& frame);
    void timedout();

public slots:
    bool present(const QVideoFrame &frame) override;
    void setColorCode(int code);

private:
    mutable QMutex m_mutex; // 用于保护 m_currentFram
    QTimer *watchdog;
    int timeoutMs;
	Timestamp timestampOffset;
	std::vector<Timestamp> timestampOffsetEstimators;
#ifdef TURBOJPEG
    tjhandle tjh;
#endif
    QString id;
    int code;
    unsigned char* yuvBuffer;
    long unsigned int yuvBufferSize;
    cv::Mat m_currentFrame; // 新增成员变量，用于存储当前帧
    bool jpeg2bmp(const QVideoFrame &in, cv::Mat &cvFrame);
    bool rgb32_2bmp(const QVideoFrame &in, cv::Mat &cvFrame);
    bool yuyv_2bmp(const QVideoFrame &in, cv::Mat &cvFrame);

    unsigned int pmIdx;
};

#endif // FRAMEGRABBER_H
