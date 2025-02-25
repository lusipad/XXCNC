#include "xxcnc/core/web/WebServer.h"
#include "xxcnc/core/web/WebAPI.h"
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <filesystem>

namespace xxcnc {
namespace web {

class WebServerImpl {
public:
    WebServerImpl(WebServer& server) : server_(server) {}

    void setStaticDir(const std::string& dir) {
        static_dir_ = dir;
        if (!static_dir_.empty()) {
            http_server_.set_mount_point("/", static_dir_);
        }
    }

    void setEnableCors(bool enable) {
        enable_cors_ = enable;
        if (enable_cors_) {
            http_server_.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
                res.set_header("Access-Control-Allow-Headers", "Content-Type");
                if (req.method == "OPTIONS") {
                    res.status = 204;
                    return httplib::Server::HandlerResponse::Handled;
                }
                return httplib::Server::HandlerResponse::Unhandled;
            });
        }
    }

    bool start(const std::string& host, int port) {
        spdlog::info("WebServerImpl::start - 设置路由");
        setupRoutes();
        spdlog::info("WebServerImpl::start - 设置路由完成");
        spdlog::info("WebServerImpl::start - 正在启动服务器: {}:{}", host, port);
        
        server_.setHost(host);
        server_.setPort(port);
        
        try {
            // 确保静态目录已经设置
            if (!static_dir_.empty()) {
                spdlog::info("WebServerImpl::start - 已设置静态目录: {}", static_dir_);
                http_server_.set_mount_point("/", static_dir_);
            }
            
            // 配置服务器选项
            http_server_.set_payload_max_length(1024 * 1024 * 10); // 设置最大上传文件大小 (10MB)
            spdlog::info("WebServerImpl::start - 服务器参数设置完成");
            
            // 启动服务器
            spdlog::info("WebServerImpl::start - 调用http_server_.listen({}, {})", host, port);
            auto ret = http_server_.listen(host, port);
            if (!ret) {
                spdlog::error("WebServerImpl::start - 服务器启动失败: {}:{}", host, port);
            } else {
                spdlog::info("WebServerImpl::start - 服务器启动成功: {}:{}", host, port);
            }
            return ret;
        } catch (const std::exception& e) {
            spdlog::error("WebServerImpl::start - 服务器启动异常: {}", e.what());
            return false;
        } catch (...) {
            spdlog::error("WebServerImpl::start - 服务器启动未知异常");
            return false;
        }
    }

    void stop() {
        http_server_.stop();
    }

    void setStatusCallback(WebServer::StatusCallback callback) {
        status_callback_ = std::move(callback);
    }

    void setCommandCallback(WebServer::CommandCallback callback) {
        command_callback_ = std::move(callback);
    }

    void setFileUploadCallback(WebServer::FileUploadCallback callback) {
        file_upload_callback_ = std::move(callback);
    }

    void setFileParseCallback(WebServer::FileParseCallback callback) {
        file_parse_callback_ = std::move(callback);
    }

    void setConfigCallback(WebServer::ConfigCallback callback) {
        config_callback_ = std::move(callback);
    }

private:
    void setupRoutes() {
        // 状态API
        http_server_.Get("/api/status", [this](const httplib::Request&, httplib::Response& res) {
            try {
                // 设置响应头
                res.set_header("Content-Type", "application/json");
                
                if (const auto& callback = server_.getStatusCallback(); callback) {
                    res.set_content((*callback)().dump(), "application/json");
                } else if (server_.api_) {
                    auto status = server_.api_->getSystemStatus();
                    
                    // 构建轨迹点数组
                    nlohmann::json trajectoryPointsJson = nlohmann::json::array();
                    
                    // 如果有轨迹点，添加到JSON数组
                    if (!status.trajectoryPoints.empty()) {
                        spdlog::info("发现 {} 个轨迹点", status.trajectoryPoints.size());
                        for (const auto& point : status.trajectoryPoints) {
                            trajectoryPointsJson.push_back({
                                {"x", point.x},
                                {"y", point.y},
                                {"z", point.z},
                                {"isRapid", point.isRapid},
                                {"command", point.command}
                            });
                        }
                    }
                    
                    // 生成JSON响应
                    nlohmann::json responseJson = {
                        {"state", status.status},
                        {"position", {
                            {"x", status.position.x},
                            {"y", status.position.y},
                            {"z", status.position.z}
                        }},
                        {"feedRate", status.feedRate},
                        {"progress", status.progress},
                        {"currentFile", status.currentFile}
                    };
                    
                    // 添加machining字段，包含轨迹点
                    if (status.status == "machining" || !trajectoryPointsJson.empty()) {
                        responseJson["machining"] = {
                            {"progress", status.progress},
                            {"trajectoryPoints", trajectoryPointsJson}
                        };
                    }
                    
                    // 输出调试信息
                    spdlog::info("状态API响应: {}", responseJson.dump());
                    
                    // 设置响应内容
                    res.set_content(responseJson.dump(), "application/json");
                } else {
                    res.status = 503;
                    res.set_content(R"({"error":"Service unavailable"})", "application/json");
                }
            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content(R"({"error":"Internal server error","message":")" + std::string(e.what()) + "\"}", "application/json");
                spdlog::error("Status error: {}", e.what());
            }
        });

        // 命令API
        http_server_.Post("/api/command", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                spdlog::info("接收到命令请求，请求体: {}", req.body);
                
                if (req.body.empty()) {
                    res.status = 400;
                    res.set_content(R"({"error":"Empty request body"})", "application/json");
                    return;
                }
                
                nlohmann::json cmd = nlohmann::json::parse(req.body);
                
                if (!cmd.contains("command") || !cmd["command"].is_string()) {
                    res.status = 400;
                    res.set_content(R"({"error":"Invalid request format. 'command' field is required and must be a string."})", "application/json");
                    return;
                }
                
                spdlog::info("解析命令: {}", cmd["command"].get<std::string>());
                
                if (const auto& callback = server_.getCommandCallback(); callback) {
                    auto response = (*callback)(cmd);
                    res.set_content(response.dump(), "application/json");
                } else if (server_.api_) {
                    bool success = server_.api_->executeCommand(cmd);
                    res.set_content(nlohmann::json{{"success", success}}.dump(), "application/json");
                } else {
                    res.status = 503;
                    res.set_content(R"({"error":"Service unavailable"})", "application/json");
                }
            } catch (const nlohmann::json::parse_error& e) {
                spdlog::error("JSON解析错误: {}", e.what());
                res.status = 400;
                res.set_content(R"({"error":"Invalid JSON format","message":")" + std::string(e.what()) + "\"}", "application/json");
            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content(R"({"error":"Internal server error","message":")" + std::string(e.what()) + "\"}", "application/json");
                spdlog::error("Command error: {}", e.what());
            }
        });

        // 文件列表API
        http_server_.Get("/api/files", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                if (const auto& callback = server_.getFileParseCallback(); callback) {
                    auto path = req.get_param_value("path");
                    if (path.empty()) {
                        path = "/";
                    }
                    auto response = (*callback)(path);
                    res.set_content(response.dump(), "application/json");
                } else if (server_.api_) {
                    auto path = req.get_param_value("path");
                    if (path.empty()) {
                        path = "/";
                    }
                    auto files = server_.api_->getFileList(path);
                    nlohmann::json response;
                    response["files"] = nlohmann::json::array();
                    response["folders"] = nlohmann::json::array();
                    response["errors"] = nlohmann::json::array();
                    for (const auto& file : files.files) {
                        response["files"].push_back(file);
                    }
                    for (const auto& folder : files.folders) {
                        response["folders"].push_back(folder);
                    }
                    for (const auto& error : files.errors) {
                        response["errors"].push_back(error);
                    }
                    res.set_content(response.dump(), "application/json");
                } else {
                    res.status = 503;
                    res.set_content(R"({"error":"Service unavailable"})", "application/json");
                }
            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content(R"({"error":"Internal server error","message":")" + std::string(e.what()) + "\"}", "application/json");
                spdlog::error("Files error: {}", e.what());
            }
        });

        // 文件上传API
        http_server_.Post("/api/files", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                spdlog::info("接收到文件上传请求");
                
                // 检查Content-Type
                auto content_type = req.get_header_value("Content-Type");
                spdlog::info("Content-Type: {}", content_type);
                
                if (!req.has_file("file")) {
                    spdlog::error("请求中没有文件");
                    for (const auto& [key, val] : req.files) {
                        spdlog::info("表单字段: {} = {}", key, val.filename);
                    }
                    
                    for (const auto& [key, val] : req.params) {
                        spdlog::info("参数: {} = {}", key, val);
                    }
                    
                    res.status = 400;
                    res.set_content(R"({"error":"No file uploaded"})", "application/json");
                    return;
                }
                
                const auto& file = req.get_file_value("file");
                spdlog::info("获取到文件: {}, 大小: {} 字节", file.filename, file.content.size());
                
                // 检查是否有原始文件名参数
                std::string filename = file.filename;
                if (req.has_param("originalFilename")) {
                    std::string originalFilename = req.get_param_value("originalFilename");
                    spdlog::info("获取到原始文件名: {}", originalFilename);
                    filename = originalFilename;
                }
                
                if (const auto& callback = server_.getFileUploadCallback(); callback) {
                    auto response = (*callback)(filename, file.content);
                    res.set_content(response.dump(), "application/json");
                } else if (server_.api_) {
                    auto response = server_.api_->uploadFile(filename, file.content);
                    nlohmann::json json_response = {
                        {"success", response.success}
                    };
                    if (!response.success) {
                        json_response["error"] = response.error;
                    }
                    res.set_content(json_response.dump(), "application/json");
                } else {
                    res.status = 503;
                    res.set_content(R"({"error":"Service unavailable"})", "application/json");
                }
            } catch (const std::exception& e) {
                spdlog::error("文件上传异常: {}", e.what());
                res.status = 500;
                res.set_content(R"({"error":"Internal server error","message":")" + std::string(e.what()) + "\"}", "application/json");
                spdlog::error("File upload error: {}", e.what());
            }
        });

        // 文件解析API
        http_server_.Get(R"(/api/files/([^/]+)/parse)", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                std::string filename = req.matches[1];
                spdlog::info("解析文件: {}", filename);
                if (const auto& callback = server_.getFileParseCallback(); callback) {
                    auto response = (*callback)(filename);
                    res.set_content(response.dump(), "application/json");
                } else if (server_.api_) {
                    auto response = server_.api_->parseFile(filename);
                    nlohmann::json json_response = {
                        {"success", response.success},
                        {"toolPathDetails", response.toolPathDetails}
                    };
                    if (!response.success) {
                        json_response["error"] = response.error;
                    } else {
                        json_response["trajectoryPoints"] = nlohmann::json::array();
                        for (const auto& point : response.trajectoryPoints) {
                            json_response["trajectoryPoints"].push_back({
                                {"x", point.x},
                                {"y", point.y},
                                {"z", point.z},
                                {"type", point.isRapid ? "rapid" : "feed"},
                                {"command", point.command}
                            });
                        }
                    }
                    res.set_content(json_response.dump(), "application/json");
                } else {
                    res.status = 503;
                    res.set_content(R"({"error":"Service unavailable"})", "application/json");
                }
            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content(R"({"error":"Internal server error","message":")" + std::string(e.what()) + "\"}", "application/json");
                spdlog::error("File parse error: {}", e.what());
            }
        });

        // 配置API - 没有实现
        http_server_.Get("/api/config", [this](const httplib::Request&, httplib::Response& res) {
            try {
                spdlog::info("获取配置");
                if (const auto& callback = server_.getConfigCallback(); callback) {
                    auto response = (*callback)(nlohmann::json::object());
                    res.set_content(response.dump(), "application/json");
                } else {
                    res.status = 503;
                    res.set_content(R"({"error":"Service unavailable"})", "application/json");
                }
            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content(R"({"error":"Internal server error","message":")" + std::string(e.what()) + "\"}", "application/json");
                spdlog::error("Config error: {}", e.what());
            }
        });
    }

    WebServer& server_;
    httplib::Server http_server_;
    std::string static_dir_;
    bool enable_cors_ = false;
    std::optional<WebServer::StatusCallback> status_callback_;
    std::optional<WebServer::CommandCallback> command_callback_;
    std::optional<WebServer::FileUploadCallback> file_upload_callback_;
    std::optional<WebServer::FileParseCallback> file_parse_callback_;
    std::optional<WebServer::ConfigCallback> config_callback_;
};

WebServer::WebServer() : impl_(std::make_unique<WebServerImpl>(*this)), host_("0.0.0.0"), port_(8080), enableCors_(false) {}

WebServer::WebServer(std::shared_ptr<WebAPI> api) : impl_(std::make_unique<WebServerImpl>(*this)), api_(api), host_("0.0.0.0"), port_(8080), enableCors_(false) {}

WebServer::~WebServer() = default;

void WebServer::setStaticDir(const std::string& dir) {
    staticDir_ = dir;
    impl_->setStaticDir(dir);
}

void WebServer::setEnableCors(bool enable) {
    enableCors_ = enable;
    impl_->setEnableCors(enable);
}

bool WebServer::start(const std::string& host, int port) {
    host_ = host;
    port_ = port;
    return impl_->start(host, port);
}

void WebServer::stop() {
    impl_->stop();
}

void WebServer::setStatusCallback(StatusCallback callback) {
    statusCallback_ = std::move(callback);
    impl_->setStatusCallback(std::move(callback));
}

void WebServer::setCommandCallback(CommandCallback callback) {
    commandCallback_ = std::move(callback);
    impl_->setCommandCallback(std::move(callback));
}

void WebServer::setFileUploadCallback(FileUploadCallback callback) {
    fileUploadCallback_ = std::move(callback);
    impl_->setFileUploadCallback(std::move(callback));
}

void WebServer::setFileParseCallback(FileParseCallback callback) {
    fileParseCallback_ = std::move(callback);
    impl_->setFileParseCallback(std::move(callback));
}

void WebServer::setConfigCallback(ConfigCallback callback) {
    configCallback_ = std::move(callback);
    impl_->setConfigCallback(std::move(callback));
}

} // namespace web
} // namespace xxcnc