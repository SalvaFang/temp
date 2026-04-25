#include "ccorneal.h"
#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>
#include"util.h"
// 构造函数
CCorneal::CCorneal() {
    window_size = 251;  // 默认窗口大小
    array_len = 360;  // 角度数组长度
}

// 析构函数：释放资源
CCorneal::~CCorneal() {
    destroy_edge_point();  // 如果需要释放动态分配的资源
}

// 设置角度的sin和cos值
void CCorneal::SetSinCosArray() {
    for (int i = 0; i < 360; i++) {
        sin_array[i] = sin(i * PI / 180);
        cos_array[i] = cos(i * PI / 180);
    }
}

void CCorneal::remove_corneal_reflection(cv::Mat &image, cv::Mat &threshold_image, int sx, int sy, int biggest_crr,
                                          int& crx, int& cry, int& crr)
{
    int crar = -1;  // 角膜反射点的近似半径
    crx = cry = crar = -1;
    SetSinCosArray();

    locate_corneal_reflection(image, threshold_image, sx, sy, (int)(biggest_crr / 2.5), crx, cry, crar);
    crr = fit_circle_radius_to_corneal_reflection(image, crx, cry, crar, (int)(biggest_crr / 2.5));
    crr = (int)(2.5 * crr);
    interpolate_corneal_reflection(image, crx, cry, crr);
}

void CCorneal::locate_corneal_reflection(cv::Mat &image, cv::Mat &threshold_image, int sx, int sy, int biggest_crr, int &crx, int &cry, int &crar)
{
    if (window_size % 2 == 0) {
        std::cout << "Error! window_size should be odd!" << std::endl;
    }

    int r = (window_size - 1) / 2;
    int startx = std::max(sx - r, 0);
    int endx = std::min(sx + r, image.cols - 1);
    int starty = std::max(sy - r, 0);
    int endy = std::min(sy + r, image.rows - 1);

    // 设置ROI区域
    cv::Mat imageROI = image(cv::Rect(startx, starty, endx - startx + 1, endy - starty + 1));
    cv::Mat thresholdROI = threshold_image(cv::Rect(startx, starty, endx - startx + 1, endy - starty + 1));

    double min_value, max_value;
    cv::Point min_loc, max_loc;
    cv::minMaxLoc(imageROI, &min_value, &max_value, &min_loc, &max_loc);  // 获取最小值和最大值

    int threshold;
    int area, max_area, sum_area;
    std::vector<std::vector<cv::Point>> contours;
    cv::Mat storage = cv::Mat::zeros(1, 1, CV_32F);  // 创建空存储

    double* scores = new double[(int)max_value + 1];
    std::fill(scores, scores + (int)max_value + 1, 0);

    for (threshold = (int)max_value; threshold >= 1; threshold--) {
        // 对图像进行阈值化处理
        cv::threshold(imageROI, thresholdROI, threshold, 255, cv::THRESH_BINARY);

        // 查找轮廓
        cv::findContours(thresholdROI, contours, storage, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

        // 确保 contours 非空
        if (contours.empty()) {
            continue;
        }

        max_area = 0;
        sum_area = 0;
        std::vector<cv::Point> max_contour = contours[0];

        // 找到最大轮廓
        for (auto& contour : contours) {
            area = contour.size();
            sum_area += area;
            if (area > max_area) {
                max_area = area;
                max_contour = contour;
            }
        }

        // 根据面积得分
        if (sum_area - max_area > 0) {
            scores[threshold - 1] = max_area / (sum_area - max_area);
        } else {
            continue;
        }

        // 计算反射点位置
        if (scores[threshold - 1] - scores[threshold] < 0) {
            crar = static_cast<int>(std::sqrt(max_area / PI));
            int sum_x = 0, sum_y = 0;
            for (const auto& point : max_contour) {
                sum_x += point.x;
                sum_y += point.y;
            }
            crx = sum_x / max_contour.size();
            cry = sum_y / max_contour.size();
            break;
        }
    }

    delete[] scores;

    if (crar > biggest_crr) {
        crx = cry = -1;
        crar = -1;
    }

    if (crx != -1 && cry != -1) {
        crx += startx;
        cry += starty;
    }
}

int CCorneal::fit_circle_radius_to_corneal_reflection(cv::Mat &image, int crx, int cry, int crar, int biggest_crr)
{
    if (crx == -1 || cry == -1 || crar == -1)
        return -1;

    double* ratio = new double[(biggest_crr - crar + 1)];
    int r, r_delta = 1;
    int x, y, x2, y2;
    double sum, sum2;

    for (r = crar; r <= biggest_crr; r++) {
        sum = 0;
        sum2 = 0;
        for (int i = 0; i < array_len; i++) {
            x = (int)(crx + (r + r_delta) * cos_array[i]);
            y = (int)(cry + (r + r_delta) * sin_array[i]);
            x2 = (int)(crx + (r - r_delta) * cos_array[i]);
            y2 = (int)(cry + (r - r_delta) * sin_array[i]);
            if ((x >= 0 && y >= 0 && x < image.cols && y < image.rows) &&
                (x2 >= 0 && y2 >= 0 && x2 < image.cols && y2 < image.rows)) {
                sum += image.at<uchar>(y, x);
                sum2 += image.at<uchar>(y2, x2);
            }
        }
        ratio[r - crar] = sum / sum2;
        if (r - crar >= 2) {
            if (ratio[r - crar - 2] < ratio[r - crar - 1] && ratio[r - crar] < ratio[r - crar - 1]) {
                delete[] ratio;
                return r - 1;
            }
        }
    }

    delete[] ratio;
    return crar;
}

void CCorneal::interpolate_corneal_reflection(cv::Mat &image, int crx, int cry, int crr)
{
    if (crx == -1 || cry == -1 || crr == -1)
        return;

    if (crx - crr < 0 || crx + crr >= image.cols || cry - crr < 0 || cry + crr >= image.rows) {
        return;
    }

    int r, r2, x, y;
    std::vector<uchar> perimeter_pixel(array_len);
    int sum = 0;
    double avg;

    for (int i = 0; i < array_len; i++) {
        x = static_cast<int>(crx + crr * cos_array[i]);
        y = static_cast<int>(cry + crr * sin_array[i]);
        perimeter_pixel[i] = image.at<uchar>(y, x);
        sum += perimeter_pixel[i];
    }
    avg = sum * 1.0 / array_len;

    for (r = 1; r < crr; r++) {
        r2 = crr - r;
        for (int i = 0; i < array_len; i++) {
            x = static_cast<int>(crx + r * cos_array[i]);
            y = static_cast<int>(cry + r * sin_array[i]);
            image.at<uchar>(y, x) = static_cast<uchar>((r2 * 1.0 / crr) * avg + (r * 1.0 / crr) * perimeter_pixel[i]);
        }
    }
}

void CCorneal::destroy_edge_point()
{
    for (auto& edge : edge_point) {
        delete edge;
    }
    edge_point.clear();
}

stuDPoint CCorneal::get_edge_mean()
{
    stuDPoint edge_mean = {0, 0};
    int sumx = 0, sumy = 0;

    for (const auto& edge : edge_point) {
        sumx += edge->x;
        sumy += edge->y;
    }

    if (!edge_point.empty()) {
        edge_mean.x = sumx / edge_point.size();
        edge_mean.y = sumy / edge_point.size();
    }
    return edge_mean;
}
