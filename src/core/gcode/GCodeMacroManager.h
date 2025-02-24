#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include "GCodeMacro.h"

namespace xxcnc {

class GCodeMacroManager {
public:
    GCodeMacroManager() = default;
    ~GCodeMacroManager() = default;

    // 注册宏指令
    void registerMacro(std::unique_ptr<GCodeMacro> macro);

    // 查找宏指令
    GCodeMacro* findMacro(const std::string& name);

    // 执行宏指令
    std::vector<std::unique_ptr<GCodeCommand>> executeMacro(
        const std::string& name,
        const std::map<std::string, double>& parameters
    );

    // 移除宏指令
    void removeMacro(const std::string& name);

    // 清空所有宏指令
    void clearMacros();

    // 获取已注册的宏指令数量
    size_t getMacroCount() const;

private:
    std::unordered_map<std::string, std::unique_ptr<GCodeMacro>> macros_;
};

} // namespace xxcnc