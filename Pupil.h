#ifndef PUPILALGOSIMPLE_PUPIL_H
#define PUPILALGOSIMPLE_PUPIL_H

/*
Copyright (c) 2018, Thiago Santini / University of Tübingen

        Permission is hereby granted, free of charge, to any person obtaining a copy of
this software, source code, and associated documentation files (the "Software")
to use, copy, and modify the Software for academic use, subject to the following
        conditions:

1) The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

2) Modifications to the source code should be made available under
free-for-academic-usage licenses.

For commercial use, please contact the authors.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
        WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 Modified 01.01.2010: Moritz Lode

*/

#include <opencv2/core/types.hpp>
#include <QMetaType>
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef CV_PI
#define CV_PI M_PI
#endif

#define NO_CONFIDENCE -1.0
#define SMALLER_THAN_NO_CONFIDENCE NO_CONFIDENCE-1.0


class PupilData : public cv::RotatedRect {

public:

    PupilData(const RotatedRect &outline, const float &confidence) :
            RotatedRect(outline), confidence(confidence), outline_confidence(NO_CONFIDENCE), eyelid(0), physicalDiameter(-1.0), undistortedDiameter(-1.0), algorithmName("") {
    }

    PupilData(const RotatedRect &outline, const float &confidence, const float &outline_confidence, const float &eyelid, const float &physicalDiameter, const float &undistortedDiameter) :
            RotatedRect(outline), confidence(confidence), outline_confidence(outline_confidence), eyelid(eyelid), physicalDiameter(physicalDiameter), undistortedDiameter(undistortedDiameter), algorithmName("") {
    }

    PupilData(const PupilData &other) :
            RotatedRect(other), confidence(other.confidence), outline_confidence(other.outline_confidence), eyelid(other.eyelid), physicalDiameter(other.physicalDiameter), undistortedDiameter(other.undistortedDiameter), algorithmName(other.algorithmName) {
    }

    PupilData(const RotatedRect &outline) :
            RotatedRect(outline), confidence(NO_CONFIDENCE), outline_confidence(NO_CONFIDENCE), eyelid(0), physicalDiameter(-1.0), undistortedDiameter(-1.0), algorithmName("") {
    }

    PupilData() {
        clear();
    }

    // Conversion constructor from original Pupil class


    ~PupilData() = default;

    float confidence;

    float outline_confidence;

    float eyelid;

    float physicalDiameter;
    float undistortedDiameter;

    std::string algorithmName;

    void clear() {
        angle = -1.0;
        center = { -1.0, -1.0 };
        size = { -1.0, -1.0 };
        confidence = NO_CONFIDENCE;
        outline_confidence = NO_CONFIDENCE;
        eyelid=0;
        physicalDiameter=-1.0;
        undistortedDiameter=-1.0;
        algorithmName="";
    }

    void resize(const float &xf, const float &yf) {
        center.x *= xf;
        center.y *= yf;
        size.width *= xf;
        size.height *= yf;
    }

    void resize(const float &f) {
        center *= f;
        size *= f;
    }

    void shift( cv::Point2f p ) {
        center += p;
    }

    std::vector<cv::Point2f> rectPoints() {
        cv::Point2f pointsArr[4];
        points(pointsArr);

        std::vector<cv::Point2f> v(std::begin(pointsArr), std::end(pointsArr));

        return v;
    }

    bool valid(const double &confidenceThreshold=SMALLER_THAN_NO_CONFIDENCE) const {
        return center.x >= 0 &&               // 修复：允许边缘瞳孔 (x=0)
               center.y >= 0 &&               // 修复：允许边缘瞳孔 (y=0)
               size.width > 0.1 &&            // 修复：允许小尺寸瞳孔
               size.height > 0.1 &&           // 修复：允许小尺寸瞳孔
               (confidence > confidenceThreshold || outline_confidence > confidenceThreshold);
    }

    bool hasOutline() const {
        return size.width > 0 && size.height > 0;
    }

    int width() const {
        return (int)size.width;
    }

    int height() const {
        return (int)size.height;
    }

    int majorAxis() const {
        // 【修复】确保无效尺寸时返回0而不是负值
        if (size.width <= 0 || size.height <= 0) return 0;
        return std::max<int>(size.width, size.height);
    }

    int minorAxis() const {
        // 【修复】确保无效尺寸时返回0而不是负值
        if (size.width <= 0 || size.height <= 0) return 0;
        return std::min<int>(size.width, size.height);
    }

    int diameter() const {
        return majorAxis();
    }

    float circumference() const {
        // 【修复】检查无效尺寸，返回0而不是-1，保持与其他函数一致
        if(size.width <= 0 || size.height <= 0) return 0.0f;

        float a = 0.5*majorAxis();
        float b = 0.5*minorAxis();
        if (a <= 0 || b <= 0) return 0.0f; // 额外保护
        return CV_PI * abs( 3*(a+b) - sqrt( 10*a*b + 3*( pow(a,2) + pow(b,2) ) ) );
    }
};

Q_DECLARE_METATYPE(PupilData);

#endif //PUPILALGOSIMPLE_PUPIL_H
