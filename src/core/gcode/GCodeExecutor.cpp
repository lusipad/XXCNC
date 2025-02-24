#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace xxcnc {

class GCodeCommand {
public:
    enum class Type {
        MOTION,     // 运动指令
        TOOL,       // 刀具相关指令
        COORDINATE, // 坐标系统指令
        MACRO,      // 宏指令
        SYSTEM      // 系统控制指令
    };

    GCodeCommand(Type type) : type_(type) {}
    virtual ~GCodeCommand() = default;

    Type getType() const { return type_; }

private:
    Type type_;
};

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

GCodeExecutor::GCodeExecutor()
    : paused_(false)
    , stopped_(false) {}

GCodeExecutor::~GCodeExecutor() {
    stop();
    clearQueue();
}

void GCodeExecutor::addCommand(std::unique_ptr<GCodeCommand> command) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    command_queue_.push(std::move(command));
    queue_condition_.notify_one();
}

bool GCodeExecutor::executeNext() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    
    // 等待队列非空且未停止
    queue_condition_.wait(lock, [this]() {
        return (!command_queue_.empty() && !paused_) || stopped_;
    });

    if (stopped_ || command_queue_.empty()) {
        return false;
    }

    // 获取并移除队首指令
    auto command = std::move(command_queue_.front());
    command_queue_.pop();

    // 在这里执行具体的指令
    // TODO: 根据command的类型调用相应的处理函数

    return true;
}

void GCodeExecutor::clearQueue() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!command_queue_.empty()) {
        command_queue_.pop();
    }
}

size_t GCodeExecutor::getPendingCommandCount() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return command_queue_.size();
}

void GCodeExecutor::pause() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    paused_ = true;
}

void GCodeExecutor::resume() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    paused_ = false;
    queue_condition_.notify_all();
}

void GCodeExecutor::stop() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    stopped_ = true;
    queue_condition_.notify_all();
}

} // namespace xxcnc