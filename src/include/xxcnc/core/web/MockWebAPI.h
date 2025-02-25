#pragma once

#include "xxcnc/core/web/WebAPI.h"
#include <filesystem>
#include <vector>

namespace xxcnc {
namespace web {

class MockWebAPI : public WebAPI {
public:
    MockWebAPI() = default;
    ~MockWebAPI() override = default;

    // 状态监控API
    StatusResponse getSystemStatus() override {
        StatusResponse response;
        
        if (isProcessing) {
            response.status = "machining";
            
            // 模拟进度增加
            currentProgress += 0.01;
            if (currentProgress > 1.0) {
                currentProgress = 1.0;
                isProcessing = false;
            }
            
            response.progress = currentProgress;
            
            // 更新位置 - 模拟机器移动
            double angle = currentProgress * 2 * 3.14159;
            response.position.x = 10.0 * std::cos(angle);
            response.position.y = 10.0 * std::sin(angle);
            response.position.z = 0.0;
        } else {
            response.status = "idle";
            response.progress = 0.0;
            response.position = {0.0, 0.0, 0.0};
        }
        
        response.feedRate = 100.0;
        response.currentFile = "test.nc";
        response.errorCode = 0;
        
        // 添加轨迹点到消息中
        if (isProcessing) {
            response.messages.push_back("Processing trajectory");
            
            // 添加一些轨迹点到响应中
            // 这些轨迹点会在WebServer.cpp中被转换为JSON
            response.trajectoryPoints.clear();
            for (int i = 0; i < 10; i++) {
                TrajectoryPoint point;
                double pointAngle = currentProgress * 2 * 3.14159 * (i / 10.0);
                point.x = 10.0 * std::cos(pointAngle);
                point.y = 10.0 * std::sin(pointAngle);
                point.z = 0.0;
                point.isRapid = (i % 5 == 0);
                point.command = "G01";
                response.trajectoryPoints.push_back(point);
            }
        }
        
        return response;
    }

    // 控制指令API
    bool executeCommand(const std::string& command) override {
        spdlog::info("执行命令: {}", command);
        
        if (command == "motion.start") {
            // 模拟开始加工
            spdlog::info("开始加工");
            isProcessing = true;
            currentProgress = 0.0;
            
            // 生成一些模拟的轨迹点
            simulatedTrajectoryPoints.clear();
            for (int i = 0; i < 100; i++) {
                TrajectoryPoint point;
                point.x = 10.0 * std::cos(i * 0.1);
                point.y = 10.0 * std::sin(i * 0.1);
                point.z = 0.0;
                point.isRapid = (i % 10 == 0);
                point.command = "G01";
                simulatedTrajectoryPoints.push_back(point);
            }
            
            return true;
        } else if (command == "motion.stop") {
            // 模拟停止加工
            spdlog::info("停止加工");
            isProcessing = false;
            return true;
        }
        
        return true;
    }

    // 文件管理API
    FileListResponse getFileList(const std::string& /* path */) override {
        FileListResponse response;
        std::filesystem::path dir_path = std::filesystem::current_path() / "uploads";
        
        try {
            if (!std::filesystem::exists(dir_path)) {
                response.errors.push_back("目录不存在");
                return response;
            }

            for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
                if (entry.is_regular_file()) {
                    response.files.push_back(entry.path().filename().string());
                } else if (entry.is_directory()) {
                    response.folders.push_back(entry.path().filename().string());
                }
            }
        } catch (const std::exception& e) {
            response.errors.push_back(e.what());
        }

        return response;
    }

    // 文件上传API
    FileUploadResponse uploadFile(const std::string& filename, const std::string& content) override {
        FileUploadResponse response;
        try {
            spdlog::info("开始上传文件: {}, 内容大小: {} 字节", filename, content.size());
            
            std::filesystem::path current_path = std::filesystem::current_path();
            spdlog::info("当前工作目录: {}", current_path.string());
            
            std::filesystem::path uploads_dir = current_path / "uploads";
            spdlog::info("上传目录: {}", uploads_dir.string());
            
            // 确保上传目录存在
            if (!std::filesystem::exists(uploads_dir)) {
                spdlog::info("上传目录不存在，创建目录");
                std::filesystem::create_directories(uploads_dir);
            }
            
            std::filesystem::path file_path = uploads_dir / filename;
            spdlog::info("文件路径: {}", file_path.string());
            
            std::ofstream file(file_path, std::ios::binary);
            if (!file.is_open()) {
                std::string error_msg = "无法创建文件: " + file_path.string();
                spdlog::error(error_msg);
                response.error = error_msg;
                return response;
            }
            
            file.write(content.c_str(), content.size());
            file.close();
            
            if (std::filesystem::exists(file_path)) {
                spdlog::info("文件已成功写入: {}, 大小: {} 字节", 
                           file_path.string(), 
                           std::filesystem::file_size(file_path));
            } else {
                spdlog::error("文件写入后不存在: {}", file_path.string());
            }
            
            response.success = true;
            spdlog::info("文件上传成功: {}", filename);
        } catch (const std::exception& e) {
            std::string error_msg = std::string("文件上传异常: ") + e.what();
            spdlog::error(error_msg);
            response.error = error_msg;
        }
        return response;
    }

    // 文件解析API
    FileParseResponse parseFile(const std::string& filename) override {
        FileParseResponse response;
        try {
            std::filesystem::path file_path = std::filesystem::current_path() / "uploads" / filename;
            spdlog::info("解析文件: {}", file_path.string());
            
            std::ifstream file(file_path);
            if (!file.is_open()) {
                spdlog::error("无法打开文件: {}", file_path.string());
                response.error = "无法打开文件: " + file_path.string();
                return response;
            }

            spdlog::info("文件已成功打开，开始解析");
            std::string line;
            while (std::getline(file, line)) {
                response.toolPathDetails.push_back(line);

                // 解析G代码行
                if (line.find('G') != std::string::npos) {
                    TrajectoryPoint point;
                    std::istringstream iss(line);
                    std::string word;

                    // 检查是否为快速定位（G0）
                    point.isRapid = (line.find("G0") != std::string::npos);
                    point.command = line;

                    while (iss >> word) {
                        if (word[0] == 'X') {
                            point.x = std::stod(word.substr(1));
                        } else if (word[0] == 'Y') {
                            point.y = std::stod(word.substr(1));
                        } else if (word[0] == 'Z') {
                            point.z = std::stod(word.substr(1));
                        }
                    }

                    response.trajectoryPoints.push_back(point);
                }
            }

            spdlog::info("文件解析完成，共读取{}行，生成{}个轨迹点", 
                       response.toolPathDetails.size(), 
                       response.trajectoryPoints.size());
            
            response.success = true;
        } catch (const std::exception& e) {
            spdlog::error("文件解析出错: {}", e.what());
            response.error = e.what();
        }
        return response;
    }

    // 配置管理API
    ConfigResponse getConfig() override {
        ConfigResponse response;
        response.config["feedRate"] = "100";
        return response;
    }

    bool updateConfig(const ConfigData& /* config */) override {
        return true;
    }

private:
    bool isProcessing = false;
    double currentProgress = 0.0;
    std::vector<TrajectoryPoint> simulatedTrajectoryPoints;
};

} // namespace web
} // namespace xxcnc
