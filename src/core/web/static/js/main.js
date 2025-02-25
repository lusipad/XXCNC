// XXCNC Web界面主控制脚本
// 全局变量
let statusUpdateInterval;
let currentDrawingTool = null;
let drawingPoints = [];
let isDrawing = false;

// API 端点
const API = {
    STATUS: '/api/status',
    COMMAND: '/api/command',
    CONFIG: '/api/config',
    FILES: '/api/files'
};

// 初始化页面
document.addEventListener('DOMContentLoaded', function() {
    console.log("DOM内容加载完成，开始初始化界面...");
    
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
            logMessage(`[操作] 点击工具栏: ${action}`);
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
            
            logMessage(`[绘图] 选择工具: ${toolType}`);
        });
    });
}

// 初始化绘图区域
function initDrawingCanvas() {
    console.log("开始初始化绘图区域...");
    const canvas = document.getElementById('trajectoryCanvas');
    
    if (!canvas) {
        console.error("找不到trajectoryCanvas元素");
        logMessage('[错误] 找不到轨迹画布元素', 'error');
        return;
    }
    
    console.log("找到trajectoryCanvas元素");
    
    // 检查TrajectoryViewer是否可用
    console.log("检查TrajectoryViewer是否可用:", typeof window.TrajectoryViewer);
    
    // 初始化轨迹查看器
    if (window.TrajectoryViewer) {
        try {
            console.log("创建TrajectoryViewer实例");
            window.trajectoryViewer = new window.TrajectoryViewer('trajectoryCanvas');
            console.log("TrajectoryViewer实例创建成功:", window.trajectoryViewer);
            // 将轨迹查看器添加到窗口对象，使其全局可访问
            logMessage('[系统] 轨迹查看器初始化完成', 'info');
        } catch (error) {
            console.error("创建TrajectoryViewer实例失败:", error);
            logMessage('[错误] 轨迹查看器初始化失败: ' + error.message, 'error');
        }
    } else {
        console.error("TrajectoryViewer库未加载");
        logMessage('[错误] 轨迹查看器库未加载', 'error');
    }
    
    // 鼠标按下事件
    canvas.addEventListener('mousedown', function(e) {
        if (!currentDrawingTool) return;
        
        isDrawing = true;
        drawingPoints = [];
        
        // 获取坐标
        const rect = canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;
        
        drawingPoints.push({x, y, z: 0});
        
        logMessage(`[绘图] 开始绘制 (${x.toFixed(1)}, ${y.toFixed(1)})`);
    });
    
    // 鼠标移动事件
    canvas.addEventListener('mousemove', function(e) {
        if (!isDrawing) return;
        
        // 获取坐标
        const rect = canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;
        
        // 根据不同工具类型处理
        if (currentDrawingTool === '绘制线条') {
            drawingPoints.push({x, y, z: 0});
            updateDrawingPreview();
        }
    });
    
    // 鼠标松开事件
    canvas.addEventListener('mouseup', function(e) {
        if (!isDrawing) return;
        
        // 获取坐标
        const rect = canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;
        
        drawingPoints.push({x, y, z: 0});
        
        // 完成绘制
        finishDrawing();
        isDrawing = false;
    });
    
    // 鼠标离开画布
    canvas.addEventListener('mouseleave', function(e) {
        if (isDrawing) {
            finishDrawing();
            isDrawing = false;
        }
    });
}

// 更新绘图预览
function updateDrawingPreview() {
    // 使用Three.js轨迹查看器显示预览
    if (window.trajectoryViewer && drawingPoints.length > 1) {
        window.trajectoryViewer.addPath(drawingPoints);
    }
}

// 完成绘制
function finishDrawing() {
    if (drawingPoints.length < 2) return;
    
    logMessage(`[绘图] 完成绘制 (${drawingPoints.length} 个点)`, 'info');
    
    // 转换为G代码或其他格式
    const gcode = convertPointsToGCode(drawingPoints);
    
    // 可以发送到服务器或本地保存
    logMessage(`[绘图] 生成G代码: ${gcode.length} 个字符`);
    
    // 在轨迹查看器中展示最终结果
    if (window.trajectoryViewer) {
        window.trajectoryViewer.addPath(drawingPoints);
    }
}

// 转换点为G代码
function convertPointsToGCode(points) {
    if (!points || points.length < 2) return '';
    
    let gcode = ';Generated by XXCNC\n';
    gcode += 'G90 ; 使用绝对坐标\n';
    gcode += 'G21 ; 使用毫米单位\n';
    
    // 第一个点，快速移动
    gcode += `G0 X${points[0].x.toFixed(3)} Y${points[0].y.toFixed(3)} Z${points[0].z.toFixed(3)}\n`;
    
    // 后续点，线性插值
    for (let i = 1; i < points.length; i++) {
        gcode += `G1 X${points[i].x.toFixed(3)} Y${points[i].y.toFixed(3)} Z${points[i].z.toFixed(3)} F1000\n`;
    }
    
    gcode += 'G0 Z10 ; 抬起Z轴\n';
    return gcode;
}

// 初始化控制按钮
function initControlButtons() {
    // 坐标控制按钮
    document.querySelectorAll('.axis-button').forEach(button => {
        button.addEventListener('click', function() {
            const action = this.textContent.trim();
            logMessage(`[控制] 轴移动: ${action}`);
            
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
        console.error("开始加工按钮未找到!");
    }
    
    const stopMachiningBtn = document.getElementById('stop-machining-btn');
    if (stopMachiningBtn) {
        stopMachiningBtn.addEventListener('click', function() {
            stopMachining();
        });
    } else {
        console.error("停止加工按钮未找到!");
    }
    
    // 清除轨迹按钮
    const clearTrajectoryBtn = document.getElementById('clear-trajectory-btn');
    if (clearTrajectoryBtn) {
        clearTrajectoryBtn.addEventListener('click', function() {
            clearTrajectory();
        });
    } else {
        console.error("清除轨迹按钮未找到!");
    }
}

// 开始加工
async function startMachining() {
    console.log("开始加工");
    logMessage('[加工] 开始加工', 'info');
    
    // 获取当前文件名
    const currentFile = document.getElementById('current-file').textContent;
    if (!currentFile || currentFile === '无') {
        console.error("没有加载文件，无法开始加工");
        logMessage('[错误] 没有加载文件，请先装载文件', 'error');
        return;
    }
    
    try {
        // 清除现有轨迹
        if (window.trajectoryViewer) {
            console.log("清除现有轨迹");
            window.trajectoryViewer.clear();
        }
        
        // 发送开始加工命令到motion模块
        const result = await sendCommand('motion.start', { filename: currentFile });
        
        if (result) {
            logMessage(`[加工] 开始加工文件: ${currentFile}`, 'info');
            
            // 更新UI状态
            document.getElementById('status').textContent = '加工中';
            
            // 开始更新进度
            startProgressUpdate();
        } else {
            logMessage('[错误] 开始加工失败', 'error');
        }
    } catch (error) {
        console.error("开始加工失败:", error);
        logMessage(`[错误] 开始加工失败: ${error.message}`, 'error');
    }
}

// 停止加工
async function stopMachining() {
    console.log("停止加工");
    logMessage('[加工] 停止加工', 'info');
    
    try {
        // 发送停止加工命令到motion模块
        const result = await sendCommand('motion.stop');
        
        if (result) {
            logMessage('[加工] 加工已停止', 'warning');
            
            // 更新UI状态
            document.getElementById('status').textContent = '已停止';
            
            // 停止更新进度
            stopProgressUpdate();
        } else {
            logMessage('[错误] 停止加工失败', 'error');
        }
    } catch (error) {
        console.error("停止加工失败:", error);
        logMessage(`[错误] 停止加工失败: ${error.message}`, 'error');
    }
}

// 开始更新进度
let progressUpdateInterval;
function startProgressUpdate() {
    console.log("开始更新进度...");
    
    // 立即更新一次状态
    updateMachiningStatus();
    
    // 每秒更新一次进度
    progressUpdateInterval = setInterval(updateMachiningStatus, 1000);
}

// 更新加工状态
async function updateMachiningStatus() {
    try {
        console.log("获取加工状态...");
        const response = await fetch(API.STATUS);
        
        if (!response.ok) {
            console.error("获取加工状态失败，HTTP状态码:", response.status);
            return;
        }
        
        const text = await response.text();
        console.log("加工状态API响应文本:", text);
        
        try {
            const data = JSON.parse(text);
            console.log("解析后的加工状态数据:", data);
            
            if (data) {
                // 更新状态显示
                updateStatusDisplay(data);
                
                // 如果有machining字段，处理加工相关数据
                if (data.machining) {
                    console.log("处理加工数据:", data.machining);
                    
                    // 更新加工进度
                    if (data.machining.progress !== undefined) {
                        console.log("更新加工进度:", data.machining.progress);
                        document.getElementById('progress').textContent = `${Math.round(data.machining.progress * 100)}%`;
                    }
                    
                    // 如果有轨迹点，绘制轨迹
                    if (data.machining.trajectoryPoints && data.machining.trajectoryPoints.length > 0) {
                        console.log(`绘制 ${data.machining.trajectoryPoints.length} 个轨迹点`);
                        drawTrajectory(data.machining.trajectoryPoints);
                    } else {
                        console.warn("没有轨迹点数据");
                    }
                } else {
                    console.warn("数据中没有machining字段");
                }
                
                // 如果加工已完成，停止更新
                if (data.state !== "machining" && progressUpdateInterval) {
                    console.log("加工已完成，停止更新");
                    clearInterval(progressUpdateInterval);
                    progressUpdateInterval = null;
                }
            } else {
                console.error("加工状态数据为空");
            }
        } catch (jsonError) {
            console.error("解析JSON失败:", jsonError, "原始文本:", text);
        }
    } catch (error) {
        console.error('获取加工状态时出错:', error);
    }
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
    console.log("检查DOM元素...");
    
    // 文件上传输入
    const fileUploadInput = document.getElementById('file-upload');
    console.log("file-upload元素:", fileUploadInput ? "找到" : "未找到");
    
    if (!fileUploadInput) {
        console.error("文件上传输入框未找到!");
        return;
    }
    
    // 文件管理按钮
    const fileManagerBtn = document.getElementById('file-manager-btn');
    console.log("file-manager-btn元素:", fileManagerBtn ? "找到" : "未找到");
    
    if (!fileManagerBtn) {
        console.error("文件管理按钮未找到!");
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
            console.error("触发文件输入点击失败:", e);
        }
    });
    
    // 文件上传输入变更事件
    fileUploadInput.addEventListener('change', function(event) {
        console.log("文件上传输入框变更:", event);
        
        if (this.files && this.files.length > 0) {
            console.log("选择的文件:", this.files[0].name);
            uploadFile(this.files[0]);
        } else {
            console.log("没有选择文件");
        }
    });
    
    // 解析按钮
    const parseBtn = document.getElementById('parse-btn');
    console.log("parse-btn元素:", parseBtn ? "找到" : "未找到");
    
    if (!parseBtn) {
        console.error("解析按钮未找到!");
        return;
    }
    
    parseBtn.addEventListener('click', function() {
        console.log("解析按钮被点击");
        const currentFile = document.getElementById('current-file').textContent;
        console.log("当前文件:", currentFile);
        
        if (currentFile && currentFile !== '无') {
            parseFile(currentFile);
        } else {
            logMessage('[错误] 没有选择文件', 'error');
        }
    });
    
    console.log("文件管理初始化完成!");
}

// 上传文件
async function uploadFile(file) {
    if (!file) {
        console.error("没有文件传入uploadFile函数");
        logMessage('[错误] 没有选择文件', 'error');
        return;
    }
    
    console.log(`开始上传文件: ${file.name}, 大小: ${file.size} 字节, 类型: ${file.type}`);
    logMessage(`[文件] 开始上传: ${file.name} (${Math.round(file.size/1024)} KB)`, 'info');
    
    // 创建FormData对象
    const formData = new FormData();
    formData.append('file', file);
    
    // 检查FormData是否包含文件
    console.log("FormData内容:");
    for (const pair of formData.entries()) {
        console.log(`${pair[0]}: ${pair[1]} (类型: ${typeof pair[1]}, 是文件: ${pair[1] instanceof File})`);
    }
    
    try {
        console.log(`发送上传请求到 ${API.FILES}`);
        const response = await fetch(API.FILES, {
            method: 'POST',
            body: formData
        });
        
        console.log(`收到服务器响应:`, response);
        
        let data;
        try {
            data = await response.json();
            console.log(`响应数据:`, data);
        } catch (jsonError) {
            console.error("解析响应JSON失败:", jsonError);
            data = { error: "无法解析服务器响应" };
        }
        
        if (response.ok) {
            console.log("文件上传成功");
            logMessage(`[文件] 上传成功: ${file.name}`, 'info');
            
            // 更新UI
            const currentFileElement = document.getElementById('current-file');
            if (currentFileElement) {
                currentFileElement.textContent = file.name;
                console.log(`已更新当前文件显示为: ${file.name}`);
            } else {
                console.error("无法找到current-file元素");
            }
            
            // 不自动解析文件，需要用户点击解析按钮
            console.log(`文件已装载: ${file.name}，可以点击解析按钮进行解析`);
            logMessage(`[文件] 已装载: ${file.name}，请点击解析按钮进行解析`, 'info');
        } else {
            console.error(`上传响应错误: ${data.error || '未知错误'}`);
            logMessage(`[错误] 上传失败: ${data.error || '未知错误'}`, 'error');
        }
    } catch (error) {
        console.error("文件上传失败:", error);
        logMessage(`[错误] 文件上传失败: ${error.message}`, 'error');
    }
}

// 解析文件
async function parseFile(filename) {
    if (!filename) {
        console.error("没有文件名传入parseFile函数");
        logMessage('[错误] 没有指定文件名', 'error');
        return;
    }
    
    console.log(`开始解析文件: ${filename}`);
    logMessage(`[文件] 开始解析: ${filename}`, 'info');
    
    try {
        const parseUrl = `${API.FILES}/${encodeURIComponent(filename)}/parse`;
        console.log(`发送解析请求到 ${parseUrl}`);
        
        const response = await fetch(parseUrl);
        console.log(`收到服务器响应:`, response);
        
        let data;
        try {
            data = await response.json();
            console.log(`响应数据:`, data);
        } catch (jsonError) {
            console.error("解析响应JSON失败:", jsonError);
            data = { error: "无法解析服务器响应" };
        }
        
        if (response.ok && data.success) {
            console.log("文件解析成功");
            logMessage(`[文件] 解析成功: ${filename}`, 'info');
            
            if (data.trajectoryPoints && data.trajectoryPoints.length > 0) {
                console.log(`获取到 ${data.trajectoryPoints.length} 个轨迹点`);
                logMessage(`[轨迹] 获取到 ${data.trajectoryPoints.length} 个轨迹点`, 'info');
                
                // 绘制轨迹
                drawTrajectory(data.trajectoryPoints);
            } else {
                console.warn("没有获取到轨迹点");
                logMessage('[警告] 没有获取到轨迹点', 'warning');
            }
        } else {
            console.error(`解析响应错误: ${data.error || '未知错误'}`);
            logMessage(`[错误] 解析失败: ${data.error || '未知错误'}`, 'error');
        }
    } catch (error) {
        console.error("文件解析失败:", error);
        logMessage(`[错误] 文件解析失败: ${error.message}`, 'error');
    }
}

// 发送命令到服务器
async function sendCommand(command, params = {}) {
    try {
        const payload = {
            command: command,
            ...params
        };
        
        const response = await fetch(API.COMMAND, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(payload)
        });
        
        const data = await response.json();
        
        if (response.ok) {
            logMessage(`[命令] 命令执行成功: ${command}`, 'info');
            return true;
        } else {
            throw new Error(data.error || '命令执行失败');
        }
    } catch (error) {
        logMessage(`[错误] 命令执行失败 (${command}): ${error.message}`, 'error');
        return false;
    }
}

// 开始定时更新状态
function startStatusUpdates() {
    // 立即更新一次
    updateStatus();
    
    // 设置定时更新
    statusUpdateInterval = setInterval(updateStatus, 1000);
}

// 更新状态
async function updateStatus() {
    try {
        console.log("开始获取状态...");
        const response = await fetch(API.STATUS);
        console.log("状态API响应状态:", response.status);
        
        if (!response.ok) {
            console.error("获取状态失败，HTTP状态码:", response.status);
            return;
        }
        
        const text = await response.text();
        console.log("状态API响应文本:", text);
        
        try {
            const data = JSON.parse(text);
            console.log("解析后的状态数据:", data);
            
            if (data) {
                updateStatusDisplay(data);
            } else {
                console.error("状态数据为空");
            }
        } catch (jsonError) {
            console.error("解析JSON失败:", jsonError, "原始文本:", text);
        }
    } catch (error) {
        console.error('获取状态时出错:', error);
    }
}

// 更新状态显示
function updateStatusDisplay(data) {
    console.log("更新状态显示:", data);
    
    // 更新位置显示
    if (data.position) {
        console.log("更新位置:", data.position);
        document.getElementById('posX').textContent = data.position.x.toFixed(3);
        document.getElementById('posY').textContent = data.position.y.toFixed(3);
        document.getElementById('posZ').textContent = data.position.z.toFixed(3);
    } else {
        console.warn("数据中没有position字段");
    }
    
    // 更新状态
    if (data.state) {
        console.log("更新状态:", data.state);
        document.getElementById('status').textContent = data.state;
    } else {
        console.warn("数据中没有state字段");
    }
    
    // 更新进给速度
    if (data.feedRate) {
        console.log("更新进给速度:", data.feedRate);
        document.getElementById('feedRate').textContent = data.feedRate;
        document.getElementById('feedRateValue').value = data.feedRate;
    } else {
        console.warn("数据中没有feedRate字段");
    }
    
    // 更新当前文件
    if (data.currentFile && data.currentFile !== '') {
        console.log("更新当前文件:", data.currentFile);
        document.getElementById('current-file').textContent = data.currentFile;
    }
    
    // 更新进度
    if (data.progress !== undefined) {
        console.log("更新进度:", data.progress);
        document.getElementById('progress').textContent = `${Math.round(data.progress * 100)}%`;
    } else {
        console.warn("数据中没有progress字段");
    }
    
    // 如果有machining字段，处理加工相关数据
    if (data.machining) {
        console.log("处理加工数据:", data.machining);
        
        // 更新加工进度
        if (data.machining.progress !== undefined) {
            console.log("更新加工进度:", data.machining.progress);
            document.getElementById('progress').textContent = `${Math.round(data.machining.progress * 100)}%`;
        }
        
        // 如果有轨迹点，绘制轨迹
        if (data.machining.trajectoryPoints && data.machining.trajectoryPoints.length > 0) {
            console.log(`绘制 ${data.machining.trajectoryPoints.length} 个轨迹点`);
            drawTrajectory(data.machining.trajectoryPoints);
        }
    }
}

// 页面卸载时清理
window.addEventListener('beforeunload', function() {
    if (statusUpdateInterval) {
        clearInterval(statusUpdateInterval);
    }
});

// 日志记录函数
function logMessage(message, level = 'info') {
    const logElement = document.getElementById('log');
    if (!logElement) {
        console.error('日志元素不存在:', 'log');
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
    console.log("开始绘制轨迹...");
    
    if (!trajectoryPoints || !Array.isArray(trajectoryPoints) || trajectoryPoints.length === 0) {
        console.error("无效的轨迹点数据");
        logMessage('[错误] 无效的轨迹点数据', 'error');
        return;
    }
    
    console.log("轨迹点数据:", JSON.stringify(trajectoryPoints.slice(0, 2)));
    
    // 检查轨迹查看器是否初始化
    if (!window.trajectoryViewer) {
        console.error("轨迹查看器未初始化");
        logMessage('[错误] 轨迹查看器未初始化，尝试重新初始化', 'error');
        
        // 尝试重新初始化轨迹查看器
        if (window.TrajectoryViewer) {
            try {
                console.log("尝试重新创建TrajectoryViewer实例");
                window.trajectoryViewer = new window.TrajectoryViewer('trajectoryCanvas');
                console.log("TrajectoryViewer实例重新创建成功");
                logMessage('[系统] 轨迹查看器已重新初始化', 'info');
            } catch (error) {
                console.error("重新创建TrajectoryViewer实例失败:", error);
                logMessage('[错误] 轨迹查看器重新初始化失败: ' + error.message, 'error');
                return;
            }
        } else {
            console.error("TrajectoryViewer库未加载，无法重新初始化");
            logMessage('[错误] 轨迹查看器库未加载，无法绘制轨迹', 'error');
            return;
        }
    }
    
    try {
        // 清除现有轨迹
        console.log("清除现有轨迹");
        window.trajectoryViewer.clear();
        
        // 添加轨迹点
        console.log(`添加 ${trajectoryPoints.length} 个轨迹点`);
        
        // 确保轨迹点数据格式正确
        const formattedPoints = trajectoryPoints.map(point => {
            // 检查点是否有x, y, z坐标
            if (point.x === undefined || point.y === undefined || point.z === undefined) {
                console.warn("轨迹点缺少坐标:", point);
                return null;
            }
            
            return {
                x: point.x,
                y: point.y,
                z: point.z,
                isRapid: !!point.isRapid,
                command: point.command || "G01"
            };
        }).filter(point => point !== null);
        
        if (formattedPoints.length === 0) {
            console.error("格式化后没有有效的轨迹点");
            return;
        }
        
        console.log("格式化后的轨迹点:", JSON.stringify(formattedPoints.slice(0, 2)));
        window.trajectoryViewer.addPath(formattedPoints);
        
        // 重设视图以适应轨迹
        console.log("重设视图");
        window.trajectoryViewer.resetView();
        
        console.log("轨迹绘制完成");
        logMessage('[轨迹] 绘制完成', 'info');
    } catch (error) {
        console.error("轨迹绘制失败:", error);
        logMessage(`[错误] 轨迹绘制失败: ${error.message}`, 'error');
    }
}

// 清除轨迹
function clearTrajectory() {
    if (window.trajectoryViewer) {
        window.trajectoryViewer.clear();
        logMessage('[轨迹] 清除轨迹', 'info');
    } else {
        console.error("轨迹查看器未初始化");
        logMessage('[错误] 轨迹查看器未初始化', 'error');
    }
}