// TrajectoryViewer类 - 用于显示轨迹的3D查看器
class TrajectoryViewer {
    constructor(canvasId) {
        this.canvasId = canvasId;
        this.init();
    }

    init() {
        console.log(`TrajectoryViewer初始化开始，使用画布ID: ${this.canvasId}`);
        const canvas = document.getElementById(this.canvasId);
        if (!canvas) {
            console.error(`Canvas with id ${this.canvasId} not found`);
            return;
        }

        // 检查THREE.js是否可用
        if (typeof THREE === 'undefined') {
            console.error("THREE.js库未加载");
            return;
        }

        // 创建Three.js场景
        this.scene = new THREE.Scene();
        this.scene.background = new THREE.Color(0x1a1a1a);

        // 创建相机
        this.camera = new THREE.PerspectiveCamera(
            75, // 视野角度
            canvas.clientWidth / canvas.clientHeight, // 宽高比
            0.1, // 近截面
            1000 // 远截面
        );
        this.camera.position.set(50, 50, 100);
        this.camera.lookAt(0, 0, 0);

        // 创建渲染器
        this.renderer = new THREE.WebGLRenderer({ 
            canvas: canvas,
            antialias: true,
            alpha: true
        });
        this.renderer.setSize(canvas.clientWidth, canvas.clientHeight);
        
        console.log(`渲染器初始化完成，尺寸: ${canvas.clientWidth}x${canvas.clientHeight}`);

        // 添加轨道控制器
        if (typeof THREE.OrbitControls === 'undefined') {
            console.error("THREE.OrbitControls未加载");
        } else {
            this.controls = new THREE.OrbitControls(this.camera, this.renderer.domElement);
            this.controls.enableDamping = true;
            this.controls.dampingFactor = 0.25;
            this.controls.screenSpacePanning = false;
            this.controls.maxPolarAngle = Math.PI / 2;
            console.log("轨道控制器初始化完成");
        }

        // 创建坐标系网格和轴
        this.createGrid();
        this.createAxes();

        // 存储轨迹数据
        this.trajectoryPoints = [];
        this.trajectoryLine = null;
        
        // 添加光源
        const ambientLight = new THREE.AmbientLight(0xffffff, 0.5);
        this.scene.add(ambientLight);
        
        const directionalLight = new THREE.DirectionalLight(0xffffff, 0.8);
        directionalLight.position.set(50, 50, 50);
        this.scene.add(directionalLight);
        
        // 记录最后绘制的点
        this.lastPoint = null;
        
        // 启动动画循环
        this.animate();

        // 响应窗口大小变化
        window.addEventListener('resize', () => this.handleResize());
        
        console.log("TrajectoryViewer初始化完成");
    }

    createGrid() {
        // 创建网格
        const gridSize = 100;
        const gridDivisions = 10;
        const gridHelper = new THREE.GridHelper(gridSize, gridDivisions, 0x606060, 0x404040);
        this.scene.add(gridHelper);
    }

    createAxes() {
        // 创建坐标轴
        const axesHelper = new THREE.AxesHelper(50);
        this.scene.add(axesHelper);
        
        // 添加X、Y、Z标签
        this.addAxisLabel('X', 55, 0, 0, 0xff0000);
        this.addAxisLabel('Y', 0, 55, 0, 0x00ff00);
        this.addAxisLabel('Z', 0, 0, 55, 0x0000ff);
    }
    
    addAxisLabel(text, x, y, z, color) {
        const canvas = document.createElement('canvas');
        canvas.width = 64;
        canvas.height = 64;
        const context = canvas.getContext('2d');
        context.fillStyle = `#${color.toString(16).padStart(6, '0')}`;
        context.font = '48px Arial';
        context.fillText(text, 16, 48);
        
        const texture = new THREE.Texture(canvas);
        texture.needsUpdate = true;
        
        const material = new THREE.SpriteMaterial({ map: texture });
        const sprite = new THREE.Sprite(material);
        sprite.position.set(x, y, z);
        sprite.scale.set(10, 10, 1);
        
        this.scene.add(sprite);
    }

    addTrajectoryPoint(point, isRapid = false) {
        // 添加一个轨迹点
        if (!point || typeof point.x === 'undefined') return;
        
        const position = { x: point.x, y: point.y, z: point.z || 0 };
        this.trajectoryPoints.push(position);
        
        // 更新轨迹线
        this.updateTrajectoryLine();
        
        // 如果是快速移动，添加一个标记
        if (isRapid) {
            this.addRapidMarker(position);
        }
        
        this.lastPoint = position;
    }
    
    addPath(points) {
        if (!points || !Array.isArray(points) || points.length < 2) {
            console.error("无效的轨迹点数组", points);
            return;
        }
        
        console.log(`TrajectoryViewer添加路径：${points.length}个点`);
        
        // 清除现有轨迹
        this.clear();
        
        // 添加所有点
        points.forEach(point => {
            if (typeof point.x === 'number' && typeof point.y === 'number') {
                this.trajectoryPoints.push({
                    x: point.x, 
                    y: point.y, 
                    z: point.z || 0
                });
            } else {
                console.warn("跳过无效的轨迹点:", point);
            }
        });
        
        // 更新轨迹线
        this.updateTrajectoryLine();
        
        // 重置视图以显示整个路径
        this.resetView();
    }
    
    addRapidMarker(position) {
        const geometry = new THREE.SphereGeometry(1, 8, 8);
        const material = new THREE.MeshBasicMaterial({ color: 0xff0000 });
        const marker = new THREE.Mesh(geometry, material);
        marker.position.set(position.x, position.y, position.z);
        this.scene.add(marker);
    }

    updateTrajectoryLine() {
        // 先移除已存在的轨迹线
        if (this.trajectoryLine) {
            this.scene.remove(this.trajectoryLine);
        }
        
        if (this.trajectoryPoints.length < 2) return;

        // 创建轨迹线
        const geometry = new THREE.BufferGeometry();
        
        // 转换点数组为Float32Array
        const positions = new Float32Array(this.trajectoryPoints.length * 3);
        for (let i = 0; i < this.trajectoryPoints.length; i++) {
            positions[i * 3] = this.trajectoryPoints[i].x;
            positions[i * 3 + 1] = this.trajectoryPoints[i].y;
            positions[i * 3 + 2] = this.trajectoryPoints[i].z;
        }
        
        // 设置位置属性
        geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
        
        // 创建轨迹线材质
        const material = new THREE.LineBasicMaterial({ 
            color: 0x00ffff,
            linewidth: 2
        });
        
        this.trajectoryLine = new THREE.Line(geometry, material);
        this.scene.add(this.trajectoryLine);
    }

    clear() {
        // 清除轨迹
        this.trajectoryPoints = [];
        
        if (this.trajectoryLine) {
            this.scene.remove(this.trajectoryLine);
            this.trajectoryLine = null;
        }
        
        // 移除所有标记
        this.scene.children = this.scene.children.filter(child => {
            return !(child instanceof THREE.Mesh && child.geometry instanceof THREE.SphereGeometry);
        });
        
        this.lastPoint = null;
    }

    resetView() {
        // 重置视图，使所有轨迹点可见
        if (this.trajectoryPoints.length === 0) {
            this.camera.position.set(50, 50, 100);
            this.camera.lookAt(0, 0, 0);
            return;
        }
        
        // 计算轨迹点的包围盒
        const boundingBox = new THREE.Box3();
        this.trajectoryPoints.forEach(point => {
            boundingBox.expandByPoint(new THREE.Vector3(point.x, point.y, point.z));
        });
        
        // 计算包围盒中心
        const center = new THREE.Vector3();
        boundingBox.getCenter(center);
        
        // 计算包围盒大小
        const size = new THREE.Vector3();
        boundingBox.getSize(size);
        
        // 设置相机位置
        const maxDim = Math.max(size.x, size.y, size.z);
        const fov = this.camera.fov * (Math.PI / 180);
        let cameraZ = Math.abs(maxDim / Math.sin(fov / 2)) * 1.5;
        
        // 更新相机位置
        this.camera.position.set(center.x, center.y, center.z + cameraZ);
        this.controls.target.copy(center);
        
        this.camera.updateProjectionMatrix();
        this.controls.update();
    }

    handleResize() {
        const canvas = document.getElementById(this.canvasId);
        if (!canvas) return;
        
        // 更新相机宽高比
        this.camera.aspect = canvas.clientWidth / canvas.clientHeight;
        this.camera.updateProjectionMatrix();
        
        // 更新渲染器大小
        this.renderer.setSize(canvas.clientWidth, canvas.clientHeight);
    }

    animate() {
        requestAnimationFrame(() => this.animate());
        
        // 更新控制器
        if (this.controls) {
            this.controls.update();
        }
        
        // 渲染场景
        this.renderer.render(this.scene, this.camera);
    }
}

// 导出轨迹查看器类
window.TrajectoryViewer = TrajectoryViewer;