#pragma once

#include <memory>
#include <string>

namespace xxcnc {

// 基础命令参数结构
struct CommandParams {
    virtual ~CommandParams() = default;
};

// 运动指令参数
struct MotionParams : public CommandParams {
    double x{0.0};
    double y{0.0};
    double z{0.0};
    double feedrate{0.0};
};

// 基础命令类
class GCodeCommand {
public:
    virtual ~GCodeCommand() = default;
    virtual CommandParams* getParams() const = 0;
};

// 运动命令类
class MotionCommand : public GCodeCommand {
public:
    explicit MotionCommand(std::unique_ptr<MotionParams> params)
        : params_(std::move(params)) {}

    CommandParams* getParams() const override {
        return params_.get();
    }

private:
    std::unique_ptr<MotionParams> params_;
};

} // namespace xxcnc 