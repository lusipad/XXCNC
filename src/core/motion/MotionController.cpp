#include "xxcnc/motion/MotionController.h"
#include <algorithm>

namespace xxcnc {
namespace motion {

MotionController::MotionController()
    : interpolationEngine_(std::make_unique<core::motion::InterpolationEngine>())
    , timeBasedInterpolator_(std::make_unique<core::motion::TimeBasedInterpolator>(1)) // 默认1ms插补周期
    , isMoving_(false)
{
}

bool MotionController::addAxis(const std::string& name, const AxisParameters& params)
{
    if (axes_.find(name) != axes_.end()) {
        return false;
    }
    axes_[name] = std::make_shared<Axis>(name, params);
    return true;
}

std::shared_ptr<Axis> MotionController::getAxis(const std::string& name)
{
    auto it = axes_.find(name);
    return (it != axes_.end()) ? it->second : nullptr;
}

bool MotionController::enableAllAxes()
{
    bool success = true;
    for (auto& [name, axis] : axes_) {
        if (!axis->enable()) {
            success = false;
        }
    }
    return success;
}

bool MotionController::disableAllAxes()
{
    bool success = true;
    for (auto& [name, axis] : axes_) {
        if (!axis->disable()) {
            success = false;
        }
    }
    isMoving_ = false;
    return success;
}

bool MotionController::moveLinear(const std::map<std::string, double>& targetPositions, double feedRate)
{
    if (isMoving_ || targetPositions.empty()) {
        return false;
    }

    // 检查所有目标轴是否存在且处于空闲状态
    for (const auto& [name, position] : targetPositions) {
        auto axis = getAxis(name);
        if (!axis || axis->getState() != AxisState::IDLE) {
            return false;
        }
    }

    // 创建起点和终点
    core::motion::Point start, end;
    auto xAxis = axes_.find("X");
    auto yAxis = axes_.find("Y");
    auto zAxis = axes_.find("Z");

    if (xAxis != axes_.end()) {
        start.x = xAxis->second->getCurrentPosition();
        end.x = targetPositions.count("X") ? targetPositions.at("X") : start.x;
    }
    if (yAxis != axes_.end()) {
        start.y = yAxis->second->getCurrentPosition();
        end.y = targetPositions.count("Y") ? targetPositions.at("Y") : start.y;
    }
    if (zAxis != axes_.end()) {
        start.z = zAxis->second->getCurrentPosition();
        end.z = targetPositions.count("Z") ? targetPositions.at("Z") : start.z;
    }

    // 设置插补参数
    core::motion::InterpolationEngine::InterpolationParams params;
    params.feedRate = feedRate;
    params.maxVelocity = std::min({
        xAxis != axes_.end() ? xAxis->second->getMaxVelocity() : 1e6,
        yAxis != axes_.end() ? yAxis->second->getMaxVelocity() : 1e6,
        zAxis != axes_.end() ? zAxis->second->getMaxVelocity() : 1e6
    });
    params.acceleration = std::min({
        xAxis != axes_.end() ? xAxis->second->getMaxAcceleration() : 1e6,
        yAxis != axes_.end() ? yAxis->second->getMaxAcceleration() : 1e6,
        zAxis != axes_.end() ? zAxis->second->getMaxAcceleration() : 1e6
    });
    params.deceleration = params.acceleration;

    // 使用基于时间的插补器规划路径
    if (!timeBasedInterpolator_->planLinearPath(start, end, params)) {
        return false;
    }

    // 启动各轴运动
    for (const auto& [name, position] : targetPositions) {
        auto axis = getAxis(name);
        if (!axis->moveTo(position, params.maxVelocity)) {
            emergencyStop();
            return false;
        }
    }

    isMoving_ = true;
    return true;
}

bool MotionController::emergencyStop()
{
    bool success = true;
    for (auto& [name, axis] : axes_) {
        if (!axis->stop(true)) {
            success = false;
        }
    }
    isMoving_ = false;
    return success;
}

void MotionController::update(double deltaTime)
{
    if (!isMoving_) {
        return;
    }

    // 检查各轴状态
    bool allIdle = true;
    for (auto& [name, axis] : axes_) {
        axis->update(deltaTime);
        if (axis->getState() == AxisState::MOVING) {
            allIdle = false;
        }
    }

    // 如果所有轴都空闲，但插补还未完成，则获取下一个插补点并移动
    if (allIdle && !timeBasedInterpolator_->isFinished()) {
        core::motion::Point nextPoint;
        if (timeBasedInterpolator_->getNextPoint(nextPoint)) {
            // 更新各轴目标位置
            std::map<std::string, double> targetPositions;
            
            auto xAxis = axes_.find("X");
            if (xAxis != axes_.end()) {
                targetPositions["X"] = nextPoint.x;
            }
            
            auto yAxis = axes_.find("Y");
            if (yAxis != axes_.end()) {
                targetPositions["Y"] = nextPoint.y;
            }
            
            auto zAxis = axes_.find("Z");
            if (zAxis != axes_.end()) {
                targetPositions["Z"] = nextPoint.z;
            }
            
            // 启动各轴运动
            for (const auto& [name, position] : targetPositions) {
                auto axis = getAxis(name);
                // 使用较高的速度，因为这是1ms周期内的短距离移动
                double maxVelocity = axis->getMaxVelocity();
                if (!axis->moveTo(position, maxVelocity)) {
                    emergencyStop();
                    return;
                }
            }
            
            allIdle = false;
        }
    }

    if (allIdle && timeBasedInterpolator_->isFinished()) {
        isMoving_ = false;
    }
}

void MotionController::setInterpolationPeriod(int periodMs)
{
    timeBasedInterpolator_->setInterpolationPeriod(periodMs);
}

int MotionController::getInterpolationPeriod() const
{
    return timeBasedInterpolator_->getInterpolationPeriod();
}

double MotionController::getInterpolationProgress() const
{
    return timeBasedInterpolator_->getProgress();
}

bool MotionController::isInterpolationFinished() const
{
    return timeBasedInterpolator_->isFinished();
}

size_t MotionController::getInterpolationQueueSize() const
{
    return timeBasedInterpolator_->getQueueSize();
}

} // namespace motion
} // namespace xxcnc