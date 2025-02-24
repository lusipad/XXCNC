#pragma once

#include "CoordinateSystem.h"
#include <map>
#include <memory>

namespace xxcnc {

class ToolCompensation {
public:
    // 刀具补偿类型
    enum class CompType {
        NONE,           // 无补偿
        LEFT,           // 左补偿（G41）
        RIGHT,          // 右补偿（G42）
        LENGTH_POSITIVE, // 正向长度补偿（G43）
        LENGTH_NEGATIVE  // 负向长度补偿（G44）
    };

    struct ToolData {
        double radius;        // 刀具半径
        double length;        // 刀具长度
        double wear_radius;   // 刀具半径磨损补偿
        double wear_length;   // 刀具长度磨损补偿

        ToolData()
            : radius(0.0)
            , length(0.0)
            , wear_radius(0.0)
            , wear_length(0.0) {}
    };

    ToolCompensation();
    ~ToolCompensation() = default;

    // 设置刀具数据
    void setToolData(int tool_id, const ToolData& data);

    // 获取刀具数据
    ToolData getToolData(int tool_id) const;

    // 设置当前刀具
    void setActiveTool(int tool_id);

    // 设置补偿类型
    void setCompensationType(CompType type);

    // 应用刀具半径补偿
    Point3D applyRadiusComp(const Point3D& target, const Point3D& current) const;

    // 应用刀具长度补偿
    Point3D applyLengthComp(const Point3D& position) const;

private:
    std::map<int, ToolData> tool_table_;    // 刀具数据表
    int active_tool_id_;                     // 当前使用的刀具编号
    CompType comp_type_;                     // 当前补偿类型

    // 计算刀具半径补偿的偏移量
    Point3D calculateRadiusOffset(const Point3D& target, const Point3D& current) const;
};

} // namespace xxcnc