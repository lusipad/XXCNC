#pragma once

#include <string>
#include <vector>
#include <memory>
#include "WebTypes.h"

namespace xxcnc {
namespace web {

/**
 * @brief Web API接口类
 * 
 * 提供CNC系统的Web API接口，包括：
 * - 状态监控
 * - 控制指令
 * - 文件管理
 * - 配置管理
 */
class WebAPI {
public:
    WebAPI() = default;
    virtual ~WebAPI() = default;

    // 状态监控API
    virtual StatusResponse getSystemStatus() = 0;

    // 控制指令API
    virtual bool executeCommand(const std::string& command) = 0;

    // 文件管理API
    virtual FileListResponse getFileList(const std::string& path) = 0;

    // 配置管理API
    virtual ConfigResponse getConfig() = 0;
    virtual bool updateConfig(const ConfigData& config) = 0;
};

} // namespace web
} // namespace xxcnc