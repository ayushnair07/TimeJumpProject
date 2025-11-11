#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"
#include <cmath>

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(pos, pos + front, up);
}

glm::mat4 Camera::GetProjection(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, 0.1f, 1000.0f);
}

void Camera::UpdateAuto(float t) {
    // simple circular path camera motion around origin
    float radius = 120.0f;
    float speedFactor = 0.08f;
    float angle = t * speedFactor;

    pos.x = radius * std::cos(angle);
    pos.z = radius * std::sin(angle);
    pos.y = 40.0f + 10.0f * std::sin(angle * 0.5f);

    front = glm::normalize(glm::vec3(-pos.x, -pos.y * 0.2f, -pos.z)); // look toward origin
}

void Camera::ProcessKeyboard(char key, float dt) {
    float velocity = speed * dt;
    glm::vec3 right = glm::normalize(glm::cross(front, up));

    if (key == 'W') pos += front * velocity;
    if (key == 'S') pos -= front * velocity;
    if (key == 'A') pos -= right * velocity;
    if (key == 'D') pos += right * velocity;
}

void Camera::ProcessMouseMovement(float dx, float dy) {
    dx *= sensitivity;
    dy *= sensitivity;

    yaw += dx;
    pitch += dy;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(dir);
}
