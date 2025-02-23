#pragma once

#include "xxcnc/motion/Axis.h"
#include "xxcnc/core/motion/InterpolationEngine.h"
#include <map>
#include <memory>
#include <string>

namespace xxcnc {
namespace motion {

/**
 * @brief 运动控制器类，负责管理和协调多轴运动
 */
class MotionController {
public:
    /**
     * @brief 构造函数
     */
    MotionController();

    /**
     * @brief 添加轴
     * @param name 轴名称
     * @param params 轴参数
     * @return 是否成功
     */
    bool addAxis(const std::string& name, const AxisParameters& params);

    /**
     * @brief 获取轴
     * @param name 轴名称
     * @return 轴指针，如果不存在返回nullptr
     */
    std::shared_ptr<Axis> getAxis(const std::string& name);

    /**
     * @brief 使能所有轴
     * @return 是否成功
     */
    bool enableAllAxes();

    /**
     * @brief 禁用所有轴
     * @return 是否成功
     */
    bool disableAllAxes();

    /**
     * @brief 多轴直线插补运动
     * @param targetPositions 目标位置映射表
     * @param feedRate 进给速度 (mm/min)
     * @return 是否成功
     */
    bool moveLinear(const std::map<std::string, double>& targetPositions, double feedRate);

    /**
     * @brief 紧急停止所有轴
     * @return 是否成功
     */
    bool emergencyStop();

    /**
     * @brief 更新所有轴的状态
     * @param deltaTime 时间增量 (s)
     */
    void update(double deltaTime);

private:
    std::map<std::string, std::shared_ptr<Axis>> axes_;
    std::unique_ptr<core::motion::InterpolationEngine> interpolationEngine_;
    bool isMoving_;
};

} // namespace motion
} // namespace xxcnc