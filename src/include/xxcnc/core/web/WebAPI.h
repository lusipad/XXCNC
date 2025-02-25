#pragma once

#include "WebTypes.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace xxcnc {
namespace web {

/**
 * @brief 文件上传响应
 */
struct FileUploadResponse {
    bool success = false;
    std::string error;
};

/**
 * @brief 文件解析响应
 */
struct FileParseResponse {
    bool success = false;
    std::vector<std::string> toolPathDetails;
    std::vector<TrajectoryPoint> trajectoryPoints;
    std::string error;
};

/**
 * @brief Web API接口类
 * 
 * 提供CNC系统的Web API接口，包括：
 * - 状态监控
 * - 控制指令
 * - 文件管理
 * - 配置管理
 * - 文件上传和解析
 */
class WebAPI {
public:
    WebAPI() = default;
    virtual ~WebAPI() = default;

    // 状态监控API
    virtual StatusResponse getSystemStatus() = 0;

    // 控制指令API
    virtual bool executeCommand(const nlohmann::json& command) = 0;

    // 文件管理API
    virtual FileListResponse getFileList(const std::string& path) = 0;

    // 文件上传API
    virtual FileUploadResponse uploadFile(const std::string& filename, const std::string& content) = 0;

    // 文件解析API
    virtual FileParseResponse parseFile(const std::string& filename) = 0;

    // 配置管理API
    virtual ConfigResponse getConfig() = 0;
    virtual bool updateConfig(const ConfigData& config) = 0;
};

} // namespace web
} // namespace xxcnc