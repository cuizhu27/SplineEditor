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

// 全局状态
std::vector<glm::vec3> controlPoints;
std::vector<float> weights; // 每个控制点的权重
int curveType = 0; // 0: Bezier, 1: B-spline, 2: NURBS
bool dragging = false;
int draggedIndex = -1;

int windowWidth = 1024;
int windowHeight = 768;


// GLFW 回调
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // 检查ImGui是否想要捕获鼠标
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        float ndcX = (2.0f * static_cast<float>(x) / windowWidth) - 1.0f;
        float ndcY = 1.0f - (2.0f * static_cast<float>(y) / windowHeight);
        glm::vec3 worldPt(ndcX, ndcY, 0.0f);

        // 拾取最近控制点（在 xy 平面）
        for (size_t i = 0; i < controlPoints.size(); ++i) {
            glm::vec2 p(controlPoints[i].x, controlPoints[i].y);
            glm::vec2 click(ndcX, ndcY);
            if (glm::distance(p, click) < 0.05f) {
                dragging = true;
                draggedIndex = static_cast<int>(i);
                return;
            }
        }
        // 未拾取到 → 添加新点
        controlPoints.push_back(worldPt);
    } else if (action == GLFW_RELEASE) {
        dragging = false;
        draggedIndex = -1;
    }
}

void cursorPosCallback(GLFWwindow* window, double x, double y) {
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    if (dragging && draggedIndex != -1 && draggedIndex < static_cast<int>(controlPoints.size())) {
        float ndcX = (2.0f * static_cast<float>(x) / windowWidth) - 1.0f;
        float ndcY = 1.0f - (2.0f * static_cast<float>(y) / windowHeight);
        controlPoints[draggedIndex] = glm::vec3(ndcX, ndcY, 0.0f);
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_DELETE && action == GLFW_PRESS && !controlPoints.empty()) {
        controlPoints.clear();
    }
}

// 错误回调
void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
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

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Spline Editor 2D", nullptr, nullptr);
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

    // 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 注册回调
    // glfwSetMouseButtonCallback(window, mouseButtonCallback);
    // glfwSetCursorPosCallback(window, cursorPosCallback);
    // glfwSetKeyCallback(window, keyCallback);

    // 创建渲染器
    Renderer renderer;
    renderer.setOrtho(-1.0f, 1.0f, -1.0f, 1.0f); // NDC 空间

    // 启用深度测试（虽 2D 不需要，但为 3D 准备）
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // 主循环
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // ImGui 新帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();

        // === 处理画布鼠标事件（仅当 ImGui 不需要时）===
        if (!io.WantCaptureMouse) {
            // 手动查询鼠标状态（因为没用回调）
            static bool wasPressed = false;
            bool isPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

            if (isPressed && !wasPressed) {
                // 鼠标按下（上升沿）
                double x, y;
                glfwGetCursorPos(window, &x, &y);
                float ndcX = (2.0f * static_cast<float>(x) / windowWidth) - 1.0f;
                float ndcY = 1.0f - (2.0f * static_cast<float>(y) / windowHeight);
                glm::vec3 worldPt(ndcX, ndcY, 0.0f);

                // 拾取或添加点（同前）
                bool found = false;
                for (size_t i = 0; i < controlPoints.size(); ++i) {
                    glm::vec2 p(controlPoints[i].x, controlPoints[i].y);
                    glm::vec2 click(ndcX, ndcY);
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
                // 鼠标释放
                dragging = false;
                draggedIndex = -1;
            }
            wasPressed = isPressed;

            // 拖拽更新
            if (dragging && draggedIndex != -1) {
                double x, y;
                glfwGetCursorPos(window, &x, &y);
                float ndcX = (2.0f * static_cast<float>(x) / windowWidth) - 1.0f;
                float ndcY = 1.0f - (2.0f * static_cast<float>(y) / windowHeight);
                controlPoints[draggedIndex] = glm::vec3(ndcX, ndcY, 0.0f);
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
            ImGui::End();
        }

        // 计算曲线
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

        // 渲染
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.render();

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