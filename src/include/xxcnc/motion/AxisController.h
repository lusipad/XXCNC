#pragma once

#include <string>

namespace xxcnc::motion {

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
    double homeVelocity;   ///< 回零速度 (mm/s)
    double softLimitMin;   ///< 软限位最小值 (mm)
    double softLimitMax;   ///< 软限位最大值 (mm)
};

/**
 * @brief 轴控制器类，实现单轴的运动控制
 */
class AxisController {
public:
    /**
     * @brief 构造函数
     * @param name 轴名称
     * @param params 轴参数
     */
    AxisController(const std::string& name, const AxisParameters& params);

    /**
     * @brief 获取轴名称
     * @return 轴名称
     */
    const std::string& getName() const;

    /**
     * @brief 获取当前位置
     * @return 当前位置 (mm)
     */
    double getCurrentPosition() const;

    /**
     * @brief 获取当前速度
     * @return 当前速度 (mm/s)
     */
    double getCurrentVelocity() const;

    /**
     * @brief 获取当前状态
     * @return 轴状态
     */
    AxisState getState() const;

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
     * @param dt 时间增量 (s)
     */
    void update(double dt);

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

private:
    std::string name_;              ///< 轴名称
    AxisParameters params_;         ///< 轴参数
    double current_position_{0.0};  ///< 当前位置
    double target_position_{0.0};   ///< 目标位置
    double current_velocity_{0.0};  ///< 当前速度
    double target_velocity_{0.0};   ///< 目标速度
    AxisState state_{AxisState::DISABLED}; ///< 当前状态

    /**
     * @brief 检查软限位并处理
     * @param expected_position 预期位置
     * @return 是否触发软限位
     */
    bool checkSoftLimit(double expected_position);

    /**
     * @brief 更新位置和速度
     * @param dt 时间增量
     */
    void updatePositionAndVelocity(double dt);
};

} // namespace xxcnc::motion