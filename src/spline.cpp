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

} // namespace Spline