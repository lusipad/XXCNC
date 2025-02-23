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
        double feedRate;        // Feed rate (mm/min)
        double maxVelocity;     // Maximum velocity (mm/s)
        double acceleration;    // Acceleration (mm/s^2)
        double deceleration;    // Deceleration (mm/s^2)
        double jerk;           // Jerk (mm/s^3)
        
        InterpolationParams(double fr = 0, double mv = 0, double acc = 0, double dec = 0, double j = 0)
            : feedRate(fr), maxVelocity(mv), acceleration(acc), deceleration(dec), jerk(j) {}
    };

    InterpolationEngine();
    ~InterpolationEngine();

    // Linear interpolation
    std::vector<Point> linearInterpolation(
        const Point& start,
        const Point& end,
        const InterpolationParams& params
    );

    // Circular interpolation
    std::vector<Point> circularInterpolation(
        const Point& start,
        const Point& end,
        const Point& center,
        bool isClockwise,
        const InterpolationParams& params
    );

    // Velocity profile planning
    void planVelocityProfile(
        double distance,
        const InterpolationParams& params,
        std::vector<double>& velocities
    );

    // Path optimization
    void optimizePath(
        std::vector<Point>& path,
        const InterpolationParams& params
    );

private:
    // Calculate distance between two points
    double calculateDistance(const Point& p1, const Point& p2);
    
    // Calculate arc angle
    double calculateArcAngle(
        const Point& start,
        const Point& end,
        const Point& center,
        bool isClockwise
    );

    // Douglas-Peucker algorithm recursive implementation
    void douglasPeuckerRecursive(
        const std::vector<Point>& points,
        size_t start,
        size_t end,
        double epsilon,
        std::vector<bool>& keep
    );

    // Calculate point to line distance
    double pointToLineDistance(
        const Point& point,
        const Point& lineStart,
        const Point& lineEnd
    );
};

} // namespace motion
} // namespace core
} // namespace xxcnc