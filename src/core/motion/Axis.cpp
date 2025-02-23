#include "xxcnc/motion/Axis.h"

namespace xxcnc {
namespace motion {

Axis::Axis(const std::string& name, const AxisParameters& params)
    : name_(name)
    , params_(params)
{
}

bool Axis::enable()
{
    if (state_ == AxisState::ERROR) {
        return false;
    }
    state_ = AxisState::IDLE;
    return true;
}

bool Axis::disable()
{
    stop(true); // 紧急停止
    state_ = AxisState::DISABLED;
    return true;
}

bool Axis::moveTo(double position, double velocity)
{
    if (state_ != AxisState::IDLE) {
        return false;
    }

    // 检查软限位
    if (position < params_.softLimitMin || position > params_.softLimitMax) {
        return false;
    }

    // 检查速度限制
    if (std::abs(velocity) > params_.maxVelocity) {
        velocity = velocity > 0 ? params_.maxVelocity : -params_.maxVelocity;
    }

    targetPosition_ = position;
    targetVelocity_ = velocity;
    state_ = AxisState::MOVING;
    return true;
}

bool Axis::moveVelocity(double velocity)
{
    if (state_ != AxisState::IDLE) {
        return false;
    }

    // 检查速度限制
    if (std::abs(velocity) > params_.maxVelocity) {
        velocity = velocity > 0 ? params_.maxVelocity : -params_.maxVelocity;
    }

    targetVelocity_ = velocity;
    state_ = AxisState::MOVING;
    return true;
}

bool Axis::stop(bool emergency)
{
    if (state_ == AxisState::DISABLED || state_ == AxisState::ERROR) {
        return false;
    }

    if (emergency) {
        currentVelocity_ = 0;
        targetVelocity_ = 0;
        state_ = AxisState::IDLE;
    } else {
        targetVelocity_ = 0;
    }

    return true;
}

bool Axis::home()
{
    if (state_ != AxisState::IDLE) {
        return false;
    }

    state_ = AxisState::HOMING;
    targetVelocity_ = params_.homeVelocity;
    return true;
}

void Axis::update(double deltaTime)
{
    if (state_ == AxisState::DISABLED || state_ == AxisState::ERROR) {
        return;
    }

    // 更新速度
    double velocityDiff = targetVelocity_ - currentVelocity_;
    if (std::abs(velocityDiff) > params_.maxAcceleration * deltaTime) {
        velocityDiff = velocityDiff > 0 ? 
            params_.maxAcceleration * deltaTime : 
            -params_.maxAcceleration * deltaTime;
    }
    currentVelocity_ += velocityDiff;

    // 更新位置
    double newPosition = currentPosition_ + currentVelocity_ * deltaTime;

    // 检查软限位
    if (newPosition < params_.softLimitMin || newPosition > params_.softLimitMax) {
        currentVelocity_ = 0;
        targetVelocity_ = 0;
        state_ = AxisState::ERROR;
        return;
    }

    currentPosition_ = newPosition;

    // 检查是否到达目标位置
    if (state_ == AxisState::MOVING) {
        if (std::abs(currentVelocity_) < 0.001 && 
            std::abs(targetVelocity_) < 0.001) {
            state_ = AxisState::IDLE;
        }
    }
}

} // namespace motion
} // namespace xxcnc