#include "eyetrackingcalibrator.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

EyeTrackingCalibrator::EyeTrackingCalibrator(CalibrationModel model)
    : m_model(model), m_isCalibrated(false), m_meanError(0.0f), m_maxError(0.0f)
{
}

void EyeTrackingCalibrator::addCalibrationData(const cv::Point2f& screenPoint, const cv::Point2f& pupilPos)
{
    m_calibrationPoints.push_back(screenPoint);
    m_pupilPositions.push_back(pupilPos);
    m_glint1Positions.push_back(cv::Point2f(-1, -1)); // 无效亮斑标记
    m_glint2Positions.push_back(cv::Point2f(-1, -1));
    m_isCalibrated = false;  // 添加新数据后需要重新标定
}

void EyeTrackingCalibrator::addCalibrationData(const cv::Point2f& screenPoint, const cv::Point2f& pupilPos,
                                              const cv::Point2f& glint1Pos, const cv::Point2f& glint2Pos)
{
    m_calibrationPoints.push_back(screenPoint);
    m_pupilPositions.push_back(pupilPos);
    m_glint1Positions.push_back(glint1Pos);
    m_glint2Positions.push_back(glint2Pos);
    m_isCalibrated = false;  // 添加新数据后需要重新标定
}

bool EyeTrackingCalibrator::calibrate()
{
    if (m_calibrationPoints.size() < 6) {
        qDebug() << "标定点数量不足，至少需要6个点";
        return false;
    }

    if (m_calibrationPoints.size() != m_pupilPositions.size()) {
        qDebug() << "标定点和瞳孔位置数量不匹配";
        return false;
    }

    if (m_calibrationPoints.size() != m_glint1Positions.size() ||
        m_calibrationPoints.size() != m_glint2Positions.size()) {
        qDebug() << "标定点和亮斑位置数量不匹配";
        return false;
    }

    bool success = false;
    switch (m_model) {
    case AFFINE_6_PARAM:
        success = calibrateAffine();
        break;
    case POLYNOMIAL_2ND_ORDER:
        success = calibratePolynomial2nd();
        break;
    case POLYNOMIAL_3RD_ORDER:
        success = calibratePolynomial3rd();
        break;
    case PUPIL_CORNEAL_DUAL:
        success = calibratePupilCornealDual();
        break;
    case CORNEAL_VECTOR:
        success = calibrateCornealVector();
        break;
    }

    if (success) {
        calculateErrors();
        m_isCalibrated = true;
        qDebug() << "标定完成，平均误差:" << m_meanError << "像素";
    }

    return success;
}

bool EyeTrackingCalibrator::calibrateAffine()
{
    // 6参数仿射变换：x' = a1*x + a2*y + a3, y' = a4*x + a5*y + a6
    int n = m_calibrationPoints.size();
    cv::Mat A(2 * n, 6, CV_32F);
    cv::Mat b(2 * n, 1, CV_32F);

    for (int i = 0; i < n; i++) {
        float x = m_pupilPositions[i].x;
        float y = m_pupilPositions[i].y;

        // X方程
        A.at<float>(2*i, 0) = x;
        A.at<float>(2*i, 1) = y;
        A.at<float>(2*i, 2) = 1;
        A.at<float>(2*i, 3) = 0;
        A.at<float>(2*i, 4) = 0;
        A.at<float>(2*i, 5) = 0;
        b.at<float>(2*i) = m_calibrationPoints[i].x;

        // Y方程
        A.at<float>(2*i+1, 0) = 0;
        A.at<float>(2*i+1, 1) = 0;
        A.at<float>(2*i+1, 2) = 0;
        A.at<float>(2*i+1, 3) = x;
        A.at<float>(2*i+1, 4) = y;
        A.at<float>(2*i+1, 5) = 1;
        b.at<float>(2*i+1) = m_calibrationPoints[i].y;
    }

    bool success = cv::solve(A, b, m_transformMatrix, cv::DECOMP_SVD);
    return success && !m_transformMatrix.empty();
}

bool EyeTrackingCalibrator::calibratePolynomial2nd()
{
    // 12参数2次多项式：
    // x' = a1 + a2*x + a3*y + a4*x*y + a5*x² + a6*y²
    // y' = b1 + b2*x + b3*y + b4*x*y + b5*x² + b6*y²

    int n = m_calibrationPoints.size();
    cv::Mat A(2 * n, 12, CV_32F);
    cv::Mat b(2 * n, 1, CV_32F);

    for (int i = 0; i < n; i++) {
        float x = m_pupilPositions[i].x;
        float y = m_pupilPositions[i].y;
        float x2 = x * x;
        float y2 = y * y;
        float xy = x * y;

        // X方程系数
        A.at<float>(2*i, 0) = 1;    // a1
        A.at<float>(2*i, 1) = x;    // a2*x
        A.at<float>(2*i, 2) = y;    // a3*y
        A.at<float>(2*i, 3) = xy;   // a4*x*y
        A.at<float>(2*i, 4) = x2;   // a5*x²
        A.at<float>(2*i, 5) = y2;   // a6*y²
        A.at<float>(2*i, 6) = 0;    // b1
        A.at<float>(2*i, 7) = 0;    // b2*x
        A.at<float>(2*i, 8) = 0;    // b3*y
        A.at<float>(2*i, 9) = 0;    // b4*x*y
        A.at<float>(2*i, 10) = 0;   // b5*x²
        A.at<float>(2*i, 11) = 0;   // b6*y²
        b.at<float>(2*i) = m_calibrationPoints[i].x;

        // Y方程系数
        A.at<float>(2*i+1, 0) = 0;   // a1
        A.at<float>(2*i+1, 1) = 0;   // a2*x
        A.at<float>(2*i+1, 2) = 0;   // a3*y
        A.at<float>(2*i+1, 3) = 0;   // a4*x*y
        A.at<float>(2*i+1, 4) = 0;   // a5*x²
        A.at<float>(2*i+1, 5) = 0;   // a6*y²
        A.at<float>(2*i+1, 6) = 1;   // b1
        A.at<float>(2*i+1, 7) = x;   // b2*x
        A.at<float>(2*i+1, 8) = y;   // b3*y
        A.at<float>(2*i+1, 9) = xy;  // b4*x*y
        A.at<float>(2*i+1, 10) = x2; // b5*x²
        A.at<float>(2*i+1, 11) = y2; // b6*y²
        b.at<float>(2*i+1) = m_calibrationPoints[i].y;
    }

    bool success = cv::solve(A, b, m_transformMatrix, cv::DECOMP_SVD);
    return success && !m_transformMatrix.empty();
}

bool EyeTrackingCalibrator::calibratePolynomial3rd()
{
    // 20参数3次多项式：包含x³, y³, x²y, xy²等项
    int n = m_calibrationPoints.size();
    if (n < 10) {
        qDebug() << "3次多项式标定至少需要10个点";
        return false;
    }

    cv::Mat A(2 * n, 20, CV_32F);
    cv::Mat b(2 * n, 1, CV_32F);

    for (int i = 0; i < n; i++) {
        float x = m_pupilPositions[i].x;
        float y = m_pupilPositions[i].y;
        float x2 = x * x;
        float y2 = y * y;
        float x3 = x2 * x;
        float y3 = y2 * y;
        float xy = x * y;
        float x2y = x2 * y;
        float xy2 = x * y2;

        // X方程系数 (10个参数)
        A.at<float>(2*i, 0) = 1;     // 常数项
        A.at<float>(2*i, 1) = x;     // x
        A.at<float>(2*i, 2) = y;     // y
        A.at<float>(2*i, 3) = xy;    // xy
        A.at<float>(2*i, 4) = x2;    // x²
        A.at<float>(2*i, 5) = y2;    // y²
        A.at<float>(2*i, 6) = x3;    // x³
        A.at<float>(2*i, 7) = y3;    // y³
        A.at<float>(2*i, 8) = x2y;   // x²y
        A.at<float>(2*i, 9) = xy2;   // xy²
        // Y方程系数置0
        for (int j = 10; j < 20; j++) {
            A.at<float>(2*i, j) = 0;
        }
        b.at<float>(2*i) = m_calibrationPoints[i].x;

        // Y方程系数
        for (int j = 0; j < 10; j++) {
            A.at<float>(2*i+1, j) = 0;
        }
        A.at<float>(2*i+1, 10) = 1;    // 常数项
        A.at<float>(2*i+1, 11) = x;    // x
        A.at<float>(2*i+1, 12) = y;    // y
        A.at<float>(2*i+1, 13) = xy;   // xy
        A.at<float>(2*i+1, 14) = x2;   // x²
        A.at<float>(2*i+1, 15) = y2;   // y²
        A.at<float>(2*i+1, 16) = x3;   // x³
        A.at<float>(2*i+1, 17) = y3;   // y³
        A.at<float>(2*i+1, 18) = x2y;  // x²y
        A.at<float>(2*i+1, 19) = xy2;  // xy²
        b.at<float>(2*i+1) = m_calibrationPoints[i].y;
    }

    bool success = cv::solve(A, b, m_transformMatrix, cv::DECOMP_SVD);
    return success && !m_transformMatrix.empty();
}

cv::Point2f EyeTrackingCalibrator::transformToScreen(const cv::Point2f& pupilPos) const
{
    if (!m_isCalibrated) {
        qDebug() << "未完成标定，无法进行坐标变换";
        return cv::Point2f(0, 0);
    }

    switch (m_model) {
    case AFFINE_6_PARAM:
        return applyPolynomialTransform(pupilPos, m_transformMatrix, 1);
    case POLYNOMIAL_2ND_ORDER:
        return applyPolynomialTransform(pupilPos, m_transformMatrix, 2);
    case POLYNOMIAL_3RD_ORDER:
        return applyPolynomialTransform(pupilPos, m_transformMatrix, 3);
    case PUPIL_CORNEAL_DUAL:
    case CORNEAL_VECTOR:
        // 这些模型需要亮斑信息，使用默认值
        qDebug() << "警告: 瞳孔-角膜模型需要亮斑信息";
        return applyPolynomialTransform(pupilPos, m_pupilTransformMatrix, 2);
    }

    return cv::Point2f(0, 0);
}

cv::Point2f EyeTrackingCalibrator::transformToScreen(const cv::Point2f& pupilPos, const cv::Point2f& glint1Pos,
                                                    const cv::Point2f& glint2Pos) const
{
    if (!m_isCalibrated) {
        qDebug() << "未完成标定，无法进行坐标变换";
        return cv::Point2f(0, 0);
    }

    switch (m_model) {
    case PUPIL_CORNEAL_DUAL:
    {
        // 双特征融合：同时使用瞳孔和角膜信息
        cv::Point2f pupilResult = applyPolynomialTransform(pupilPos, m_pupilTransformMatrix, 2);
        cv::Point2f cornealCenter = calculatePupilCornealCenter(pupilPos, glint1Pos, glint2Pos);
        cv::Point2f cornealResult = applyPolynomialTransform(cornealCenter, m_cornealTransformMatrix, 2);

        // 权重融合 (可根据亮斑稳定性调整权重)
        float pupilWeight = 0.6f;
        float cornealWeight = 0.4f;

        return cv::Point2f(pupilWeight * pupilResult.x + cornealWeight * cornealResult.x,
                          pupilWeight * pupilResult.y + cornealWeight * cornealResult.y);
    }
    case CORNEAL_VECTOR:
    {
        // 角膜矢量模型：使用瞳孔到亮斑的矢量信息
        cv::Point2f cornealVector = calculateCornealVector(pupilPos, glint1Pos, glint2Pos);
        return applyPolynomialTransform(cornealVector, m_transformMatrix, 2);
    }
    default:
        // 其他模型忽略亮斑信息，只使用瞳孔
        return transformToScreen(pupilPos);
    }
}

cv::Point2f EyeTrackingCalibrator::applyPolynomialTransform(const cv::Point2f& point, const cv::Mat& coeffs, int order) const
{
    float x = point.x;
    float y = point.y;

    if (order == 1) {  // 仿射变换
        float screen_x = coeffs.at<float>(0) * x + coeffs.at<float>(1) * y + coeffs.at<float>(2);
        float screen_y = coeffs.at<float>(3) * x + coeffs.at<float>(4) * y + coeffs.at<float>(5);
        return cv::Point2f(screen_x, screen_y);
    }
    else if (order == 2) {  // 2次多项式
        float x2 = x * x;
        float y2 = y * y;
        float xy = x * y;

        float screen_x = coeffs.at<float>(0) +          // 常数项
                        coeffs.at<float>(1) * x +       // x项
                        coeffs.at<float>(2) * y +       // y项
                        coeffs.at<float>(3) * xy +      // xy项
                        coeffs.at<float>(4) * x2 +      // x²项
                        coeffs.at<float>(5) * y2;       // y²项

        float screen_y = coeffs.at<float>(6) +          // 常数项
                        coeffs.at<float>(7) * x +       // x项
                        coeffs.at<float>(8) * y +       // y项
                        coeffs.at<float>(9) * xy +      // xy项
                        coeffs.at<float>(10) * x2 +     // x²项
                        coeffs.at<float>(11) * y2;      // y²项

        return cv::Point2f(screen_x, screen_y);
    }
    else if (order == 3) {  // 3次多项式
        float x2 = x * x;
        float y2 = y * y;
        float x3 = x2 * x;
        float y3 = y2 * y;
        float xy = x * y;
        float x2y = x2 * y;
        float xy2 = x * y2;

        float screen_x = coeffs.at<float>(0) +          // 常数项
                        coeffs.at<float>(1) * x +       // x
                        coeffs.at<float>(2) * y +       // y
                        coeffs.at<float>(3) * xy +      // xy
                        coeffs.at<float>(4) * x2 +      // x²
                        coeffs.at<float>(5) * y2 +      // y²
                        coeffs.at<float>(6) * x3 +      // x³
                        coeffs.at<float>(7) * y3 +      // y³
                        coeffs.at<float>(8) * x2y +     // x²y
                        coeffs.at<float>(9) * xy2;      // xy²

        float screen_y = coeffs.at<float>(10) +         // 常数项
                        coeffs.at<float>(11) * x +      // x
                        coeffs.at<float>(12) * y +      // y
                        coeffs.at<float>(13) * xy +     // xy
                        coeffs.at<float>(14) * x2 +     // x²
                        coeffs.at<float>(15) * y2 +     // y²
                        coeffs.at<float>(16) * x3 +     // x³
                        coeffs.at<float>(17) * y3 +     // y³
                        coeffs.at<float>(18) * x2y +    // x²y
                        coeffs.at<float>(19) * xy2;     // xy²

        return cv::Point2f(screen_x, screen_y);
    }

    return cv::Point2f(0, 0);
}

void EyeTrackingCalibrator::calculateErrors()
{
    m_calibrationErrors.clear();
    float totalError = 0.0f;
    m_maxError = 0.0f;

    for (size_t i = 0; i < m_calibrationPoints.size(); i++) {
        cv::Point2f predicted = transformToScreen(m_pupilPositions[i]);
        cv::Point2f actual = m_calibrationPoints[i];

        float error = cv::norm(predicted - actual);
        m_calibrationErrors.push_back(error);
        totalError += error;

        if (error > m_maxError) {
            m_maxError = error;
        }
    }

    m_meanError = totalError / m_calibrationErrors.size();
}

bool EyeTrackingCalibrator::validateCalibration(float maxAllowedError)
{
    if (!m_isCalibrated) return false;

    return m_meanError <= maxAllowedError && m_maxError <= (maxAllowedError * 2.0f);
}

void EyeTrackingCalibrator::clear()
{
    m_calibrationPoints.clear();
    m_pupilPositions.clear();
    m_glint1Positions.clear();
    m_glint2Positions.clear();
    m_calibrationErrors.clear();
    m_transformMatrix.release();
    m_pupilTransformMatrix.release();
    m_cornealTransformMatrix.release();
    m_isCalibrated = false;
    m_meanError = 0.0f;
    m_maxError = 0.0f;
}

bool EyeTrackingCalibrator::saveCalibration(const QString& filename) const
{
    if (!m_isCalibrated) return false;

    QJsonObject calibData;
    calibData["model"] = (int)m_model;
    calibData["mean_error"] = m_meanError;
    calibData["max_error"] = m_maxError;

    // 保存变换矩阵
    QJsonArray matrixArray;
    for (int i = 0; i < m_transformMatrix.rows; i++) {
        matrixArray.append(m_transformMatrix.at<float>(i));
    }
    calibData["transform_matrix"] = matrixArray;

    QJsonDocument doc(calibData);
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        return true;
    }
    return false;
}

bool EyeTrackingCalibrator::loadCalibration(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject calibData = doc.object();
    m_model = (CalibrationModel)calibData["model"].toInt();
    m_meanError = calibData["mean_error"].toDouble();
    m_maxError = calibData["max_error"].toDouble();

    // 加载变换矩阵
    QJsonArray matrixArray = calibData["transform_matrix"].toArray();
    int matrixSize = matrixArray.size();
    m_transformMatrix = cv::Mat(matrixSize, 1, CV_32F);
    for (int i = 0; i < matrixSize; i++) {
        m_transformMatrix.at<float>(i) = matrixArray[i].toDouble();
    }

    m_isCalibrated = true;
    return true;
}

bool EyeTrackingCalibrator::calibratePupilCornealDual()
{
    // 瞳孔-角膜双特征融合模型
    // 分别为瞳孔位置和角膜中心计算变换矩阵

    qDebug() << "开始瞳孔-角膜双特征融合标定";

    int n = m_calibrationPoints.size();
    if (n < 6) {
        qDebug() << "双特征融合标定至少需要6个点";
        return false;
    }

    // 检查亮斑数据完整性
    int validGlintCount = 0;
    for (int i = 0; i < n; i++) {
        if (m_glint1Positions[i].x > 0 && m_glint1Positions[i].y > 0 &&
            m_glint2Positions[i].x > 0 && m_glint2Positions[i].y > 0) {
            validGlintCount++;
        }
    }

    qDebug() << "有效亮斑数据:" << validGlintCount << "/" << n;

    if (validGlintCount < 4) {
        qDebug() << "有效亮斑数据不足，退回到纯瞳孔标定";
        return calibratePolynomial2nd();
    }

    // 1. 瞳孔位置2次多项式标定
    m_pupilTransformMatrix = cv::Mat(12, 1, CV_32F);
    cv::Mat A_pupil(2 * n, 12, CV_32F);
    cv::Mat b_pupil(2 * n, 1, CV_32F);

    for (int i = 0; i < n; i++) {
        float x = m_pupilPositions[i].x;
        float y = m_pupilPositions[i].y;
        float x2 = x * x;
        float y2 = y * y;
        float xy = x * y;

        // X方程
        A_pupil.at<float>(2*i, 0) = 1;    A_pupil.at<float>(2*i, 1) = x;    A_pupil.at<float>(2*i, 2) = y;
        A_pupil.at<float>(2*i, 3) = xy;   A_pupil.at<float>(2*i, 4) = x2;   A_pupil.at<float>(2*i, 5) = y2;
        for (int j = 6; j < 12; j++) A_pupil.at<float>(2*i, j) = 0;
        b_pupil.at<float>(2*i) = m_calibrationPoints[i].x;

        // Y方程
        for (int j = 0; j < 6; j++) A_pupil.at<float>(2*i+1, j) = 0;
        A_pupil.at<float>(2*i+1, 6) = 1;  A_pupil.at<float>(2*i+1, 7) = x;  A_pupil.at<float>(2*i+1, 8) = y;
        A_pupil.at<float>(2*i+1, 9) = xy; A_pupil.at<float>(2*i+1, 10) = x2; A_pupil.at<float>(2*i+1, 11) = y2;
        b_pupil.at<float>(2*i+1) = m_calibrationPoints[i].y;
    }

    cv::solve(A_pupil, b_pupil, m_pupilTransformMatrix, cv::DECOMP_SVD);

    // 2. 角膜中心2次多项式标定
    m_cornealTransformMatrix = cv::Mat(12, 1, CV_32F);
    cv::Mat A_corneal(2 * n, 12, CV_32F);
    cv::Mat b_corneal(2 * n, 1, CV_32F);

    for (int i = 0; i < n; i++) {
        cv::Point2f cornealCenter = calculatePupilCornealCenter(m_pupilPositions[i],
                                                               m_glint1Positions[i],
                                                               m_glint2Positions[i]);
        float x = cornealCenter.x;
        float y = cornealCenter.y;
        float x2 = x * x;
        float y2 = y * y;
        float xy = x * y;

        // X方程
        A_corneal.at<float>(2*i, 0) = 1;    A_corneal.at<float>(2*i, 1) = x;    A_corneal.at<float>(2*i, 2) = y;
        A_corneal.at<float>(2*i, 3) = xy;   A_corneal.at<float>(2*i, 4) = x2;   A_corneal.at<float>(2*i, 5) = y2;
        for (int j = 6; j < 12; j++) A_corneal.at<float>(2*i, j) = 0;
        b_corneal.at<float>(2*i) = m_calibrationPoints[i].x;

        // Y方程
        for (int j = 0; j < 6; j++) A_corneal.at<float>(2*i+1, j) = 0;
        A_corneal.at<float>(2*i+1, 6) = 1;  A_corneal.at<float>(2*i+1, 7) = x;  A_corneal.at<float>(2*i+1, 8) = y;
        A_corneal.at<float>(2*i+1, 9) = xy; A_corneal.at<float>(2*i+1, 10) = x2; A_corneal.at<float>(2*i+1, 11) = y2;
        b_corneal.at<float>(2*i+1) = m_calibrationPoints[i].y;
    }

    cv::solve(A_corneal, b_corneal, m_cornealTransformMatrix, cv::DECOMP_SVD);

    qDebug() << "瞳孔-角膜双特征融合标定完成";
    return !m_pupilTransformMatrix.empty() && !m_cornealTransformMatrix.empty();
}

bool EyeTrackingCalibrator::calibrateCornealVector()
{
    // 角膜矢量模型：使用瞳孔到亮斑的矢量作为特征
    qDebug() << "开始角膜矢量模型标定";

    int n = m_calibrationPoints.size();
    if (n < 6) {
        qDebug() << "角膜矢量标定至少需要6个点";
        return false;
    }

    // 检查亮斑数据完整性
    int validGlintCount = 0;
    for (int i = 0; i < n; i++) {
        if (m_glint1Positions[i].x > 0 && m_glint1Positions[i].y > 0 &&
            m_glint2Positions[i].x > 0 && m_glint2Positions[i].y > 0) {
            validGlintCount++;
        }
    }

    if (validGlintCount < 4) {
        qDebug() << "有效亮斑数据不足，退回到纯瞳孔标定";
        return calibratePolynomial2nd();
    }

    // 使用角膜矢量进行2次多项式标定
    cv::Mat A(2 * n, 12, CV_32F);
    cv::Mat b(2 * n, 1, CV_32F);

    for (int i = 0; i < n; i++) {
        cv::Point2f cornealVector = calculateCornealVector(m_pupilPositions[i],
                                                          m_glint1Positions[i],
                                                          m_glint2Positions[i]);
        float x = cornealVector.x;
        float y = cornealVector.y;
        float x2 = x * x;
        float y2 = y * y;
        float xy = x * y;

        // X方程
        A.at<float>(2*i, 0) = 1;    A.at<float>(2*i, 1) = x;    A.at<float>(2*i, 2) = y;
        A.at<float>(2*i, 3) = xy;   A.at<float>(2*i, 4) = x2;   A.at<float>(2*i, 5) = y2;
        for (int j = 6; j < 12; j++) A.at<float>(2*i, j) = 0;
        b.at<float>(2*i) = m_calibrationPoints[i].x;

        // Y方程
        for (int j = 0; j < 6; j++) A.at<float>(2*i+1, j) = 0;
        A.at<float>(2*i+1, 6) = 1;  A.at<float>(2*i+1, 7) = x;  A.at<float>(2*i+1, 8) = y;
        A.at<float>(2*i+1, 9) = xy; A.at<float>(2*i+1, 10) = x2; A.at<float>(2*i+1, 11) = y2;
        b.at<float>(2*i+1) = m_calibrationPoints[i].y;
    }

    cv::solve(A, b, m_transformMatrix, cv::DECOMP_SVD);

    qDebug() << "角膜矢量模型标定完成";
    return !m_transformMatrix.empty();
}

cv::Point2f EyeTrackingCalibrator::calculatePupilCornealCenter(const cv::Point2f& pupilPos,
                                                              const cv::Point2f& glint1Pos,
                                                              const cv::Point2f& glint2Pos) const
{
    // 计算瞳孔中心和角膜反射中心的综合位置
    if (glint1Pos.x < 0 || glint2Pos.x < 0) {
        // 亮斑数据无效，返回瞳孔位置
        return pupilPos;
    }

    // 角膜反射中心为两个亮斑的中点
    cv::Point2f glintCenter((glint1Pos.x + glint2Pos.x) / 2.0f,
                           (glint1Pos.y + glint2Pos.y) / 2.0f);

    // 综合瞳孔和角膜信息：瞳孔权重更高
    float pupilWeight = 0.7f;
    float cornealWeight = 0.3f;

    return cv::Point2f(pupilWeight * pupilPos.x + cornealWeight * glintCenter.x,
                      pupilWeight * pupilPos.y + cornealWeight * glintCenter.y);
}

cv::Point2f EyeTrackingCalibrator::calculateCornealVector(const cv::Point2f& pupilPos,
                                                         const cv::Point2f& glint1Pos,
                                                         const cv::Point2f& glint2Pos) const
{
    // 计算从瞳孔中心到角膜反射中心的矢量
    if (glint1Pos.x < 0 || glint2Pos.x < 0) {
        // 亮斑数据无效，返回零矢量
        return cv::Point2f(0, 0);
    }

    // 角膜反射中心
    cv::Point2f glintCenter((glint1Pos.x + glint2Pos.x) / 2.0f,
                           (glint1Pos.y + glint2Pos.y) / 2.0f);

    // 从瞳孔到角膜反射的矢量
    cv::Point2f vector = glintCenter - pupilPos;

    // 矢量的模长和角度信息都很重要，这里直接返回矢量坐标
    return vector;
}

float EyeTrackingCalibrator::calculateGlintStability(const std::vector<cv::Point2f>& glint1Positions,
                                                    const std::vector<cv::Point2f>& glint2Positions) const
{
    // 计算亮斑位置的稳定性，用于动态调整权重
    if (glint1Positions.size() != glint2Positions.size() || glint1Positions.size() < 2) {
        return 0.0f;
    }

    float totalVariance = 0.0f;
    int validCount = 0;

    for (size_t i = 1; i < glint1Positions.size(); i++) {
        if (glint1Positions[i].x > 0 && glint1Positions[i-1].x > 0 &&
            glint2Positions[i].x > 0 && glint2Positions[i-1].x > 0) {

            float glint1Dist = cv::norm(glint1Positions[i] - glint1Positions[i-1]);
            float glint2Dist = cv::norm(glint2Positions[i] - glint2Positions[i-1]);

            totalVariance += glint1Dist + glint2Dist;
            validCount++;
        }
    }

    if (validCount == 0) return 0.0f;

    float avgVariance = totalVariance / validCount;
    // 返回稳定性分数（方差越小，稳定性越高）
    return std::max(0.0f, 1.0f - avgVariance / 10.0f);
}
