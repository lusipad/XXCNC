# Web服务器模块

# 查找所需的依赖包
find_package(httplib CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# 复制静态资源到构建目录
file(COPY ${CMAKE_SOURCE_DIR}/src/core/web/static/ DESTINATION ${CMAKE_BINARY_DIR}/static)

add_library(xxcnc_web
    WebServer.cpp
)

target_include_directories(xxcnc_web
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/include>
        $<INSTALL_INTERFACE:include>
        ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}
)

target_link_libraries(xxcnc_web
    PUBLIC
        httplib::httplib
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

target_compile_features(xxcnc_web
    PUBLIC
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
