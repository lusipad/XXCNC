cmake_minimum_required(VERSION 3.15)

project(XXCNC VERSION 0.1.0 LANGUAGES CXX)

# 启用测试
enable_testing()

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(MSVC)
    add_compile_options(/W4 /WX /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# 添加子目录
add_subdirectory(src)
add_subdirectory(tests)

# 安装配置
include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/XXCNC)

# 导出配置
install(
    EXPORT XXCNCTargets
    NAMESPACE XXCNC::
    DESTINATION ${INSTALL_CONFIGDIR}
)

# 添加编译选项
target_compile_options(xxcnc
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/wd4996 /wd26495>  # 禁用特定警告
)

target_include_directories(xxcnc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)