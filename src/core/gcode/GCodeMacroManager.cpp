#include "xxcnc/core/gcode/GCodeMacroManager.h"
#include <stdexcept>

namespace xxcnc {

void GCodeMacroManager::registerMacro(std::unique_ptr<GCodeMacro> macro) {
    if (!macro) {
        throw std::invalid_argument("无效的宏指令对象");
    }

    const std::string& name = macro->getName();
    if (macros_.find(name) != macros_.end()) {
        throw std::runtime_error("宏指令已存在: " + name);
    }

    macros_[name] = std::move(macro);
}

GCodeMacro* GCodeMacroManager::findMacro(const std::string& name) {
    auto it = macros_.find(name);
    if (it == macros_.end()) {
        return nullptr;
    }
    return it->second.get();
}

std::vector<std::unique_ptr<GCodeCommand>> GCodeMacroManager::executeMacro(
    const std::string& name,
    const std::map<std::string, double>& parameters
) {
    auto macro = findMacro(name);
    if (!macro) {
        throw std::runtime_error("未找到宏指令: " + name);
    }

    // 设置参数
    for (const auto& param : parameters) {
        macro->setParameter(param.first, param.second);
    }

    // 验证参数
    if (!macro->validateParameters()) {
        throw std::runtime_error("宏指令参数验证失败: " + name);
    }

    // 执行宏指令
    return macro->execute();
}

void GCodeMacroManager::removeMacro(const std::string& name) {
    auto it = macros_.find(name);
    if (it == macros_.end()) {
        throw std::runtime_error("未找到宏指令: " + name);
    }

    macros_.erase(it);
}

void GCodeMacroManager::clearMacros() {
    macros_.clear();
}

size_t GCodeMacroManager::getMacroCount() const {
    return macros_.size();
}

} // namespace xxcnc