# 测试配置

# 查找GTest包
find_package(GTest REQUIRED)

# 添加测试可执行文件
add_executable(xxcnc_tests
    # 核心控制模块测试
    core/CoreControllerTest.cpp
)

# 链接测试库
target_link_libraries(xxcnc_tests
    PRIVATE
        xxcnc
        GTest::GTest
        GTest::Main
)

# 添加测试
gtest_discover_tests(xxcnc_tests)

# 链接测试库
target_link_libraries(xxcnc_tests
    PRIVATE
        xxcnc
        GTest::GTest
        GTest::Main
)

# 添加测试
gtest_discover_tests(xxcnc_tests)