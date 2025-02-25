class TrajectoryViewer {
    constructor(canvasId) {
        this.canvas = document.getElementById(canvasId);
        if (!this.canvas) {
            throw new Error(`Canvas element with id '${canvasId}' not found`);
        }

        this.scene = new THREE.Scene();
        this.camera = new THREE.PerspectiveCamera(75, this.canvas.clientWidth / this.canvas.clientHeight, 0.1, 1000);
        this.renderer = new THREE.WebGLRenderer({
            canvas: this.canvas,
            antialias: true
        });

        // 设置相机位置
        this.camera.position.set(100, 100, 100);
        this.camera.lookAt(0, 0, 0);

        // 设置渲染器尺寸和背景色
        this.renderer.setClearColor(0x000000);  // 黑色背景
        this.updateRendererSize();

        // 添加网格和坐标轴
        this.setupScene();

        // 轨迹数据
        this.trajectoryPoints = [];
        this.trajectoryLines = [];  // 存储轨迹线段
        this.rapidColor = 0xff0000;  // 快速定位颜色 - 红色
        this.feedColor = 0x00ff00;   // 进给颜色 - 绿色

        // 视图控制
        this.setupControls();

        // 添加事件监听
        this.addEventListeners();

        // 开始渲染循环
        this.animate();
    }

    setupScene() {
        // 添加网格
        const gridHelper = new THREE.GridHelper(200, 20, 0x404040, 0x404040);
        this.scene.add(gridHelper);

        // 添加坐标轴
        const axesHelper = new THREE.AxesHelper(100);
        this.scene.add(axesHelper);

        // 添加环境光
        const ambientLight = new THREE.AmbientLight(0xffffff, 0.7);
        this.scene.add(ambientLight);

        // 添加平行光
        const directionalLight = new THREE.DirectionalLight(0xffffff, 0.5);
        directionalLight.position.set(100, 100, 100);
        this.scene.add(directionalLight);
    }

    setupControls() {
        // 实现轨道控制器
        this.controls = new THREE.OrbitControls(this.camera, this.renderer.domElement);
        this.controls.enableDamping = true;
        this.controls.dampingFactor = 0.05;
        this.controls.enablePan = true;
        this.controls.enableZoom = true;
        this.controls.enableRotate = true;
        this.controls.autoRotate = false;
    }

    updateRendererSize() {
        const rect = this.canvas.parentElement.getBoundingClientRect();
        const width = rect.width;
        const height = rect.height;
        
        this.canvas.style.width = '100%';
        this.canvas.style.height = '100%';
        
        this.renderer.setSize(width, height, false);
        this.camera.aspect = width / height;
        this.camera.updateProjectionMatrix();
    }

    addTrajectoryPoint(point, isRapid = false) {
        if (!point || typeof point.x !== 'number' || typeof point.y !== 'number' || typeof point.z !== 'number') {
            console.error('Invalid trajectory point:', point);
            return;
        }

        const newPoint = new THREE.Vector3(point.x, point.y, point.z);

        if (this.trajectoryPoints.length === 0) {
            this.trajectoryPoints.push(newPoint);
            return;
        }

        const lastPoint = this.trajectoryPoints[this.trajectoryPoints.length - 1];
        const geometry = new THREE.BufferGeometry().setFromPoints([lastPoint, newPoint]);

        const material = new THREE.LineBasicMaterial({
            color: isRapid ? this.rapidColor : this.feedColor,
            linewidth: 2  // 注意：由于WebGL限制，线宽可能在某些浏览器中不起作用
        });

        const line = new THREE.Line(geometry, material);
        this.scene.add(line);
        this.trajectoryLines.push(line);  // 保存线段引用
        this.trajectoryPoints.push(newPoint);
    }

    clearTrajectory() {
        // 移除所有轨迹线
        this.trajectoryLines.forEach(line => {
            this.scene.remove(line);
            line.geometry.dispose();
            line.material.dispose();
        });
        this.trajectoryLines = [];
        this.trajectoryPoints = [];
    }

    resetView() {
        if (this.trajectoryPoints.length === 0) {
            // 如果没有轨迹点，使用默认视图
            this.camera.position.set(100, 100, 100);
            this.camera.lookAt(0, 0, 0);
            return;
        }

        // 计算轨迹的边界框
        const box = new THREE.Box3();
        this.trajectoryPoints.forEach(point => box.expandByPoint(point));

        // 计算边界框的中心和大小
        const center = new THREE.Vector3();
        box.getCenter(center);
        const size = new THREE.Vector3();
        box.getSize(size);

        // 计算合适的相机距离
        const maxDim = Math.max(size.x, size.y, size.z);
        const distance = maxDim * 2;

        // 设置相机位置
        this.camera.position.set(
            center.x + distance,
            center.y + distance,
            center.z + distance
        );
        this.camera.lookAt(center);

        // 更新控制器
        this.controls.target.copy(center);
        this.controls.update();
    }

    animate() {
        requestAnimationFrame(() => this.animate());
        this.controls.update();
        this.renderer.render(this.scene, this.camera);
    }

    addEventListeners() {
        // 视图控制
        const zoomIn = document.getElementById('zoomIn');
        const zoomOut = document.getElementById('zoomOut');
        const resetView = document.getElementById('resetView');
        const clearTrajectory = document.getElementById('clearTrajectory');

        if (zoomIn) {
            zoomIn.addEventListener('click', () => {
                this.camera.position.multiplyScalar(0.8);
            });
        }

        if (zoomOut) {
            zoomOut.addEventListener('click', () => {
                this.camera.position.multiplyScalar(1.2);
            });
        }

        if (resetView) {
            resetView.addEventListener('click', () => {
                this.resetView();
            });
        }

        if (clearTrajectory) {
            clearTrajectory.addEventListener('click', () => {
                this.clearTrajectory();
            });
        }

        // 窗口大小变化时更新渲染器尺寸
        const resizeObserver = new ResizeObserver(() => this.updateRendererSize());
        resizeObserver.observe(this.canvas.parentElement);
    }
}

// 导出轨迹查看器类
window.TrajectoryViewer = TrajectoryViewer;