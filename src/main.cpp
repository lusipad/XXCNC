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
#include "xxcnc/core/web/RealWebAPI.h"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace xxcnc::web;
using json = nlohmann::json;

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

        // 创建 RealWebAPI 实例
        auto api = std::make_shared<RealWebAPI>();
        spdlog::info("Created RealWebAPI instance");

        // 创建 WebServer 实例
        xxcnc::web::WebServer server(api);
        
        // 设置静态目录和上传目录
        std::filesystem::path static_dir;
        std::filesystem::path uploads_dir;
        
        // 获取可执行文件路径
        std::filesystem::path exe_path;
        try {
            #ifdef _WIN32
            char path[MAX_PATH] = { 0 };
            GetModuleFileNameA(NULL, path, MAX_PATH);
            exe_path = std::filesystem::path(path).parent_path();
            static_dir = exe_path / "static";  // 可执行文件目录下的static
            uploads_dir = exe_path / "uploads"; // 可执行文件目录下的uploads
            spdlog::info("可执行文件路径: {}", exe_path.string());
            #else
            // 在非Windows系统上使用当前路径
            exe_path = std::filesystem::current_path();
            static_dir = exe_path / "static";
            uploads_dir = exe_path / "uploads";
            #endif
            
            spdlog::info("静态文件目录: {}", static_dir.string());
            spdlog::info("上传文件目录: {}", uploads_dir.string());
        } catch (const std::exception& e) {
            spdlog::error("无法获取可执行文件路径: {}", e.what());
            std::cerr << "无法获取可执行文件路径: " << e.what() << std::endl;
            return 1;
        }
        
        // 检查静态文件目录是否存在
        if (!std::filesystem::exists(static_dir)) {
            spdlog::error("找不到静态文件目录: {}", static_dir.string());
            std::cerr << "找不到静态文件目录: " << static_dir.string() << std::endl;
            
            // 输出可执行文件目录内容
            spdlog::info("可执行文件目录内容:");
            for (const auto& entry : std::filesystem::directory_iterator(exe_path)) {
                spdlog::info("  - {}", entry.path().string());
            }
            
            return 1;
        }
        
        spdlog::info("使用静态文件目录: {}", static_dir.string());
        std::cout << "使用静态文件目录: " << static_dir.string() << std::endl;
        
        server.setStaticDir(static_dir.string());
        
        // 确保uploads目录存在
        if (!std::filesystem::exists(uploads_dir)) {
            spdlog::info("创建uploads目录: {}", uploads_dir.string());
            try {
                std::filesystem::create_directories(uploads_dir);
            } catch (const std::exception& e) {
                spdlog::error("创建uploads目录失败: {}", e.what());
                std::cerr << "创建uploads目录失败: " << e.what() << std::endl;
                return 1;
            }
        }
        
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
