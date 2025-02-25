#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>
#include "GCodeCommands.h"

namespace xxcnc {

class GCodeMacro {
public:
    // 宏指令参数定义
    struct MacroParameter {
        std::string name;        // 参数名称
        double value;            // 参数值
        bool is_required;        // 是否必需
        double default_value;    // 默认值

        MacroParameter()
            : value(0.0)
            , is_required(false)
            , default_value(0.0) {}
    };

    explicit GCodeMacro(const std::string& name) : name_(name) {}
    virtual ~GCodeMacro() = default;

    // 设置宏参数
    void setParameter(const std::string& name, double value);

    // 获取宏参数
    double getParameter(const std::string& name) const;

    // 添加参数定义
    void addParameterDefinition(const std::string& name, bool required, double default_value = 0.0);

    // 验证参数
    bool validateParameters() const;

    // 执行宏指令
    virtual std::vector<std::unique_ptr<GCodeCommand>> execute() = 0;

    // 获取宏名称
    const std::string& getName() const { return name_; }

protected:
    std::string name_;                                      // 宏名称
    std::map<std::string, MacroParameter> parameters_;      // 参数列表
};

} // namespace xxcnc 