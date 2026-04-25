#ifndef CMATH_FIX_H
#define CMATH_FIX_H

// VS2015 cmath兼容性修复头文件
// 必须在任何包含cmath的头文件之前包含

#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#include <stdlib.h>

// 防止cmath被重复包含
#ifndef _CMATH_
#define _CMATH_

// 在std命名空间中提供数学函数
namespace std {
    // 基本数学函数
    using ::fabs;
    using ::sqrt;
    using ::sin;
    using ::cos;
    using ::tan;
    using ::asin;
    using ::acos;
    using ::atan;
    using ::atan2;
    using ::exp;
    using ::log;
    using ::log10;
    using ::pow;
    using ::ceil;
    using ::floor;
    using ::fmod;

    // 双精度版本
    using ::abs;

    // C99函数（如果可用）
    #ifdef _MSC_VER
    inline double cbrt(double x) { return pow(x, 1.0/3.0); }
    inline float cbrtf(float x) { return powf(x, 1.0f/3.0f); }
    inline long double cbrtl(long double x) { return powl(x, 1.0L/3.0L); }

    inline bool isfinite(double x) { return _finite(x) != 0; }
    inline bool isfinite(float x) { return _finite(x) != 0; }
    inline bool isfinite(long double x) { return _finite(x) != 0; }

    inline bool isinf(double x) { return !_finite(x) && !_isnan(x); }
    inline bool isinf(float x) { return !_finite(x) && !_isnan(x); }
    inline bool isinf(long double x) { return !_finite(x) && !_isnan(x); }

    inline bool isnan(double x) { return _isnan(x) != 0; }
    inline bool isnan(float x) { return _isnan(x) != 0; }
    inline bool isnan(long double x) { return _isnan(x) != 0; }

    inline bool isnormal(double x) { return isfinite(x) && (x != 0.0); }
    inline bool isnormal(float x) { return isfinite(x) && (x != 0.0f); }
    inline bool isnormal(long double x) { return isfinite(x) && (x != 0.0L); }
    #endif
}

#endif // _CMATH_

#endif // CMATH_FIX_H
