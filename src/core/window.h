#pragma once

#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window
{
public:
    Window(int width, int height, std::string name);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool ShouldClose() { return glfwWindowShouldClose(m_WindowHandle); }
    [[nodiscard]] VkExtent2D GetExtent() const { return { static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height)}; }

    [[nodiscard]] bool WasWindowResized() const { return m_FramebufferResized; }
    void ResetWindowResizedFlag() { m_FramebufferResized = false; }

    void CreateWindowSurface(VkInstance instance, VkSurfaceKHR* outSurface);
    static void FramebufferResizeCallback(GLFWwindow *window, int width, int height);
    static void OnMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void OnScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void OnMouseMoveCallback(GLFWwindow* window, double xpos, double ypos);

    [[nodiscard]] GLFWwindow* GetGLFWwindow() const { return m_WindowHandle; }

private:

    void InitializeWindow();

    bool m_FramebufferResized = false;
    int m_Width;
    int m_Height;

    std::string m_WindowName;
    GLFWwindow* m_WindowHandle{};
};