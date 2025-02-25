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

// 测试文件上传
async function testFileUpload() {
    console.log("测试文件上传");
    logMessage('[测试] 开始测试文件上传', 'info');
    
    try {
        // 创建一个简单的文本文件
        const content = `G0 X0 Y0 Z0
G1 X10 Y0 Z0
G1 X10 Y10 Z0
G1 X0 Y10 Z0
G1 X0 Y0 Z0`;
        
        // 创建Blob对象
        const blob = new Blob([content], { type: 'text/plain' });
        
        // 创建File对象
        const file = new File([blob], 'test_upload.nc', { type: 'text/plain' });
        
        console.log(`创建测试文件: ${file.name}, 大小: ${file.size} 字节`);
        logMessage(`[测试] 创建文件: ${file.name}`, 'info');
        
        // 上传文件
        await uploadFile(file);
    } catch (error) {
        console.error("测试文件上传失败:", error);
        logMessage(`[错误] 测试文件上传失败: ${error.message}`, 'error');
    }
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
            
            // 自动解析文件
            console.log(`准备解析文件: ${file.name}`);
            parseFile(file.name);
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
        const response = await fetch(API.STATUS);
        const data = await response.json();
        
        if (response.ok) {
            updateStatusDisplay(data);
        }
    } catch (error) {
        console.error('Error updating status:', error);
    }
}

// 更新状态显示
function updateStatusDisplay(data) {
    // 更新位置显示
    if (data.position) {
        document.getElementById('posX').textContent = data.position.x.toFixed(3);
        document.getElementById('posY').textContent = data.position.y.toFixed(3);
        document.getElementById('posZ').textContent = data.position.z.toFixed(3);
    }
    
    // 更新状态
    if (data.state) {
        document.getElementById('status').textContent = data.state;
    }
    
    // 更新进给速度
    if (data.feedRate) {
        document.getElementById('feedRate').textContent = data.feedRate;
        document.getElementById('feedRateValue').value = data.feedRate;
    }
    
    // 更新当前文件
    if (data.currentFile && data.currentFile !== '') {
        document.getElementById('current-file').textContent = data.currentFile;
    }
    
    // 更新进度
    if (data.progress !== undefined) {
        document.getElementById('progress').textContent = `${Math.round(data.progress * 100)}%`;
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
        window.trajectoryViewer.addPath(trajectoryPoints);
        
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

// 生成测试轨迹
function generateTestTrajectory() {
    console.log("生成测试轨迹");
    logMessage('[测试] 生成测试轨迹', 'info');
    
    const points = [];
    
    // 生成一个正弦波形
    for (let i = -50; i <= 50; i++) {
        points.push({
            x: i * 2,
            y: 30 * Math.sin(i * 0.1),
            z: 0,
            type: i % 5 === 0 ? "rapid" : "feed"
        });
    }
    
    // 加上一个方形
    const squareSize = 40;
    const squareOffset = 0;
    points.push({x: -squareSize + squareOffset, y: -squareSize + squareOffset, z: 0, type: "rapid"});
    points.push({x: squareSize + squareOffset, y: -squareSize + squareOffset, z: 0, type: "feed"});
    points.push({x: squareSize + squareOffset, y: squareSize + squareOffset, z: 0, type: "feed"});
    points.push({x: -squareSize + squareOffset, y: squareSize + squareOffset, z: 0, type: "feed"});
    points.push({x: -squareSize + squareOffset, y: -squareSize + squareOffset, z: 0, type: "feed"});
    
    console.log(`生成了 ${points.length} 个测试轨迹点`);
    
    // 绘制轨迹
    drawTrajectory(points);
    logMessage('[轨迹] 测试轨迹已生成', 'info');
}