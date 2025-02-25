#pragma once

#include "xxcnc/core/motion/InterpolationEngine.h"
#include <vector>
#include <chrono>
#include <queue>
#include <mutex>

namespace xxcnc {
namespace core {
namespace motion {

/**
 * @brief 基于时间的插补器，按照固定周期（1ms）将规划产生的距离拆分
 */
class TimeBasedInterpolator {
public:
    /**
     * @brief 构造函数
     * @param interpolationPeriodMs 插补周期（毫秒），默认为1ms
     */
    TimeBasedInterpolator(int interpolationPeriodMs = 1);
    
    /**
     * @brief 析构函数
     */
    ~TimeBasedInterpolator();
    
    /**
     * @brief 设置插补周期
     * @param periodMs 周期（毫秒）
     */
    void setInterpolationPeriod(int periodMs);
    
    /**
     * @brief 获取插补周期
     * @return 插补周期（毫秒）
     */
    int getInterpolationPeriod() const;
    
    /**
     * @brief 规划一条直线路径
     * @param start 起点
     * @param end 终点
     * @param params 插补参数
     * @return 是否成功
     */
    bool planLinearPath(
        const Point& start,
        const Point& end,
        const InterpolationEngine::InterpolationParams& params
    );
    
    /**
     * @brief 规划一条圆弧路径
     * @param start 起点
     * @param end 终点
     * @param center 圆心
     * @param isClockwise 是否顺时针
     * @param params 插补参数
     * @return 是否成功
     */
    bool planCircularPath(
        const Point& start,
        const Point& end,
        const Point& center,
        bool isClockwise,
        const InterpolationEngine::InterpolationParams& params
    );
    
    /**
     * @brief 获取下一个插补点
     * @param point 输出参数，下一个插补点
     * @return 是否成功获取到点
     */
    bool getNextPoint(Point& point);
    
    /**
     * @brief 清空插补队列
     */
    void clearQueue();
    
    /**
     * @brief 获取当前队列中的点数
     * @return 队列中的点数
     */
    size_t getQueueSize() const;
    
    /**
     * @brief 检查插补是否完成
     * @return 是否完成
     */
    bool isFinished() const;
    
    /**
     * @brief 获取当前插补进度
     * @return 进度（0.0-1.0）
     */
    double getProgress() const;
    
private:
    /**
     * @brief 将路径点按照时间周期拆分
     * @param path 原始路径点
     * @param params 插补参数
     */
    void segmentPathByTime(
        const std::vector<Point>& path,
        const InterpolationEngine::InterpolationParams& params
    );
    
    /**
     * @brief 计算两点间的距离
     * @param p1 点1
     * @param p2 点2
     * @return 距离
     */
    double calculateDistance(const Point& p1, const Point& p2) const;
    
    std::unique_ptr<InterpolationEngine> interpolationEngine_;
    std::queue<Point> interpolationQueue_;
    mutable std::mutex queueMutex_;
    int interpolationPeriodMs_;
    double totalDistance_;
    double completedDistance_;
    Point currentPosition_;
    bool isPlanning_;
};

} // namespace motion
} // namespace core
} // namespace xxcnc
