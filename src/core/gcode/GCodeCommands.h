#pragma once

#include <memory>
#include <string>
#include <vector>

namespace xxcnc {

class GCodeCommand;

// 基础命令参数结构
struct CommandParams {
    virtual ~CommandParams() = default;
};

// 运动指令参数
struct MotionParams : public CommandParams {
    double x{0.0}, y{0.0}, z{0.0};    // 目标位置
    double feedrate{0.0};              // 进给速度
    bool rapid{false};                 // 是否为快速定位
};

// 刀具指令参数
struct ToolParams : public CommandParams {
    int tool_number{0};                // 刀具编号
    double offset_x{0.0};              // X轴偏置
    double offset_y{0.0};              // Y轴偏置
    double offset_z{0.0};              // Z轴偏置
};

// 坐标系统参数
struct CoordinateParams : public CommandParams {
    int coord_system{0};               // 坐标系统编号(G54-G59)
    double offset_x{0.0};              // X轴偏置
    double offset_y{0.0};              // Y轴偏置
    double offset_z{0.0};              // Z轴偏置
};

// 宏指令参数
struct MacroParams : public CommandParams {
    std::string macro_name;            // 宏名称
    std::vector<double> arguments;      // 宏参数
};

// 具体命令类
class MotionCommand : public GCodeCommand {
public:
    MotionCommand(std::unique_ptr<MotionParams> params)
        : GCodeCommand(Type::MOTION)
        , params_(std::move(params)) {}

    const MotionParams* getParams() const { return params_.get(); }

private:
    std::unique_ptr<MotionParams> params_;
};

class ToolCommand : public GCodeCommand {
public:
    ToolCommand(std::unique_ptr<ToolParams> params)
        : GCodeCommand(Type::TOOL)
        , params_(std::move(params)) {}

    const ToolParams* getParams() const { return params_.get(); }

private:
    std::unique_ptr<ToolParams> params_;
};

class CoordinateCommand : public GCodeCommand {
public:
    CoordinateCommand(std::unique_ptr<CoordinateParams> params)
        : GCodeCommand(Type::COORDINATE)
        , params_(std::move(params)) {}

    const CoordinateParams* getParams() const { return params_.get(); }

private:
    std::unique_ptr<CoordinateParams> params_;
};

class MacroCommand : public GCodeCommand {
public:
    MacroCommand(std::unique_ptr<MacroParams> params)
        : GCodeCommand(Type::MACRO)
        , params_(std::move(params)) {}

    const MacroParams* getParams() const { return params_.get(); }

private:
    std::unique_ptr<MacroParams> params_;
};

} // namespace xxcnc