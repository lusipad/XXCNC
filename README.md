# XXCNC
一个用来验证一些想法的简单原型项目，主要由AI进行编写。

## 当前功能

### Web 界面
- 实时状态显示
  - 机器运行状态
  - XYZ轴位置
  - 进给速度控制
- 手动控制功能
  - 回零
  - 停止
  - 紧急停止
- G代码文件管理
  - 文件上传
  - 文件列表

### REST API
- `/health` - 健康检查
- `/api/status` - 获取系统状态
- `/api/command` - 发送控制命令
- `/api/files` - 文件管理
- `/api/config` - 配置管理

## 构建和运行

### 依赖
- C++17
- CMake 3.15+
- cpp-httplib
- nlohmann/json
- spdlog
- GTest (测试用)

### 构建
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### 运行
```bash
./xxcnc_server
```
服务器默认在 http://localhost:8080 启动

## 开发说明

本项目主要用于验证想法，代码由 AI 辅助编写。主要使用：
- Claude (API设计和核心功能)
- ChatGPT (问题修复和优化)

## 项目状态

目前处于原型验证阶段，功能在持续添加中。
