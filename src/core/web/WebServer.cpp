#include "xxcnc/core/web/WebServer.h"
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <filesystem>

namespace xxcnc::web {

class WebServerImpl {
public:
    WebServerImpl(std::shared_ptr<WebAPI> api, WebServer& server) 
        : api_(api)
        , server_(server)
        , http_server_() {
        spdlog::info("Initializing web server");
        setupRoutes();
    }

    bool start(const std::string& host, int port) {
        spdlog::info("Starting web server on {}:{}", host, port);
        
        // 检查静态文件目录是否存在
        if (!std::filesystem::exists(server_.getConfig().static_dir)) {
            spdlog::error("Static directory '{}' does not exist", server_.getConfig().static_dir);
            return false;
        }

        // 设置静态文件目录
        http_server_.set_mount_point("/", server_.getConfig().static_dir);

        // 设置CORS
        if (server_.getConfig().enable_cors) {
            http_server_.set_default_headers({
                {"Access-Control-Allow-Origin", "*"},
                {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
                {"Access-Control-Allow-Headers", "Content-Type, Authorization"},
            });
        }

        // 注意：当前版本的 httplib 使用单线程模式
        // TODO: 在未来版本中添加线程池支持
        spdlog::info("Running in single-thread mode");

        // 启动服务器
        return http_server_.listen(host.c_str(), port);
    }

    void stop() {
        spdlog::info("Stopping web server");
        http_server_.stop();
    }

private:
    void setupRoutes() {
        // 健康检查
        http_server_.Get("/health", [](const httplib::Request&, httplib::Response& res) {
            res.set_content(R"({"status":"ok"})", "application/json");
        });

        // API 路由
        http_server_.Get("/api/status", [this](const httplib::Request&, httplib::Response& res) {
            try {
                if (const auto& callback = server_.getStatusCallback(); callback) {
                    res.set_content((*callback)().dump(), "application/json");
                } else if (api_) {
                    auto status = api_->getSystemStatus();
                    res.set_content(statusToJson(status).dump(), "application/json");
                } else {
                    res.status = 503;
                    res.set_content(R"({"error":"Service unavailable"})", "application/json");
                }
            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content(R"({"error":"Internal server error","message":")" + std::string(e.what()) + "\"}", "application/json");
                spdlog::error("Status API error: {}", e.what());
            }
        });

        http_server_.Post("/api/command", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                if (const auto& callback = server_.getCommandCallback(); callback) {
                    auto json_req = nlohmann::json::parse(req.body);
                    res.set_content((*callback)(json_req).dump(), "application/json");
                } else if (api_) {
                    bool success = api_->executeCommand(req.body);
                    res.set_content(commandResultToJson(success).dump(), "application/json");
                } else {
                    res.status = 503;
                    res.set_content(R"({"error":"Service unavailable"})", "application/json");
                }
            } catch (const nlohmann::json::exception& e) {
                res.status = 400;
                res.set_content(R"({"error":"Invalid JSON format","message":")" + std::string(e.what()) + "\"}", "application/json");
                spdlog::error("Command API JSON error: {}", e.what());
            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content(R"({"error":"Internal server error","message":")" + std::string(e.what()) + "\"}", "application/json");
                spdlog::error("Command API error: {}", e.what());
            }
        });

        http_server_.Get("/api/files", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                if (const auto& callback = server_.getFileUploadCallback(); callback) {
                    auto path = req.get_param_value("path");
                    res.set_content((*callback)(path, "").dump(), "application/json");
                } else if (api_) {
                    auto path = req.get_param_value("path");
                    auto files = api_->getFileList(path);
                    res.set_content(fileListToJson(files).dump(), "application/json");
                } else {
                    res.status = 503;
                    res.set_content(R"({"error":"Service unavailable"})", "application/json");
                }
            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content(R"({"error":"Internal server error","message":")" + std::string(e.what()) + "\"}", "application/json");
                spdlog::error("Files API error: {}", e.what());
            }
        });

        // 文件上传
        http_server_.Post("/api/files", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                if (!req.has_file("file")) {
                    res.status = 400;
                    res.set_content(R"({"error":"No file uploaded"})", "application/json");
                    return;
                }

                const auto& file = req.get_file_value("file");
                if (const auto& callback = server_.getFileUploadCallback(); callback) {
                    res.set_content((*callback)(file.filename, file.content).dump(), "application/json");
                } else if (api_) {
                    auto result = api_->uploadFile(file.filename, file.content);
                    res.set_content(nlohmann::json{
                        {"success", result.success},
                        {"error", result.error}
                    }.dump(), "application/json");
                } else {
                    res.status = 503;
                    res.set_content(R"({"error":"Service unavailable"})", "application/json");
                }
            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content(R"({"error":"Internal server error","message":")" + std::string(e.what()) + "\"}", "application/json");
                spdlog::error("File upload error: {}", e.what());
            }
        });

        // 文件解析
        http_server_.Get(R"(/api/files/([^/]+)/parse)", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                auto filename = req.matches[1];
                if (const auto& callback = server_.getFileParseCallback(); callback) {
                    res.set_content((*callback)(filename).dump(), "application/json");
                } else if (api_) {
                    auto result = api_->parseFile(filename);
                    res.set_content(nlohmann::json{
                        {"success", result.success},
                        {"toolPathDetails", result.toolPathDetails},
                        {"trajectoryPoints", result.trajectoryPoints},
                        {"error", result.error}
                    }.dump(), "application/json");
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

        // 默认路由 - 重定向到 index.html
        http_server_.Get("/", [](const httplib::Request&, httplib::Response& res) {
            res.set_redirect("/index.html");
        });

        // 404 处理
        http_server_.set_error_handler([](const httplib::Request&, httplib::Response& res) {
            if (res.status == 404) {
                res.set_content(R"({"error":"Not found"})", "application/json");
            }
        });
    }

    nlohmann::json statusToJson(const StatusResponse& status) const {
        return nlohmann::json{
            {"state", status.state},
            {"position", {
                {"x", status.position.x},
                {"y", status.position.y},
                {"z", status.position.z}
            }},
            {"feedRate", status.feedRate},
            {"currentFile", status.currentFile},
            {"progress", status.progress},
            {"errorCode", status.errorCode},
            {"messages", status.messages}
        };
    }

    nlohmann::json commandResultToJson(bool success) const {
        return nlohmann::json{
            {"success", success}
        };
    }

    nlohmann::json fileListToJson(const FileListResponse& files) const {
        return nlohmann::json{
            {"files", files.files},
            {"errors", files.errors}
        };
    }

    std::shared_ptr<WebAPI> api_;
    WebServer& server_;
    httplib::Server http_server_;
};

WebServer::WebServer()
    : impl_(std::make_unique<WebServerImpl>(nullptr, *this)) {
    spdlog::info("WebServer created without API");
}

WebServer::WebServer(std::shared_ptr<WebAPI> api)
    : m_api(std::move(api))
    , impl_(std::make_unique<WebServerImpl>(m_api, *this)) {
    spdlog::info("WebServer created with API");
}

WebServer::~WebServer() = default;

bool WebServer::start(const std::string& host, int port) {
    spdlog::info("Starting WebServer on {}:{}", host, port);
    bool result = impl_->start(host, port);
    if (result) {
        spdlog::info("WebServer started successfully");
    } else {
        spdlog::error("WebServer failed to start");
    }
    return result;
}

void WebServer::stop() {
    spdlog::info("Stopping WebServer");
    impl_->stop();
}

void WebServer::setStatusCallback(StatusCallback callback) {
    spdlog::info("Setting status callback");
    statusCallback_ = std::move(callback);
}

void WebServer::setCommandCallback(CommandCallback callback) {
    spdlog::info("Setting command callback");
    commandCallback_ = std::move(callback);
}

void WebServer::setFileUploadCallback(FileUploadCallback callback) {
    spdlog::info("Setting file upload callback");
    fileUploadCallback_ = std::move(callback);
}

void WebServer::setFileParseCallback(FileParseCallback callback) {
    spdlog::info("Setting file parse callback");
    fileParseCallback_ = std::move(callback);
}

void WebServer::setConfigCallback(ConfigCallback callback) {
    spdlog::info("Setting config callback");
    configCallback_ = std::move(callback);
}

const WebServer::StatusCallback& WebServer::getStatusCallback() const {
    return statusCallback_;
}

const WebServer::CommandCallback& WebServer::getCommandCallback() const {
    return commandCallback_;
}

const WebServer::FileUploadCallback& WebServer::getFileUploadCallback() const {
    return fileUploadCallback_;
}

const WebServer::FileParseCallback& WebServer::getFileParseCallback() const {
    return fileParseCallback_;
}

const WebServer::ConfigCallback& WebServer::getConfigCallback() const {
    return configCallback_;
}

} // namespace xxcnc::web