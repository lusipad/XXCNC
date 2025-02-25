# 源代码编译配置

# 查找依赖包
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)

# 添加库目标
add_library(xxcnc
    # 核心控制模块
    core/CoreController.cpp
    # G代码解析器
    core/gcode/GCodeParser.cpp
    # G代码宏指令管理器
    core/gcode/GCodeMacro.cpp
    core/gcode/GCodeMacroManager.cpp
    # 插补引擎
    core/motion/InterpolationEngine.cpp
    # 基于时间的插补器
    core/motion/TimeBasedInterpolator.cpp
    # 轴控制模块
    core/motion/AxisController.cpp
)

# 设置包含目录
target_include_directories(xxcnc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 添加web子目录
add_subdirectory(core/web)

# 链接依赖库
target_link_libraries(xxcnc
    PRIVATE
        spdlog::spdlog
        xxcnc_web
)

# 添加编译选项
target_compile_options(xxcnc
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/wd4996>  # 禁用 C4996 警告
)

# 设置库属性
set_target_properties(xxcnc PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# 导出目标
install(
    TARGETS xxcnc
    EXPORT XXCNCTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 添加服务器可执行文件
add_executable(xxcnc_server
    main.cpp
)

target_link_libraries(xxcnc_server
    PRIVATE
        xxcnc
        xxcnc_web
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)