# Spline Editor

一个基于 OpenGL 的样条曲线和曲面编辑器，支持 Bézier、B样条和NURBS曲线及曲面的实时编辑和可视化。

## 功能特性

- 支持三种样条类型：
  - Bézier 曲线和曲面（de Casteljau 算法）
  - B样条曲线和曲面
  - NURBS 曲线和曲面
- 实时交互式控制点编辑
- 2D曲线编辑模式
- 3D曲面编辑模式
  - 支持XY平面拖拽和Z轴拖拽两种编辑模式
  - 可视化控制网格线框
- ImGui 图形用户界面
- 跨平台支持（主要针对 Windows + MinGW）

## 依赖项

项目已包含以下第三方库：

- [GLFW 3.4](https://www.glfw.org/) - 窗口和输入管理
- [GLAD](https://github.com/Dav1dde/glad) - OpenGL 函数加载器
- [GLM](https://github.com/g-truc/glm) - 数学库
- [ImGui](https://github.com/ocornut/imgui) - 即时 GUI 库

## 构建说明

### Windows (MinGW)

```bash
# 克隆仓库
git clone <repository-url>
cd SplineEditor

# 创建构建目录
mkdir build
cd build

# 配置项目
cmake .. -G "MinGW Makefiles"

# 编译项目
make

# 运行程序
./app.exe
```

## 项目结构

```
├── src/                 # 源代码目录
├── libs/                # 第三方库目录
│   ├── glfw-3.4.bin.WIN64/  # 预编译的 GLFW 库
│   ├── glm/             # GLM 数学库
│   ├── glad/            # GLAD OpenGL 加载器
│   └── imgui/           # ImGui GUI 库
├── CMakeLists.txt       # CMake 构建配置
└── README.md           # 项目说明文档
```

## 使用方法

### 2D曲线模式
1. 运行程序后，默认处于2D曲线编辑模式
2. 在绘图区域内点击鼠标左键添加控制点
3. 拖拽已存在的控制点可以调整其位置
4. 使用左侧控制面板：
   - 选择不同的曲线类型（Bezier、B样条、NURBS）
   - 对于NURBS曲线，可以调整各控制点的权重参数
   - 点击"Clear All"按钮清除所有控制点
5. 按 Delete 键快速清除所有控制点

### 3D曲面模式
1. 在左侧控制面板中勾选"Enable 3D View"进入3D模式
2. 程序会自动创建一个4×4的初始控制点网格
3. 控制点交互：
   - 点击控制点选中并拖拽可移动控制点位置
   - 鼠标右键点击任意处切换拖拽模式：
     * XY平面拖拽模式（默认）：控制点在XY平面内移动
     * Z轴拖拽模式：控制点沿Z轴方向移动
4. 视图控制：
   - 按住鼠标左键并拖拽旋转视图
   - 使用鼠标滚轮缩放视图
5. 曲面控制：
   - 在控制面板中选择不同的曲面类型（Bezier曲面、B样条曲面、NURBS曲面）
   - 勾选"Show Control Points"显示/隐藏控制点和网格线框
   - 对于NURBS曲面，可以调整各控制点的权重参数
   - 点击"Reset Surface"按钮恢复到初始的4×4网格

## 注意事项

- 项目使用 C++17 标准
- Windows 平台使用 MinGW 编译器
- 构建时会自动复制 [glfw3.dll](file://d:\code\project\SplineEditor\build\glfw3.dll) 到输出目录
- 需要支持 OpenGL 3.3+ 的显卡驱动