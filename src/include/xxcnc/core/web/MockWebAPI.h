#pragma once

#include "xxcnc/core/web/WebAPI.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>
#include "xxcnc/core/motion/TimeBasedInterpolator.h"
#include <nlohmann/json.hpp>

namespace xxcnc {
namespace web {

class MockWebAPI : public WebAPI {
public:
    MockWebAPI() : 
        interpolator_(std::make_unique<core::motion::TimeBasedInterpolator>(1)),
        lastUpdateTime_(std::chrono::steady_clock::now())
    {
        // 创建上传目录
        std::filesystem::path uploads_dir = std::filesystem::current_path() / "uploads";
        if (!std::filesystem::exists(uploads_dir)) {
            std::filesystem::create_directories(uploads_dir);
            spdlog::info("创建上传目录: {}", uploads_dir.string());
        }
    }
    ~MockWebAPI() override = default;

    // 状态监控API
    StatusResponse getSystemStatus() override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        StatusResponse response;
        
        // 更新进度
        if (isProcessing) {
            response.status = "machining";
            
            // 计算时间差，更新进度
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime_).count();
            lastUpdateTime_ = now;
            
            // 模拟进度增加，降低增加速度，每秒增加1%
            if (elapsed > 0) {
                double progressIncrement = (elapsed / 1000.0) * 0.01;
                currentProgress += progressIncrement;
                spdlog::info("进度更新: +{:.2f}%, 当前 {:.2f}%", 
                    progressIncrement * 100, currentProgress * 100);
                
                if (currentProgress > 1.0) {
                    currentProgress = 1.0;
                    isProcessing = false;
                    response.status = "idle";
                    spdlog::info("加工完成");
                }
            }
            
            response.progress = currentProgress;
            
            // 使用基于时间的插补器获取当前位置
            core::motion::Point currentPoint;
            if (interpolator_->getNextPoint(currentPoint)) {
                response.position.x = currentPoint.x;
                response.position.y = currentPoint.y;
                response.position.z = currentPoint.z;
                spdlog::debug("从插补器获取位置: ({:.3f}, {:.3f}, {:.3f})", 
                    currentPoint.x, currentPoint.y, currentPoint.z);
            } else if (!simulatedTrajectoryPoints.empty()) {
                // 如果插补器没有点，则使用模拟轨迹点
                size_t pointIndex = static_cast<size_t>(currentProgress * (simulatedTrajectoryPoints.size() - 1));
                if (pointIndex < simulatedTrajectoryPoints.size()) {
                    const auto& point = simulatedTrajectoryPoints[pointIndex];
                    response.position.x = point.x;
                    response.position.y = point.y;
                    response.position.z = point.z;
                    spdlog::debug("使用模拟轨迹点[{}]: ({:.3f}, {:.3f}, {:.3f})", 
                        pointIndex, point.x, point.y, point.z);
                }
            }
            
            // 添加所有轨迹点到响应中（始终返回）
            if (!simulatedTrajectoryPoints.empty()) {
                // 根据进度确定需要显示的轨迹点
                size_t displayPointCount = static_cast<size_t>(currentProgress * simulatedTrajectoryPoints.size());
                displayPointCount = std::min(displayPointCount, simulatedTrajectoryPoints.size());
                
                // 只发送已经完成的轨迹点
                response.trajectoryPoints.clear();
                for (size_t i = 0; i < displayPointCount; i++) {
                    response.trajectoryPoints.push_back(simulatedTrajectoryPoints[i]);
                }
                
                spdlog::info("状态API返回 {} 个轨迹点（总计 {} 个）", 
                    response.trajectoryPoints.size(), simulatedTrajectoryPoints.size());
            }
        } else {
            response.status = "idle";
            response.progress = 0.0;
            response.position = {0.0, 0.0, 0.0};
        }
        
        response.feedRate = 100.0;
        response.currentFile = "test.nc";
        response.errorCode = 0;
        
        return response;
    }

    // 控制指令API
    bool executeCommand(const nlohmann::json& cmdJson) override {
        try {
            if (!cmdJson.contains("command") || !cmdJson["command"].is_string()) {
                spdlog::error("无效的命令格式，缺少command字段或类型不正确");
                return false;
            }
            
            std::string command = cmdJson["command"].get<std::string>();
            spdlog::info("执行命令: {}", command);
            
            if (command == "motion.start") {
                // 检查是否有文件名参数
                if (!cmdJson.contains("filename") || !cmdJson["filename"].is_string()) {
                    spdlog::error("motion.start命令缺少filename参数");
                    return false;
                }
                
                std::string filename = cmdJson["filename"].get<std::string>();
                spdlog::info("开始加工文件: {}", filename);
                
                // 解析文件获取轨迹点
                auto parseResponse = parseFile(filename);
                if (!parseResponse.success) {
                    spdlog::error("解析文件失败: {}", parseResponse.error);
                    return false;
                }
                
                // 使用解析的轨迹点
                simulatedTrajectoryPoints = parseResponse.trajectoryPoints;
                spdlog::info("成功加载轨迹点: {} 个", simulatedTrajectoryPoints.size());
                
                // 使用基于时间的插补器规划路径
                if (!simulatedTrajectoryPoints.empty()) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    
                    // 清空现有插补队列
                    interpolator_->clearQueue();
                    
                    // 设置插补参数
                    core::motion::InterpolationEngine::InterpolationParams params;
                    params.feedRate = 1000.0; // 默认进给速度 mm/min
                    params.maxVelocity = 50.0; // 最大速度 mm/s
                    params.acceleration = 500.0; // 加速度 mm/s^2
                    params.deceleration = 500.0; // 减速度 mm/s^2
                    
                    // 规划路径
                    for (size_t i = 0; i < simulatedTrajectoryPoints.size() - 1; ++i) {
                        const auto& start = simulatedTrajectoryPoints[i];
                        const auto& end = simulatedTrajectoryPoints[i + 1];
                        
                        core::motion::Point startPoint(start.x, start.y, start.z);
                        core::motion::Point endPoint(end.x, end.y, end.z);
                        
                        // 根据是否为快速定位设置不同的进给速度
                        params.feedRate = end.isRapid ? 3000.0 : 1000.0;
                        
                        interpolator_->planLinearPath(startPoint, endPoint, params);
                    }
                    
                    spdlog::info("成功规划插补路径，队列中有 {} 个点", interpolator_->getQueueSize());
                }
                
                // 开始加工流程
                isProcessing = true;
                currentProgress = 0.0;
                lastUpdateTime_ = std::chrono::steady_clock::now();
                return true;
            } else if (command == "motion.stop") {
                // 模拟停止加工
                spdlog::info("停止加工");
                std::lock_guard<std::mutex> lock(mutex_);
                isProcessing = false;
                interpolator_->clearQueue();
                return true;
            }
            
            return true;
        } catch (const std::exception& e) {
            spdlog::error("执行命令时发生错误: {}", e.what());
            return false;
        }
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
    std::unique_ptr<core::motion::TimeBasedInterpolator> interpolator_;
    std::chrono::time_point<std::chrono::steady_clock> lastUpdateTime_;
    std::mutex mutex_;
};

} // namespace web
} // namespace xxcnc
