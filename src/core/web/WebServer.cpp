#include <xxcnc/web/WebServer.h>
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace xxcnc {
namespace web {

class WebServerImpl {
friend class WebServer;
public:
    WebServerImpl() : server_(nullptr), 
        status_callback_([]() -> nlohmann::json { return {"error", "Status callback not set"}; }),
        command_callback_([](const nlohmann::json& cmd [[maybe_unused]]) -> nlohmann::json { return {"error", "Command callback not set"}; }),
        file_upload_callback_([](const std::string& filename [[maybe_unused]], const std::string& content [[maybe_unused]]) -> nlohmann::json { return {"error", "File upload callback not set"}; }),
        config_callback_([](const nlohmann::json& cfg [[maybe_unused]]) -> nlohmann::json { return {"error", "Config callback not set"}; }) {}
    ~WebServerImpl() {
        if (server_) {
            server_->stop();
        }
    }

    bool start(const std::string& host, int port) {
        try {
            server_ = std::make_unique<httplib::Server>();

            // 设置基本路由
            setupRoutes();

            // 启动服务器
            if (!server_->listen(host.c_str(), port)) {
                spdlog::error("Failed to start web server on {}:{}", host, port);
                return false;
            }

            spdlog::info("Web server started on {}:{}", host, port);
            return true;
        } catch (const std::exception& e) {
            spdlog::error("Exception in starting web server: {}", e.what());
            return false;
        }
    }

    void stop() {
        if (server_) {
            server_->stop();
            spdlog::info("Web server stopped");
        }
    }

private:
    void setupRoutes() {
        // 配置静态文件服务
        server_->set_mount_point("/static", "./src/core/web/static", httplib::Headers());

        // 处理根路径，返回index.html
        server_->Get("/", [](const httplib::Request&, httplib::Response& res) {
            res.set_header("Content-Type", "text/html");
            std::ifstream file("./src/core/web/static/index.html");
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                res.set_content(buffer.str(), "text/html");
            } else {
                res.status = 404;
                res.set_content("Not Found", "text/plain");
            }
        });

        // SSE处理
        server_->Get("/api/events", [this](const httplib::Request&, httplib::Response& res) {
            res.set_header("Content-Type", "text/event-stream");
            res.set_header("Cache-Control", "no-cache");
            res.set_header("Connection", "keep-alive");
            res.set_header("Access-Control-Allow-Origin", "*");

            res.set_content_provider(
                "text/event-stream",
                [this](size_t, httplib::DataSink& sink) {
                    auto status = status_callback_();
                    std::string event = "data: " + status.dump() + "\n\n";
                    sink.write(event.c_str(), event.size());
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    return true;
                }
            );
        });

        // 健康检查接口
        server_->Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
            res.set_content(R"({"status":"ok"})", "application/json");
        });

        // 系统状态接口
        server_->Get("/api/status", [this](const httplib::Request&, httplib::Response& res) {
            auto status = status_callback_();
            res.set_content(status.dump(), "application/json");
        });

        // 控制指令接口
        server_->Post("/api/command", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                auto command = nlohmann::json::parse(req.body);
                auto result = command_callback_(command);
                res.set_content(result.dump(), "application/json");
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", e.what()}}).dump(), "application/json");
            }
        });

        // 文件上传接口
        server_->Post("/api/file/upload", [this](const httplib::Request& req, httplib::Response& res) {
            if (!req.has_file("file")) {
                res.status = 400;
                res.set_content(R"({"error":"No file uploaded"})", "application/json");
                return;
            }
            auto file = req.get_file_value("file");
            auto result = file_upload_callback_(file.filename, file.content);
            res.set_content(result.dump(), "application/json");
        });

        // 配置管理接口
        server_->Post("/api/config", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                auto config = nlohmann::json::parse(req.body);
                auto result = config_callback_(config);
                res.set_content(result.dump(), "application/json");
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(nlohmann::json({{"error", e.what()}}).dump(), "application/json");
            }
        });
    }

    std::unique_ptr<httplib::Server> server_;
    std::function<nlohmann::json()> status_callback_;
    std::function<nlohmann::json(const nlohmann::json&)> command_callback_;
    std::function<nlohmann::json(const std::string&, const std::string&)> file_upload_callback_;
    std::function<nlohmann::json(const nlohmann::json&)> config_callback_;
};

WebServer::WebServer() : impl_(std::make_unique<WebServerImpl>()) {}

WebServer::~WebServer() = default;

bool WebServer::start(const std::string& host, int port) {
    return impl_->start(host, port);
}

void WebServer::stop() {
    impl_->stop();
}

void WebServer::setStatusCallback(std::function<nlohmann::json()> callback) {
    impl_->status_callback_ = std::move(callback);
}

void WebServer::setCommandCallback(std::function<nlohmann::json(const nlohmann::json&)> callback) {
    impl_->command_callback_ = std::move(callback);
}

void WebServer::setFileUploadCallback(std::function<nlohmann::json(const std::string&, const std::string&)> callback) {
    impl_->file_upload_callback_ = std::move(callback);
}

void WebServer::setConfigCallback(std::function<nlohmann::json(const nlohmann::json&)> callback) {
    impl_->config_callback_ = std::move(callback);
}

} // namespace web
} // namespace xxcnc