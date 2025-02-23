#pragma once
#include <spdlog/logger.h>
#include <memory>

namespace xxcnc {
namespace core {

// System state enumeration
enum class SystemState {
    IDLE,           // Idle state
    RUNNING,        // Running state
    PAUSED,         // Paused state
    ERROR,          // Error state
    HOMING,         // Homing state
    EMERGENCY_STOP  // Emergency stop state
};

class CoreController {
public:
    CoreController();
    ~CoreController();

    // System initialization
    bool initialize();

    // Start system
    bool start();

    // Stop system
    bool stop();

    // Pause system
    bool pause();

    // Resume system
    bool resume();

    // Emergency stop
    bool emergencyStop();

    // Start homing procedure
    bool startHoming();

    // Get current system state
    SystemState getState() const;

private:
    SystemState currentState{SystemState::IDLE};
    std::shared_ptr<spdlog::logger> logger;
};

} // namespace core
} // namespace xxcnc