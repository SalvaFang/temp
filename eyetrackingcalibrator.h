#ifndef EYETRACKINGCALIBRATOR_H
#define EYETRACKINGCALIBRATOR_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <QDebug>

class EyeTrackingCalibrator {
public:
    enum CalibrationModel {
        AFFINE_6_PARAM,        // 6参数仿射变换（原始方法）
        POLYNOMIAL_2ND_ORDER,  // 12参数2次多项式
        POLYNOMIAL_3RD_ORDER,  // 20参数3次多项式
        PUPIL_CORNEAL_DUAL,    // 瞳孔-角膜双特征融合模型
        CORNEAL_VECTOR         // 角膜矢量模型
    };

private:
    std::vector<cv::Point2f> m_calibrationPoints;    // 标定点坐标
    std::vector<cv::Point2f> m_pupilPositions;       // 对应的瞳孔位置
    std::vector<cv::Point2f> m_glint1Positions;      // 对应的第一个亮斑位置
    std::vector<cv::Point2f> m_glint2Positions;      // 对应的第二个亮斑位置
    cv::Mat m_transformMatrix;                       // 变换矩阵
    cv::Mat m_pupilTransformMatrix;                  // 瞳孔变换矩阵（双特征模型）
    cv::Mat m_cornealTransformMatrix;                // 角膜变换矩阵（双特征模型）
    CalibrationModel m_model;                        // 标定模型
    bool m_isCalibrated;

    // 用于数据验证
    std::vector<float> m_calibrationErrors;          // 每个标定点的误差
    float m_meanError;                               // 平均误差
    float m_maxError;                                // 最大误差

public:
    EyeTrackingCalibrator(CalibrationModel model = POLYNOMIAL_2ND_ORDER);

    // 添加标定数据
    void addCalibrationData(const cv::Point2f& screenPoint, const cv::Point2f& pupilPos);
    void addCalibrationData(const cv::Point2f& screenPoint, const cv::Point2f& pupilPos,
                           const cv::Point2f& glint1Pos, const cv::Point2f& glint2Pos);

    // 执行标定计算
    bool calibrate();

    // 应用标定变换
    cv::Point2f transformToScreen(const cv::Point2f& pupilPos) const;
    cv::Point2f transformToScreen(const cv::Point2f& pupilPos, const cv::Point2f& glint1Pos,
                                 const cv::Point2f& glint2Pos) const;

    // 验证标定质量
    bool validateCalibration(float maxAllowedError = 50.0f);

    // 获取标定统计信息
    float getMeanError() const { return m_meanError; }
    float getMaxError() const { return m_maxError; }
    std::vector<float> getAllErrors() const { return m_calibrationErrors; }

    // 保存和加载标定参数
    bool saveCalibration(const QString& filename) const;
    bool loadCalibration(const QString& filename);

    // 清空标定数据
    void clear();

private:
    // 不同模型的标定实现
    bool calibrateAffine();
    bool calibratePolynomial2nd();
    bool calibratePolynomial3rd();
    bool calibratePupilCornealDual();
    bool calibrateCornealVector();

    // 构建多项式特征矩阵
    cv::Mat buildPolynomialMatrix(const std::vector<cv::Point2f>& points, int order);

    // 应用多项式变换
    cv::Point2f applyPolynomialTransform(const cv::Point2f& point, const cv::Mat& coeffs, int order) const;

    // 计算标定误差
    void calculateErrors();

    // 瞳孔-角膜融合相关函数
    cv::Point2f calculatePupilCornealCenter(const cv::Point2f& pupilPos,
                                           const cv::Point2f& glint1Pos,
                                           const cv::Point2f& glint2Pos) const;
    cv::Point2f calculateCornealVector(const cv::Point2f& pupilPos,
                                      const cv::Point2f& glint1Pos,
                                      const cv::Point2f& glint2Pos) const;
    float calculateGlintStability(const std::vector<cv::Point2f>& glint1Positions,
                                 const std::vector<cv::Point2f>& glint2Positions) const;
};

#endif // EYETRACKINGCALIBRATOR_H
