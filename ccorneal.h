#ifndef CCORNEAL_H
#define CCORNEAL_H

#include <opencv2/opencv.hpp>
#include <vector>
#include"util.h"
// 常量定义
#define PI CV_PI


class CCorneal {
public:
    CCorneal();      // 构造函数声明
    ~CCorneal();     // 析构函数声明

    void SetSinCosArray();
    void remove_corneal_reflection(cv::Mat &image, cv::Mat &threshold_image, int sx, int sy, int biggest_crr,
                                   int& crx, int& cry, int& crr);
    void locate_corneal_reflection(cv::Mat &image, cv::Mat &threshold_image, int sx, int sy, int biggest_crr, int &crx, int &cry, int &crar);
    int fit_circle_radius_to_corneal_reflection(cv::Mat &image, int crx, int cry, int crar, int biggest_crr);
    void interpolate_corneal_reflection(cv::Mat &image, int crx, int cry, int crr);

private:
    // 私有成员变量
    double sin_array[360];
    double cos_array[360];
    int window_size;
    int array_len;
    double circle_param[5];

    // 存储边缘点的容器
    std::vector<stuDPoint*> edge_point;
    std::vector<double> edge_intensity_diff;

    // 辅助函数：销毁边缘点数据
    void destroy_edge_point();
    stuDPoint get_edge_mean();
};

#endif // CCORNEAL_H
