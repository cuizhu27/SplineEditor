#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>

#include "spline.h"
#include "renderer.h"
#include "camera.h"

Camera camera;
bool enable3DView = false; // 是否为3D视角
bool isShowControlPoints = true;
std::vector<glm::vec3> controlPoints;
std::vector<float> weights; // 每个控制点的权重
int curveType = 0; // 0: Bezier, 1: B-spline, 2: NURBS
std::vector<std::vector<glm::vec3>> surfaceControlPoints;
std::vector<std::vector<float>> surfaceWeights;
int surfaceType = 0;

bool dragging = false;
int draggedIndex = -1;

int windowWidth = 1024;
int windowHeight = 768;

// 3D拖拽使用变量
glm::vec3 dragPlaneNormal;     // 拖拽平面法向（=相机视线方向）
float dragPlaneDistance;       // 平面到原点的距离 (d in ax+by+cz=d)
int hovered3DIndex = -1;        // 鼠标悬停的点（用于高亮 + XOY 拖拽）
int selected3DIndex = -1;       // 已点击选中的点（用于 Z 轴编辑）
bool isZEditMode = false;       // 是否处于 Z 轴编辑模式
bool isDraggingPoint = false;   // 当前是否正在拖拽控制点（XOY 或 Z）

// 3D相机控制变量
double cameraLastX = 0.0;
double cameraLastY = 0.0;
bool cameraFirstMouse = true;

// 错误回调
void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// 窗口大小回调
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
}


// 将 NDC 坐标转换为世界坐标（仅用于 2D 模式下的拾取）
glm::vec3 screenToNDC(double x, double y, int width, int height) {
    float ndcX = (2.0f * static_cast<float>(x) / width) - 1.0f;
    float ndcY = 1.0f - (2.0f * static_cast<float>(y) / height);
    return glm::vec3(ndcX, ndcY, 0.0f);
}

// 2D 控制点拾取与编辑
void handle2DMouseInteraction(GLFWwindow* window, 
                              std::vector<glm::vec3>& controlPoints,
                              std::vector<float>& weights,
                              bool& dragging, int& draggedIndex,
                              int width, int height) {
    static bool wasPressed = false;
    bool isPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (isPressed && !wasPressed) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        glm::vec3 worldPt = screenToNDC(x, y, width, height);

        bool found = false;
        for (size_t i = 0; i < controlPoints.size(); ++i) {
            glm::vec2 p(controlPoints[i].x, controlPoints[i].y);
            glm::vec2 click(worldPt.x, worldPt.y);
            if (glm::distance(p, click) < 0.05f) {
                dragging = true;
                draggedIndex = static_cast<int>(i);
                found = true;
                break;
            }
        }
        if (!found) {
            controlPoints.push_back(worldPt);
            weights.push_back(1.0f);
        }
    } else if (!isPressed && wasPressed) {
        dragging = false;
        draggedIndex = -1;
    }
    wasPressed = isPressed;

    if (dragging && draggedIndex != -1) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        glm::vec3 worldPt = screenToNDC(x, y, width, height);
        controlPoints[draggedIndex] = worldPt;
    }
}

// 将屏幕坐标 (x, y) 转换为世界空间射线（起点 + 方向）
std::pair<glm::vec3, glm::vec3> screenToWorldRay(double x, double y, int width, int height,
                                                 const glm::mat4& view, const glm::mat4& proj) {
    // NDC 坐标 [-1,1]
    float ndcX = (2.0f * static_cast<float>(x) / width) - 1.0f;
    float ndcY = 1.0f - (2.0f * static_cast<float>(y) / height);

    glm::vec4 rayStartNDC(ndcX, ndcY, -1.0f, 1.0f); // near plane
    glm::vec4 rayEndNDC(ndcX, ndcY,  1.0f, 1.0f); // far plane

    glm::mat4 invVP = glm::inverse(proj * view);
    glm::vec4 rayStartWorld = invVP * rayStartNDC;
    glm::vec4 rayEndWorld   = invVP * rayEndNDC;

    rayStartWorld /= rayStartWorld.w;
    rayEndWorld   /= rayEndWorld.w;

    glm::vec3 rayOrigin = glm::vec3(rayStartWorld);
    glm::vec3 rayDir    = glm::normalize(glm::vec3(rayEndWorld - rayStartWorld));

    return {rayOrigin, rayDir};
}

// 判断射线是否击中以 center 为中心、radius 为半径的球
bool rayIntersectsSphere(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                         const glm::vec3& center, float radius, float& t) {
    glm::vec3 oc = rayOrigin - center;
    float a = glm::dot(rayDir, rayDir);
    float b = 2.0f * glm::dot(oc, rayDir);
    float c = glm::dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) return false;

    t = (-b - sqrt(discriminant)) / (2 * a); // 取近交点
    return t >= 0;
}

// 射线与 Z = fixedZ 的平面相交（XOY 平面）
bool rayIntersectXOYPlane(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                          float fixedZ, glm::vec3& outHit) {
    if (std::abs(rayDir.z) < 1e-6f) return false; // 射线平行于 XOY 平面
    float t = (fixedZ - rayOrigin.z) / rayDir.z;
    if (t < 0) return false;
    outHit = rayOrigin + t * rayDir;
    return true;
}

// 创建一个穿过控制点且平行于YZ平面的平面
bool rayIntersectYZPlane(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                         float fixedX, glm::vec3& outHit) {
    if (std::abs(rayDir.x) < 1e-6f) return false;
    float t = (fixedX - rayOrigin.x) / rayDir.x;
    if (t < 0) return false;
    outHit = rayOrigin + t * rayDir;
    return true;
}

// 3D求交

// 3D交互处理函数专门用于曲面控制点
void handle3DSurfaceInteraction(GLFWwindow* window,
                               Camera& camera,
                               const ImGuiIO& io,
                               std::vector<std::vector<glm::vec3>>& surfaceControlPoints,
                               int& hovered3DIndex,
                               int& selected3DIndex,
                               bool& isZEditMode,
                               bool& isDraggingPoint,
                               int windowWidth,
                               int windowHeight) {
    
    static bool wasPressed = false;
    bool isPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    static bool wasRightPressed = false;
    bool isRightPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                      static_cast<float>(windowWidth) / windowHeight,
                                      0.1f, 100.0f);
    glm::mat4 view = camera.getViewMatrix();
    auto [rayOrigin, rayDir] = screenToWorldRay(mouseX, mouseY, windowWidth, windowHeight, view, proj);

    // 将二维网格转换为一维索引来处理
    std::vector<glm::vec3> flatControlPoints;
    std::vector<std::pair<int, int>> indexMap; // 映射一维索引到二维坐标
    
    for (int i = 0; i < static_cast<int>(surfaceControlPoints.size()); ++i) {
        for (int j = 0; j < static_cast<int>(surfaceControlPoints[i].size()); ++j) {
            flatControlPoints.push_back(surfaceControlPoints[i][j]);
            indexMap.emplace_back(i, j);
        }
    }

    // === 1. 更新悬停状态（每帧）===
    hovered3DIndex = -1;
    constexpr float hoverRadius = 0.12f;
    for (size_t i = 0; i < flatControlPoints.size(); ++i) {
        float t;
        if (rayIntersectsSphere(rayOrigin, rayDir, flatControlPoints[i], hoverRadius, t)) {
            hovered3DIndex = static_cast<int>(i);
            break;
        }
    }

    // === 2. 鼠标按下事件（左键）===
    if (isPressed && !wasPressed && isShowControlPoints) {
        // 左键按下
        if (hovered3DIndex != -1) {
            selected3DIndex = hovered3DIndex;
            isDraggingPoint = true;
        } else {
            // 点击空白：退出选中
            selected3DIndex = -1;
            isDraggingPoint = false;
        }
    } 

    // === 3. 鼠标释放 ===
    if (!isPressed && wasPressed && isShowControlPoints) {
        isDraggingPoint = false;
        selected3DIndex = -1;
    }
    if (!isRightPressed && wasRightPressed && isShowControlPoints) {
        // 右击：切换 Z 编辑模式
        isZEditMode = !isZEditMode;
    }

    // === 4. 拖拽更新 ===
    if (isPressed && isDraggingPoint && selected3DIndex != -1 && selected3DIndex < static_cast<int>(indexMap.size()) && isShowControlPoints) {
        // 将一维索引转换回二维坐标
        auto [row, col] = indexMap[selected3DIndex];
        
        if (isZEditMode) {
            // Z 轴模式
            glm::vec3 hit;
            if (rayIntersectYZPlane(rayOrigin, rayDir, surfaceControlPoints[row][col].x, hit)) {
                surfaceControlPoints[row][col].z = hit.z;
            }
        } else {
            // XOY 平面模式
            glm::vec3 hit;
            if (rayIntersectXOYPlane(rayOrigin, rayDir, surfaceControlPoints[row][col].z, hit)) {
                surfaceControlPoints[row][col].x = hit.x;
                surfaceControlPoints[row][col].y = hit.y;
            }
        }
    }

    // === 5. 相机交互：仅当未拖拽点时 ===
    if (isPressed && !isDraggingPoint) {
        if (cameraFirstMouse) {
            cameraLastX = mouseX;
            cameraLastY = mouseY;
            cameraFirstMouse = false;
        }

        double dx = mouseX - cameraLastX;
        double dy = mouseY - cameraLastY;

        if (io.KeyShift) {
            camera.pan(static_cast<float>(dx), static_cast<float>(dy));
        } else {
            camera.rotate(static_cast<float>(dx), static_cast<float>(-dy));
        }

        cameraLastX = mouseX;
        cameraLastY = mouseY;
    } else {
        cameraFirstMouse = true;
    }

    // === 6. 滚轮缩放（始终可用，只要不在 ImGui）===
    if (io.MouseWheel != 0.0f) {
        camera.zoom(io.MouseWheel);
    }

    wasPressed = isPressed;
    wasRightPressed = isRightPressed;
}

// 主函数
int main() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // 设置 OpenGL 版本（Core Profile）
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Spline Editor", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 设置窗口大小回调
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 创建渲染器
    Renderer renderer;
    renderer.setOrtho(-1.0f, 1.0f, -1.0f, 1.0f); // NDC 空间

    // 启用深度测试
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    glm::mat4 perspectiveProj = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(windowWidth) / windowHeight,
        0.1f,
        100.0f
    );

    // 预生成初始4x4控制点网格按钮
    std::vector<std::vector<glm::vec3>> initial_surfaceControlPoints;
    std::vector<std::vector<float>> initial_surfaceWeights;
    for (int i = 0; i < 4; ++i) {
        std::vector<glm::vec3> row;
        std::vector<float> weightRow;
        for (int j = 0; j < 4; ++j) {
            // 生成网格状分布的控制点
            glm::vec3 point(
                (static_cast<float>(i) - 1.5f) * 0.5f,  // x坐标
                (static_cast<float>(j) - 1.5f) * 0.5f,  // y坐标
                (sin(static_cast<float>(i)) + cos(static_cast<float>(j))) * 0.3f  // z坐标，形成波浪形状
            );
            row.push_back(point);
            weightRow.push_back(1.0f);
        }
        initial_surfaceControlPoints.push_back(row);
        initial_surfaceWeights.push_back(weightRow);
    }
    surfaceControlPoints = initial_surfaceControlPoints;
    surfaceWeights = initial_surfaceWeights;

    // 主循环
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // 设置view、projection矩阵
        if(enable3DView) {
            renderer.setViewMatrix(camera.getViewMatrix());
            renderer.setProjectionMatrix(
                glm::perspective(
                    glm::radians(45.0f),
                    static_cast<float>(windowWidth) / windowHeight,
                    0.1f,
                    100.0f
                )
            );

        } else {
            renderer.setViewMatrix(glm::mat4(1.0f));
            renderer.setProjectionMatrix(glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f));
        }

        // ImGui 新帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();

        // === 处理画布鼠标事件 ===
        if (!io.WantCaptureMouse) {
            if (enable3DView) {
                handle3DSurfaceInteraction(window, camera, io, surfaceControlPoints,
                                    hovered3DIndex, selected3DIndex, isZEditMode, isDraggingPoint,
                                    windowWidth, windowHeight);
            } else {
                handle2DMouseInteraction(window, controlPoints, weights, dragging, draggedIndex, windowWidth, windowHeight);
            }
        }

        // Delete 键（同样检查 WantCaptureKeyboard）
        if (!io.WantCaptureKeyboard && glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS) {
            controlPoints.clear();
            weights.clear();
        }

        // UI 控制面板
        {
            ImGui::Begin("Spline Control");
            ImGui::Checkbox("Enable 3D View", &enable3DView);
            if (enable3DView) {

                const char* surfaceTypes[] = {"Bezier Surface", "B-spline Surface", "NURBS Surface"};
                ImGui::Combo("Surface Type", &surfaceType, surfaceTypes, 3);

                ImGui::Text("Surface Control Points: %dx%d", 
                        surfaceControlPoints.size(), 
                        surfaceControlPoints.empty() ? 0 : surfaceControlPoints[0].size());
                
                ImGui::Text("Drag Mode: %s", isZEditMode ? "Z-axis" : "XY-plane");

                // 与isShowControlPoints绑定
                ImGui::Checkbox("Show Control Points", &isShowControlPoints);
                
                if (ImGui::Button("Reset Surface")) {
                    surfaceControlPoints.clear();
                    surfaceWeights.clear();
                    surfaceControlPoints = initial_surfaceControlPoints;
                    surfaceWeights = initial_surfaceWeights;
                }
                
                // 显示权重调整（仅NURBS）
                if (surfaceType == 2 && !surfaceControlPoints.empty()) {
                    ImGui::Separator();
                    ImGui::Text("Weights:");
                    for (size_t i = 0; i < surfaceControlPoints.size(); ++i) {
                        for (size_t j = 0; j < surfaceControlPoints[i].size(); ++j) {
                            std::string label = "w[" + std::to_string(i) + "][" + std::to_string(j) + "]";
                            ImGui::DragFloat(label.c_str(), &surfaceWeights[i][j], 0.05f, 0.01f, 10.0f);
                        }
                    }
                }
            } else {
                
                const char* types[] = {"Bezier", "B-spline", "NURBS"};
                ImGui::Combo("Curve Type", &curveType, types, 3);
                ImGui::Text("Control Points: %d", (int)controlPoints.size());
                if (ImGui::Button("Clear All")) {
                    controlPoints.clear();
                    weights.clear();
                }
                if (curveType == 2 && !controlPoints.empty()) { // 仅在 NURBS 模式下显示
                    ImGui::Separator();
                    ImGui::Text("Weights:");
                    for (size_t i = 0; i < controlPoints.size(); ++i) {
                        std::string label = "w[" + std::to_string(i) + "]";
                        // 使用 DragFloat 允许用户拖动调整（范围 0.1 ~ 10.0，可自定义）
                        ImGui::DragFloat(label.c_str(), &weights[i], 0.05f, 0.01f, 10.0f);
                    }
                }
            }
            ImGui::End();
        }

        std::vector<glm::vec3> surfaceVertices;
        std::vector<unsigned int> surfaceIndices;

        if (enable3DView && !surfaceControlPoints.empty()) {
            // 计算曲面
            int uSamples = 30;
            int vSamples = 30;
            
            if (!surfaceControlPoints.empty() && !surfaceControlPoints[0].empty()) {
                if (surfaceType == 0) {
                    // Bezier Surface
                    surfaceVertices = Spline::evaluateBezierSurface(surfaceControlPoints, uSamples, vSamples);
                } else if (surfaceType == 1) {
                    // B-spline Surface
                    surfaceVertices = Spline::evaluateBSplineSurface(surfaceControlPoints, 3, 3, uSamples, vSamples);
                } else if (surfaceType == 2) {
                    // NURBS Surface
                    surfaceVertices = Spline::evaluateNURBSSurface(surfaceControlPoints, surfaceWeights, 3, 3, uSamples, vSamples);
                }
                
                // 生成索引
                surfaceIndices = Spline::generateSurfaceIndices(uSamples, vSamples);
            }
            
            // 将曲面控制点展平为一维数组用于渲染
            std::vector<glm::vec3> flatControlPoints;
            for (const auto& row : surfaceControlPoints) {
                for (const auto& point : row) {
                    flatControlPoints.push_back(point);
                }
            }

            // 生成控制网格线框数据
            std::vector<glm::vec3> controlWireframeLines;
            int rows = static_cast<int>(surfaceControlPoints.size());
            int cols = static_cast<int>(surfaceControlPoints[0].size());

            // 生成横向线段
            for (int i = 0; i < rows; ++i) {
                for (int j = 0; j < cols - 1; ++j) {
                    controlWireframeLines.push_back(surfaceControlPoints[i][j]);
                    controlWireframeLines.push_back(surfaceControlPoints[i][j + 1]);
                }
            }
            
            // 生成纵向线段
            for (int j = 0; j < cols; ++j) {
                for (int i = 0; i < rows - 1; ++i) {
                    controlWireframeLines.push_back(surfaceControlPoints[i][j]);
                    controlWireframeLines.push_back(surfaceControlPoints[i + 1][j]);
                }
            }
            
            renderer.updateControlPoints(flatControlPoints);
            renderer.updateSurface(surfaceVertices, surfaceIndices);
            renderer.updateWireframe(controlWireframeLines);
        } else {
            // 原有的曲线计算逻辑
            std::vector<glm::vec3> curve;
            if (!controlPoints.empty()) {
                if (curveType == 0) {
                    curve = Spline::evaluateBezier(controlPoints, 100);
                } else if (curveType == 1) {
                    curve = Spline::evaluateBSpline(controlPoints, 3, 100);
                } else if (curveType == 2) {
                    // 为每个控制点分配权重（默认 1.0）
                    // 确保 weights 长度匹配（安全起见）
                    if (weights.size() != controlPoints.size()) {
                        weights.assign(controlPoints.size(), 1.0f);
                    }
                    curve = Spline::evaluateNURBS(controlPoints, weights, 3, 100);
                }
            }

            // 更新渲染器数据
            renderer.updateControlPoints(controlPoints);
            renderer.updateControlPolygon(controlPoints);
            renderer.updateCurve(curve);
        }

        // 渲染
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        if (enable3DView) {
            if(isShowControlPoints) {
                renderer.renderControlPoints();
                renderer.renderAxes(); 
                renderer.renderWireframe();
            }
            renderer.renderSurface();
            
        } else {
            renderer.render();
        }

        // 渲染 ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // 清理
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}