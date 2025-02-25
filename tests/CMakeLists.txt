# 测试配置

# 查找GTest包（包含GMock）
find_package(GTest CONFIG REQUIRED)

# 启用测试
enable_testing()

# 添加测试可执行文件
add_executable(xxcnc_tests
    # 核心控制模块测试
    core/CoreControllerTest.cpp
    # 插补引擎测试
    core/InterpolationEngineTest.cpp
    # G代码解析器测试
    core/GCodeParserTest.cpp
    # G代码宏指令管理器测试
    core/gcode/GCodeMacroManagerTest.cpp
    # 轴控制模块测试
    core/motion/AxisControllerTest.cpp
    # Web服务器模块测试
    core/web/WebServerTest.cpp
)

# 设置包含目录
target_include_directories(xxcnc_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/include
)

# 链接测试库
target_link_libraries(xxcnc_tests
    PRIVATE
        xxcnc
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

# 直接添加测试
add_test(NAME xxcnc_tests COMMAND xxcnc_tests)
