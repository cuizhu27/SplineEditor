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

} // namespace Spline