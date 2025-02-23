#include <gtest/gtest.h>
#include "xxcnc/core/CoreController.h"

namespace xxcnc {
namespace core {
namespace test {

class CoreControllerTest : public ::testing::Test {
protected:
    CoreController controller;
};

TEST_F(CoreControllerTest, InitialStateIsIdle) {
    EXPECT_EQ(controller.getState(), SystemState::IDLE);
}

TEST_F(CoreControllerTest, InitializeReturnsTrue) {
    EXPECT_TRUE(controller.initialize());
}

TEST_F(CoreControllerTest, StartReturnsTrue) {
    EXPECT_TRUE(controller.start());
}

TEST_F(CoreControllerTest, StopReturnsTrue) {
    EXPECT_TRUE(controller.stop());
}

} // namespace test
} // namespace core
} // namespace xxcnc