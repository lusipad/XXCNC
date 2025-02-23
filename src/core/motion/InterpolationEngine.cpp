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
    
    // Calculate total distance
    const double distance = calculateDistance(start, end);
    if (distance < 1e-6) {
        points.push_back(end);
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
    for (const double velocity : velocities) {
        const double step = velocity / 60.0;  // Convert to mm/s
        currentDist += step;
        
        if (currentDist > distance) break;
        
        points.push_back(Point(
            start.x + dx * currentDist,
            start.y + dy * currentDist,
            start.z + dz * currentDist
        ));
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
    const double timeStep = 0.001; // 1ms
    const double acceleration = params.acceleration;
    const double deceleration = params.deceleration;
    const double feedRate = params.feedRate / 60.0;  // Convert to mm/s
    const double targetVelocity = std::min(feedRate, params.maxVelocity);
    
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
        
        // Acceleration phase
        double currentTime = 0.0;
        while (currentTime <= accelerationTime) {
            const double v = acceleration * currentTime;
            velocities.push_back(v * 60.0);
            currentTime += timeStep;
        }
        
        // Constant velocity phase
        while (currentTime <= (startConstantTime + constantVelocityTime)) {
            velocities.push_back(targetVelocity * 60.0);
            currentTime += timeStep;
        }
        
        // Deceleration phase
        while (currentTime <= (startDecelTime + decelerationTime)) {
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
} // namespace xxcnc