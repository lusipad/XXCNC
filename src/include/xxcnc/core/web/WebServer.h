#pragma once

#include <string>
#include <memory>
#include <functional>
#include <optional>
#include <nlohmann/json.hpp>
#include "WebAPI.h"
#include "WebTypes.h"

namespace xxcnc::web {

class WebServerImpl;

class WebServer {
public:
    // 服务器配置结构体
    struct Config {
        std::string static_dir = "./static";  // 静态文件目录
        std::string host = "0.0.0.0";         // 监听地址
        int port = 8080;                      // 监听端口
        bool enable_cors = false;             // 是否启用CORS
    };

    using json = nlohmann::json;
    using StatusCallback = std::function<json()>;
    using CommandCallback = std::function<json(const json&)>;
    using FileUploadCallback = std::function<json(const std::string&, const std::string&)>;
    using ConfigCallback = std::function<json(const json&)>;

    WebServer();  // 添加默认构造函数
    explicit WebServer(std::shared_ptr<WebAPI> api);
    ~WebServer();

    // 禁用拷贝
    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;

    bool start(const std::string& host, int port);
    void stop();

    // 设置回调函数
    void setStatusCallback(StatusCallback callback);
    void setCommandCallback(CommandCallback callback);
    void setFileUploadCallback(FileUploadCallback callback);
    void setConfigCallback(ConfigCallback callback);

    // API处理函数
    StatusResponse handleStatusRequest() {
        return m_api->getSystemStatus();
    }

    bool handleCommandRequest(const std::string& command) {
        return m_api->executeCommand(command);
    }

    FileListResponse handleFileListRequest(const std::string& path) {
        return m_api->getFileList(path);
    }

    ConfigResponse handleConfigRequest() {
        return m_api->getConfig();
    }

    bool handleConfigUpdateRequest(const ConfigData& config) {
        return m_api->updateConfig(config);
    }

    // 回调函数访问方法
    const std::optional<StatusCallback>& getStatusCallback() const { return statusCallback_; }
    const std::optional<CommandCallback>& getCommandCallback() const { return commandCallback_; }
    const std::optional<FileUploadCallback>& getFileUploadCallback() const { return fileUploadCallback_; }
    const std::optional<ConfigCallback>& getConfigCallback() const { return configCallback_; }

    void setConfig(const Config& config) { config_ = config; }
    const Config& getConfig() const { return config_; }

private:
    std::shared_ptr<WebAPI> m_api;
    std::unique_ptr<WebServerImpl> impl_;

    // 回调函数
    std::optional<StatusCallback> statusCallback_;
    std::optional<CommandCallback> commandCallback_;
    std::optional<FileUploadCallback> fileUploadCallback_;
    std::optional<ConfigCallback> configCallback_;

    Config config_;

    friend class WebServerImpl;  // 添加友元声明
};

} // namespace xxcnc::web