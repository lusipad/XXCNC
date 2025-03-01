// XXCNC Web 界面主控制脚本
// 全局变量
let statusUpdateInterval;
let trajectoryUpdateInterval;
let currentDrawingTool = null;
let drawingPoints = [];
let isDrawing = false;
let cachedTrajectoryPoints = [];
let lastTrajectoryUpdateTime = 0;

// API 端点
const API = {
    STATUS: '/api/status',
    COMMAND: '/api/command',
    CONFIG: '/api/config',
    FILES: '/api/files'
};

// 初始化页面
document.addEventListener('DOMContentLoaded', function() {
    console.log("页面加载完成，初始化...");
    
    // 初始化轨迹画布
    const canvas = document.getElementById('trajectoryCanvas');
    if (canvas) {
        console.log("找到轨迹画布元素");
        // 设置画布尺寸为父容器大小
        const container = canvas.parentElement;
        canvas.width = container.clientWidth;
        canvas.height = 300;
    } else {
        console.error("找不到轨迹画布元素");
    }
    
    // 绑定工具栏按钮
    initToolbarButtons();
    
    // 绑定绘图区域事件
    initDrawingCanvas();
    
    // 绑定控制按钮事件
    initControlButtons();
    
    // 绑定文件管理
    console.log("准备初始化文件管理...");
    initFileManagement();
    console.log("文件管理初始化完成");
    
    // 开始定时更新状态
    startStatusUpdates();
    
    // 立即执行一次状态和轨迹更新
    updateSystemStatus();
    updateTrajectory();
    
    // 移除默认轨迹生成代码
    // setTimeout(() => {
    //     console.log("延迟1秒后生成测试轨迹数据");
    //     generateTestTrajectory();
    // }, 1000);
    
    // 记录初始化完成
    logMessage('[系统] 界面初始化完成', 'info');
    console.log("界面初始化全部完成");
});

// 初始化工具栏按钮
function initToolbarButtons() {
    // 顶部工具栏按钮
    document.querySelectorAll('.toolbar-button').forEach(button => {
        button.addEventListener('click', function() {
            const action = this.textContent.trim();
            logMessage(`[操作] 点击工具栏：${action}`);
        });
    });
    
    // 左侧工具栏按钮 (绘图工具)
    document.querySelectorAll('.sidebar-button').forEach(button => {
        button.addEventListener('click', function() {
            // 移除其他按钮的活动状态
            document.querySelectorAll('.sidebar-button').forEach(btn => {
                btn.classList.remove('active');
            });
            
            // 设置当前按钮为活动状态
            this.classList.add('active');
            
            // 获取工具类型
            const toolType = this.getAttribute('title');
            currentDrawingTool = toolType;
            
            logMessage(`[绘图] 选择工具：${toolType}`);
        });
    });
}

// 初始化绘图画布
function initDrawingCanvas() {
    console.log("初始化绘图画布...");
    
    // 获取画布元素
    const canvas = document.getElementById('trajectoryCanvas');
    if (!canvas) {
        console.error("找不到轨迹画布元素");
        return;
    }
    
    // 设置2D画布
    canvas.width = canvas.clientWidth;
    canvas.height = canvas.clientHeight;
    const ctx = canvas.getContext('2d');
    
    // 初始化3D轨迹查看器
    try {
        // 检查THREE.js是否已加载
        if (typeof THREE === 'undefined') {
            console.error("THREE.js库未加载，无法初始化3D轨迹查看器");
            return;
        }
        
        console.log("开始初始化3D轨迹查看器...");
        window.trajectoryViewer = new TrajectoryViewer('trajectoryCanvas3D');
        
        // 移除测试轨迹
        // console.log("测试轨迹查看器...");
        // const testPoints = [
        //     { x: 0, y: 0, z: 0 },
        //     { x: 10, y: 0, z: 0 },
        //     { x: 10, y: 10, z: 0 },
        //     { x: 0, y: 10, z: 0 },
        //     { x: 0, y: 0, z: 0 }
        // ];
        // window.trajectoryViewer.addPath(testPoints);
        // console.log("轨迹查看器测试完成");
    } catch (error) {
        console.error("初始化3D轨迹查看器时出错:", error);
    }
    
    // 设置绘图区域事件监听
    setupDrawingEvents(canvas, ctx);
    
    // 绘制网格
    drawGrid(ctx, canvas.width, canvas.height);
    
    // 添加清除轨迹按钮事件
    const clearTrajectoryBtn = document.getElementById('clear-trajectory-btn');
    if (clearTrajectoryBtn) {
        clearTrajectoryBtn.addEventListener('click', function() {
            console.log("清除轨迹按钮被点击");
            clearTrajectory();
            logMessage('[轨迹] 轨迹已清除', 'info');
        });
    } else {
        console.error("清除轨迹按钮未找到！");
    }
}

// 绘制网格背景
function drawGrid(ctx, width, height) {
    const gridSize = 20; // 网格大小
    
    ctx.clearRect(0, 0, width, height);
    ctx.strokeStyle = 'rgba(100, 100, 100, 0.2)';
    ctx.lineWidth = 0.5;
    
    // 绘制水平线
    for (let y = 0; y <= height; y += gridSize) {
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(width, y);
        ctx.stroke();
    }
    
    // 绘制垂直线
    for (let x = 0; x <= width; x += gridSize) {
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, height);
        ctx.stroke();
    }
    
    // 绘制坐标轴
    ctx.strokeStyle = 'rgba(255, 0, 0, 0.5)';
    ctx.lineWidth = 1;
    
    // X轴
    ctx.beginPath();
    ctx.moveTo(0, height / 2);
    ctx.lineTo(width, height / 2);
    ctx.stroke();
    
    // Y轴
    ctx.beginPath();
    ctx.moveTo(width / 2, 0);
    ctx.lineTo(width / 2, height);
    ctx.stroke();
}

// 初始化控制按钮
function initControlButtons() {
    console.log("初始化控制按钮...");
    
    // 坐标控制按钮
    document.querySelectorAll('.axis-button').forEach(button => {
        button.addEventListener('click', function() {
            const action = this.textContent.trim();
            logMessage(`[控制] 轴移动：${action}`);
            
            // 解析按钮动作
            if (action === 'H') {
                sendCommand('home');
            } else if (action.includes('X') || action.includes('Y') || action.includes('Z') || 
                     action.includes('A') || action.includes('B')) {
                const axis = action.charAt(0);
                const direction = action.charAt(1) === '+' ? 1 : -1;
                sendCommand('jog', { axis, direction });
            } else if (action === '↑') {
                sendCommand('jog', { axis: 'Y', direction: 1 });
            } else if (action === '↓') {
                sendCommand('jog', { axis: 'Y', direction: -1 });
            } else if (action === '←') {
                sendCommand('jog', { axis: 'X', direction: -1 });
            } else if (action === '→') {
                sendCommand('jog', { axis: 'X', direction: 1 });
            }
        });
    });
    
    // 进给速度滑块
    document.getElementById('feedRateValue').addEventListener('input', function() {
        const value = this.value;
        document.getElementById('feedRate').textContent = value;
        sendCommand('setFeedRate', { value: parseInt(value) });
    });
    
    // 控制按钮
    document.getElementById('btnHome').addEventListener('click', function() {
        sendCommand('home');
        logMessage('[命令] 执行回零操作', 'info');
    });
    
    document.getElementById('btnStop').addEventListener('click', function() {
        sendCommand('stop');
        logMessage('[命令] 执行停止操作', 'warning');
    });
    
    document.getElementById('btnEStop').addEventListener('click', function() {
        sendCommand('estop');
        logMessage('[命令] 执行紧急停止操作', 'error');
    });
    
    document.getElementById('start-btn').addEventListener('click', function() {
        sendCommand('start');
        logMessage('[命令] 开始执行程序', 'info');
    });
    
    // 开始加工和停止加工按钮
    const startMachiningBtn = document.getElementById('start-machining-btn');
    if (startMachiningBtn) {
        startMachiningBtn.addEventListener('click', function() {
            startMachining();
        });
    } else {
        console.error("开始加工按钮未找到！");
    }
    
    const stopMachiningBtn = document.getElementById('stop-machining-btn');
    if (stopMachiningBtn) {
        stopMachiningBtn.addEventListener('click', function() {
            stopMachining();
        });
    } else {
        console.error("停止加工按钮未找到！");
    }
    
    // 清除轨迹按钮
    const clearTrajectoryBtn = document.getElementById('clear-trajectory-btn');
    if (clearTrajectoryBtn) {
        clearTrajectoryBtn.addEventListener('click', function() {
            logMessage('[操作] 点击清除轨迹按钮', 'info');
            clearTrajectory();
        });
    } else {
        console.error("找不到清除轨迹按钮");
    }
    
    console.log("控制按钮初始化完成");
}

// 开始加工
async function startMachining() {
    console.log("开始加工");
    logMessage('[加工] 开始加工', 'info');
    
    try {
        // 清除现有轨迹
        clearTrajectory();
        
        // 获取当前文件名
        const currentFile = document.getElementById('current-file').textContent;
        console.log(`当前文件名: "${currentFile}"`);
        
        if (!currentFile || currentFile === '无') {
            console.error("没有加载文件，无法开始加工");
            logMessage('[错误] 没有加载文件，无法开始加工', 'error');
            return false;
        }
        
        logMessage(`[加工] 开始加工文件：${currentFile}`, 'info');
        
        // 发送开始加工命令到后端
        console.log(`准备发送motion.start命令，文件名: ${currentFile}`);
        const result = await sendCommand('motion.start', { filename: currentFile });
        console.log(`motion.start命令执行结果: ${result ? '成功' : '失败'}`);
        
        if (result) {
            // 更新 UI 状态
            document.getElementById('status').textContent = '加工中';
            
            // 开始定时更新状态和轨迹
            console.log("开始定时更新状态和轨迹");
            startProgressUpdate();
            
            logMessage('[加工] 加工命令已发送，开始执行', 'success');
            return true;
        } else {
            console.error("开始加工命令发送失败");
            logMessage('[错误] 开始加工命令发送失败', 'error');
            return false;
        }
    } catch (error) {
        console.error("开始加工失败：", error);
        logMessage(`[错误] 开始加工失败：${error.message}`, 'error');
        return false;
    }
}

// 停止加工
async function stopMachining() {
    console.log("停止加工");
    logMessage('[加工] 正在停止加工...', 'warning');
    
    try {
        // 禁用停止按钮，防止重复点击
        const stopButton = document.getElementById('stopButton');
        if (stopButton) {
            stopButton.disabled = true;
            stopButton.textContent = '正在停止...';
        }
        
        // 发送停止加工命令到 motion 模块
        console.log("发送停止命令: motion.stop");
        const result = await sendCommand('motion.stop');
        console.log("停止命令执行结果:", result);
        
        if (result) {
            logMessage('[加工] 加工已停止', 'success');
            
            // 更新 UI 状态
            document.getElementById('status').textContent = '已停止';
            
            // 停止更新进度
            console.log("停止更新进度");
            stopProgressUpdate();
            
            // 重新启用停止按钮
            if (stopButton) {
                stopButton.disabled = false;
                stopButton.textContent = '停止';
            }
            
            // 更新一次状态，确保显示最终位置
            console.log("更新最终状态和轨迹");
            await updateSystemStatus();
            await updateTrajectory();
            
            console.log("停止过程完成");
        } else {
            logMessage('[错误] 停止加工失败', 'error');
            console.error("停止命令返回失败");
            
            // 重新启用停止按钮
            if (stopButton) {
                stopButton.disabled = false;
                stopButton.textContent = '停止';
            }
        }
    } catch (error) {
        console.error("停止加工失败：", error);
        logMessage(`[错误] 停止加工失败：${error.message}`, 'error');
        
        // 重新启用停止按钮
        const stopButton = document.getElementById('stopButton');
        if (stopButton) {
            stopButton.disabled = false;
            stopButton.textContent = '停止';
        }
    }
}

// 开始更新进度
let progressUpdateInterval;
function startProgressUpdate() {
    console.log("开始更新进度...");
    logMessage('[状态] 开始定时更新状态和轨迹', 'info');
    
    // 清除可能存在的旧定时器
    if (progressUpdateInterval) {
        console.log("清除现有进度更新定时器");
        clearInterval(progressUpdateInterval);
        progressUpdateInterval = null;
    }
    
    // 立即更新一次状态
    updateMachiningStatus();
    
    // 每秒更新一次进度
    progressUpdateInterval = setInterval(updateMachiningStatus, 1000);
    console.log("状态更新定时器已设置");
}

// 停止更新进度
function stopProgressUpdate() {
    if (progressUpdateInterval) {
        clearInterval(progressUpdateInterval);
        progressUpdateInterval = null;
    }
}

// 初始化文件管理
function initFileManagement() {
    console.log("开始初始化文件管理...");
    console.log("检查 DOM 元素...");
    
    // 文件上传输入
    const fileUploadInput = document.getElementById('file-upload');
    console.log("file-upload 元素：", fileUploadInput ? "找到" : "未找到");
    
    if (!fileUploadInput) {
        console.error("文件上传输入框未找到！");
        return;
    }
    
    // 文件管理按钮
    const fileManagerBtn = document.getElementById('file-manager-btn');
    console.log("file-manager-btn 元素：", fileManagerBtn ? "找到" : "未找到");
    
    if (!fileManagerBtn) {
        console.error("文件管理按钮未找到！");
        return;
    }
    
    console.log("绑定事件...");
    
    // 文件管理按钮点击事件
    fileManagerBtn.addEventListener('click', function(event) {
        console.log("文件管理按钮被点击", event);
        logMessage('[文件] 打开文件选择器', 'info');
        console.log("准备触发文件输入点击...");
        
        try {
            // 触发文件输入点击
            fileUploadInput.click();
            console.log("文件输入点击成功触发");
        } catch (e) {
            console.error("触发文件输入点击失败：", e);
        }
    });
    
    // 文件上传输入变更事件
    fileUploadInput.addEventListener('change', function(event) {
        console.log("文件上传输入框变更：", event);
        
        if (this.files && this.files.length > 0) {
            console.log("选择的文件：", this.files[0].name);
            uploadFile(this.files[0]);
        } else {
            console.log("没有选择文件");
        }
    });
    
    // 解析按钮
    const parseBtn = document.getElementById('parse-btn');
    console.log("parse-btn 元素：", parseBtn ? "找到" : "未找到");
    
    if (!parseBtn) {
        console.error("解析按钮未找到！");
        return;
    }
    
    parseBtn.addEventListener('click', function() {
        console.log("解析按钮被点击");
        const currentFile = document.getElementById('current-file').textContent;
        console.log("当前文件：", currentFile);
        
        if (currentFile && currentFile !== '无') {
            parseFile(currentFile);
        } else {
            logMessage('[错误] 没有选择文件', 'error');
        }
    });
    
    console.log("文件管理初始化完成！");
}

// 上传文件
async function uploadFile(file) {
    if (!file) {
        console.error("没有文件传入 uploadFile 函数");
        logMessage('[错误] 没有选择文件', 'error');
        return;
    }
    
    console.log(`开始上传文件：${file.name}, 大小：${file.size} 字节，类型：${file.type}`);
    logMessage(`[文件] 开始上传：${file.name} (${Math.round(file.size/1024)} KB)`, 'info');
    
    const formData = new FormData();
    formData.append('file', file);
    
    try {
        const response = await fetch('/api/files', {
            method: 'POST',
            body: formData
        });
        
        const data = await response.json();
        
        if (data.success) {
            console.log("文件上传成功");
            logMessage(`[文件] 上传成功：${file.name}`, 'success');
            
            // 更新 UI
            const currentFileElement = document.getElementById('current-file');
            if (currentFileElement) {
                currentFileElement.textContent = file.name;
                console.log(`已更新当前文件显示为：${file.name}`);
            } else {
                console.error("无法找到 current-file 元素");
            }
            
            // 不自动解析文件，需要用户点击解析按钮
            console.log(`文件已装载：${file.name}，可以点击解析按钮进行解析`);
            logMessage(`[文件] 已装载：${file.name}，请点击解析按钮进行解析`, 'info');
            
            return true;
        } else {
            console.error("文件上传失败:", data.error);
            logMessage(`[错误] 文件上传失败: ${data.error || '未知错误'}`, 'error');
            return false;
        }
    } catch (error) {
        console.error("文件上传出错:", error);
        logMessage(`[错误] 文件上传出错: ${error.message}`, 'error');
        return false;
    }
}

// 解析文件
async function parseFile(filename) {
    if (!filename) {
        console.error("没有文件名传入 parseFile 函数");
        logMessage('[错误] 没有指定文件名', 'error');
        return;
    }
    
    console.log(`开始解析文件：${filename}`);
    logMessage(`[文件] 开始解析：${filename}`, 'info');
    
    try {
        // 模拟文件解析成功
        console.log("模拟文件解析成功");
        logMessage(`[文件] 解析成功：${filename}`, 'info');
        
        // 生成测试轨迹点
        const testTrajectoryPoints = generateTestTrajectory();
        
        // 绘制轨迹
        drawTrajectory(testTrajectoryPoints);
        
        return true;
    } catch (error) {
        console.error("文件解析失败：", error);
        logMessage(`[错误] 文件解析失败：${error.message}`, 'error');
        return false;
    }
}

// 发送命令到后端
async function sendCommand(command, params = {}) {
    try {
        console.log(`发送命令: ${command}，参数:`, params);
        logMessage(`[命令] 发送命令: ${command}`, 'info');
        
        const payload = {
            command: command,
            ...params
        };
        
        const payloadStr = JSON.stringify(payload);
        console.log(`发送到API: ${API.COMMAND}，请求体:`, payloadStr);
        
        // 使用真实API
        const response = await fetch(API.COMMAND, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: payloadStr
        });
        
        console.log(`收到响应，状态码: ${response.status}`);
        
        const text = await response.text();
        console.log(`API响应文本: ${text}`);
        logMessage(`[响应] 状态码: ${response.status}, 内容: ${text}`, 'info');
        
        try {
            const data = JSON.parse(text);
            console.log(`API响应数据:`, data);
            
            if (response.ok) {
                console.log(`命令 ${command} 执行成功`);
                logMessage(`[命令] 命令执行成功：${command}`, 'success');
                return true;
            } else {
                console.error(`命令 ${command} 执行失败:`, data.error || '未知错误');
                throw new Error(data.error || '命令执行失败');
            }
        } catch (jsonError) {
            console.error(`解析响应JSON失败:`, jsonError);
            console.error(`原始响应:`, text);
            logMessage(`[错误] 解析响应JSON失败: ${jsonError.message}`, 'error');
            throw new Error(`解析响应失败: ${jsonError.message}`);
        }
    } catch (error) {
        console.error(`命令执行失败 (${command}):`, error);
        logMessage(`[错误] 命令执行失败 (${command}): ${error.message}`, 'error');
        return false;
    }
}

// 开始定时更新状态
function startStatusUpdates() {
    console.log("开始定时更新状态...");
    
    // 清除可能存在的旧定时器
    stopStatusUpdates();
    
    // 每秒更新一次状态
    statusUpdateInterval = setInterval(updateSystemStatus, 1000);
    console.log("状态更新定时器已设置");
    
    // 每秒更新一次轨迹
    trajectoryUpdateInterval = setInterval(updateTrajectory, 1000);
    console.log("轨迹更新定时器已设置");
    
    logMessage('[系统] 开始定时更新状态和轨迹', 'info');
}

// 停止更新状态
function stopStatusUpdates() {
    if (statusUpdateInterval) {
        clearInterval(statusUpdateInterval);
        statusUpdateInterval = null;
    }
    
    if (trajectoryUpdateInterval) {
        clearInterval(trajectoryUpdateInterval);
        trajectoryUpdateInterval = null;
    }
}

// 更新系统状态
async function updateSystemStatus() {
    try {
        console.log("开始获取状态...");
        const response = await fetch(API.STATUS);
        console.log("状态 API 响应状态：", response.status);
        
        if (!response.ok) {
            console.error("获取状态失败，HTTP 状态码：", response.status);
            return;
        }
        
        const text = await response.text();
        console.log("状态 API 响应文本：", text);
        
        try {
            const data = JSON.parse(text);
            console.log("解析后的状态数据：", data);
            
            if (data) {
                updateStatusDisplay(data);
            } else {
                console.error("状态数据为空");
            }
        } catch (jsonError) {
            console.error("解析 JSON 失败：", jsonError, "原始文本：", text);
        }
    } catch (error) {
        console.error('获取状态时出错：', error);
    }
}

// 更新轨迹
async function updateTrajectory() {
    try {
        console.log("开始获取轨迹...");
        const response = await fetch(API.STATUS);
        
        if (!response.ok) {
            console.error("获取轨迹失败，HTTP 状态码：", response.status);
            return;
        }
        
        const text = await response.text();
        console.log("轨迹 API 响应文本长度：", text.length);
        
        try {
            const data = JSON.parse(text);
            console.log("解析后的轨迹数据：", data);
            
            if (data) {
                // 直接检查 trajectoryPoints 字段
                if (data.trajectoryPoints && data.trajectoryPoints.length > 0) {
                    console.log(`获取到 ${data.trajectoryPoints.length} 个轨迹点，准备绘制`);
                    console.log("轨迹点示例：", data.trajectoryPoints.slice(0, Math.min(5, data.trajectoryPoints.length)));
                    
                    // 确保轨迹点有效
                    const validPoints = data.trajectoryPoints.filter(point => 
                        typeof point.x === 'number' && 
                        typeof point.y === 'number' && 
                        typeof point.z === 'number');
                    
                    if (validPoints.length > 0) {
                        console.log(`有效轨迹点：${validPoints.length} 个，开始绘制`);
                        drawTrajectory(validPoints);
                    } else {
                        console.warn("没有有效的轨迹点可以绘制");
                        console.warn("无效轨迹点示例：", data.trajectoryPoints.slice(0, Math.min(5, data.trajectoryPoints.length)));
                    }
                } else {
                    console.log("没有轨迹点数据");
                    if (data.trajectoryPoints) {
                        console.log("轨迹点数组为空");
                    } else {
                        console.log("响应中没有 trajectoryPoints 字段");
                    }
                }
            } else {
                console.error("轨迹数据为空");
            }
        } catch (jsonError) {
            console.error("解析 JSON 失败：", jsonError, "原始文本：", text);
        }
    } catch (error) {
        console.error('获取轨迹时出错：', error);
    }
}

// 清除轨迹
async function clearTrajectory() {
    try {
        console.log("清除轨迹...");
        logMessage('[轨迹] 正在清除轨迹...', 'info');
        
        // 禁用清除轨迹按钮，防止重复点击
        const clearButton = document.getElementById('clear-trajectory-btn');
        if (clearButton) {
            clearButton.disabled = true;
            clearButton.textContent = '正在清除...';
        }
        
        // 发送清除轨迹命令
        console.log("发送清除轨迹命令: trajectory.clear");
        const result = await sendCommand('trajectory.clear');
        console.log("清除轨迹命令执行结果:", result);
        
        if (result) {
            logMessage('[轨迹] 轨迹已清除', 'success');
            
            // 清除3D轨迹查看器
            if (window.trajectoryViewer) {
                console.log("清除3D轨迹");
                window.trajectoryViewer.clear();
            }
            
            // 清除2D轨迹
            const canvas = document.getElementById('trajectoryCanvas');
            if (canvas) {
                console.log("清除2D轨迹");
                const ctx = canvas.getContext('2d');
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                drawGrid(ctx, canvas.width, canvas.height);
            }
            
            console.log("轨迹清除完成");
        } else {
            logMessage('[错误] 清除轨迹失败', 'error');
            console.error("清除轨迹命令返回失败");
        }
        
        // 重新启用清除轨迹按钮
        if (clearButton) {
            clearButton.disabled = false;
            clearButton.textContent = '清除轨迹';
        }
    } catch (error) {
        console.error("清除轨迹失败：", error);
        logMessage(`[错误] 清除轨迹失败：${error.message}`, 'error');
        
        // 重新启用清除轨迹按钮
        const clearButton = document.getElementById('clear-trajectory-btn');
        if (clearButton) {
            clearButton.disabled = false;
            clearButton.textContent = '清除轨迹';
        }
    }
}

// 更新状态显示
function updateStatusDisplay(data) {
    console.log("更新状态显示：", data);
    
    // 更新位置显示
    if (data.position) {
        console.log("更新位置：", data.position);
        document.getElementById('posX').textContent = data.position.x.toFixed(3);
        document.getElementById('posY').textContent = data.position.y.toFixed(3);
        document.getElementById('posZ').textContent = data.position.z.toFixed(3);
    } else {
        console.warn("数据中没有 position 字段");
    }
    
    // 更新状态
    if (data.state) {
        console.log("更新状态：", data.state);
        document.getElementById('status').textContent = data.state;
    } else {
        console.warn("数据中没有 state 字段");
    }
    
    // 更新进给速度
    if (data.feedRate) {
        console.log("更新进给速度：", data.feedRate);
        document.getElementById('feedRate').textContent = data.feedRate;
        document.getElementById('feedRateValue').value = data.feedRate;
    } else {
        console.warn("数据中没有 feedRate 字段");
    }
    
    // 更新当前文件
    if (data.currentFile && data.currentFile !== '') {
        console.log("更新当前文件：", data.currentFile);
        document.getElementById('current-file').textContent = data.currentFile;
    }
    
    // 更新进度
    if (data.progress !== undefined) {
        console.log("更新进度：", data.progress);
        document.getElementById('progress').textContent = `${Math.round(data.progress * 100)}%`;
    } else {
        console.warn("数据中没有 progress 字段");
    }
}

// 页面卸载时清理
window.addEventListener('beforeunload', function() {
    if (statusUpdateInterval) {
        clearInterval(statusUpdateInterval);
    }
    
    if (trajectoryUpdateInterval) {
        clearInterval(trajectoryUpdateInterval);
    }
});

// 日志记录函数
function logMessage(message, level = 'info') {
    const logElement = document.getElementById('log');
    if (!logElement) {
        console.error('日志元素不存在：', 'log');
        return;
    }
    
    const entry = document.createElement('div');
    entry.className = 'log-entry';
    if (level) {
        entry.classList.add(`log-${level}`);
    }
    entry.textContent = message;
    
    logElement.appendChild(entry);
    logElement.scrollTop = logElement.scrollHeight;
    
    // 同时输出到控制台以便调试
    console.log(`[${level}] ${message}`);
}

// 绘制轨迹
function drawTrajectory(trajectoryPoints) {
    if (!trajectoryPoints || !Array.isArray(trajectoryPoints) || trajectoryPoints.length === 0) {
        console.warn("无效的轨迹点数组:", trajectoryPoints);
        return;
    }
    
    console.log(`绘制轨迹，点数量: ${trajectoryPoints.length}`);
    
    // 记录前几个点用于调试
    if (trajectoryPoints.length > 0) {
        console.log("轨迹点示例:", trajectoryPoints.slice(0, Math.min(5, trajectoryPoints.length)));
    }
    
    // 使用3D轨迹查看器绘制
    if (window.trajectoryViewer) {
        try {
            console.log("使用3D轨迹查看器绘制轨迹");
            // 清除现有轨迹并添加新轨迹
            window.trajectoryViewer.addPath(trajectoryPoints);
            console.log("3D轨迹绘制完成");
        } catch (error) {
            console.error("使用轨迹查看器绘制轨迹时出错:", error);
        }
    } else {
        console.warn("轨迹查看器未初始化，无法绘制3D轨迹");
    }
    
    // 同时在2D画布上绘制
    const canvas = document.getElementById('trajectoryCanvas');
    if (!canvas) {
        console.error("找不到轨迹画布元素");
        return;
    }
    
    const ctx = canvas.getContext('2d');
    
    // 清除画布
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    // 重新绘制网格
    drawGrid(ctx, canvas.width, canvas.height);
    
    // 绘制轨迹
    if (trajectoryPoints.length > 0) {
        console.log("开始在2D画布上绘制轨迹");
        ctx.beginPath();
        ctx.strokeStyle = '#00FFFF';
        ctx.lineWidth = 2;
        
        // 移动到第一个点
        const firstPoint = trajectoryPoints[0];
        const canvasX1 = firstPoint.x * 10 + canvas.width / 2;
        const canvasY1 = canvas.height / 2 - firstPoint.y * 10;
        ctx.moveTo(canvasX1, canvasY1);
        console.log(`第一个点: (${firstPoint.x}, ${firstPoint.y}) -> 画布坐标: (${canvasX1}, ${canvasY1})`);
        
        // 连接到其他点
        for (let i = 1; i < trajectoryPoints.length; i++) {
            const point = trajectoryPoints[i];
            const canvasX = point.x * 10 + canvas.width / 2;
            const canvasY = canvas.height / 2 - point.y * 10;
            ctx.lineTo(canvasX, canvasY);
            
            if (i % 10 === 0 || i === trajectoryPoints.length - 1) {
                console.log(`第 ${i} 个点: (${point.x}, ${point.y}) -> 画布坐标: (${canvasX}, ${canvasY})`);
            }
        }
        
        ctx.stroke();
        console.log("2D轨迹绘制完成");
    }
}

// 更新加工状态
async function updateMachiningStatus() {
    try {
        console.log("获取加工状态...");
        logMessage('[状态] 正在获取加工状态', 'info');
        
        console.log(`请求状态API: ${API.STATUS}`);
        const response = await fetch(API.STATUS);
        console.log(`状态API响应状态码: ${response.status}`);
        
        if (!response.ok) {
            console.error("获取加工状态失败，HTTP 状态码：", response.status);
            logMessage(`[错误] 获取加工状态失败，HTTP 状态码: ${response.status}`, 'error');
            return;
        }
        
        const text = await response.text();
        console.log("加工状态 API 响应文本：", text);
        
        try {
            const data = JSON.parse(text);
            console.log("解析后的加工状态数据：", data);
            
            if (data) {
                updateStatusDisplay(data);
                
                // 如果有 machining 字段，处理加工相关数据
                if (data.machining) {
                    console.log("处理加工数据：", data.machining);
                    
                    // 更新加工进度
                    if (data.machining.progress !== undefined) {
                        console.log("更新加工进度：", data.machining.progress);
                        document.getElementById('progress').textContent = `${Math.round(data.machining.progress * 100)}%`;
                    }
                    
                    // 如果有轨迹点，绘制轨迹
                    if (data.machining.trajectoryPoints && data.machining.trajectoryPoints.length > 0) {
                        console.log(`绘制 ${data.machining.trajectoryPoints.length} 个轨迹点`);
                        logMessage(`[轨迹] 收到 ${data.machining.trajectoryPoints.length} 个轨迹点`, 'info');
                        
                        // 确保轨迹点有效
                        const validPoints = data.machining.trajectoryPoints.filter(point => 
                            typeof point.x === 'number' && 
                            typeof point.y === 'number');
                        
                        if (validPoints.length > 0) {
                            console.log(`有效轨迹点：${validPoints.length} 个，开始绘制`);
                            drawTrajectory(validPoints);
                        } else {
                            console.warn("没有有效的轨迹点可以绘制");
                            logMessage("[警告] 没有有效的轨迹点可以绘制", "warning");
                        }
                    } else {
                        console.log("没有轨迹点数据");
                        logMessage("[轨迹] 当前没有轨迹点数据", "warning");
                    }
                } else {
                    console.warn("数据中没有 machining 字段");
                }
                
                // 如果加工已完成，停止更新
                if (data.state !== "machining" && progressUpdateInterval) {
                    console.log("加工已完成，停止更新");
                    clearInterval(progressUpdateInterval);
                    progressUpdateInterval = null;
                    logMessage("[加工] 加工已完成", "success");
                }
            } else {
                console.error("状态数据为空");
                logMessage("[错误] 状态数据为空", "error");
            }
        } catch (jsonError) {
            console.error("解析 JSON 失败：", jsonError, "原始文本：", text);
            logMessage(`[错误] 解析状态 JSON 失败: ${jsonError.message}`, "error");
        }
    } catch (error) {
        console.error('获取状态时出错：', error);
        logMessage(`[错误] 获取状态时出错: ${error.message}`, "error");
    }
}

// 生成测试轨迹数据
function generateTestTrajectory() {
    console.log("生成测试轨迹数据");
    
    // 创建一个简单的正方形轨迹
    const size = 50;
    const center = { x: 0, y: 0, z: 0 };
    
    const trajectoryPoints = [
        { x: center.x - size, y: center.y - size, z: 0 },
        { x: center.x + size, y: center.y - size, z: 0 },
        { x: center.x + size, y: center.y + size, z: 0 },
        { x: center.x - size, y: center.y + size, z: 0 },
        { x: center.x - size, y: center.y - size, z: 0 }
    ];
    
    // 添加一个螺旋上升的部分
    for (let i = 0; i < 50; i++) {
        const angle = i * 0.2;
        const radius = size * (1 - i / 100);
        trajectoryPoints.push({
            x: center.x + radius * Math.cos(angle),
            y: center.y + radius * Math.sin(angle),
            z: i / 2
        });
    }
    
    console.log(`生成了 ${trajectoryPoints.length} 个测试轨迹点`);
    
    // 绘制测试轨迹
    drawTrajectory(trajectoryPoints);
    
    return trajectoryPoints;
}