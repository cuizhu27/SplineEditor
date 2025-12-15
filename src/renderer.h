#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Renderer {
public:
    Renderer();
    ~Renderer();

    void renderAxes(); // 渲染 XYZ 坐标轴

    void updateControlPoints(const std::vector<glm::vec3>& points);
    void updateControlPolygon(const std::vector<glm::vec3>& points);
    void updateCurve(const std::vector<glm::vec3>& points);
    void updateSurface(const std::vector<glm::vec3>& positions, const std::vector<unsigned int>& indices); 
    void setSurfaceRenderMode(bool wireframe);
    void renderControlPoints();
    void renderSurface(); // 新增渲染函数
    void updateWireframe(const std::vector<glm::vec3>& lines);
    void renderWireframe();
    void render();

    // 设置正交投影（2D 模式）
    void setOrtho(float left, float right, float bottom, float top);
    void setViewMatrix(const glm::mat4& view);
    void setProjectionMatrix(const glm::mat4& proj);

private:
    // VAO/VBO
    unsigned int pointVAO = 0, pointVBO = 0;
    unsigned int polyVAO = 0, polyVBO = 0;
    unsigned int curveVAO = 0, curveVBO = 0;
    unsigned int axesVAO, axesVBO;
    unsigned int gridVAO = 0, gridVBO = 0;
    unsigned int surfaceVAO = 0, surfaceVBO = 0, surfaceEBO = 0;
    unsigned int wireframeVAO = 0, wireframeVBO = 0;

    // CPU 数据缓存（用于脏检查）
    std::vector<glm::vec3> controlPoints, controlPolygon, curve;
    std::vector<glm::vec3> surfacePositions;   // 顶点位置（不重复，M×N 个）
    std::vector<unsigned int> surfaceIndices;  // 索引列表（三角形索引）
    std::vector<glm::vec3> wireframeLines;

    // 着色器
    class Shader* pointShader = nullptr;
    class Shader* lineShader = nullptr;
    class Shader* curveShader = nullptr;

    // 相机矩阵（2D 使用正交）
    glm::mat4 viewMat;
    glm::mat4 projMat;

    bool initialized = false;
    // 渲染模式：true = 线框，false = 实体
    bool renderSurfaceAsWireframe = false;
};