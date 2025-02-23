#include "xxcnc/motion/AxisController.h"
#include <stdexcept>

namespace xxcnc::motion {

AxisController::AxisController(const std::string& name, const AxisParameters& params)
    : name_(name)
    , params_(params)
    , current_position_(0.0)
    , target_position_(0.0)
    , current_velocity_(0.0)
    , state_(AxisState::DISABLED) {
    if (params.maxVelocity <= 0.0 || params.maxAcceleration <= 0.0) {
        throw std::invalid_argument("Invalid axis parameters");
    }
}

bool AxisController::enable() {
    if (state_ == AxisState::ERROR) {
        return false;
    }
    state_ = AxisState::IDLE;
    return true;
}

bool AxisController::disable() {
    if (state_ == AxisState::MOVING || state_ == AxisState::HOMING) {
        return false;
    }
    state_ = AxisState::DISABLED;
    return true;
}

bool AxisController::moveTo(double position, double velocity) {
    if (state_ == AxisState::DISABLED || state_ == AxisState::ERROR) {
        return false;
    }

    // 检查软限位
    if (position < params_.softLimitMin || position > params_.softLimitMax) {
        return false;
    }

    // 限制速度在最大值范围内
    if (std::abs(velocity) > params_.maxVelocity) {
        velocity = velocity > 0 ? params_.maxVelocity : -params_.maxVelocity;
    }

    target_position_ = position;
    target_velocity_ = velocity;
    state_ = AxisState::MOVING;
    return true;
}

bool AxisController::moveVelocity(double velocity) {
    if (state_ == AxisState::DISABLED || state_ == AxisState::ERROR) {
        return false;
    }

    // 限制速度在最大值范围内
    if (std::abs(velocity) > params_.maxVelocity) {
        velocity = velocity > 0 ? params_.maxVelocity : -params_.maxVelocity;
    }

    target_velocity_ = velocity;
    state_ = AxisState::MOVING;
    return true;
}

bool AxisController::stop(bool emergency) {
    if (state_ != AxisState::MOVING && state_ != AxisState::HOMING) {
        return false;
    }

    if (emergency) {
        current_velocity_ = 0.0;
        state_ = AxisState::IDLE;
    } else {
        target_velocity_ = 0.0;
    }

    return true;
}

bool AxisController::home() {
    if (state_ != AxisState::IDLE) {
        return false;
    }

    state_ = AxisState::HOMING;
    target_velocity_ = params_.homeVelocity;
    return true;
}

void AxisController::update(double dt)
{
    if (state_ == AxisState::DISABLED || state_ == AxisState::ERROR) {
        return;
    }

    // 计算预期位置
    double expected_position = current_position_ + current_velocity_ * dt;

    // 检查软限位
    if (expected_position > params_.softLimitMax || expected_position < params_.softLimitMin) {
        current_velocity_ = 0.0;
        target_velocity_ = 0.0;
        state_ = AxisState::ERROR;
        current_position_ = std::min(std::max(current_position_, 
            params_.softLimitMin + 0.1), params_.softLimitMax - 0.1);
        return;
    }

    updatePositionAndVelocity(dt);

    // 检查是否到达目标位置
    if (state_ == AxisState::MOVING) {
        if (std::abs(current_velocity_) < 0.001 && 
            std::abs(target_velocity_) < 0.001 &&
            std::abs(target_position_ - current_position_) < 0.001) {
            state_ = AxisState::IDLE;
        }
    }
}

void AxisController::updatePositionAndVelocity(double dt)
{
    // 计算加速度
    double velocity_diff = target_velocity_ - current_velocity_;
    double max_velocity_change = params_.maxAcceleration * dt;
    
    if (std::abs(velocity_diff) > max_velocity_change) {
        velocity_diff = velocity_diff > 0 ? max_velocity_change : -max_velocity_change;
    }
    
    // 更新速度，使用梯形积分提高精度
    double avg_velocity = current_velocity_ + velocity_diff * 0.5;
    current_velocity_ += velocity_diff;
    
    // 计算预期位置
    double expected_position = current_position_ + avg_velocity * dt;
    
    // 检查软限位
    if (expected_position > params_.softLimitMax) {
        if (current_velocity_ > 0) {
            current_velocity_ = std::max(0.0, current_velocity_ - params_.maxAcceleration * dt);
            expected_position = current_position_ + current_velocity_ * dt;
            if (expected_position > params_.softLimitMax) {
                current_position_ = params_.softLimitMax - 0.1;
                current_velocity_ = 0.0;
                target_velocity_ = 0.0;
                state_ = AxisState::ERROR;
                return;
            }
        }
    } else if (expected_position < params_.softLimitMin) {
        if (current_velocity_ < 0) {
            current_velocity_ = std::min(0.0, current_velocity_ + params_.maxAcceleration * dt);
            expected_position = current_position_ + current_velocity_ * dt;
            if (expected_position < params_.softLimitMin) {
                current_position_ = params_.softLimitMin + 0.1;
                current_velocity_ = 0.0;
                target_velocity_ = 0.0;
                state_ = AxisState::ERROR;
                return;
            }
        }
    }
    
    current_position_ = expected_position;
}
}

const std::string& xxcnc::motion::AxisController::getName() const {
    return name_;
}

double xxcnc::motion::AxisController::getCurrentPosition() const {
    return current_position_;
}

double xxcnc::motion::AxisController::getCurrentVelocity() const {
    return current_velocity_;
}

xxcnc::motion::AxisState xxcnc::motion::AxisController::getState() const {
    return state_;
}