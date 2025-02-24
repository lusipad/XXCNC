#pragma once

#include <memory>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>

namespace xxcnc {
namespace web {

class WebServerImpl;

/**
 * @brief Web服务器类，提供HTTP和WebSocket服务
 */
class WebServer {
public:
    WebServer();
    ~WebServer();

    /**
     * @brief 启动Web服务器
     * @param host 服务器主机地址
     * @param port 服务器端口
     * @return 启动是否成功
     */
    bool start(const std::string& host, int port);

    /**
     * @brief 停止Web服务器
     */
    void stop();

    /**
     * @brief 设置状态回调函数
     * @param callback 状态回调函数
     */
    void setStatusCallback(std::function<nlohmann::json()> callback);

    /**
     * @brief 设置命令回调函数
     * @param callback 命令回调函数
     */
    void setCommandCallback(std::function<nlohmann::json(const nlohmann::json&)> callback);

    /**
     * @brief 设置文件上传回调函数
     * @param callback 文件上传回调函数
     */
    void setFileUploadCallback(std::function<nlohmann::json(const std::string&, const std::string&)> callback);

    /**
     * @brief 设置配置回调函数
     * @param callback 配置回调函数
     */
    void setConfigCallback(std::function<nlohmann::json(const nlohmann::json&)> callback);

private:
    std::unique_ptr<WebServerImpl> impl_;
};

} // namespace web
} // namespace xxcnc