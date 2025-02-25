#include <xxcnc/core/web/WebServer.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>

using namespace xxcnc::web;
using json = nlohmann::json;

// 模拟系统状态
struct SystemStatus {
    bool running = false;
    double x_pos = 0.0;
    double y_pos = 0.0;
    double z_pos = 0.0;
};

int main() {
    WebServer server;
    SystemStatus status;

    // 设置服务器配置
    xxcnc::web::WebServer::Config config;
    config.static_dir = "static";  // 相对于执行目录的路径
    server.setConfig(config);

    // 设置状态回调
    server.setStatusCallback([&status]() -> json {
        return json({
            {"running", status.running},
            {"position", {
                {"x", status.x_pos},
                {"y", status.y_pos},
                {"z", status.z_pos}
            }}
        });
    });

    // 设置命令回调
    server.setCommandCallback([&status](const json& cmd) -> json {
        try {
            if (cmd["type"] == "start") {
                status.running = true;
                return {"status", "success"};
            } else if (cmd["type"] == "stop") {
                status.running = false;
                return {"status", "success"};
            } else if (cmd["type"] == "move") {
                status.x_pos = cmd["x"].get<double>();
                status.y_pos = cmd["y"].get<double>();
                status.z_pos = cmd["z"].get<double>();
                return {"status", "success"};
            }
            return {"error", "Unknown command"};
        } catch (const std::exception& e) {
            return {"error", e.what()};
        }
    });

    // 设置文件上传回调
    server.setFileUploadCallback([](const std::string& filename, const std::string& content) -> json {
        std::cout << "Received file: " << filename << ", size: " << content.size() << " bytes" << std::endl;
        return {"status", "success"};
    });

    // 设置配置回调
    server.setConfigCallback([](const json& cfg) -> json {
        std::cout << "Received config: " << cfg.dump(2) << std::endl;
        return {"status", "success"};
    });

    // 启动服务器
    if (!server.start("0.0.0.0", 8080)) {
        std::cerr << "Failed to start web server" << std::endl;
        return 1;
    }

    std::cout << "Web server started at http://localhost:8080" << std::endl;
    std::cout << "Available endpoints:" << std::endl;
    std::cout << "  GET  /api/health  - Health check" << std::endl;
    std::cout << "  GET  /api/status  - Get system status" << std::endl;
    std::cout << "  POST /api/command - Send commands" << std::endl;
    std::cout << "  POST /api/file/upload - Upload files" << std::endl;
    std::cout << "  POST /api/config  - Update configuration" << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;

    // 保持程序运行
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}