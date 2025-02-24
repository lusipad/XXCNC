#pragma once

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "GCodeCommands.h"

namespace xxcnc {

class GCodeExecutor {
public:
    GCodeExecutor();
    ~GCodeExecutor();

    // 添加指令到队列
    void addCommand(std::unique_ptr<GCodeCommand> command);

    // 执行队列中的下一条指令
    bool executeNext();

    // 清空指令队列
    void clearQueue();

    // 获取队列中待执行的指令数量
    size_t getPendingCommandCount() const;

    // 暂停执行
    void pause();

    // 恢复执行
    void resume();

    // 停止执行
    void stop();

private:
    std::queue<std::unique_ptr<GCodeCommand>> command_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    bool paused_;
    bool stopped_;
};

} // namespace xxcnc