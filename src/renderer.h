#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Renderer {
public:
    Renderer();
    ~Renderer();

    void updateControlPoints(const std::vector<glm::vec3>& points);
    void updateControlPolygon(const std::vector<glm::vec3>& points);
    void updateCurve(const std::vector<glm::vec3>& points);
    void render();

    // 设置正交投影（2D 模式）
    void setOrtho(float left, float right, float bottom, float top);

private:
    // VAO/VBO
    unsigned int pointVAO = 0, pointVBO = 0;
    unsigned int polyVAO = 0, polyVBO = 0;
    unsigned int curveVAO = 0, curveVBO = 0;

    // CPU 数据缓存（用于脏检查）
    std::vector<glm::vec3> controlPoints, controlPolygon, curve;

    // 着色器
    class Shader* pointShader = nullptr;
    class Shader* lineShader = nullptr;
    class Shader* curveShader = nullptr;

    // 相机矩阵（2D 使用正交）
    glm::mat4 viewMat;
    glm::mat4 projMat;

    bool initialized = false;
};