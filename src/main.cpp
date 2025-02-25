#include <xxcnc/web/WebServer.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <regex>
#include <vector>
#include <string>
#include "spdlog/spdlog.h"

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
};

int main() {
    WebServer server;
    SystemStatus status;

    // 设置服务器配置
    xxcnc::web::WebServer::Config config;
    config.static_dir = std::filesystem::current_path().string() + "/static";  // 使用绝对路径
    config.enable_cors = true;  // 启用CORS
    server.setConfig(config);

    // 设置状态回调
    server.setStatusCallback([&status]() -> json {
        return json({
            {"state", status.running ? "运行中" : "空闲"},
            {"position", {
                {"x", status.x_pos},
                {"y", status.y_pos},
                {"z", status.z_pos}
            }},
            {"feedRate", status.feed_rate},
            {"progress", status.progress},
            {"currentFile", status.current_file}
        });
    });

    // 设置命令回调
    server.setCommandCallback([&status](const json& cmd) -> json {
        try {
            const auto& type = cmd["command"];
            if (type == "start") {
                status.running = true;
                return json({{"success", true}});
            }
            else if (type == "stop") {
                status.running = false;
                return json({{"success", true}});
            }
            else if (type == "pause") {
                status.running = false;
                return json({{"success", true}});
            }
            else if (type == "resume") {
                status.running = true;
                return json({{"success", true}});
            }
            else if (type == "home") {
                status.x_pos = 0;
                status.y_pos = 0;
                status.z_pos = 0;
                return json({{"success", true}});
            }
            else if (type == "setFeedRate") {
                if (cmd.contains("value")) {
                    status.feed_rate = cmd["value"];
                    return json({{"success", true}});
                }
            }
            else if (type == "loadFile") {
                if (cmd.contains("filename")) {
                    status.current_file = cmd["filename"];
                    return json({{"success", true}});
                }
            }
            return json({
                {"success", false},
                {"error", "Unknown command"}
            });
        }
        catch (const std::exception& e) {
            return json({
                {"success", false},
                {"error", e.what()}
            });
        }
    });

    // 设置文件上传回调
    server.setFileUploadCallback([&status](const std::string& filename, const std::string& content) -> json {
        try {
            spdlog::info("开始处理文件上传：{}", filename);
            spdlog::info("文件内容大小：{} 字节", content.size());

            std::vector<json> trajectoryPoints;
            std::vector<std::string> toolPathLines;
            std::stringstream ss(content);
            std::string line;
            double x = 0, y = 0, z = 0;
            bool isRapid = false;

            // 解析每一行G代码
            while (std::getline(ss, line)) {
                // 去除空白字符和注释
                if (line.empty() || line[0] == ';') {
                    spdlog::debug("跳过空行或注释：{}", line);
                    continue;
                }

                // 保存原始G代码行
                toolPathLines.push_back(line);
                spdlog::debug("处理G代码行：{}", line);

                // 判断移动类型
                if (line.find("G0") != std::string::npos) {
                    isRapid = true;
                    spdlog::debug("检测到快速定位指令");
                } else if (line.find("G1") != std::string::npos) {
                    isRapid = false;
                    spdlog::debug("检测到进给指令");
                }

                // 解析坐标值
                size_t pos;
                if ((pos = line.find('X')) != std::string::npos) {
                    x = std::stod(line.substr(pos + 1));
                    spdlog::debug("X坐标：{}", x);
                }
                if ((pos = line.find('Y')) != std::string::npos) {
                    y = std::stod(line.substr(pos + 1));
                    spdlog::debug("Y坐标：{}", y);
                }
                if ((pos = line.find('Z')) != std::string::npos) {
                    z = std::stod(line.substr(pos + 1));
                    spdlog::debug("Z坐标：{}", z);
                }

                // 添加轨迹点
                if (line.find('X') != std::string::npos || 
                    line.find('Y') != std::string::npos || 
                    line.find('Z') != std::string::npos) {
                    json point = {
                        {"x", x},
                        {"y", y},
                        {"z", z},
                        {"type", isRapid ? "rapid" : "feed"},
                        {"command", line}
                    };
                    trajectoryPoints.push_back(point);
                    spdlog::debug("添加轨迹点：{}", point.dump());
                }
            }

            spdlog::info("文件处理完成，生成了 {} 个轨迹点", trajectoryPoints.size());

            // 更新当前文件
            status.current_file = filename;

            // 返回处理结果
            json result = {
                {"success", true},
                {"filename", filename},
                {"fileSize", content.size()},
                {"trajectoryPoints", trajectoryPoints},
                {"toolPathDetails", toolPathLines}
            };

            spdlog::info("返回数据：{}", result.dump());
            return result;
        }
        catch (const std::exception& e) {
            spdlog::error("文件处理错误：{}", e.what());
            return json({
                {"success", false},
                {"error", e.what()}
            });
        }
    });

    // 启动服务器
    if (!server.start("0.0.0.0", 8080)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    std::cout << "Server started at http://localhost:8080" << std::endl;
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();

    server.stop();
    return 0;
}