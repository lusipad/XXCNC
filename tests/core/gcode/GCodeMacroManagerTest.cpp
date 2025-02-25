#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "xxcnc/core/gcode/GCodeMacroManager.h"
#include <iostream>

using namespace xxcnc;
using namespace testing;

int main(int argc, char **argv) {
    std::cout << "Starting GCodeMacroManager tests..." << std::endl;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// 创建一个测试用的宏指令类
class TestMacro : public GCodeMacro {
public:
    TestMacro(const std::string& name) : GCodeMacro(name) {}

    std::vector<std::unique_ptr<GCodeCommand>> execute() override {
        std::vector<std::unique_ptr<GCodeCommand>> commands;
        // 创建一个简单的运动指令作为测试
        auto params = std::make_unique<MotionParams>();
        params->x = getParameter("X");
        params->y = getParameter("Y");
        params->feedrate = getParameter("F");
        commands.push_back(std::make_unique<MotionCommand>(std::move(params)));
        return commands;
    }
};

class GCodeMacroManagerTest : public Test {
protected:
    void SetUp() override {
        manager = std::make_unique<GCodeMacroManager>();
    }

    std::unique_ptr<GCodeMacroManager> manager;
};

TEST_F(GCodeMacroManagerTest, RegisterMacro) {
    auto macro = std::make_unique<TestMacro>("TEST_MACRO");
    EXPECT_NO_THROW(manager->registerMacro(std::move(macro)));
    EXPECT_EQ(manager->getMacroCount(), 1);
}

TEST_F(GCodeMacroManagerTest, RegisterDuplicateMacro) {
    auto macro1 = std::make_unique<TestMacro>("TEST_MACRO");
    auto macro2 = std::make_unique<TestMacro>("TEST_MACRO");
    
    manager->registerMacro(std::move(macro1));
    EXPECT_THROW(manager->registerMacro(std::move(macro2)), std::runtime_error);
}

TEST_F(GCodeMacroManagerTest, FindMacro) {
    auto macro = std::make_unique<TestMacro>("TEST_MACRO");
    auto* macroPtr = macro.get();
    manager->registerMacro(std::move(macro));

    EXPECT_EQ(manager->findMacro("TEST_MACRO"), macroPtr);
    EXPECT_EQ(manager->findMacro("NONEXISTENT"), nullptr);
}

TEST_F(GCodeMacroManagerTest, ExecuteMacro) {
    auto macro = std::make_unique<TestMacro>("TEST_MACRO");
    macro->addParameterDefinition("X", true);
    macro->addParameterDefinition("Y", true);
    macro->addParameterDefinition("F", true);
    manager->registerMacro(std::move(macro));

    std::map<std::string, double> params = {
        {"X", 100.0},
        {"Y", 200.0},
        {"F", 1000.0}
    };

    auto commands = manager->executeMacro("TEST_MACRO", params);
    ASSERT_EQ(commands.size(), 1);

    auto* cmd = commands[0].get();
    auto* motionParams = dynamic_cast<MotionParams*>(cmd->getParams());
    ASSERT_NE(motionParams, nullptr);
    EXPECT_DOUBLE_EQ(motionParams->x, 100.0);
    EXPECT_DOUBLE_EQ(motionParams->y, 200.0);
    EXPECT_DOUBLE_EQ(motionParams->feedrate, 1000.0);
}

TEST_F(GCodeMacroManagerTest, ExecuteNonexistentMacro) {
    std::map<std::string, double> params;
    EXPECT_THROW(manager->executeMacro("NONEXISTENT", params), std::runtime_error);
}

TEST_F(GCodeMacroManagerTest, RemoveMacro) {
    auto macro = std::make_unique<TestMacro>("TEST_MACRO");
    manager->registerMacro(std::move(macro));
    EXPECT_NO_THROW(manager->removeMacro("TEST_MACRO"));
    EXPECT_EQ(manager->getMacroCount(), 0);
}

TEST_F(GCodeMacroManagerTest, RemoveNonexistentMacro) {
    EXPECT_THROW(manager->removeMacro("NONEXISTENT"), std::runtime_error);
}

TEST_F(GCodeMacroManagerTest, ClearMacros) {
    manager->registerMacro(std::make_unique<TestMacro>("MACRO1"));
    manager->registerMacro(std::make_unique<TestMacro>("MACRO2"));
    EXPECT_EQ(manager->getMacroCount(), 2);

    manager->clearMacros();
    EXPECT_EQ(manager->getMacroCount(), 0);
}

TEST_F(GCodeMacroManagerTest, MacroParameterValidation) {
    auto macro = std::make_unique<TestMacro>("TEST_MACRO");
    macro->addParameterDefinition("X", true);  // 必需参数
    macro->addParameterDefinition("Y", false, 0.0);  // 可选参数
    macro->addParameterDefinition("F", true);  // 添加必需的进给速度参数
    manager->registerMacro(std::move(macro));

    // 缺少必需参数
    std::map<std::string, double> invalid_params = {
        {"Y", 200.0}
    };
    EXPECT_THROW(manager->executeMacro("TEST_MACRO", invalid_params), std::runtime_error);

    // 有效参数
    std::map<std::string, double> valid_params = {
        {"X", 100.0},
        {"Y", 200.0}
    };
    EXPECT_NO_THROW(manager->executeMacro("TEST_MACRO", valid_params));
} 