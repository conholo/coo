#pragma once

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

struct InputState
{
    // angles.x is the rotation of the camera around the global vertical axis, affected by mouse.x
    // angles.y is the rotation of the camera around its local horizontal axis, affected by mouse.y
    glm::vec2 Angles = {0.8f, 0.5f};
    // zoom is the position of the camera along its local forward axis, affected by the scroll wheel
    float Zoom = -1.2f;
};

struct DragState
{
    // Whether a drag action is ongoing (i.e., we are between mouse press and mouse release)
    bool Active = false;
    // The position of the mouse at the beginning of the drag action
    glm::vec2 StartMouse{};
    // The input state at the beginning of the drag action
    InputState StartInputState;

    // Constant settings
    inline static float Sensitivity = 0.01f;
    inline static float ScrollSensitivity = 0.1f;

    // Inertia
    glm::vec2 Velocity = {0.0, 0.0};
    glm::vec2 PreviousDelta{};
    float Inertia = 0.9f;
};

class Camera
{
public:
    void SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
    void SetPerspectiveProjection(float fovy, float aspect, float near, float far);
    void UpdateView();

    [[nodiscard]] const glm::mat4& GetProjection() const { return m_ProjectionMatrix; }
    [[nodiscard]] const glm::mat4& GetView() const { return m_ViewMatrix; }
    [[nodiscard]] const glm::mat4& GetInvView() const { return m_InvViewMatrix; }
    [[nodiscard]] const glm::mat4& GetInvProjection() const { return m_InverseProjectionMatrix; }
    glm::vec3 GetPosition() { return {m_InvViewMatrix[3] }; }

    void Tick(float deltaTime);
    void OnMouseButton(int button, int action, int /* modifiers */, double cursorX, double cursorY);
    void OnScroll(double/* xoffset*/, double yoffset);
    void OnMouseMove(double xpos, double ypos);

private:
    InputState m_CurrentInputState;
    DragState m_DragState;

    glm::mat4 m_InverseProjectionMatrix{1.0f};
    glm::mat4 m_ProjectionMatrix{1.0f};
    glm::mat4 m_ViewMatrix{1.0f};
    glm::mat4 m_InvViewMatrix{1.0f};
};
