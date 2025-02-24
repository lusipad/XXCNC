#include "GCodeMacro.h"
#include <stdexcept>

namespace xxcnc {

GCodeMacro::GCodeMacro(const std::string& name)
    : name_(name) {
}

void GCodeMacro::setParameter(const std::string& name, double value) {
    auto it = parameters_.find(name);
    if (it == parameters_.end()) {
        throw std::runtime_error("未定义的宏参数: " + name);
    }
    it->second.value = value;
}

double GCodeMacro::getParameter(const std::string& name) const {
    auto it = parameters_.find(name);
    if (it == parameters_.end()) {
        throw std::runtime_error("未定义的宏参数: " + name);
    }
    return it->second.value;
}

void GCodeMacro::addParameterDefinition(const std::string& name, bool required, double default_value) {
    MacroParameter param;
    param.name = name;
    param.is_required = required;
    param.default_value = default_value;
    param.value = default_value;
    
    parameters_[name] = param;
}

bool GCodeMacro::validateParameters() const {
    for (const auto& pair : parameters_) {
        const auto& param = pair.second;
        if (param.is_required && param.value == param.default_value) {
            return false;
        }
    }
    return true;
}

} // namespace xxcnc