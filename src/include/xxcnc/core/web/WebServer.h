#pragma once

#include <string>
#include <memory>
#include "WebAPI.h"
#include "WebTypes.h"

namespace xxcnc {
namespace web {

class WebServer {
public:
    explicit WebServer(std::shared_ptr<WebAPI> api)
        : m_api(std::move(api)) {}
    
    // 状态监控API处理
    StatusResponse handleStatusRequest() {
        return m_api->getSystemStatus();
    }

    // 控制指令API处理
    bool handleCommandRequest(const std::string& command) {
        return m_api->executeCommand(command);
    }

    // 文件管理API处理
    FileListResponse handleFileListRequest(const std::string& path) {
        return m_api->getFileList(path);
    }

    // 配置管理API处理
    ConfigResponse handleConfigRequest() {
        return m_api->getConfig();
    }

    bool handleConfigUpdateRequest(const ConfigData& config) {
        return m_api->updateConfig(config);
    }

private:
    std::shared_ptr<WebAPI> m_api;  ///< Web API接口
};

} // namespace web
} // namespace xxcnc