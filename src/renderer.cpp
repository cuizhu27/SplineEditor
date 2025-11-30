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
}

void Renderer::setOrtho(float left, float right, float bottom, float top) {
    projMat = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
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

// --- Render ---
void Renderer::render() {
    if (!initialized) return;

    // 控制点
    if (!controlPoints.empty() && pointShader) {
        pointShader->use();
        pointShader->setFloat("pointSize", 5.0f);
        pointShader->setVec3("uColor", 1.0f, 0.5f, 0.0f);
        glUniformMatrix4fv(glGetUniformLocation(pointShader->ID, "uView"), 1, GL_FALSE, &viewMat[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(pointShader->ID, "uProjection"), 1, GL_FALSE, &projMat[0][0]);
        glBindVertexArray(pointVAO);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(controlPoints.size()));
        glBindVertexArray(0);
    }

    // 控制多边形
    if (controlPolygon.size() > 1 && lineShader) {
        lineShader->use();
        lineShader->setVec3("uColor", 0.5f, 0.5f, 0.5f);
        glUniformMatrix4fv(glGetUniformLocation(lineShader->ID, "uView"), 1, GL_FALSE, &viewMat[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(lineShader->ID, "uProjection"), 1, GL_FALSE, &projMat[0][0]);
        glBindVertexArray(polyVAO);
        glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(controlPolygon.size()));
        glBindVertexArray(0);
    }

    // 样条曲线
    if (curve.size() > 1 && curveShader) {
        curveShader->use();
        curveShader->setVec3("uColor", 0.0f, 1.0f, 0.0f);
        glUniformMatrix4fv(glGetUniformLocation(curveShader->ID, "uView"), 1, GL_FALSE, &viewMat[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(curveShader->ID, "uProjection"), 1, GL_FALSE, &projMat[0][0]);
        glBindVertexArray(curveVAO);
        glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(curve.size()));
        glBindVertexArray(0);
    }
}