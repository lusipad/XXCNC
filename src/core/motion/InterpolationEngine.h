#pragma once

#include <vector>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace xxcnc {
namespace core {
namespace motion {

struct Point {
    double x;
    double y;
    double z;
    
    Point(double x_ = 0, double y_ = 0, double z_ = 0)
        : x(x_), y(y_), z(z_) {}
};

class InterpolationEngine {
public:
    struct InterpolationParams {
        double feedRate;        // 进给速度 (mm/min)
        double acceleration;    // 加速度 (mm/s^2)
        double deceleration;    // 减速度 (mm/s^2)
        double jerk;           // 加加速度 (mm/s^3)
    };

    InterpolationEngine();
    ~InterpolationEngine();

    // 直线插补
    std::vector<Point> linearInterpolation(
        const Point& start,
        const Point& end,
        const InterpolationParams& params
    );

    // 圆弧插补
    std::vector<Point> circularInterpolation(
        const Point& start,
        const Point& end,
        const Point& center,
        bool isClockwise,
        const InterpolationParams& params
    );

    // 速度规划
    void planVelocityProfile(
        double distance,
        const InterpolationParams& params,
        std::vector<double>& velocities
    );

    // 路径优化
    void optimizePath(
        std::vector<Point>& path,
        const InterpolationParams& params
    );

private:
    // 计算两点间距离
    double calculateDistance(const Point& p1, const Point& p2);
    
    // 计算圆弧角度
    double calculateArcAngle(
        const Point& start,
        const Point& end,
        const Point& center,
        bool isClockwise
    );

    // Douglas-Peucker算法递归实现
    void douglasPeuckerRecursive(
        const std::vector<Point>& points,
        size_t start,
        size_t end,
        double epsilon,
        std::vector<bool>& keep
    );

    // 计算点到直线的距离
    double pointToLineDistance(
        const Point& point,
        const Point& lineStart,
        const Point& lineEnd
    );
};

} // namespace motion
} // namespace core
} // namespace xxcnc