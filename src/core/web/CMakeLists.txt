# Web服务器模块

# 查找所需的依赖包
find_package(httplib CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# 复制静态资源到构建目录
file(COPY ${CMAKE_SOURCE_DIR}/static/ DESTINATION ${CMAKE_BINARY_DIR}/static)

add_library(xxcnc_web
    WebServer.cpp
)

target_include_directories(xxcnc_web
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries(xxcnc_web
    PRIVATE
        httplib::httplib
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

target_compile_features(xxcnc_web
    PRIVATE
        cxx_std_17
)

# 导出目标
install(
    TARGETS xxcnc_web
    EXPORT XXCNCTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
