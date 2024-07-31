#include "window.h"

#include <utility>
#include <stdexcept>

Window::Window(int width, int height, std::string name)
    :m_Width(width), m_Height(height), m_WindowName(std::move(name))
{
    InitializeWindow();
}

Window::~Window()
{
    glfwDestroyWindow(m_WindowHandle);
    glfwTerminate();
}

void Window::InitializeWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_WindowHandle = glfwCreateWindow(m_Width, m_Height, m_WindowName.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(m_WindowHandle, this);
    glfwSetFramebufferSizeCallback(m_WindowHandle, FramebufferResizeCallback);
    glfwSetMouseButtonCallback(m_WindowHandle, OnMouseButtonCallback);
    glfwSetScrollCallback(m_WindowHandle, OnScrollCallback);
    glfwSetCursorPosCallback(m_WindowHandle, OnMouseMoveCallback);
}

void Window::CreateWindowSurface(VkInstance instance, VkSurfaceKHR *outSurface)
{
    if(glfwCreateWindowSurface(instance, m_WindowHandle, nullptr, outSurface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface.");
}

void Window::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto baseWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    baseWindow->m_FramebufferResized = true;
    baseWindow->m_Width = width;
    baseWindow->m_Height = height;
}

void Window::OnMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    double cursorX, cursorY;
    glfwGetCursorPos(window, &cursorX, &cursorY);
}

void Window::OnScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
}

void Window::OnMouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
}

