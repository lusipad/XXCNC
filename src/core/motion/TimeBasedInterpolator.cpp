#include "xxcnc/core/motion/TimeBasedInterpolator.h"
#include <stdexcept>
#include <cmath>
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>

namespace xxcnc {
namespace core {
namespace motion {

TimeBasedInterpolator::TimeBasedInterpolator(int interpolationPeriodMs)
    : interpolationPeriodMs_(interpolationPeriodMs)
    , totalDistance_(0.0)
    , completedDistance_(0.0)
    , isPlanning_(false)
{
    if (interpolationPeriodMs <= 0) {
        throw std::invalid_argument("插补周期必须为正数");
    }
    
    interpolationEngine_ = std::make_unique<InterpolationEngine>();
}

TimeBasedInterpolator::~TimeBasedInterpolator() {
    clearQueue();
}

void TimeBasedInterpolator::setInterpolationPeriod(int periodMs) {
    if (periodMs <= 0) {
        throw std::invalid_argument("插补周期必须为正数");
    }
    
    std::lock_guard<std::mutex> lock(queueMutex_);
    interpolationPeriodMs_ = periodMs;
}

int TimeBasedInterpolator::getInterpolationPeriod() const {
    return interpolationPeriodMs_;
}

bool TimeBasedInterpolator::planLinearPath(
    const Point& start,
    const Point& end,
    const InterpolationEngine::InterpolationParams& params
) {
    try {
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        // 清空现有队列
        clearQueue();
        
        // 设置当前位置
        currentPosition_ = start;
        
        // 使用插补引擎生成路径点
        std::vector<Point> path = interpolationEngine_->linearInterpolation(start, end, params);
        
        // 按照时间周期拆分路径
        segmentPathByTime(path, params);
        
        // 计算总距离
        totalDistance_ = calculateDistance(start, end);
        completedDistance_ = 0.0;
        
        return true;
    } catch (const std::exception&) {
        // 记录错误
        return false;
    }
}

bool TimeBasedInterpolator::planCircularPath(
    const Point& start,
    const Point& end,
    const Point& center,
    bool isClockwise,
    const InterpolationEngine::InterpolationParams& params
) {
    try {
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        // 清空现有队列
        clearQueue();
        
        // 设置当前位置
        currentPosition_ = start;
        
        // 使用插补引擎生成路径点
        std::vector<Point> path = interpolationEngine_->circularInterpolation(
            start, end, center, isClockwise, params
        );
        
        // 按照时间周期拆分路径
        segmentPathByTime(path, params);
        
        // 计算总距离（圆弧长度）
        double radius = calculateDistance(start, center);
        double startAngle = atan2(start.y - center.y, start.x - center.x);
        double endAngle = atan2(end.y - center.y, end.x - center.x);
        
        // 确保角度在正确的方向上
        if (isClockwise) {
            if (endAngle > startAngle) {
                endAngle -= 2 * M_PI;
            }
        } else {
            if (startAngle > endAngle) {
                endAngle += 2 * M_PI;
            }
        }
        
        double angle = isClockwise ? (startAngle - endAngle) : (endAngle - startAngle);
        totalDistance_ = radius * angle;
        completedDistance_ = 0.0;
        
        return true;
    } catch (const std::exception&) {
        // 记录错误
        return false;
    }
}

bool TimeBasedInterpolator::getNextPoint(Point& point) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    if (interpolationQueue_.empty()) {
        return false;
    }
    
    point = interpolationQueue_.front();
    interpolationQueue_.pop();
    
    // 更新已完成距离
    completedDistance_ += calculateDistance(currentPosition_, point);
    currentPosition_ = point;
    
    return true;
}

void TimeBasedInterpolator::clearQueue() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    while (!interpolationQueue_.empty()) {
        interpolationQueue_.pop();
    }
    
    totalDistance_ = 0.0;
    completedDistance_ = 0.0;
}

size_t TimeBasedInterpolator::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return interpolationQueue_.size();
}

bool TimeBasedInterpolator::isFinished() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return interpolationQueue_.empty() && !isPlanning_;
}

double TimeBasedInterpolator::getProgress() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    if (totalDistance_ < 1e-6) {
        return 1.0;
    }
    
    return std::min(1.0, completedDistance_ / totalDistance_);
}

void TimeBasedInterpolator::segmentPathByTime(
    const std::vector<Point>& path,
    const InterpolationEngine::InterpolationParams& params
) {
    if (path.size() < 2) {
        return;
    }
    
    // 计算每个时间周期的距离增量
    double feedRatePerMs = params.feedRate / (60.0 * 1000.0); // 转换为mm/ms
    double distancePerPeriod = feedRatePerMs * interpolationPeriodMs_;
    
    // 标记正在规划
    isPlanning_ = true;
    
    // 遍历原始路径点，按照时间周期拆分
    for (size_t i = 0; i < path.size() - 1; ++i) {
        const Point& p1 = path[i];
        const Point& p2 = path[i + 1];
        
        double segmentDistance = calculateDistance(p1, p2);
        if (segmentDistance < 1e-6) {
            continue;
        }
        
        // 计算方向向量
        double dx = (p2.x - p1.x) / segmentDistance;
        double dy = (p2.y - p1.y) / segmentDistance;
        double dz = (p2.z - p1.z) / segmentDistance;
        
        // 计算需要拆分的点数
        int numPoints = static_cast<int>(std::ceil(segmentDistance / distancePerPeriod));
        
        // 生成拆分点
        for (int j = 1; j <= numPoints; ++j) {
            double t = std::min(1.0, j * distancePerPeriod / segmentDistance);
            
            Point interpolatedPoint(
                p1.x + dx * t * segmentDistance,
                p1.y + dy * t * segmentDistance,
                p1.z + dz * t * segmentDistance
            );
            
            interpolationQueue_.push(interpolatedPoint);
        }
    }
    
    // 确保最后一个点是精确的终点
    interpolationQueue_.push(path.back());
    
    // 标记规划完成
    isPlanning_ = false;
}

double TimeBasedInterpolator::calculateDistance(const Point& p1, const Point& p2) const {
    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    double dz = p2.z - p1.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace motion
} // namespace core
} // namespace xxcnc
