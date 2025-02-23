#include "xxcnc/core/motion/InterpolationEngine.h"
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
    // Validate parameters
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
    
    // Add start point
    points.push_back(start);
    
    // Calculate total distance
    const double distance = calculateDistance(start, end);
    if (distance < 1e-6) {
        return points;
    }
    
    // Calculate direction vector
    const double dx = (end.x - start.x) / distance;
    const double dy = (end.y - start.y) / distance;
    const double dz = (end.z - start.z) / distance;
    
    // Generate velocity profile
    std::vector<double> velocities;
    planVelocityProfile(distance, params, velocities);
    
    // Generate interpolation points based on velocity
    double currentDist = 0.0;
    double lastPointDist = 0.0;
    double minPointDistance = params.feedRate * 0.45 / 60.0; // 增加最小点距阈值到45%
    
    // 预分配内存以减少重新分配
    points.reserve(static_cast<size_t>(distance / minPointDistance) + 2);
    
    for (const double velocity : velocities) {
        const double step = velocity / 60.0;  // Convert to mm/s
        currentDist += step;
        
        // 只有当距离上一个点的距离超过最小点距时才生成新点
        if (currentDist - lastPointDist >= minPointDistance && currentDist < distance) {
            points.push_back(Point(
                start.x + dx * currentDist,
                start.y + dy * currentDist,
                start.z + dz * currentDist
            ));
            lastPointDist = currentDist;
        }
    }
    
    points.push_back(end);  // Ensure exact end point
    return points;
}

std::vector<Point> InterpolationEngine::circularInterpolation(
    const Point& start,
    const Point& end,
    const Point& center,
    bool isClockwise,
    const InterpolationParams& params
) {
    // Validate parameters
    if (params.feedRate <= 0.0) {
        throw std::invalid_argument("Feed rate must be positive");
    }
    if (params.acceleration <= 0.0) {
        throw std::invalid_argument("Acceleration must be positive");
    }
    if (params.deceleration <= 0.0) {
        throw std::invalid_argument("Deceleration must be positive");
    }

    // Validate center point
    if (calculateDistance(start, center) < 1e-6 || calculateDistance(end, center) < 1e-6) {
        throw std::invalid_argument("Center point cannot be the same as start or end point");
    }

    std::vector<Point> points;
    
    // Calculate radius and angles
    const double radius = calculateDistance(start, center);
    const double startAngle = std::atan2(start.y - center.y, start.x - center.x);
    const double totalAngle = calculateArcAngle(start, end, center, isClockwise);
    
    // Calculate arc length
    const double arcLength = std::fabs(totalAngle) * radius;
    if (arcLength < 1e-6) {
        points.push_back(end);
        return points;
    }
    
    // Generate velocity profile
    std::vector<double> velocities;
    planVelocityProfile(arcLength, params, velocities);
    
    // Generate interpolation points based on velocity
    double currentDist = 0.0;
    for (const double velocity : velocities) {
        const double step = velocity / 60.0;  // Convert to mm/s
        currentDist += step;
        
        if (currentDist > arcLength) break;
        
        // Calculate current angle
        const double angle = startAngle + (currentDist / arcLength) * totalAngle;
        
        points.push_back(Point(
            center.x + radius * std::cos(angle),
            center.y + radius * std::sin(angle),
            start.z + (end.z - start.z) * (currentDist / arcLength)
        ));
    }
    
    points.push_back(end);  // Ensure exact end point
    return points;
}

void InterpolationEngine::planVelocityProfile(
    double distance,
    const InterpolationParams& params,
    std::vector<double>& velocities
) {
    velocities.clear();
    
    // Basic parameters
    const double timeStep = 0.25; // 增加时间步长到250ms
    const double acceleration = std::max(params.acceleration, 0.001);
    const double deceleration = std::max(params.deceleration, 0.001);
    const double feedRate = std::max(params.feedRate / 60.0, 0.001);
    const double targetVelocity = std::max(std::min(feedRate, params.maxVelocity), 0.001);
    
    // 预分配内存，增加最小点距阈值
    const double minPointDistance = targetVelocity * timeStep * 4.0; // 增加最小点距阈值到40%
    const size_t estimatedPoints = static_cast<size_t>(distance / minPointDistance * 1.02); // 减少缓冲区大小到2%
    velocities.reserve(estimatedPoints);
    
    // Calculate acceleration and deceleration times
    const double accelerationTime = targetVelocity / acceleration;
    const double decelerationTime = targetVelocity / deceleration;
    
    // Calculate acceleration and deceleration distances
    const double accelerationDist = 0.5 * acceleration * accelerationTime * accelerationTime;
    const double decelerationDist = 0.5 * deceleration * decelerationTime * decelerationTime;
    
    // If total distance is less than required for acceleration and deceleration
    if (distance < (accelerationDist + decelerationDist)) {
        // Recalculate maximum velocity
        const double maxVelocity = std::sqrt(2.0 * acceleration * deceleration * distance / (acceleration + deceleration));
        const double newAccelTime = maxVelocity / acceleration;
        const double newDecelTime = maxVelocity / deceleration;
        const double startDecelTime = newAccelTime;
        
        // Acceleration phase
        double currentTime = 0.0;
        while (currentTime <= newAccelTime) {
            const double v = acceleration * currentTime;
            velocities.push_back(v * 60.0);
            currentTime += timeStep;
        }
        
        // Deceleration phase
        while (currentTime <= (startDecelTime + newDecelTime)) {
            const double v = maxVelocity - deceleration * (currentTime - startDecelTime);
            velocities.push_back(v * 60.0);
            currentTime += timeStep;
        }
    } else {
        // Normal acceleration and deceleration
        const double constantVelocityDist = distance - accelerationDist - decelerationDist;
        const double constantVelocityTime = constantVelocityDist / targetVelocity;
        const double startConstantTime = accelerationTime;
        const double startDecelTime = startConstantTime + constantVelocityTime;
        const double epsilon = 1e-6;  // 添加浮点数比较的容差值

        // Acceleration phase
        double currentTime = 0.0;
        while (currentTime <= accelerationTime + epsilon) {
            const double v = acceleration * currentTime;
            velocities.push_back(v * 60.0);
            currentTime += timeStep;
        }
        
        // Constant velocity phase
        while (currentTime <= startConstantTime + constantVelocityTime + epsilon) {
            velocities.push_back(targetVelocity * 60.0);
            currentTime += timeStep;
        }
        
        // Deceleration phase
        while (currentTime <= startDecelTime + decelerationTime + epsilon) {
            const double v = targetVelocity - deceleration * (currentTime - startDecelTime);
            velocities.push_back(v * 60.0);
            currentTime += timeStep;
        }
    }
}

double InterpolationEngine::calculateDistance(const Point& p1, const Point& p2) {
    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    double dz = p2.z - p1.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

double InterpolationEngine::calculateArcAngle(
    const Point& start,
    const Point& end,
    const Point& center,
    bool isClockwise
) {
    double startAngle = std::atan2(start.y - center.y, start.x - center.x);
    double endAngle = std::atan2(end.y - center.y, end.x - center.x);
    
    if (isClockwise) {
        if (endAngle > startAngle) {
            endAngle -= 2.0 * M_PI;
        }
    } else {
        if (endAngle < startAngle) {
            endAngle += 2.0 * M_PI;
        }
    }
    
    return endAngle - startAngle;
}

void InterpolationEngine::douglasPeuckerRecursive(
    const std::vector<Point>& points,
    size_t start,
    size_t end,
    double epsilon,
    std::vector<bool>& keep
) {
    if (end - start <= 1) return;
    
    double maxDistance = 0.0;
    size_t maxIndex = start;
    
    const Point& lineStart = points[start];
    const Point& lineEnd = points[end];
    
    for (size_t i = start + 1; i < end; ++i) {
        double distance = pointToLineDistance(points[i], lineStart, lineEnd);
        if (distance > maxDistance) {
            maxDistance = distance;
            maxIndex = i;
        }
    }
    
    if (maxDistance > epsilon) {
        keep[maxIndex] = true;
        douglasPeuckerRecursive(points, start, maxIndex, epsilon, keep);
        douglasPeuckerRecursive(points, maxIndex, end, epsilon, keep);
    }
}

double InterpolationEngine::pointToLineDistance(
    const Point& point,
    const Point& lineStart,
    const Point& lineEnd
) {
    double dx = lineEnd.x - lineStart.x;
    double dy = lineEnd.y - lineStart.y;
    double dz = lineEnd.z - lineStart.z;
    
    double lineLength = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (lineLength < 1e-6) return 0.0;
    
    double t = ((point.x - lineStart.x) * dx +
               (point.y - lineStart.y) * dy +
               (point.z - lineStart.z) * dz) / (lineLength * lineLength);
    
    if (t < 0.0) return calculateDistance(point, lineStart);
    if (t > 1.0) return calculateDistance(point, lineEnd);
    
    Point projection(
        lineStart.x + t * dx,
        lineStart.y + t * dy,
        lineStart.z + t * dz
    );
    
    return calculateDistance(point, projection);
}

} // namespace motion
} // namespace core
void xxcnc::core::motion::InterpolationEngine::optimizePath(
    std::vector<Point>& path,
    const InterpolationParams& params
) {
    if (path.size() <= 2) return; // No need to optimize paths with 2 or fewer points
    
    // Use Douglas-Peucker algorithm for path simplification
    // The epsilon value determines how aggressively we simplify the path
    // We'll base it on the feed rate - higher feed rates allow for more simplification
    double epsilon = params.feedRate * 0.0001; // 0.01% of feed rate as tolerance
    
    std::vector<bool> keep(path.size(), false);
    keep.front() = true; // Always keep first point
    keep.back() = true;  // Always keep last point
    
    // Apply Douglas-Peucker algorithm
    douglasPeuckerRecursive(path, 0, path.size() - 1, epsilon, keep);
    
    // Create new path with only kept points
    std::vector<Point> optimizedPath;
    for (size_t i = 0; i < path.size(); ++i) {
        if (keep[i]) {
            optimizedPath.push_back(path[i]);
        }
    }
    
    path = std::move(optimizedPath);
}

} // namespace motion
