#pragma once

#include <string>
#include <vector>
#include <map>

namespace xxcnc {
namespace web {

/**
 * @brief 轨迹点
 */
struct TrajectoryPoint {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    bool isRapid = false;
    std::string command;
};

/**
 * @brief 系统状态响应
 */
struct StatusResponse {
    std::string status;    ///< 系统当前状态
    struct {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    } position;             ///< 当前位置
    double feedRate = 100;  ///< 进给速度
    std::string currentFile;///< 当前文件
    double progress = 0;    ///< 进度
    int errorCode = 0;      ///< 错误代码
    std::vector<std::string> messages;  ///< 状态消息列表
    std::vector<TrajectoryPoint> trajectoryPoints; ///< 轨迹点列表

    bool operator==(const StatusResponse& other) const {
        return status == other.status &&
               position.x == other.position.x &&
               position.y == other.position.y &&
               position.z == other.position.z &&
               feedRate == other.feedRate &&
               currentFile == other.currentFile &&
               progress == other.progress &&
               errorCode == other.errorCode &&
               messages == other.messages &&
               trajectoryPoints.size() == other.trajectoryPoints.size();
    }
};

/**
 * @brief 文件列表响应
 */
struct FileListResponse {
    std::vector<std::string> files;     ///< 文件列表
    std::vector<std::string> folders;   ///< 文件夹列表
    std::vector<std::string> errors;    ///< 错误信息列表

    bool operator==(const FileListResponse& other) const {
        return files == other.files &&
               folders == other.folders &&
               errors == other.errors;
    }
};

/**
 * @brief 配置数据
 */
struct ConfigData {
    std::map<std::string, std::string> config;  ///< 配置键值对映射

    bool operator==(const ConfigData& other) const {
        return config == other.config;
    }
};

/**
 * @brief 配置响应
 */
struct ConfigResponse {
    std::map<std::string, std::string> config;  ///< 配置键值对映射

    bool operator==(const ConfigResponse& other) const {
        return config == other.config;
    }
};

} // namespace web
} // namespace xxcnc