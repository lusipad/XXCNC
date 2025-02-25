// 全局变量
let statusUpdateInterval;
let viewer;

// API 端点
const API = {
    STATUS: '/api/status',
    COMMAND: '/api/command',
    FILE_UPLOAD: '/api/files'
};

// DOM 元素
const elements = {
    status: document.getElementById('status'),
    posX: document.getElementById('posX'),
    posY: document.getElementById('posY'),
    posZ: document.getElementById('posZ'),
    feedRate: document.getElementById('feedRate'),
    feedRateValue: document.getElementById('feedRateValue'),
    fileInput: document.getElementById('file-upload'),
    fileList: document.getElementById('file-list'),
    currentFile: document.getElementById('current-file'),
    progress: document.getElementById('progress'),
    logContent: document.getElementById('log-content')
};

// 初始化页面
window.addEventListener('load', () => {
    // 初始化轨迹查看器
    viewer = new TrajectoryViewer('trajectoryCanvas');
    window.trajectoryViewer = viewer;

    // 控制按钮事件监听
    document.getElementById('btnHome').addEventListener('click', () => sendCommand('home'));
    document.getElementById('btnStop').addEventListener('click', () => sendCommand('stop'));
    document.getElementById('btnEStop').addEventListener('click', () => sendCommand('estop'));
    document.getElementById('start-btn').addEventListener('click', () => sendCommand('start'));
    document.getElementById('pause-btn').addEventListener('click', () => sendCommand('pause'));
    document.getElementById('resume-btn').addEventListener('click', () => sendCommand('resume'));

    // 进给速度滑块事件监听
    elements.feedRateValue.addEventListener('input', (e) => {
        elements.feedRate.textContent = e.target.value;
        sendCommand('setFeedRate', { value: parseInt(e.target.value) });
    });

    // 文件上传事件监听
    elements.fileInput.addEventListener('change', uploadFile);

    // 启动状态更新
    updateStatus();
    statusUpdateInterval = setInterval(updateStatus, 1000);
});

// 更新机器状态
async function updateStatus() {
    try {
        const response = await fetch(API.STATUS);
        const data = await response.json();
        
        if (response.ok) {
            updateStatusDisplay(data);
        } else {
            console.error('Status update failed:', data.error);
        }
    } catch (error) {
        console.error('Error updating status:', error);
    }
}

// 更新状态显示
function updateStatusDisplay(data) {
    elements.status.textContent = data.state || '未知';
    elements.posX.textContent = data.position?.x?.toFixed(3) || '0.000';
    elements.posY.textContent = data.position?.y?.toFixed(3) || '0.000';
    elements.posZ.textContent = data.position?.z?.toFixed(3) || '0.000';
    elements.feedRate.textContent = data.feedRate || '100';
    elements.feedRateValue.value = data.feedRate || '100';

    // 更新当前文件信息
    if (data.currentFile) {
        elements.currentFile.textContent = data.currentFile;
    }

    // 更新轨迹
    if (data.position) {
        viewer.addTrajectoryPoint(data.position);
    }

    // 更新刀路信息
    updateToolPathInfo(data.toolPathDetails);

    // 更新进度
    if (data.progress !== undefined) {
        elements.progress.textContent = `${Math.round(data.progress * 100)}%`;
    }
}

// 更新刀路详细信息
function updateToolPathInfo(toolPathDetails) {
    const toolPathInfoElement = document.getElementById('tool-path-info');
    if (Array.isArray(toolPathDetails)) {
        // 如果是数组，说明是G代码行的数组
        toolPathInfoElement.textContent = toolPathDetails.join('\n');
    } else if (typeof toolPathDetails === 'string') {
        // 如果是字符串，直接显示
        toolPathInfoElement.textContent = toolPathDetails;
    } else {
        toolPathInfoElement.textContent = '无刀路信息';
    }
}

// 发送控制命令
async function sendCommand(command, params = {}) {
    try {
        const response = await fetch(API.COMMAND, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                command,
                ...params
            })
        });

        const data = await response.json();
        if (!response.ok) {
            throw new Error(data.error || '命令执行失败');
        }

        logAction(`执行命令：${command}`);
        return data;
    } catch (error) {
        console.error('发送命令时发生错误：', error);
        alert(`命令执行失败：${error.message}`);
    }
}

// 上传文件
async function uploadFile() {
    const file = elements.fileInput.files[0];
    if (!file) {
        alert('请选择要上传的文件');
        return;
    }

    const formData = new FormData();
    formData.append('file', file);

    try {
        logAction(`开始上传文件：${file.name}`);
        
        const response = await fetch(API.FILE_UPLOAD, {
            method: 'POST',
            body: formData
        });

        console.log('Upload response:', response);  
        const data = await response.json();
        console.log('Upload data:', data);  

        if (response.ok) {
            logAction(`文件上传成功：${file.name}`);
            elements.fileInput.value = '';
            
            // 发送设置当前文件的命令
            await sendCommand('setCurrentFile', { filename: file.name });
            elements.currentFile.textContent = file.name;

            // 请求文件解析状态
            const parseResponse = await fetch(`/api/files/${encodeURIComponent(file.name)}/parse`);
            const parseData = await parseResponse.json();
            
            if (parseResponse.ok) {
                // 更新刀路详细信息
                if (parseData.toolPathDetails) {
                    updateToolPathInfo(parseData.toolPathDetails);
                }
                
                // 更新轨迹
                if (parseData.trajectoryPoints && Array.isArray(parseData.trajectoryPoints)) {
                    logAction(`开始加载轨迹（${parseData.trajectoryPoints.length} 个点）`);
                    viewer.clearTrajectory();
                    
                    // 添加所有轨迹点
                    parseData.trajectoryPoints.forEach((point, index) => {
                        viewer.addTrajectoryPoint({
                            x: point.x,
                            y: point.y,
                            z: point.z
                        }, point.type === 'rapid');
                        
                        // 每100个点记录一次进度
                        if ((index + 1) % 100 === 0) {
                            logAction(`已加载轨迹点：${index + 1}/${parseData.trajectoryPoints.length}`);
                        }
                    });
                    
                    logAction('轨迹加载完成');
                    viewer.resetView();  // 重置视图以显示完整轨迹
                } else {
                    logAction('警告：没有找到有效的轨迹点数据');
                }
            } else {
                throw new Error(parseData.error || '文件解析失败');
            }
        } else {
            throw new Error(data.error || '文件上传失败');
        }
    } catch (error) {
        console.error('Error uploading file:', error);
        logAction(`错误：文件上传失败 - ${error.message}`);
        alert(`文件上传失败：${error.message}`);
    }
}

// 页面卸载时清理定时器
window.addEventListener('unload', () => {
    if (statusUpdateInterval) {
        clearInterval(statusUpdateInterval);
    }
});

function logAction(action) {
    const timestamp = new Date().toLocaleString();
    elements.logContent.textContent += `[${timestamp}] ${action}\n`;
    elements.logContent.scrollTop = elements.logContent.scrollHeight;
}