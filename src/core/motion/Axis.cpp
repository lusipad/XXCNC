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

    targetPosition_.store(position);
    targetVelocity_.store(velocity);
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

    targetVelocity_.store(velocity);
    state_ = AxisState::MOVING;
    return true;
}

bool Axis::stop(bool emergency)
{
    if (state_ == AxisState::DISABLED || state_ == AxisState::ERROR) {
        return false;
    }

    if (emergency) {
        currentVelocity_.store(0);
        targetVelocity_.store(0);
        state_ = AxisState::IDLE;
    } else {
        targetVelocity_.store(0);
    }

    return true;
}

bool Axis::home()
{
    if (state_ != AxisState::IDLE) {
        return false;
    }

    state_ = AxisState::HOMING;
    targetVelocity_.store(params_.homeVelocity);
    return true;
}

void Axis::update(double deltaTime)
{
    if (state_ == AxisState::DISABLED || state_ == AxisState::ERROR) {
        return;
    }

    // 更新速度
    double velocityDiff = targetVelocity_.load() - currentVelocity_.load();
    if (std::abs(velocityDiff) > params_.maxAcceleration * deltaTime) {
        velocityDiff = velocityDiff > 0 ? 
            params_.maxAcceleration * deltaTime : 
            -params_.maxAcceleration * deltaTime;
    }
    currentVelocity_.store(currentVelocity_.load() + velocityDiff);

    // 计算预期位置
    double expectedPosition = currentPosition_.load() + currentVelocity_.load() * deltaTime;

    // 提前检查软限位并减速
    double safetyMargin = std::abs(currentVelocity_.load() * deltaTime * 2); // 减小安全距离
    if (expectedPosition + safetyMargin >= params_.softLimitMax || 
        expectedPosition - safetyMargin <= params_.softLimitMin) {
        // 立即停止并进入错误状态
        currentPosition_.store(expectedPosition >= params_.softLimitMax ? 
            params_.softLimitMax - 0.1 : params_.softLimitMin + 0.1);
        currentVelocity_.store(0);
        targetVelocity_.store(0);
        state_ = AxisState::ERROR;
        return;
    }

    // 更新位置，使用梯形积分提高精度
    currentPosition_.store(currentPosition_.load() + (currentVelocity_.load() + velocityDiff * 0.5) * deltaTime);

    // 检查是否到达目标位置
    if (state_ == AxisState::MOVING) {
        if (std::abs(currentVelocity_.load()) < 0.001 && 
            std::abs(targetVelocity_.load()) < 0.001) {
            state_ = AxisState::IDLE;
        }
    }
}

bool Axis::clearTrajectory() {
    // 重置目标位置为当前位置，停止任何规划的轨迹
    targetPosition_.store(currentPosition_.load());
    targetVelocity_.store(0.0);
    
    // 如果轴处于运动状态，则设置为空闲状态
    if (state_ == AxisState::MOVING) {
        state_ = AxisState::IDLE;
    }
    
    return true;
}

} // namespace motion
} // namespace xxcnc