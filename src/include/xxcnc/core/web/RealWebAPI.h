#pragma once

#include "xxcnc/core/web/WebAPI.h"
#include "xxcnc/motion/MotionController.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace xxcnc {
namespace web {

/**
 * @brief 真实的Web API实现，使用实际的运动控制器
 */
class RealWebAPI : public WebAPI {
public:
    RealWebAPI() : 
        motionController_(std::make_shared<motion::MotionController>()),
        lastUpdateTime_(std::chrono::steady_clock::now())
    {
        // 初始化运动控制器
        initializeMotionController();
        
        // 创建上传目录
        std::filesystem::path uploads_dir = std::filesystem::current_path() / "uploads";
        if (!std::filesystem::exists(uploads_dir)) {
            std::filesystem::create_directories(uploads_dir);
            spdlog::info("创建上传目录: {}", uploads_dir.string());
        }
    }
    
    ~RealWebAPI() override = default;

    // 状态监控API
    StatusResponse getSystemStatus() override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        StatusResponse response;
        
        // 更新进度
        if (isProcessing) {
            response.status = "machining";
            
            // 获取当前进度
            response.progress = motionController_->getInterpolationProgress();
            
            // 如果插补已完成，则设置为空闲状态
            if (motionController_->isInterpolationFinished()) {
                isProcessing = false;
                response.status = "idle";
                response.progress = 1.0; // 完成
                spdlog::info("加工完成");
            }
            
            // 获取当前位置
            auto xAxis = motionController_->getAxis("X");
            auto yAxis = motionController_->getAxis("Y");
            auto zAxis = motionController_->getAxis("Z");
            
            if (xAxis && yAxis && zAxis) {
                response.position.x = xAxis->getCurrentPosition();
                response.position.y = yAxis->getCurrentPosition();
                response.position.z = zAxis->getCurrentPosition();
                
                // 创建当前轨迹点
                TrajectoryPoint currentPoint = {
                    response.position.x,
                    response.position.y,
                    response.position.z,
                    false // 非快速定位
                };
                
                // 添加到轨迹历史
                trajectoryHistory_.push_back(currentPoint);
                
                // 记录轨迹点数量
                spdlog::info("当前轨迹历史点数: {}", trajectoryHistory_.size());
                spdlog::info("当前位置: ({}, {}, {})", currentPoint.x, currentPoint.y, currentPoint.z);
            }
        } else {
            response.status = "idle";
            response.progress = 0.0;
            
            // 获取当前位置
            auto xAxis = motionController_->getAxis("X");
            auto yAxis = motionController_->getAxis("Y");
            auto zAxis = motionController_->getAxis("Z");
            
            if (xAxis && yAxis && zAxis) {
                response.position.x = xAxis->getCurrentPosition();
                response.position.y = yAxis->getCurrentPosition();
                response.position.z = zAxis->getCurrentPosition();
                
                // 创建当前轨迹点
                TrajectoryPoint currentPoint = {
                    response.position.x,
                    response.position.y,
                    response.position.z,
                    false // 非快速定位
                };
                
                // 添加到轨迹历史（仅当位置发生变化时）
                if (trajectoryHistory_.empty() || 
                    std::abs(trajectoryHistory_.back().x - currentPoint.x) > 0.001 ||
                    std::abs(trajectoryHistory_.back().y - currentPoint.y) > 0.001 ||
                    std::abs(trajectoryHistory_.back().z - currentPoint.z) > 0.001) {
                    
                    trajectoryHistory_.push_back(currentPoint);
                    spdlog::info("添加新轨迹点: ({}, {}, {})", currentPoint.x, currentPoint.y, currentPoint.z);
                }
                
                // 记录轨迹点数量
                spdlog::info("当前轨迹历史点数: {}", trajectoryHistory_.size());
                spdlog::info("当前位置: ({}, {}, {})", currentPoint.x, currentPoint.y, currentPoint.z);
            } else {
                response.position = {0.0, 0.0, 0.0};
            }
        }
        
        // 无论是否处于加工状态，都返回完整的轨迹历史
        if (!trajectoryHistory_.empty()) {
            response.trajectoryPoints = trajectoryHistory_;
            spdlog::info("响应中的轨迹点数: {}", response.trajectoryPoints.size());
            
            // 记录部分轨迹点用于调试
            if (trajectoryHistory_.size() > 0) {
                size_t sampleSize = std::min(size_t(5), trajectoryHistory_.size());
                spdlog::info("轨迹点示例（前{}个）:", sampleSize);
                for (size_t i = 0; i < sampleSize; i++) {
                    spdlog::info("  点 {}: ({}, {}, {})", 
                                i, 
                                trajectoryHistory_[i].x, 
                                trajectoryHistory_[i].y, 
                                trajectoryHistory_[i].z);
                }
                
                if (trajectoryHistory_.size() > 5) {
                    size_t lastIndex = trajectoryHistory_.size() - 1;
                    spdlog::info("  ... 及最后一个点 {}: ({}, {}, {})",
                                lastIndex,
                                trajectoryHistory_[lastIndex].x,
                                trajectoryHistory_[lastIndex].y,
                                trajectoryHistory_[lastIndex].z);
                }
            }
        } else {
            spdlog::info("轨迹历史为空");
        }
        
        response.feedRate = currentFeedRate_;
        response.currentFile = currentFile_;
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
                std::vector<TrajectoryPoint> trajectoryPoints = parseResponse.trajectoryPoints;
                spdlog::info("成功加载轨迹点: {} 个", trajectoryPoints.size());
                
                // 使用运动控制器规划路径
                if (!trajectoryPoints.empty()) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    
                    // 确保所有轴都已使能
                    motionController_->enableAllAxes();
                    
                    // 清除之前的运动规划
                    motionController_->clearTrajectory();
                    
                    // 执行轨迹
                    for (size_t i = 0; i < trajectoryPoints.size(); ++i) {
                        const auto& point = trajectoryPoints[i];
                        
                        // 创建目标位置映射
                        std::map<std::string, double> targetPositions;
                        targetPositions["X"] = point.x;
                        targetPositions["Y"] = point.y;
                        targetPositions["Z"] = point.z;
                        
                        // 设置进给速度
                        double feedRate = point.isRapid ? 3000.0 : currentFeedRate_;
                        
                        // 执行直线插补运动
                        if (!motionController_->moveLinear(targetPositions, feedRate)) {
                            spdlog::error("运动规划失败，位置: ({}, {}, {})", point.x, point.y, point.z);
                            return false;
                        }
                    }
                    
                    spdlog::info("成功规划运动路径");
                    
                    // 开始执行运动
                    if (!motionController_->startMotion()) {
                        spdlog::error("启动运动失败");
                        return false;
                    }
                }
                
                // 开始加工流程
                isProcessing = true;
                currentFile_ = filename;
                lastUpdateTime_ = std::chrono::steady_clock::now();
                return true;
            } else if (command == "motion.stop") {
                // 停止加工
                spdlog::info("停止加工");
                std::lock_guard<std::mutex> lock(mutex_);
                isProcessing = false;
                
                // 确保停止所有轴的运动
                spdlog::info("调用 emergencyStop");
                bool stopResult = motionController_->emergencyStop();
                spdlog::info("emergencyStop 结果: {}", stopResult ? "成功" : "失败");
                
                // 清除运动规划
                spdlog::info("调用 clearTrajectory");
                motionController_->clearTrajectory();
                
                // 等待轴停止运动
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // 清除当前文件
                currentFile_.clear();
                
                spdlog::info("加工已停止");
                return true;
            } else if (command == "trajectory.clear") {
                // 清除轨迹历史
                spdlog::info("清除轨迹历史");
                std::lock_guard<std::mutex> lock(mutex_);
                
                // 记录当前轨迹点数量
                spdlog::info("当前轨迹历史点数: {}", trajectoryHistory_.size());
                
                // 清除轨迹历史
                trajectoryHistory_.clear();
                spdlog::info("轨迹历史已清除，当前点数: {}", trajectoryHistory_.size());
                
                // 通知前端清除轨迹
                spdlog::info("通知前端清除轨迹");
                motionController_->clearTrajectory();
                
                spdlog::info("轨迹清除完成");
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
            if (std::filesystem::exists(dir_path) && std::filesystem::is_directory(dir_path)) {
                for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
                    if (entry.is_regular_file()) {
                        response.files.push_back(entry.path().filename().string());
                    } else if (entry.is_directory()) {
                        response.folders.push_back(entry.path().filename().string());
                    }
                }
            } else {
                response.errors.push_back("目录不存在: " + dir_path.string());
            }
        } catch (const std::exception& e) {
            response.errors.push_back(std::string("获取文件列表异常: ") + e.what());
        }
        
        return response;
    }

    // 文件上传API
    FileUploadResponse uploadFile(const std::string& filename, const std::string& content) override {
        FileUploadResponse response;
        try {
            std::filesystem::path file_path = std::filesystem::current_path() / "uploads" / filename;
            
            // 确保uploads目录存在
            std::filesystem::path uploads_dir = std::filesystem::current_path() / "uploads";
            if (!std::filesystem::exists(uploads_dir)) {
                std::filesystem::create_directories(uploads_dir);
            }
            
            std::ofstream file(file_path, std::ios::binary);
            if (!file.is_open()) {
                response.error = "无法创建文件: " + file_path.string();
                spdlog::error(response.error);
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
        response.config["feedRate"] = std::to_string(currentFeedRate_);
        return response;
    }

    bool updateConfig(const ConfigData& config) override {
        try {
            auto it = config.config.find("feedRate");
            if (it != config.config.end()) {
                currentFeedRate_ = std::stod(it->second);
                spdlog::info("更新进给速度: {}", currentFeedRate_);
            }
            return true;
        } catch (const std::exception& e) {
            spdlog::error("更新配置出错: {}", e.what());
            return false;
        }
    }

private:
    // 初始化运动控制器
    void initializeMotionController() {
        // 添加X轴
        motion::AxisParameters xAxisParams;
        xAxisParams.maxVelocity = 500.0;       // mm/s
        xAxisParams.maxAcceleration = 1000.0;  // mm/s^2
        xAxisParams.maxJerk = 5000.0;          // mm/s^3
        xAxisParams.homePosition = 0.0;        // mm
        xAxisParams.softLimitMin = -1000.0;    // mm
        xAxisParams.softLimitMax = 1000.0;     // mm
        motionController_->addAxis("X", xAxisParams);
        
        // 添加Y轴
        motion::AxisParameters yAxisParams;
        yAxisParams.maxVelocity = 500.0;       // mm/s
        yAxisParams.maxAcceleration = 1000.0;  // mm/s^2
        yAxisParams.maxJerk = 5000.0;          // mm/s^3
        yAxisParams.homePosition = 0.0;        // mm
        yAxisParams.softLimitMin = -1000.0;    // mm
        yAxisParams.softLimitMax = 1000.0;     // mm
        motionController_->addAxis("Y", yAxisParams);
        
        // 添加Z轴
        motion::AxisParameters zAxisParams;
        zAxisParams.maxVelocity = 300.0;       // mm/s
        zAxisParams.maxAcceleration = 800.0;   // mm/s^2
        zAxisParams.maxJerk = 3000.0;          // mm/s^3
        zAxisParams.homePosition = 0.0;        // mm
        zAxisParams.softLimitMin = -500.0;     // mm
        zAxisParams.softLimitMax = 500.0;      // mm
        motionController_->addAxis("Z", zAxisParams);
        
        // 设置插补周期
        motionController_->setInterpolationPeriod(1); // 1ms
        
        spdlog::info("运动控制器初始化完成");
    }

    std::shared_ptr<motion::MotionController> motionController_;
    bool isProcessing = false;
    double currentFeedRate_ = 1000.0; // mm/min
    std::string currentFile_;
    std::chrono::time_point<std::chrono::steady_clock> lastUpdateTime_;
    std::mutex mutex_;
    std::vector<TrajectoryPoint> trajectoryHistory_; // 存储轨迹历史
};

} // namespace web
} // namespace xxcnc
