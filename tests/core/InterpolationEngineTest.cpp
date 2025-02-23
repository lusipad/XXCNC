#include <gtest/gtest.h>
#include "xxcnc/core/InterpolationEngine.h"

class InterpolationEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<InterpolationEngine>();
    }

    void TearDown() override {
        engine.reset();
    }

    std::unique_ptr<InterpolationEngine> engine;
};

// 直线插补测试
TEST_F(InterpolationEngineTest, LinearInterpolationBasic) {
    // 测试基本的直线插补
    Point start{0.0, 0.0, 0.0};
    Point end{10.0, 10.0, 0.0};
    double feedRate = 1000.0; // mm/min

    auto segments = engine->calculateLinearPath(start, end, feedRate);
    ASSERT_FALSE(segments.empty());

    // 验证起点和终点
    EXPECT_DOUBLE_EQ(segments.front().startPoint.x, start.x);
    EXPECT_DOUBLE_EQ(segments.front().startPoint.y, start.y);
    EXPECT_DOUBLE_EQ(segments.back().endPoint.x, end.x);
    EXPECT_DOUBLE_EQ(segments.back().endPoint.y, end.y);

    // 验证路径的连续性
    for (size_t i = 1; i < segments.size(); ++i) {
        EXPECT_DOUBLE_EQ(segments[i-1].endPoint.x, segments[i].startPoint.x);
        EXPECT_DOUBLE_EQ(segments[i-1].endPoint.y, segments[i].startPoint.y);
    }
}

// 圆弧插补测试
TEST_F(InterpolationEngineTest, ArcInterpolationBasic) {
    // 测试基本的圆弧插补
    Point start{0.0, 0.0, 0.0};
    Point end{10.0, 0.0, 0.0};
    Point center{5.0, 5.0, 0.0};
    bool isClockwise = true;
    double feedRate = 1000.0; // mm/min

    auto segments = engine->calculateArcPath(start, end, center, isClockwise, feedRate);
    ASSERT_FALSE(segments.empty());

    // 验证起点和终点
    EXPECT_DOUBLE_EQ(segments.front().startPoint.x, start.x);
    EXPECT_DOUBLE_EQ(segments.front().startPoint.y, start.y);
    EXPECT_DOUBLE_EQ(segments.back().endPoint.x, end.x);
    EXPECT_DOUBLE_EQ(segments.back().endPoint.y, end.y);

    // 验证所有点到圆心的距离相等（允许小误差）
    double radius = std::sqrt(std::pow(start.x - center.x, 2) + std::pow(start.y - center.y, 2));
    for (const auto& segment : segments) {
        double startRadius = std::sqrt(std::pow(segment.startPoint.x - center.x, 2) + 
                                     std::pow(segment.startPoint.y - center.y, 2));
        EXPECT_NEAR(startRadius, radius, 0.001);
    }
}

// 边界条件测试
TEST_F(InterpolationEngineTest, EdgeCases) {
    // 测试起点和终点重合的情况
    Point point{0.0, 0.0, 0.0};
    double feedRate = 1000.0;

    auto segments = engine->calculateLinearPath(point, point, feedRate);
    EXPECT_TRUE(segments.empty());

    // 测试极小距离的移动
    Point nearPoint{0.001, 0.001, 0.0};
    segments = engine->calculateLinearPath(point, nearPoint, feedRate);
    ASSERT_FALSE(segments.empty());
    EXPECT_LE(segments.size(), 2); // 应该只需要很少的分段
}

// 性能测试
TEST_F(InterpolationEngineTest, Performance) {
    Point start{0.0, 0.0, 0.0};
    Point end{1000.0, 1000.0, 0.0};
    double feedRate = 5000.0; // 高速进给

    auto startTime = std::chrono::high_resolution_clock::now();
    auto segments = engine->calculateLinearPath(start, end, feedRate);
    auto endTime = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    EXPECT_LT(duration.count(), 1000); // 期望计算时间小于1ms

    // 验证分段数量在合理范围内
    size_t expectedSegments = static_cast<size_t>(std::sqrt(std::pow(end.x - start.x, 2) + 
                                                          std::pow(end.y - start.y, 2))) + 1;
    EXPECT_LE(segments.size(), expectedSegments);
}

// 错误处理测试
TEST_F(InterpolationEngineTest, ErrorHandling) {
    Point start{0.0, 0.0, 0.0};
    Point end{10.0, 10.0, 0.0};
    Point center{5.0, 5.0, 0.0};

    // 测试非法进给速度
    EXPECT_THROW(engine->calculateLinearPath(start, end, -1.0), std::invalid_argument);
    EXPECT_THROW(engine->calculateLinearPath(start, end, 0.0), std::invalid_argument);

    // 测试圆弧插补中的非法参数
    EXPECT_THROW(engine->calculateArcPath(start, end, start, true, 1000.0), std::invalid_argument); // 圆心不能是起点
    EXPECT_THROW(engine->calculateArcPath(start, end, end, true, 1000.0), std::invalid_argument);   // 圆心不能是终点
}