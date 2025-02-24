#pragma once

#include <string>
#include <vector>

namespace xxcnc {
namespace web {

/**
 * @brief Web服务器配置数据结构
 */
struct ConfigData {
    std::string host;      ///< 服务器主机地址
    uint16_t port;         ///< 服务器端口
    std::string root_path; ///< Web根目录路径
    std::string config;    ///< 配置数据

    /**
     * @brief 相等运算符重载
     * @param other 要比较的另一个ConfigData对象
     * @return 如果两个对象的所有成员都相等则返回true
     */
    bool operator==(const ConfigData& other) const {
        return host == other.host &&
               port == other.port &&
               root_path == other.root_path &&
               config == other.config;
    }
};

/**
 * @brief 配置响应数据结构
 */
struct ConfigResponse {
    ConfigData config;     ///< 配置数据

    /**
     * @brief 相等运算符重载
     * @param other 要比较的另一个ConfigResponse对象
     * @return 如果两个对象的config成员相等则返回true
     */
    bool operator==(const ConfigResponse& other) const {
        return config == other.config;
    }
};

/**
 * @brief 文件列表响应数据结构
 */
struct FileListResponse {
    std::vector<std::string> files;  ///< 文件列表
    std::vector<std::string> errors; ///< 错误信息列表

    /**
     * @brief 相等运算符重载
     * @param other 要比较的另一个FileListResponse对象
     * @return 如果两个对象的files和errors成员都相等则返回true
     */
    bool operator==(const FileListResponse& other) const {
        return files == other.files && errors == other.errors;
    }
};

} // namespace web
} // namespace xxcnc