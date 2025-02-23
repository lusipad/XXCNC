#ifndef XXCNC_TYPES_H
#define XXCNC_TYPES_H

#include <cstdint>
#include <array>

namespace xxcnc {

// 基础数值类型定义
using AxisPosition = double;    // 轴位置类型
using AxisVelocity = double;    // 轴速度类型
using AxisAccel = double;       // 轴加速度类型

// 错误码定义
enum class ErrorCode : int32_t {
    OK = 0,
    GENERAL_ERROR = -1,
    INVALID_PARAMETER = -2,
    AXIS_ERROR = -100,
    MOTION_ERROR = -200,
    INTERPOLATION_ERROR = -300,
    GCODE_ERROR = -400,
    SYSTEM_ERROR = -500
};

// 系统状态定义
enum class SystemState : uint8_t {
    IDLE = 0,
    RUNNING = 1,
    PAUSED = 2,
    ERROR = 3,
    HOMING = 4,
    EMERGENCY_STOP = 5
};

// 坐标系统定义
struct Point {
    AxisPosition x{0.0};
    AxisPosition y{0.0};
    AxisPosition z{0.0};
};

// 运动参数定义
struct MotionParameters {
    AxisVelocity maxVelocity{0.0};        // 最大速度
    AxisVelocity startVelocity{0.0};      // 起始速度
    AxisVelocity endVelocity{0.0};        // 结束速度
    AxisAccel maxAcceleration{0.0};       // 最大加速度
    AxisAccel maxDeceleration{0.0};       // 最大减速度
    AxisAccel jerkLimit{0.0};             // 加加速度限制
};

} // namespace xxcnc

#endif // XXCNC_TYPES_H