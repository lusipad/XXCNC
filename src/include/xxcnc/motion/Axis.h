#pragma once

#include <string>
#include <memory>
#include <atomic>

namespace xxcnc {
namespace motion {

/**
 * @brief 轴的运动状态枚举
 */
enum class AxisState {
    IDLE,       ///< 空闲状态
    MOVING,     ///< 运动中
    ERROR,      ///< 错误状态
    HOMING,     ///< 回零中
    DISABLED    ///< 禁用状态
};

/**
 * @brief 轴的运动参数结构
 */
struct AxisParameters {
    double maxVelocity;      ///< 最大速度 (mm/s)
    double maxAcceleration; ///< 最大加速度 (mm/s^2)
    double maxJerk;        ///< 最大加加速度 (mm/s^3)
    double homeVelocity;   ///< 回零速度 (mm/s)
    double softLimitMin;   ///< 软限位最小值 (mm)
    double softLimitMax;   ///< 软限位最大值 (mm)
    double homePosition;
};

/**
 * @brief 轴类，实现单轴的运动控制
 */
class Axis {
public:
    /**
     * @brief 构造函数
     * @param name 轴名称
     * @param params 轴参数
     */
    Axis(const std::string& name, const AxisParameters& params);

    /**
     * @brief 获取轴名称
     * @return 轴名称
     */
    const std::string& getName() const { return name_; }

    /**
     * @brief 获取当前位置
     * @return 当前位置 (mm)
     */
    double getCurrentPosition() const { return currentPosition_; }

    /**
     * @brief 获取当前速度
     * @return 当前速度 (mm/s)
     */
    double getCurrentVelocity() const { return currentVelocity_; }

    /**
     * @brief 获取当前状态
     * @return 轴状态
     */
    AxisState getState() const { return state_; }

    /**
     * @brief 获取最大速度
     * @return 最大速度 (mm/s)
     */
    double getMaxVelocity() const { return params_.maxVelocity; }

    /**
     * @brief 获取最大加速度
     * @return 最大加速度 (mm/s^2)
     */
    double getMaxAcceleration() const { return params_.maxAcceleration; }

    /**
     * @brief 使能轴
     * @return 是否成功
     */
    bool enable();

    /**
     * @brief 禁用轴
     * @return 是否成功
     */
    bool disable();

    /**
     * @brief 移动到指定位置
     * @param position 目标位置 (mm)
     * @param velocity 运动速度 (mm/s)
     * @return 是否成功
     */
    bool moveTo(double position, double velocity);

    /**
     * @brief 以指定速度连续运动
     * @param velocity 运动速度 (mm/s)
     * @return 是否成功
     */
    bool moveVelocity(double velocity);

    /**
     * @brief 停止运动
     * @param emergency 是否紧急停止
     * @return 是否成功
     */
    bool stop(bool emergency = false);

    /**
     * @brief 回零操作
     * @return 是否成功
     */
    bool home();

    /**
     * @brief 更新轴状态
     * @param deltaTime 时间间隔 (s)
     */
    void update(double deltaTime);

private:
    std::string name_;              ///< 轴名称
    AxisParameters params_;         ///< 轴参数
    std::atomic<double> currentPosition_{0.0};   ///< 当前位置
    std::atomic<double> currentVelocity_{0.0};   ///< 当前速度
    std::atomic<double> targetPosition_{0.0};    ///< 目标位置
    std::atomic<double> targetVelocity_{0.0};    ///< 目标速度
    std::atomic<AxisState> state_{AxisState::DISABLED}; ///< 当前状态
};

} // namespace motion
} // namespace xxcnc
