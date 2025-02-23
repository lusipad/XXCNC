#pragma once

namespace xxcnc {
namespace core {

// System state enumeration
enum class SystemState {
    IDLE,       // Idle state
    RUNNING,    // Running state
    PAUSED,     // Paused state
    ERROR       // Error state
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

    // Get current system state
    SystemState getState() const;

private:
    SystemState currentState{SystemState::IDLE};
};

} // namespace core
} // namespace xxcnc