#include "spline.h"
#include <cassert>
#include <vector>
#include <cmath>
#include <iostream>

namespace Spline {

// ========================
// 1. Bezier Curve (de Casteljau)
// ========================
std::vector<glm::vec3> evaluateBezier(const std::vector<glm::vec3>& controlPoints, int numSamples) {
    std::vector<glm::vec3> curve;
    if (controlPoints.empty()) return curve;
    if (controlPoints.size() == 1) {
        curve.push_back(controlPoints[0]);
        return curve;
    }

    for (int i = 0; i <= numSamples; ++i) {
        float t = static_cast<float>(i) / numSamples;
        std::vector<glm::vec3> temp = controlPoints;

        // de Casteljau 递归
        for (size_t level = 1; level < controlPoints.size(); ++level) {
            for (size_t j = 0; j < controlPoints.size() - level; ++j) {
                temp[j] = (1.0f - t) * temp[j] + t * temp[j + 1];
            }
        }
        curve.push_back(temp[0]);
    }
    return curve;
}

// ========================
// 2. Cox-de Boor Basis Function (recursive)
// ========================
float coxDeBoor(int i, int k, float u, const std::vector<float>& knots) {
    if (k == 0) {
        if (u >= knots[i] && u < knots[i + 1]) return 1.0f;
        // Handle last point
        if (u == knots.back() && i == static_cast<int>(knots.size()) - 2) return 1.0f;
        return 0.0f;
    }

    float denom1 = knots[i + k] - knots[i];
    float term1 = 0.0f;
    if (denom1 > 1e-6f) {
        term1 = ((u - knots[i]) / denom1) * coxDeBoor(i, k - 1, u, knots);
    }

    float denom2 = knots[i + k + 1] - knots[i + 1];
    float term2 = 0.0f;
    if (denom2 > 1e-6f) {
        term2 = ((knots[i + k + 1] - u) / denom2) * coxDeBoor(i + 1, k - 1, u, knots);
    }

    return term1 + term2;
}

// Helper: generate clamped uniform knot vector
std::vector<float> generateClampedKnotVector(int numControlPoints, int degree) {
    if (numControlPoints <= 0 || degree < 1) {
        return {0.0f, 1.0f}; // fallback
    }
    int p = degree;
    int numKnots = numControlPoints + p + 1;
    std::vector<float> knots(numKnots, 0.0f);

    for (int i = 0; i <= p; ++i) {
        knots[i] = 0.0f;
        knots[numKnots - 1 - i] = 1.0f;
    }

    int numInterior = numKnots - 2 * (p + 1);
    if (numInterior > 0) {
        for (int i = 0; i < numInterior; ++i) {
            knots[p + 1 + i] = static_cast<float>(i + 1) / static_cast<float>(numInterior + 1);
        }
    }
    return knots;
}
// ========================
// 3. B-Spline Curve
// ========================
std::vector<glm::vec3> evaluateBSpline(const std::vector<glm::vec3>& controlPoints, int degree, int numSamples) {
    std::vector<glm::vec3> curve;
    size_t n = controlPoints.size();
    if (n == 0) return curve;
    if (degree >= static_cast<int>(n)) degree = static_cast<int>(n) - 1;
    if (degree < 1) {
        return controlPoints;
    }

    auto knots = generateClampedKnotVector(static_cast<int>(n), degree);

    // 采样 [0, 1)
    for (int s = 0; s < numSamples; ++s) {
        float u = static_cast<float>(s) / numSamples;
        glm::vec3 pt(0.0f);
        for (size_t i = 0; i < n; ++i) {
            float basis = coxDeBoor(static_cast<int>(i), degree, u, knots);
            pt += basis * controlPoints[i];
        }
        curve.push_back(pt);
    }

    // 添加终点
    curve.push_back(controlPoints.back());
    return curve;
}

// ========================
// 4. NURBS Curve
// ========================
std::vector<glm::vec3> evaluateNURBS(const std::vector<glm::vec3>& controlPoints,
                                     const std::vector<float>& weights,
                                     int degree,
                                     int numSamples) {
    assert(controlPoints.size() == weights.size());
    std::vector<glm::vec3> curve;
    size_t n = controlPoints.size();
    if (n == 0) return curve;
    if (degree >= static_cast<int>(n)) degree = static_cast<int>(n) - 1;
    if (degree < 1) {
        return controlPoints;
    }

    auto knots = generateClampedKnotVector(static_cast<int>(n), degree);

    for (int s = 0; s < numSamples; ++s) {
        float u = static_cast<float>(s) / numSamples;
        float denominator = 0.0f;
        glm::vec3 numerator(0.0f);
        for (size_t i = 0; i < n; ++i) {
            float basis = coxDeBoor(static_cast<int>(i), degree, u, knots);
            float w = weights[i];
            numerator += w * basis * controlPoints[i];
            denominator += w * basis;
        }
        if (std::abs(denominator) > 1e-6f) {
            curve.push_back(numerator / denominator);
        } else {
            glm::vec3 pt(0.0f);
            for (size_t i = 0; i < n; ++i) {
                float basis = coxDeBoor(static_cast<int>(i), degree, u, knots);
                pt += basis * controlPoints[i];
            }
            curve.push_back(pt);
        }
    }

    // 添加终点
    curve.push_back(controlPoints.back());
    return curve;
}


// ========================
// 5. Bezier Surface
// ========================
std::vector<glm::vec3> evaluateBezierSurface(const std::vector<std::vector<glm::vec3>>& controlPoints, 
                                           int uSamples, int vSamples) {
    std::vector<glm::vec3> surfacePoints;
    if (controlPoints.empty() || controlPoints[0].empty()) return surfacePoints;
    
    int n = static_cast<int>(controlPoints.size()) - 1;      // u方向控制点数-1
    int m = static_cast<int>(controlPoints[0].size()) - 1;   // v方向控制点数-1
    
    if (n < 0 || m < 0) return surfacePoints;
    
    // 双线性插值计算贝塞尔曲面点
    for (int i = 0; i <= uSamples; ++i) {
        float u = static_cast<float>(i) / uSamples;
        for (int j = 0; j <= vSamples; ++j) {
            float v = static_cast<float>(j) / vSamples;
            
            glm::vec3 point(0.0f);
            for (int k = 0; k <= n; ++k) {
                float bernsteinU = bernsteinPolynomial(n, k, u);
                for (int l = 0; l <= m; ++l) {
                    float bernsteinV = bernsteinPolynomial(m, l, v);
                    point += bernsteinU * bernsteinV * controlPoints[k][l];
                }
            }
            surfacePoints.push_back(point);
        }
    }
    
    return surfacePoints;
}

// 辅助函数：伯恩斯坦基函数
float bernsteinPolynomial(int n, int i, float t) {
    return binomialCoefficient(n, i) * pow(t, i) * pow(1.0f - t, n - i);
}

// 辅助函数：二项式系数
int binomialCoefficient(int n, int k) {
    if (k > n || k < 0) return 0;
    if (k == 0 || k == n) return 1;
    
    int result = 1;
    for (int i = 0; i < k; ++i) {
        result = result * (n - i) / (i + 1);
    }
    return result;
}

// ========================
// 6. B-Spline Surface
// ========================
std::vector<glm::vec3> evaluateBSplineSurface(const std::vector<std::vector<glm::vec3>>& controlPoints,
                                             int degreeU, int degreeV,
                                             int uSamples, int vSamples) {
    std::vector<glm::vec3> surfacePoints;
    if (controlPoints.empty() || controlPoints[0].empty()) return surfacePoints;
    
    int n = static_cast<int>(controlPoints.size()) - 1;      // u方向控制点数-1
    int m = static_cast<int>(controlPoints[0].size()) - 1;   // v方向控制点数-1
    
    if (n < 0 || m < 0) return surfacePoints;
    
    // 限制次数不超过控制点数-1
    if (degreeU > n) degreeU = n;
    if (degreeV > m) degreeV = m;
    if (degreeU < 1) degreeU = 1;
    if (degreeV < 1) degreeV = 1;
    
    // 生成节点向量
    auto knotsU = generateClampedKnotVector(n + 1, degreeU);
    auto knotsV = generateClampedKnotVector(m + 1, degreeV);
    
    // 计算曲面点
    for (int i = 0; i <= uSamples; ++i) {
        float u = static_cast<float>(i) / uSamples;
        for (int j = 0; j <= vSamples; ++j) {
            float v = static_cast<float>(j) / vSamples;
            
            glm::vec3 point(0.0f);
            for (int k = 0; k <= n; ++k) {
                float basisU = coxDeBoor(k, degreeU, u, knotsU);
                for (int l = 0; l <= m; ++l) {
                    float basisV = coxDeBoor(l, degreeV, v, knotsV);
                    point += basisU * basisV * controlPoints[k][l];
                }
            }
            surfacePoints.push_back(point);
        }
    }
    
    return surfacePoints;
}

// ========================
// 7. NURBS Surface
// ========================
std::vector<glm::vec3> evaluateNURBSSurface(const std::vector<std::vector<glm::vec3>>& controlPoints,
                                           const std::vector<std::vector<float>>& weights,
                                           int degreeU, int degreeV,
                                           int uSamples, int vSamples) {
    std::vector<glm::vec3> surfacePoints;
    if (controlPoints.empty() || controlPoints[0].empty()) return surfacePoints;
    if (controlPoints.size() != weights.size() || 
        (controlPoints.size() > 0 && controlPoints[0].size() != weights[0].size())) {
        return surfacePoints; // 权重和控制点维度必须一致
    }
    
    int n = static_cast<int>(controlPoints.size()) - 1;      // u方向控制点数-1
    int m = static_cast<int>(controlPoints[0].size()) - 1;   // v方向控制点数-1
    
    if (n < 0 || m < 0) return surfacePoints;
    
    // 限制次数不超过控制点数-1
    if (degreeU > n) degreeU = n;
    if (degreeV > m) degreeV = m;
    if (degreeU < 1) degreeU = 1;
    if (degreeV < 1) degreeV = 1;
    
    // 生成节点向量
    auto knotsU = generateClampedKnotVector(n + 1, degreeU);
    auto knotsV = generateClampedKnotVector(m + 1, degreeV);
    
    // 计算曲面点
    for (int i = 0; i <= uSamples; ++i) {
        float u = static_cast<float>(i) / uSamples;
        for (int j = 0; j <= vSamples; ++j) {
            float v = static_cast<float>(j) / vSamples;
            
            float denominator = 0.0f;
            glm::vec3 numerator(0.0f);
            
            for (int k = 0; k <= n; ++k) {
                float basisU = coxDeBoor(k, degreeU, u, knotsU);
                for (int l = 0; l <= m; ++l) {
                    float basisV = coxDeBoor(l, degreeV, v, knotsV);
                    float w = weights[k][l];
                    float weight = w * basisU * basisV;
                    numerator += weight * controlPoints[k][l];
                    denominator += weight;
                }
            }
            
            if (std::abs(denominator) > 1e-6f) {
                surfacePoints.push_back(numerator / denominator);
            } else {
                // 退化情况，使用普通B样条
                glm::vec3 point(0.0f);
                for (int k = 0; k <= n; ++k) {
                    float basisU = coxDeBoor(k, degreeU, u, knotsU);
                    for (int l = 0; l <= m; ++l) {
                        float basisV = coxDeBoor(l, degreeV, v, knotsV);
                        point += basisU * basisV * controlPoints[k][l];
                    }
                }
                surfacePoints.push_back(point);
            }
        }
    }
    
    return surfacePoints;
}

// ========================
// 8. 生成曲面索引（用于渲染）
// ========================
std::vector<unsigned int> generateSurfaceIndices(int uSamples, int vSamples) {
    std::vector<unsigned int> indices;
    
    for (int i = 0; i < uSamples; ++i) {
        for (int j = 0; j < vSamples; ++j) {
            unsigned int topLeft = i * (vSamples + 1) + j;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (i + 1) * (vSamples + 1) + j;
            unsigned int bottomRight = bottomLeft + 1;
            
            // 第一个三角形
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            // 第二个三角形
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    return indices;
}

} // namespace Spline