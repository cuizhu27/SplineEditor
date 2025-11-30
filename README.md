# Spline Editor

一个基于 OpenGL 的样条曲线编辑器，支持 Bézier、B 样条和 NURBS 曲线的实时编辑和可视化。

## 功能特性

- 支持三种样条曲线类型：
  - Bézier 曲线（de Casteljau 算法）
  - B 样条曲线
  - NURBS 曲线
- 实时交互式控制点编辑
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
mingw32-make

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

1. 运行程序后，可以在窗口中点击添加控制点
2. 拖拽控制点可以调整其位置
3. 使用左侧控制面板选择不同的曲线类型以及可以拖动参数值来改变权重
4. 按 Delete 键清除所有控制点

## 注意事项

- 项目使用 C++17 标准
- Windows 平台使用 MinGW 编译器
- 构建时会自动复制 [glfw3.dll](file://d:\code\project\SplineEditor\build\glfw3.dll) 到输出目录
- 需要支持 OpenGL 3.3+ 的显卡驱动