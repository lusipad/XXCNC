#include "InterpolationEngine.h"
#include <cmath>
#include <stdexcept>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xxcnc {
namespace core {
namespace motion {

using Point = xxcnc::core::motion::Point;

InterpolationEngine::InterpolationEngine() {}

InterpolationEngine::~InterpolationEngine() {}

std::vector<Point> InterpolationEngine::linearInterpolation(
    const Point& start,
    const Point& end,
    const InterpolationParams& params
) {
    // 验证参数
    if (params.feedRate <= 0.0) {
        throw std::invalid_argument("Feed rate must be positive");
    }
    if (params.acceleration <= 0.0) {
        throw std::invalid_argument("Acceleration must be positive");
    }
    if (params.deceleration <= 0.0) {
        throw std::invalid_argument("Deceleration must be positive");
    }

    std::vector<Point> points;
    
    // 计算总距离
    const double distance = calculateDistance(start, end);
    
    // 计算方向向量
    const double dx = (end.x - start.x) / distance;
    const double dy = (end.y - start.y) / distance;
    const double dz = (end.z - start.z) / distance;
    
    // 生成速度规划
    std::vector<double> velocities;
    planVelocityProfile(distance, params, velocities);
    
    // 根据速度生成插补点
    double currentDist = 0.0;
    for (const double velocity : velocities) {
        const double step = velocity / 60.0;  // 转换为 mm/s
        currentDist += step;
        
        if (currentDist > distance) break;
        
        points.push_back(Point(
            start.x + dx * currentDist,
            start.y + dy * currentDist,
            start.z + dz * currentDist
        ));
    }
    
    points.push_back(end);  // 确保终点精确
    return points;
}

std::vector<Point> InterpolationEngine::circularInterpolation(
    const Point& start,
    const Point& end,
    const Point& center,
    bool isClockwise,
    const InterpolationParams& params
) {
    // 验证参数
    if (params.feedRate <= 0.0) {
        throw std::invalid_argument("Feed rate must be positive");
    }
    if (params.acceleration <= 0.0) {
        throw std::invalid_argument("Acceleration must be positive");
    }
    if (params.deceleration <= 0.0) {
        throw std::invalid_argument("Deceleration must be positive");
    }

    // 验证圆心不能是起点或终点
    if (calculateDistance(start, center) < 1e-6 || calculateDistance(end, center) < 1e-6) {
        throw std::invalid_argument("Center point cannot be the same as start or end point");
    }

    std::vector<Point> points;
    
    // 计算半径
    const double radius = calculateDistance(start, center);
    
    // 计算起始角度和终止角度
    const double startAngle = std::atan2(start.y - center.y, start.x - center.x);
    const double endAngle = std::atan2(end.y - center.y, end.x - center.x);
    
    // 计算总角度
    const double totalAngle = calculateArcAngle(start, end, center, isClockwise);
    
    // 计算圆弧长度
    const double arcLength = std::fabs(totalAngle) * radius;
    
    // 生成速度规划
    std::vector<double> velocities;
    planVelocityProfile(arcLength, params, velocities);
    
    // 根据速度生成插补点
    double currentDist = 0.0;
    for (const double velocity : velocities) {
        const double step = velocity / 60.0;  // 转换为 mm/s
        currentDist += step;
        
        if (currentDist > arcLength) break;
        
        // 计算当前角度
        const double angle = startAngle + (currentDist / arcLength) * totalAngle;
        
        points.push_back(Point(
            center.x + radius * std::cos(angle),
            center.y + radius * std::sin(angle),
            start.z + (end.z - start.z) * (currentDist / arcLength)
        ));
    }
    
    points.push_back(end);  // 确保终点精确
    return points;
}

void InterpolationEngine::planVelocityProfile(
    double distance,
    const InterpolationParams& params,
    std::vector<double>& velocities
) {
    velocities.clear();
    
    // 将进给速度转换为 mm/s
    const double targetVelocity = params.feedRate / 60.0;
    const double acceleration = params.acceleration;
    const double deceleration = params.deceleration;
    
    // 计算加速和减速所需的距离
    const double accelerationTime = targetVelocity / acceleration;
    const double accelerationDist = 0.5 * acceleration * accelerationTime * accelerationTime;
    
    const double decelerationTime = targetVelocity / deceleration;
    const double decelerationDist = 0.5 * deceleration * decelerationTime * decelerationTime;
    
    // 检查是否有足够的距离进行完整的加减速
    if (accelerationDist + decelerationDist > distance) {
        // 需要调整最大速度
        const double peakVelocity = std::sqrt(2.0 * distance * acceleration * deceleration / (acceleration + deceleration));
        const double newAccelerationTime = peakVelocity / acceleration;
        const double newDecelerationTime = peakVelocity / deceleration;
        const double newTargetVelocity = peakVelocity;
        
        // 生成速度点
        double currentTime = 0.0;
        const double timeStep = 0.001;  // 1ms采样
        
        // 加速阶段
        while (currentTime <= newAccelerationTime) {
            const double v = acceleration * currentTime;
            velocities.push_back(v * 60.0);  // 转换回 mm/min
            currentTime += timeStep;
        }
        
        // 减速阶段
        const double startDecelTime = currentTime;
        while (currentTime <= startDecelTime + newDecelerationTime) {
            const double v = newTargetVelocity - deceleration * (currentTime - startDecelTime);
            velocities.push_back(v * 60.0);
            currentTime += timeStep;
        }
    } else {
        // 生成速度点
        double currentTime = 0.0;
        const double timeStep = 0.001;  // 1ms采样
        
        // 加速阶段
        while (currentTime <= accelerationTime) {
            const double v = acceleration * currentTime;
            velocities.push_back(v * 60.0);  // 转换回 mm/min
            currentTime += timeStep;
        }
        
        // 匀速阶段
        const double constantSpeedDist = distance - accelerationDist - decelerationDist;
        if (constantSpeedDist > 0) {
            const double constantSpeedTime = constantSpeedDist / targetVelocity;
            while (currentTime <= accelerationTime + constantSpeedTime) {
                velocities.push_back(targetVelocity * 60.0);
                currentTime += timeStep;
            }
        }
        
        // 减速阶段
        const double startDecelTime = currentTime;
        while (currentTime <= startDecelTime + decelerationTime) {
            const double v = targetVelocity - deceleration * (currentTime - startDecelTime);
            velocities.push_back(v * 60.0);
            currentTime += timeStep;
        }
    }
}

void InterpolationEngine::optimizePath(
    std::vector<Point>& path,
    const InterpolationParams& params
) {
    if (path.size() < 3) return;
    
    std::vector<Point> optimizedPath;
    optimizedPath.push_back(path[0]);
    
    // 使用三点平滑算法
    for (size_t i = 1; i < path.size() - 1; ++i) {
        const Point& prev = path[i - 1];
        const Point& curr = path[i];
        const Point& next = path[i + 1];
        
        // 简单的加权平均
        Point smoothed(
            0.25 * prev.x + 0.5 * curr.x + 0.25 * next.x,
            0.25 * prev.y + 0.5 * curr.y + 0.25 * next.y,
            0.25 * prev.z + 0.5 * curr.z + 0.25 * next.z
        );
        
        optimizedPath.push_back(smoothed);
    }
    
    optimizedPath.push_back(path.back());
    
    // 使用Douglas-Peucker算法进行路径简化
    std::vector<bool> keep(optimizedPath.size(), true);
    douglasPeuckerRecursive(optimizedPath, 0, optimizedPath.size() - 1, 0.01, keep);
    
    // 应用简化结果
    std::vector<Point> finalPath;
    for (size_t i = 0; i < optimizedPath.size(); ++i) {
        if (keep[i]) {
            finalPath.push_back(optimizedPath[i]);
        }
    }
    
    path = std::move(finalPath);
}

double InterpolationEngine::calculateDistance(const Point& p1, const Point& p2) {
    const double dx = p2.x - p1.x;
    const double dy = p2.y - p1.y;
    const double dz = p2.z - p1.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

double InterpolationEngine::calculateArcAngle(
    const Point& start,
    const Point& end,
    const Point& center,
    bool isClockwise
) {
    const double startAngle = std::atan2(start.y - center.y, start.x - center.x);
    const double endAngle = std::atan2(end.y - center.y, end.x - center.x);
    
    double angle = endAngle - startAngle;
    
    if (isClockwise) {
        if (angle >= 0) angle -= 2 * M_PI;
    } else {
        if (angle <= 0) angle += 2 * M_PI;
    }
    
    return angle;
}

void InterpolationEngine::douglasPeuckerRecursive(
    const std::vector<Point>& points,
    size_t start,
    size_t end,
    double epsilon,
    std::vector<bool>& keep
) {
    if (end <= start + 1) return;
    
    double maxDist = 0;
    size_t maxIndex = start;
    
    for (size_t i = start + 1; i < end; ++i) {
        const double dist = pointToLineDistance(points[i], points[start], points[end]);
        if (dist > maxDist) {
            maxDist = dist;
            maxIndex = i;
        }
    }
    
    if (maxDist > epsilon) {
        keep[maxIndex] = true;
        douglasPeuckerRecursive(points, start, maxIndex, epsilon, keep);
        douglasPeuckerRecursive(points, maxIndex, end, epsilon, keep);
    } else {
        for (size_t i = start + 1; i < end; ++i) {
            keep[i] = false;
        }
    }
}

double InterpolationEngine::pointToLineDistance(
    const Point& point,
    const Point& lineStart,
    const Point& lineEnd
) {
    const double dx = lineEnd.x - lineStart.x;
    const double dy = lineEnd.y - lineStart.y;
    const double dz = lineEnd.z - lineStart.z;
    
    const double length = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (length < 1e-6) return 0;
    
    const double t = ((point.x - lineStart.x) * dx +
                     (point.y - lineStart.y) * dy +
                     (point.z - lineStart.z) * dz) / (length * length);
    
    if (t < 0) return calculateDistance(point, lineStart);
    if (t > 1) return calculateDistance(point, lineEnd);
    
    Point projection(
        lineStart.x + t * dx,
        lineStart.y + t * dy,
        lineStart.z + t * dz
    );
    
    return calculateDistance(point, projection);
}

} // namespace motion
} // namespace core
} // namespace xxcnc