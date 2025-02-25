// 全局变量
let statusUpdateInterval;

// API 端点
const API = {
    STATUS: '/api/status',
    COMMAND: '/api/command',
    FILE_UPLOAD: '/api/file/upload'
};

// DOM 元素
const elements = {
    status: document.getElementById('status'),
    posX: document.getElementById('posX'),
    posY: document.getElementById('posY'),
    posZ: document.getElementById('posZ'),
    speed: document.getElementById('speed'),
    feedRate: document.getElementById('feedRate'),
    feedRateValue: document.getElementById('feedRateValue'),
    fileInput: document.getElementById('fileInput'),
    fileList: document.getElementById('fileList')
};

// 控制按钮事件监听
document.getElementById('btnHome').addEventListener('click', () => sendCommand('home'));
document.getElementById('btnStop').addEventListener('click', () => sendCommand('stop'));
document.getElementById('btnPause').addEventListener('click', () => sendCommand('pause'));
document.getElementById('btnResume').addEventListener('click', () => sendCommand('resume'));

// 进给速度滑块事件监听
elements.feedRate.addEventListener('input', (e) => {
    elements.feedRateValue.textContent = e.target.value;
    sendCommand('setFeedRate', { value: parseInt(e.target.value) });
});

// 文件上传按钮事件监听
document.getElementById('btnUpload').addEventListener('click', uploadFile);

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
    elements.speed.textContent = data.feedRate || '0';

    // 更新轨迹和刀路信息
    updateTrajectory(data.position);
    updateToolPathInfo(data.toolPathDetails);
}

// 更新刀路详细信息
function updateToolPathInfo(toolPathDetails) {
    const toolPathInfoElement = document.getElementById('tool-path-info');
    toolPathInfoElement.textContent = toolPathDetails || '无刀路信息';
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

        return data;
    } catch (error) {
        console.error('Error sending command:', error);
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
        const response = await fetch(API.FILE_UPLOAD, {
            method: 'POST',
            body: formData
        });

        const data = await response.json();
        if (response.ok) {
            alert('文件上传成功');
            elements.fileInput.value = '';
        } else {
            throw new Error(data.error || '文件上传失败');
        }
    } catch (error) {
        console.error('Error uploading file:', error);
        alert(`文件上传失败：${error.message}`);
    }
}

// 启动状态更新定时器
statusUpdateInterval = setInterval(updateStatus, 1000);

// 页面卸载时清理定时器
window.addEventListener('unload', () => {
    if (statusUpdateInterval) {
        clearInterval(statusUpdateInterval);
    }
});