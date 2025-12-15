#pragma

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 position;
    glm::vec3 target;      // 注视点（通常为 (0,0,0)）
    glm::vec3 up;

    float distance;        // 从 target 到 position 的距离
    float yaw;             // 水平旋转角（绕 Y 轴）
    float pitch;           // 垂直旋转角（绕 X 轴）

    Camera() {
        target = glm::vec3(0.0f, 0.0f, 0.0f);
        distance = 5.0f;
        yaw = 0.0f;
        pitch = 0.0f;
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        updatePosition();
    }

    void rotate(float dx, float dy) {
        yaw += dx * 0.01f;
        pitch += dy * 0.01f;
        // 限制俯仰角，避免翻转
        pitch = glm::clamp(pitch, -glm::radians(89.0f), glm::radians(89.0f));
        updatePosition();
    }

    void zoom(float offset) {
        distance -= offset * 0.5f;
        distance = glm::max(distance, 0.5f); // 最小距离
        distance = glm::min(distance, 50.0f); // 最大距离
        updatePosition();
    }

    void pan(float dx, float dy) {
        // 获取右向量和上向量
        glm::vec3 front = glm::normalize(target - position);
        glm::vec3 right = glm::normalize(glm::cross(front, up));
        glm::vec3 worldUp = glm::normalize(glm::cross(right, front));

        target += right * (-dx * 0.01f) + worldUp * (dy * 0.01f);
        updatePosition();
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, target, up);
    }

    glm::vec3 getFront() const {
        return glm::vec3(
            cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            sin(glm::radians(yaw)) * cos(glm::radians(pitch))
        );
    }

private:
    void updatePosition() {
        // 从 target 出发，沿球坐标方向后退 distance
        glm::vec3 direction;
        direction.x = glm::cos(yaw) * glm::cos(pitch);
        direction.y = glm::sin(pitch);
        direction.z = glm::sin(yaw) * glm::cos(pitch);
        direction = glm::normalize(direction);

        position = target - distance * direction;
    }
};
