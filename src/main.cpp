#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "xxcnc/core/web/WebServer.h"
#include "xxcnc/core/web/WebTypes.h"
#include "xxcnc/core/web/MockWebAPI.h"

using namespace xxcnc::web;
using json = nlohmann::json;

// 模拟系统状态
struct SystemStatus {
    bool running = false;
    double x_pos = 0.0;
    double y_pos = 0.0;
    double z_pos = 0.0;
    double feed_rate = 100.0;  // 进给速度（百分比）
    double progress = 0.0;     // 执行进度（0-1）
    std::string current_file;  // 当前加载的文件
    bool emergency_stop = false;  // 紧急停止状态
};

// 解析G代码行，返回位置信息
struct GCodePosition {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    bool is_rapid = false;  // 是否为快速定位
};

GCodePosition parseGCodeLine(const std::string& line) {
    GCodePosition pos;
    std::istringstream iss(line);
    std::string word;
    
    // 检查是否为快速定位（G0）
    if (line.find("G0") != std::string::npos) {
        pos.is_rapid = true;
    }
    
    while (iss >> word) {
        if (word[0] == 'X') {
            pos.x = std::stod(word.substr(1));
        } else if (word[0] == 'Y') {
            pos.y = std::stod(word.substr(1));
        } else if (word[0] == 'Z') {
            pos.z = std::stod(word.substr(1));
        }
    }
    return pos;
}

int main() {
    try {
        std::cout << "Starting XXCNC server..." << std::endl;
        
        // 确保日志目录存在
        std::filesystem::create_directories("debug_logs");
        std::cout << "Created debug_logs directory" << std::endl;
        
        // 设置日志级别
        spdlog::set_level(spdlog::level::debug);
        // 设置日志输出到文件
        try {
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("debug_logs/server.log", true);
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            spdlog::logger logger("multi_sink", {console_sink, file_sink});
            logger.set_level(spdlog::level::debug);
            spdlog::set_default_logger(std::make_shared<spdlog::logger>(logger));
            std::cout << "Logging system initialized" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize logging system: " << e.what() << std::endl;
        }
        
        spdlog::info("Starting XXCNC server...");

        // 创建 MockWebAPI 实例
        auto api = std::make_shared<MockWebAPI>();
        spdlog::info("Created MockWebAPI instance");

        // 创建 WebServer 实例
        xxcnc::web::WebServer server(api);
        
        // 设置静态目录和上传目录
        auto static_dir = std::filesystem::current_path() / "static";
        spdlog::info("Static directory path: {}", static_dir.string());
        std::cout << "Static directory path: " << static_dir.string() << std::endl;

        if (!std::filesystem::exists(static_dir)) {
            spdlog::error("Static directory not found: {}", static_dir.string());
            std::cerr << "Static directory not found: " << static_dir.string() << std::endl;
            
            // 查找可能的静态目录位置
            std::cout << "Looking for static directory..." << std::endl;
            for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
                std::cout << "Found: " << entry.path().string() << std::endl;
            }
            
            return 1;
        }
        spdlog::info("Static directory exists");

        server.setStaticDir(static_dir.string());
        spdlog::info("Set static directory: {}", static_dir.string());
        std::cout << "Set static directory: " << static_dir.string() << std::endl;
        
        // 设置uploads目录
        auto uploads_dir = std::filesystem::current_path() / "uploads";
        spdlog::info("Uploads directory path: {}", uploads_dir.string());
        std::cout << "Uploads directory path: " << uploads_dir.string() << std::endl;
        
        if (!std::filesystem::exists(uploads_dir)) {
            spdlog::error("Uploads directory not found, creating it: {}", uploads_dir.string());
            std::cout << "Uploads directory not found, creating it: " << uploads_dir.string() << std::endl;
            try {
                std::filesystem::create_directories(uploads_dir);
                spdlog::info("Created uploads directory: {}", uploads_dir.string());
                std::cout << "Created uploads directory: " << uploads_dir.string() << std::endl;
            } catch (const std::exception& e) {
                spdlog::error("Failed to create uploads directory: {}", e.what());
                std::cerr << "Failed to create uploads directory: " << e.what() << std::endl;
                return 1;
            }
        }
        spdlog::info("Uploads directory exists: {}", uploads_dir.string());
        std::cout << "Uploads directory exists: " << uploads_dir.string() << std::endl;

        server.setEnableCors(true);
        spdlog::info("Enabled CORS");

        // 启动服务器
        std::cout << "Starting server on 0.0.0.0:8080..." << std::endl;
        spdlog::info("Starting server on 0.0.0.0:8080");
        
        bool server_started = false;
        try {
            server_started = server.start("0.0.0.0", 8080);
            if (server_started) {
                std::cout << "Server started successfully on 0.0.0.0:8080" << std::endl;
                spdlog::info("Server started successfully on 0.0.0.0:8080");
            } else {
                std::cerr << "Failed to start server on 0.0.0.0:8080" << std::endl;
                spdlog::error("Failed to start server on 0.0.0.0:8080");
                return 1;
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception occurred while starting server: " << e.what() << std::endl;
            spdlog::error("Exception occurred while starting server: {}", e.what());
            return 1;
        } catch (...) {
            std::cerr << "Unknown exception occurred while starting server" << std::endl;
            spdlog::error("Unknown exception occurred while starting server");
            return 1;
        }
        
        // 服务器运行中
        std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
        spdlog::info("Server is running. Press Ctrl+C to stop.");
        
        // 保持主线程运行，直到用户按下 Ctrl+C
        std::string cmd;
        std::getline(std::cin, cmd);
        
        // 停止服务器
        std::cout << "Stopping server..." << std::endl;
        spdlog::info("Stopping server");
        server.stop();
        std::cout << "Server stopped" << std::endl;
        spdlog::info("Server stopped");
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        spdlog::error("Unhandled exception: {}", e.what());
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        spdlog::error("Unknown exception occurred");
        return 1;
    }
}