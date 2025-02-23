#include <gtest/gtest.h>
#include "xxcnc/core/CoreController.h"

namespace xxcnc::core::test {

class CoreControllerTest : public ::testing::Test {
protected:
    CoreController controller;
};

TEST_F(CoreControllerTest, InitialStateIsIdle) {
    EXPECT_EQ(controller.getState(), SystemState::IDLE);
}

TEST_F(CoreControllerTest, InitializeSucceedsInIdleState) {
    EXPECT_TRUE(controller.initialize());
}

TEST_F(CoreControllerTest, InitializeFailsInNonIdleState) {
    controller.start();
    EXPECT_FALSE(controller.initialize());
}

TEST_F(CoreControllerTest, StartSucceedsInIdleState) {
    EXPECT_TRUE(controller.start());
    EXPECT_EQ(controller.getState(), SystemState::RUNNING);
}

TEST_F(CoreControllerTest, StartFailsInRunningState) {
    controller.start();
    EXPECT_FALSE(controller.start());
}

TEST_F(CoreControllerTest, StopSucceedsInRunningState) {
    controller.start();
    EXPECT_TRUE(controller.stop());
    EXPECT_EQ(controller.getState(), SystemState::IDLE);
}

TEST_F(CoreControllerTest, StopFailsInIdleState) {
    EXPECT_FALSE(controller.stop());
}

TEST_F(CoreControllerTest, EmergencyStopSucceedsInAnyState) {
    EXPECT_TRUE(controller.emergencyStop());
    EXPECT_EQ(controller.getState(), SystemState::EMERGENCY_STOP);

    // 重置状态并从运行状态测试
    controller = CoreController();
    controller.start();
    EXPECT_TRUE(controller.emergencyStop());
    EXPECT_EQ(controller.getState(), SystemState::EMERGENCY_STOP);
}

TEST_F(CoreControllerTest, EmergencyStopFailsWhenAlreadyInEmergencyState) {
    controller.emergencyStop();
    EXPECT_FALSE(controller.emergencyStop());
}

TEST_F(CoreControllerTest, HomingSucceedsInIdleState) {
    EXPECT_TRUE(controller.startHoming());
    // 归零完成后，状态应该返回到IDLE
    EXPECT_EQ(controller.getState(), SystemState::IDLE);
}

TEST_F(CoreControllerTest, HomingFailsInNonIdleState) {
    controller.start();
    EXPECT_FALSE(controller.startHoming());
    EXPECT_EQ(controller.getState(), SystemState::RUNNING);
}

} // namespace xxcnc::core::test