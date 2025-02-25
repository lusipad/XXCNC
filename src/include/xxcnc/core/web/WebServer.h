#pragma once

#include <string>
#include <memory>
#include <functional>
#include <optional>
#include <nlohmann/json.hpp>
#include "WebTypes.h"

namespace xxcnc {
namespace web {

class WebServerImpl;
class WebAPI;

class WebServer {
public:
    using json = nlohmann::json;
    using StatusCallback = std::function<json()>;
    using CommandCallback = std::function<json(const json&)>;
    using FileUploadCallback = std::function<json(const std::string&, const std::string&)>;
    using FileParseCallback = std::function<json(const std::string&)>;
    using ConfigCallback = std::function<json(const json&)>;

    WebServer();
    explicit WebServer(std::shared_ptr<WebAPI> api);
    ~WebServer();

    // 禁用拷贝
    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;

    // 配置服务器
    void setStaticDir(const std::string& dir);
    void setEnableCors(bool enable);
    bool start(const std::string& host, int port);
    void stop();

    // 设置回调函数
    void setStatusCallback(StatusCallback callback);
    void setCommandCallback(CommandCallback callback);
    void setFileUploadCallback(FileUploadCallback callback);
    void setFileParseCallback(FileParseCallback callback);
    void setConfigCallback(ConfigCallback callback);

    // 获取回调函数
    const std::optional<StatusCallback>& getStatusCallback() const { return statusCallback_; }
    const std::optional<CommandCallback>& getCommandCallback() const { return commandCallback_; }
    const std::optional<FileUploadCallback>& getFileUploadCallback() const { return fileUploadCallback_; }
    const std::optional<FileParseCallback>& getFileParseCallback() const { return fileParseCallback_; }
    const std::optional<ConfigCallback>& getConfigCallback() const { return configCallback_; }

    // 配置方法
    void setHost(const std::string& host) { host_ = host; }
    void setPort(int port) { port_ = port; }
    std::string getHost() const { return host_; }
    int getPort() const { return port_; }
    std::string getStaticDir() const { return staticDir_; }
    bool getEnableCors() const { return enableCors_; }

private:
    std::unique_ptr<WebServerImpl> impl_;
    std::shared_ptr<WebAPI> api_;
    std::optional<StatusCallback> statusCallback_;
    std::optional<CommandCallback> commandCallback_;
    std::optional<FileUploadCallback> fileUploadCallback_;
    std::optional<FileParseCallback> fileParseCallback_;
    std::optional<ConfigCallback> configCallback_;
    std::string host_;
    int port_;
    std::string staticDir_;
    bool enableCors_;

    friend class WebServerImpl;
};

} // namespace web
} // namespace xxcnc