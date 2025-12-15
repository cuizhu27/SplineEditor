#include "renderer.h"
#include "shader_s.h"
#include <glad/glad.h>
#include <iostream>

Renderer::Renderer() {
    // --- 初始化 VAO/VBO（3D 顶点）---
    auto setupVAO = [](unsigned int& vao, unsigned int& vbo) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0); // ← 3 components
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    };

    setupVAO(pointVAO, pointVBO);
    setupVAO(polyVAO, polyVBO);
    setupVAO(curveVAO, curveVBO);

    // 加载着色器
    try {
        pointShader = new Shader("../src/shaders/shader.vs", "../src/shaders/shader.fs");
        lineShader  = new Shader("../src/shaders/shader.vs", "../src/shaders/shader.fs");
        curveShader = new Shader("../src/shaders/shader.vs", "../src/shaders/shader.fs");
    } catch (...) {
        std::cerr << "Failed to load shaders!" << std::endl;
    }

    // 默认 2D 正交视图（NDC 空间 [-1,1]）
    viewMat = glm::mat4(1.0f);
    projMat = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

    initialized = true;

    // --- 创建坐标轴 VAO/VBO ---
    glGenVertexArrays(1, &axesVAO);
    glGenBuffers(1, &axesVBO);

    glBindVertexArray(axesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axesVBO);

    // 6 个顶点：3 条线段（每条 2 点）
    GLfloat axesVertices[] = {
        // X axis
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        // Y axis
        0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        // Z axis
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(axesVertices), axesVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);

    // 定义一个 10x10 的正方形（Z=0），中心在原点
    GLfloat planeVertices[] = {
        -5.0f, -5.0f, 0.0f,
         5.0f, -5.0f, 0.0f,
         5.0f,  5.0f, 0.0f,
        -5.0f,  5.0f, 0.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);


    // --- 初始化曲面 VAO/VBO/EBO ---
    glGenVertexArrays(1, &surfaceVAO);
    glGenBuffers(1, &surfaceVBO);
    glGenBuffers(1, &surfaceEBO); // ← 新增

    glBindVertexArray(surfaceVAO);

    // 顶点缓冲
    glBindBuffer(GL_ARRAY_BUFFER, surfaceVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // 索引缓冲
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surfaceEBO); // ← 绑定到 GL_ELEMENT_ARRAY_BUFFER
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);


    // --- 创建线框 VAO/VBO ---
    glGenVertexArrays(1, &wireframeVAO);
    glGenBuffers(1, &wireframeVBO);
    glBindVertexArray(wireframeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, wireframeVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

Renderer::~Renderer() {
    if (pointShader) delete pointShader;
    if (lineShader)  delete lineShader;
    if (curveShader) delete curveShader;

    glDeleteVertexArrays(1, &pointVAO);
    glDeleteVertexArrays(1, &polyVAO);
    glDeleteVertexArrays(1, &curveVAO);

    glDeleteBuffers(1, &pointVBO);
    glDeleteBuffers(1, &polyVBO);
    glDeleteBuffers(1, &curveVBO);
    glDeleteVertexArrays(1, &axesVAO);
    glDeleteBuffers(1, &axesVBO);
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteBuffers(1, &surfaceVAO);
    glDeleteBuffers(1, &surfaceVBO);
    glDeleteBuffers(1, &surfaceEBO);
    glDeleteBuffers(1, &wireframeVAO);
    glDeleteBuffers(1, &wireframeVBO);
}

void Renderer::setOrtho(float left, float right, float bottom, float top) {
    projMat = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
}

void Renderer::setViewMatrix(const glm::mat4& view) {
    viewMat = view;
}

void Renderer::setProjectionMatrix(const glm::mat4& proj) {
    projMat = proj;
}


// --- Update functions (vec3) ---
void Renderer::updateControlPoints(const std::vector<glm::vec3>& points) {
    if (points == controlPoints) return;
    controlPoints = points;
    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_DYNAMIC_DRAW);
}

void Renderer::updateControlPolygon(const std::vector<glm::vec3>& points) {
    if (points == controlPolygon) return;
    controlPolygon = points;
    glBindBuffer(GL_ARRAY_BUFFER, polyVBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_DYNAMIC_DRAW);
}

void Renderer::updateCurve(const std::vector<glm::vec3>& points) {
    if (points == curve) return;
    curve = points;
    glBindBuffer(GL_ARRAY_BUFFER, curveVBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_DYNAMIC_DRAW);
}

void Renderer::updateSurface(
    const std::vector<glm::vec3>& positions,
    const std::vector<unsigned int>& indices
) {
    if (positions == surfacePositions && indices == surfaceIndices) return;

    surfacePositions = positions;
    surfaceIndices = indices;

    // 更新 VBO
    glBindBuffer(GL_ARRAY_BUFFER, surfaceVBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_DYNAMIC_DRAW);

    // 更新 EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surfaceEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);
}

void Renderer::updateWireframe(const std::vector<glm::vec3>& lines) {
    if (lines == wireframeLines) return;
    wireframeLines = lines;
    glBindBuffer(GL_ARRAY_BUFFER, wireframeVBO);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(glm::vec3), lines.data(), GL_DYNAMIC_DRAW);
}

// --- 2DRender ---
void Renderer::render() {
    if (!initialized) return;

    // 控制点
    if (!controlPoints.empty() && pointShader) {
        pointShader->use();
        pointShader->setFloat("pointSize", 5.0f);
        pointShader->setVec4("uColor", 1.0f, 0.5f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(pointShader->ID, "uView"), 1, GL_FALSE, &viewMat[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(pointShader->ID, "uProjection"), 1, GL_FALSE, &projMat[0][0]);
        glBindVertexArray(pointVAO);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(controlPoints.size()));
        glBindVertexArray(0);
    }

    // 控制多边形
    if (controlPolygon.size() > 1 && lineShader) {
        lineShader->use();
        lineShader->setVec4("uColor", 0.5f, 0.5f, 0.5f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(lineShader->ID, "uView"), 1, GL_FALSE, &viewMat[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(lineShader->ID, "uProjection"), 1, GL_FALSE, &projMat[0][0]);
        glBindVertexArray(polyVAO);
        glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(controlPolygon.size()));
        glBindVertexArray(0);
    }

    // 样条曲线
    if (curve.size() > 1 && curveShader) {
        curveShader->use();
        curveShader->setVec4("uColor", 0.0f, 1.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(curveShader->ID, "uView"), 1, GL_FALSE, &viewMat[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(curveShader->ID, "uProjection"), 1, GL_FALSE, &projMat[0][0]);
        glBindVertexArray(curveVAO);
        glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(curve.size()));
        glBindVertexArray(0);
    }
}

// --- 3DRender ---

void Renderer::renderAxes() {
    if (!lineShader) return;

    lineShader->use();
    glUniformMatrix4fv(glGetUniformLocation(lineShader->ID, "uView"), 1, GL_FALSE, &viewMat[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(lineShader->ID, "uProjection"), 1, GL_FALSE, &projMat[0][0]);

    glBindVertexArray(axesVAO);

    // X axis - Red
    lineShader->setVec4("uColor", 1.0f, 0.0f, 0.0f, 1.0f);
    glDrawArrays(GL_LINES, 0, 2);

    // Y axis - Green
    lineShader->setVec4("uColor", 0.0f, 1.0f, 0.0f, 1.0f);
    glDrawArrays(GL_LINES, 2, 2);

    // Z axis - Blue
    lineShader->setVec4("uColor", 0.0f, 0.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINES, 4, 2);

    glBindVertexArray(0);
}


void Renderer::renderControlPoints() {
    // 3D模式：只渲染控制点（曲面控制点）
    if (!controlPoints.empty() && pointShader) {
        pointShader->use();
        pointShader->setFloat("pointSize", 5.0f);
        pointShader->setVec4("uColor", 1.0f, 0.5f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(pointShader->ID, "uView"), 1, GL_FALSE, &viewMat[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(pointShader->ID, "uProjection"), 1, GL_FALSE, &projMat[0][0]);
        glBindVertexArray(pointVAO);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(controlPoints.size()));
        glBindVertexArray(0);
    }
}

void Renderer::renderSurface() {
    if (surfacePositions.empty() || surfaceIndices.empty() || !curveShader) return;
    if (renderSurfaceAsWireframe) return; // 实心模式才绘制

    curveShader->use();
    glUniformMatrix4fv(glGetUniformLocation(curveShader->ID, "uView"), 1, GL_FALSE, &viewMat[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(curveShader->ID, "uProjection"), 1, GL_FALSE, &projMat[0][0]);
    curveShader->setVec4("uColor", 0.0f, 0.8f, 1.0f, 0.6f); // 青蓝色半透明

    glBindVertexArray(surfaceVAO);
    // 注意：EBO 已经在 VAO 中绑定，无需再 bind
    glDrawElements(GL_TRIANGLES, 
                   static_cast<GLsizei>(surfaceIndices.size()), 
                   GL_UNSIGNED_INT, 
                   (void*)0);
    glBindVertexArray(0);
}


void Renderer::renderWireframe() {
    if (wireframeLines.empty() || !lineShader) return;
    lineShader->use();
    lineShader->setVec4("uColor", 0.0f, 0.0f, 0.0f, 0.8f); // 黑色线框
    glUniformMatrix4fv(glGetUniformLocation(lineShader->ID, "uView"), 1, GL_FALSE, &viewMat[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(lineShader->ID, "uProjection"), 1, GL_FALSE, &projMat[0][0]);
    glBindVertexArray(wireframeVAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(wireframeLines.size()));
    glBindVertexArray(0);
}
