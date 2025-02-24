# 测试配置

# 查找GTest包（包含GMock）
find_package(GTest REQUIRED)

# 添加测试可执行文件
add_executable(xxcnc_tests
    # 核心控制模块测试
    core/CoreControllerTest.cpp
    # 插补引擎测试
    core/InterpolationEngineTest.cpp
    # G代码解析器测试
    core/GCodeParserTest.cpp
    # 轴控制模块测试
    core/motion/AxisControllerTest.cpp
    # Web服务器模块测试
    core/web/WebServerTest.cpp
)

# 链接测试库
target_link_libraries(xxcnc_tests
    PRIVATE
        xxcnc
        GTest::GTest
        GTest::Main
        GTest::gmock
)

# 添加测试
gtest_discover_tests(xxcnc_tests)
