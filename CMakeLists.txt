cmake_minimum_required(VERSION 3.15)

# 设置 vcpkg 工具链文件
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "D:/Repos/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# 设置 Visual Studio 环境
set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_VS_PLATFORM_NAME "x64")
set(CMAKE_VS_PLATFORM_TOOLSET "v143")
set(CMAKE_SYSTEM_PROCESSOR AMD64)

project(XXCNC VERSION 0.1.0 LANGUAGES CXX)

# 启用测试
enable_testing()

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(MSVC)
    add_compile_options(/W4 /WX /utf-8 /wd4996 /wd26495 /wd4251 /wd4275)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0")
    set(CMAKE_VS_PLATFORM_TOOLSET "v143")
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 设置包含目录
include_directories(${CMAKE_SOURCE_DIR}/src/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

# 查找依赖包
find_package(httplib CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# 添加子目录
add_subdirectory(src)
add_subdirectory(tests)

# 安装配置
# include(GNUInstallDirs)
# set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/XXCNC)

# 导出配置
# install(
#     EXPORT XXCNCTargets
#     NAMESPACE XXCNC::
#     DESTINATION ${INSTALL_CONFIGDIR}
# )

# 添加编译选项
# target_compile_options(xxcnc
#     PRIVATE
#         $<$<CXX_COMPILER_ID:MSVC>:/wd4996 /wd26495>  # 禁用特定警告
# )

# target_include_directories(xxcnc
#     PUBLIC
#         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#         $<INSTALL_INTERFACE:include>
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/src
# )

# 设置安装目标
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/src/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# 导出目标
install(
    EXPORT XXCNCTargets
    FILE XXCNCTargets.cmake
    NAMESPACE XXCNC::
    DESTINATION lib/cmake/XXCNC
)