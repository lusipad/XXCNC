#pragma once

#include <string>
#include <vector>
#include <map>

namespace xxcnc {
namespace web {

/**
 * @brief 系统状态响应
 */
struct StatusResponse {
    std::string status;      ///< 系统当前状态
    int errorCode;           ///< 错误代码
    std::vector<std::string> messages;  ///< 状态消息列表

    bool operator==(const StatusResponse& other) const {
        return status == other.status &&
               errorCode == other.errorCode &&
               messages == other.messages;
    }
};

/**
 * @brief 文件列表响应
 */
struct FileListResponse {
    std::vector<std::string> files;     ///< 文件列表
    std::vector<std::string> folders;   ///< 文件夹列表
    std::vector<std::string> errors;    ///< 错误信息列表

    bool operator==(const FileListResponse& other) const {
        return files == other.files &&
               folders == other.folders &&
               errors == other.errors;
    }
};

/**
 * @brief 配置数据
 */
struct ConfigData {
    std::map<std::string, std::string> config;  ///< 配置键值对映射

    bool operator==(const ConfigData& other) const {
        return config == other.config;
    }
};

/**
 * @brief 配置响应
 */
struct ConfigResponse {
    std::map<std::string, std::string> config;  ///< 配置键值对映射

    bool operator==(const ConfigResponse& other) const {
        return config == other.config;
    }
};

} // namespace web
} // namespace xxcnc