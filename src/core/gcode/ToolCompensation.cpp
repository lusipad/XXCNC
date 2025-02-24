#include "ToolCompensation.h"
#include <cmath>
#include <stdexcept>

namespace xxcnc {

ToolCompensation::ToolCompensation()
    : active_tool_id_(0)
    , comp_type_(CompType::NONE) {}

void ToolCompensation::setToolData(int tool_id, const ToolData& data) {
    tool_table_[tool_id] = data;
}

ToolCompensation::ToolData ToolCompensation::getToolData(int tool_id) const {
    auto it = tool_table_.find(tool_id);
    if (it == tool_table_.end()) {
        throw std::runtime_error("Tool ID not found");
    }
    return it->second;
}

void ToolCompensation::setActiveTool(int tool_id) {
    if (tool_table_.find(tool_id) == tool_table_.end()) {
        throw std::runtime_error("Invalid tool ID");
    }
    active_tool_id_ = tool_id;
}

void ToolCompensation::setCompensationType(CompType type) {
    comp_type_ = type;
}

Point3D ToolCompensation::calculateRadiusOffset(const Point3D& target, const Point3D& current) const {
    // 计算移动方向向量
    Point3D direction(target.x - current.x, target.y - current.y, 0.0);
    
    // 计算向量长度
    double length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length < 1e-6) {
        return Point3D(0.0, 0.0, 0.0);
    }
    
    // 单位化向量
    direction.x /= length;
    direction.y /= length;
    
    // 获取刀具半径（包含磨损补偿）
    const ToolData& tool = tool_table_.at(active_tool_id_);
    double total_radius = tool.radius + tool.wear_radius;
    
    // 根据补偿方向计算垂直偏移向量
    double offset_x, offset_y;
    if (comp_type_ == CompType::LEFT) {
        offset_x = -direction.y * total_radius;
        offset_y = direction.x * total_radius;
    } else if (comp_type_ == CompType::RIGHT) {
        offset_x = direction.y * total_radius;
        offset_y = -direction.x * total_radius;
    } else {
        return Point3D(0.0, 0.0, 0.0);
    }
    
    return Point3D(offset_x, offset_y, 0.0);
}

Point3D ToolCompensation::applyRadiusComp(const Point3D& target, const Point3D& current) const {
    if (comp_type_ != CompType::LEFT && comp_type_ != CompType::RIGHT) {
        return target;
    }
    
    Point3D offset = calculateRadiusOffset(target, current);
    return Point3D(target.x + offset.x, target.y + offset.y, target.z);
}

Point3D ToolCompensation::applyLengthComp(const Point3D& position) const {
    if (comp_type_ != CompType::LENGTH_POSITIVE && comp_type_ != CompType::LENGTH_NEGATIVE) {
        return position;
    }
    
    const ToolData& tool = tool_table_.at(active_tool_id_);
    double total_length = tool.length + tool.wear_length;
    
    if (comp_type_ == CompType::LENGTH_NEGATIVE) {
        total_length = -total_length;
    }
    
    return Point3D(position.x, position.y, position.z + total_length);
}

} // namespace xxcnc