#include "InterpolationEngine.h"

namespace xxcnc {
namespace core {
namespace motion {

InterpolationEngine::InterpolationEngine() {}

InterpolationEngine::~InterpolationEngine() {}

std::vector<InterpolationEngine::Point> InterpolationEngine::linearInterpolation(
    const Point& start,
    const Point& end,
    const InterpolationParams& params
) {
    std::vector<Point> points;
    
    // 计算总距离
    double distance = calculateDistance(start, end);
    
    // 计算方向向量
    double dx = (end.x - start.x) / distance;
    double dy = (end.y - start.y) / distance;
    double dz = (end.z - start.z) / distance;
    
    // 生成速度规划
    std::vector<double> velocities;
    planVelocityProfile(distance, params, velocities);
    
    // 根据速度生成插补点
    double currentDist = 0;
    for (double velocity : velocities) {
        double step = velocity / 60.0;  // 转换为 mm/s
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

std::vector<InterpolationEngine::Point> InterpolationEngine::circularInterpolation(
    const Point& start,
    const Point& end,
    const Point& center,
    bool isClockwise,
    const InterpolationParams& params
) {
    std::vector<Point> points;
    
    // 计算半径
    double radius = calculateDistance(start, center);
    
    // 计算起始角度和终止角度
    double startAngle = atan2(start.y - center.y, start.x - center.x);
    double endAngle = atan2(end.y - center.y, end.x - center.x);
    
    // 计算总角度
    double totalAngle = calculateArcAngle(start, end, center, isClockwise);
    
    // 计算圆弧长度
    double arcLength = fabs(totalAngle) * radius;
    
    // 生成速度规划
    std::vector<double> velocities;
    planVelocityProfile(arcLength, params, velocities);
    
    // 根据速度生成插补点
    double currentDist = 0;
    for (double velocity : velocities) {
        double step = velocity / 60.0;  // 转换为 mm/s
        currentDist += step;
        
        if (currentDist > arcLength) break;
        
        // 计算当前角度
        double angle = startAngle + (currentDist / arcLength) * totalAngle;
        
        points.push_back(Point(
            center.x + radius * cos(angle),
            center.y + radius * sin(angle),
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
    // 将进给速度转换为 mm/s
    double targetVelocity = params.feedRate / 60.0;
    double currentVelocity = 0;
    
    // 计算加速和减速距离
    double accelDist = (targetVelocity * targetVelocity) / (2 * params.acceleration);
    double decelDist = (targetVelocity * targetVelocity) / (2 * params.deceleration);
    
    // 如果总距离不足以达到目标速度
    if (accelDist + decelDist > distance) {
        // 重新计算峰值速度
        targetVelocity = sqrt(
            (2 * distance * params.acceleration * params.deceleration) /
            (params.acceleration + params.deceleration)
        );
        accelDist = (targetVelocity * targetVelocity) / (2 * params.acceleration);
        decelDist = (targetVelocity * targetVelocity) / (2 * params.deceleration);
    }
    
    // 生成速度序列
    double currentDist = 0;
    
    // 加速阶段
    while (currentDist < accelDist) {
        currentVelocity = sqrt(2 * params.acceleration * currentDist);
        velocities.push_back(currentVelocity * 60.0);  // 转换回 mm/min
        currentDist += currentVelocity / 100.0;  // 假设控制周期为 10ms
    }
    
    // 匀速阶段
    while (currentDist < distance - decelDist) {
        velocities.push_back(targetVelocity * 60.0);
        currentDist += targetVelocity / 100.0;
    }
    
    // 减速阶段
    while (currentDist < distance) {
        double remainDist = distance - currentDist;
        currentVelocity = sqrt(2 * params.deceleration * remainDist);
        velocities.push_back(currentVelocity * 60.0);
        currentDist += currentVelocity / 100.0;
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
        Point& prev = path[i - 1];
        Point& curr = path[i];
        Point& next = path[i + 1];
        
        // 简单的加权平均
        Point smoothed(
            0.25 * prev.x + 0.5 * curr.x + 0.25 * next.x,
            0.25 * prev.y + 0.5 * curr.y + 0.25 * next.y,
            0.25 * prev.z + 0.5 * curr.z + 0.25 * next.z
        );
        
        optimizedPath.push_back(smoothed);
    }
    
    optimizedPath.push_back(path.back());
    path = optimizedPath;
}

double InterpolationEngine::calculateDistance(const Point& p1, const Point& p2) {
    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    double dz = p2.z - p1.z;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

double InterpolationEngine::calculateArcAngle(
    const Point& start,
    const Point& end,
    const Point& center,
    bool isClockwise
) {
    double startAngle = atan2(start.y - center.y, start.x - center.x);
    double endAngle = atan2(end.y - center.y, end.x - center.x);
    
    double angle = endAngle - startAngle;
    
    // 调整角度范围
    if (isClockwise) {
        if (angle >= 0) angle -= 2 * M_PI;
    } else {
        if (angle <= 0) angle += 2 * M_PI;
    }
    
    return angle;
}

} // namespace motion
} // namespace core
} // namespace xxcnc