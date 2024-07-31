#include "camera.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <limits>

void Camera::SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far)
{
    m_ProjectionMatrix = glm::mat4(1.0f);
    m_ProjectionMatrix[0][0] = 2.0f / (right - left);
    m_ProjectionMatrix[1][1] = 2.0f / (bottom - top);
    m_ProjectionMatrix[2][2] = 1.0f / (far - near);
    m_ProjectionMatrix[3][0] = -(right + left) / (right - left);
    m_ProjectionMatrix[3][1] = -(bottom + top) / (bottom - top);
    m_ProjectionMatrix[3][2] = -near / (far - near);
}

void Camera::SetPerspectiveProjection(float fovy, float aspect, float near, float far)
{
    assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
    const float tanHalfFovy = glm::tan(fovy / 2.0f);
    m_ProjectionMatrix = glm::mat4{0.0f};
    m_ProjectionMatrix[0][0] = 1.0f / (aspect * tanHalfFovy);
    m_ProjectionMatrix[1][1] = 1.0f / (tanHalfFovy);
    m_ProjectionMatrix[2][2] = far / (far - near);
    m_ProjectionMatrix[2][3] = 1.0f;
    m_ProjectionMatrix[3][2] = -(far * near) / (far - near);
    m_InverseProjectionMatrix = glm::inverse(m_ProjectionMatrix);
}

void Camera::UpdateView()
{
    const float cx = cos(m_CurrentInputState.Angles.x);
    const float sx = sin(m_CurrentInputState.Angles.x);
    const float cy = cos(m_CurrentInputState.Angles.y);
    const float sy = sin(m_CurrentInputState.Angles.y);
    const glm::vec3 eye = glm::vec3(cx * cy, sx * cy, sy) * std::exp(-m_CurrentInputState.Zoom);
    constexpr auto target = glm::vec3(0.0);
    constexpr auto up = glm::vec3(0.0, 0.0, 1.0);

    // Orthonormal basis - Left handed coordinate system
    const glm::vec3 f{glm::normalize(target - eye)};
    const glm::vec3 r{glm::normalize(glm::cross(f, up))};
    const glm::vec3 u{glm::cross(f, r)};

    m_ViewMatrix = glm::mat4{1.f};
    m_ViewMatrix[0][0] = r.x;
    m_ViewMatrix[1][0] = r.y;
    m_ViewMatrix[2][0] = r.z;
    m_ViewMatrix[0][1] = u.x;
    m_ViewMatrix[1][1] = u.y;
    m_ViewMatrix[2][1] = u.z;
    m_ViewMatrix[0][2] = f.x;
    m_ViewMatrix[1][2] = f.y;
    m_ViewMatrix[2][2] = f.z;
    m_ViewMatrix[3][0] = -glm::dot(r, eye);
    m_ViewMatrix[3][1] = -glm::dot(u, eye);
    m_ViewMatrix[3][2] = -glm::dot(f, eye);

    m_InvViewMatrix = glm::mat4{1.f};
    m_InvViewMatrix[0][0] = r.x;
    m_InvViewMatrix[0][1] = r.y;
    m_InvViewMatrix[0][2] = r.z;
    m_InvViewMatrix[1][0] = u.x;
    m_InvViewMatrix[1][1] = u.y;
    m_InvViewMatrix[1][2] = u.z;
    m_InvViewMatrix[2][0] = f.x;
    m_InvViewMatrix[2][1] = f.y;
    m_InvViewMatrix[2][2] = f.z;
    m_InvViewMatrix[3][0] = eye.x;
    m_InvViewMatrix[3][1] = eye.y;
    m_InvViewMatrix[3][2] = eye.z;
}

void Camera::OnScroll(double/* xoffset*/, double yoffset)
{
    m_CurrentInputState.Zoom += DragState::ScrollSensitivity * static_cast<float>(yoffset);
    m_CurrentInputState.Zoom = glm::clamp(m_CurrentInputState.Zoom, -3.0f, 2.0f);
    UpdateView();
}

void Camera::Tick(float deltaTime)
{
    constexpr float eps = 1e-4f;
    // Apply inertia only when the user released the click.

    if (m_DragState.Active) return;

    // Avoid updating the matrix when the velocity is no longer noticeable
    if (std::abs(m_DragState.Velocity.x) < eps && std::abs(m_DragState.Velocity.y) < eps)
        return;

    m_CurrentInputState.Angles += m_DragState.Velocity;
    m_CurrentInputState.Angles.y = glm::clamp(
        m_CurrentInputState.Angles.y,
        -glm::pi<float>() / 2 + 1e-5f,
        glm::pi<float>() / 2 - 1e-5f);

    // Dampen the velocity so that it decreases exponentially and stops after a few frames.
    m_DragState.Velocity *= m_DragState.Inertia;
    UpdateView();
}


void Camera::OnMouseButton(int button, int action, int /* modifiers */, double cursorX, double cursorY)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        switch (action)
        {
            case GLFW_PRESS:
                m_DragState.Active = true;
                m_DragState.StartMouse = glm::vec2(-static_cast<float>(cursorX), static_cast<float>(cursorY));
                m_DragState.StartInputState = m_CurrentInputState;
                break;
            case GLFW_RELEASE:
                m_DragState.Active = false;
                break;
        }
    }
}

void Camera::OnMouseMove(double xpos, double ypos)
{
    if(!m_DragState.Active) return;

    glm::vec2 currentMouse = glm::vec2(-static_cast<float>(xpos), static_cast<float>(ypos));
    glm::vec2 delta = (currentMouse - m_DragState.StartMouse) * DragState::Sensitivity;

    m_CurrentInputState.Angles = m_DragState.StartInputState.Angles + delta;
    // Clamp to avoid going too far when orbiting up/down
    m_CurrentInputState.Angles.y = glm::clamp(
        m_CurrentInputState.Angles.y,
        -glm::pi<float>() / 2 + 1e-5f,
        glm::pi<float>() / 2 - 1e-5f);
    UpdateView();

    m_DragState.Velocity = delta - m_DragState.PreviousDelta;
    m_DragState.PreviousDelta = delta;
}