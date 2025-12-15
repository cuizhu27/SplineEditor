#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace Spline {

// Bezier 曲线：de Casteljau 算法
std::vector<glm::vec3> evaluateBezier(const std::vector<glm::vec3>& controlPoints, int numSamples = 100);

// B 样条曲线
// degree: 阶数（通常 3 表示三次样条）
std::vector<glm::vec3> evaluateBSpline(const std::vector<glm::vec3>& controlPoints, int degree = 3, int numSamples = 100);

// NURBS 曲线
// weights: 每个控制点对应一个权重（必须与 controlPoints 同长度）
std::vector<glm::vec3> evaluateNURBS(const std::vector<glm::vec3>& controlPoints,
                                     const std::vector<float>& weights,
                                     int degree = 3,
                                     int numSamples = 100);

std::vector<glm::vec3> evaluateBezierSurface(const std::vector<std::vector<glm::vec3>>& controlPoints, 
                                            int uSamples, int vSamples);

std::vector<glm::vec3> evaluateBSplineSurface(const std::vector<std::vector<glm::vec3>>& controlPoints,
                                             int degreeU, int degreeV,
                                             int uSamples, int vSamples);

std::vector<glm::vec3> evaluateNURBSSurface(const std::vector<std::vector<glm::vec3>>& controlPoints,
                                           const std::vector<std::vector<float>>& weights,
                                           int degreeU, int degreeV,
                                           int uSamples, int vSamples);

// 辅助函数
std::vector<unsigned int> generateSurfaceIndices(int uSamples, int vSamples);

// 内部辅助函数声明
float bernsteinPolynomial(int n, int i, float t);
int binomialCoefficient(int n, int k);

} // namespace Spline