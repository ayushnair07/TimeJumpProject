#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CamMode { AUTO, FREE };

class Camera {
public:
    CamMode mode = CamMode::AUTO;

    glm::vec3 pos{ 0, 5, 20 };
    glm::vec3 front{ 0, 0, -1 };
    glm::vec3 up{ 0, 1, 0 };
    float yaw = -90.0f;
    float pitch = -10.0f;
    float fov = 45.0f;

    float speed = 25.0f;
    float sensitivity = 0.1f;

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjection(float aspect) const;

    void UpdateAuto(float timeSeconds);
    void ProcessKeyboard(char key, float dt);
    void ProcessMouseMovement(float dx, float dy);
};
