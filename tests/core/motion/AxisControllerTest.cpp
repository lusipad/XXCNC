#include <gtest/gtest.h>
#include "xxcnc/motion/AxisController.h"

using namespace xxcnc::motion;

class AxisControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        AxisParameters params;
        params.maxVelocity = 1000.0;      // 最大速度 1000mm/s
        params.maxAcceleration = 500.0;   // 最大加速度 500mm/s²
        params.softLimitMin = -100.0;     // 软限位最小值 -100mm
        params.softLimitMax = 100.0;      // 软限位最大值 100mm
        params.homeVelocity = 10.0;       // 回零速度 10mm/s
        axis_ = std::make_unique<AxisController>("X", params);
    }

    std::unique_ptr<AxisController> axis_;
};

TEST_F(AxisControllerTest, InitialState) {
    EXPECT_EQ(axis_->getName(), "X");
    EXPECT_EQ(axis_->getCurrentPosition(), 0.0);
    EXPECT_EQ(axis_->getCurrentVelocity(), 0.0);
    EXPECT_EQ(axis_->getState(), AxisState::DISABLED);
}

TEST_F(AxisControllerTest, EnableDisable) {
    EXPECT_TRUE(axis_->enable());
    EXPECT_EQ(axis_->getState(), AxisState::IDLE);

    EXPECT_TRUE(axis_->disable());
    EXPECT_EQ(axis_->getState(), AxisState::DISABLED);
}

TEST_F(AxisControllerTest, MoveTo) {
    axis_->enable();

    // 测试正常移动
    EXPECT_TRUE(axis_->moveTo(50.0, 100.0));
    EXPECT_EQ(axis_->getState(), AxisState::MOVING);

    // 测试超出软限位
    EXPECT_FALSE(axis_->moveTo(150.0, 100.0));

    // 测试速度限制
    EXPECT_TRUE(axis_->moveTo(50.0, 2000.0)); // 速度会被限制在1000mm/s
}

TEST_F(AxisControllerTest, MoveVelocity) {
    axis_->enable();

    // 测试正常速度移动
    EXPECT_TRUE(axis_->moveVelocity(500.0));
    EXPECT_EQ(axis_->getState(), AxisState::MOVING);

    // 测试速度限制
    EXPECT_TRUE(axis_->moveVelocity(2000.0)); // 速度会被限制在1000mm/s
}

TEST_F(AxisControllerTest, Stop) {
    axis_->enable();
    axis_->moveVelocity(500.0);

    // 测试正常停止
    EXPECT_TRUE(axis_->stop(false));

    // 测试紧急停止
    axis_->moveVelocity(500.0);
    EXPECT_TRUE(axis_->stop(true));
    EXPECT_EQ(axis_->getCurrentVelocity(), 0.0);
    EXPECT_EQ(axis_->getState(), AxisState::IDLE);
}

TEST_F(AxisControllerTest, Home) {
    axis_->enable();

    EXPECT_TRUE(axis_->home());
    EXPECT_EQ(axis_->getState(), AxisState::HOMING);
}

TEST_F(AxisControllerTest, Update) {
    axis_->enable();
    axis_->moveTo(50.0, 100.0);

    // 测试位置和速度更新
    axis_->update(0.1); // 更新0.1秒
    EXPECT_GT(axis_->getCurrentVelocity(), 0.0);
    EXPECT_GT(axis_->getCurrentPosition(), 0.0);

    // 测试软限位保护
    axis_->moveTo(-150.0, 1000.0);
    axis_->update(1.0);
    EXPECT_EQ(axis_->getState(), AxisState::ERROR);
    EXPECT_EQ(axis_->getCurrentVelocity(), 0.0);
}

TEST_F(AxisControllerTest, AccelerationControl) {
    axis_->enable();
    
    // 测试加速过程
    axis_->moveVelocity(1000.0);
    axis_->update(0.1);
    double velocity1 = axis_->getCurrentVelocity();
    axis_->update(0.1);
    double velocity2 = axis_->getCurrentVelocity();
    EXPECT_GT(velocity2, velocity1);
    EXPECT_LE(velocity2 - velocity1, 500.0 * 0.1); // 确保加速度不超过最大值

    // 测试减速过程
    axis_->stop(false);
    axis_->update(0.1);
    velocity1 = axis_->getCurrentVelocity();
    axis_->update(0.1);
    velocity2 = axis_->getCurrentVelocity();
    EXPECT_LT(velocity2, velocity1);
    EXPECT_GE(velocity1 - velocity2, 0.0);
    EXPECT_LE(velocity1 - velocity2, 500.0 * 0.1); // 确保减速度不超过最大值
}

TEST_F(AxisControllerTest, SoftLimitProtection) {
    axis_->enable();

    // 测试正向软限位
    axis_->moveTo(90.0, 500.0);
    axis_->update(1.0);
    EXPECT_LT(axis_->getCurrentPosition(), 100.0);
    EXPECT_EQ(axis_->getState(), AxisState::MOVING);

    // 测试负向软限位
    axis_->moveTo(-90.0, 500.0);
    axis_->update(1.0);
    EXPECT_GT(axis_->getCurrentPosition(), -100.0);
    EXPECT_EQ(axis_->getState(), AxisState::MOVING);

    // 测试软限位触发后的停止
    axis_->moveTo(150.0, 1000.0);
    axis_->update(1.0);
    EXPECT_LE(axis_->getCurrentPosition(), 100.0);
    EXPECT_EQ(axis_->getState(), AxisState::ERROR);
    EXPECT_EQ(axis_->getCurrentVelocity(), 0.0);
}

TEST_F(AxisControllerTest, PositionControl) {
    axis_->enable();

    // 测试位置控制精度
    EXPECT_TRUE(axis_->moveTo(50.0, 200.0));
    
    double lastPos = axis_->getCurrentPosition();
    double totalDistance = 0.0;
    
    // 模拟运动过程并检查位置变化
    for (int i = 0; i < 50; i++) {
        axis_->update(0.02);
        double currentPos = axis_->getCurrentPosition();
        double delta = currentPos - lastPos;
        totalDistance += std::abs(delta);
        lastPos = currentPos;
        
        // 确保每次位置更新合理
        EXPECT_LE(std::abs(delta), axis_->getCurrentVelocity() * 0.02 * 1.1); // 允许10%的误差
    }
    
    // 验证总移动距离接近期望值
    EXPECT_NEAR(totalDistance, 50.0, 1.0);
}