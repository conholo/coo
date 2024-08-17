#pragma once
#include "core/event/event.h"

#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <functional>

struct WindowProperties
{
    std::string Title{};
    uint32_t	Width;
    uint32_t	Height;

    explicit WindowProperties(std::string title = "coo", uint32_t width = 800, uint32_t height = 600)
            : Title(std::move(title)), Width(width), Height(height) {}
};


class Window
{
public:
    using EventCallbackFn = std::function<void(Event&)>;

    explicit Window(const WindowProperties& props = WindowProperties());
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool ShouldClose() { return glfwWindowShouldClose(m_WindowHandle); }
    VkExtent2D GetExtent() const { return { static_cast<uint32_t>(m_Data.Width), static_cast<uint32_t>(m_Data.Height)}; }

    bool WasWindowResized() const { return m_Data.WasWindowResized; }
    void ResetWindowResizedFlag() { m_Data.WasWindowResized = false; }

    void CreateWindowSurface(VkInstance instance, VkSurfaceKHR* outSurface);
    void SetEventCallback(const EventCallbackFn& callback) { m_Data.Callback = callback; }

	void* GetWindowPtr() const { return static_cast<void*>(m_WindowHandle); }

private:
    void Initialize(const WindowProperties& props);

    GLFWwindow* m_WindowHandle{};

    struct WindowData
    {
        std::string	 Title;
        unsigned int Width, Height;
        bool WasWindowResized = false;

        EventCallbackFn Callback;
    };

    WindowData m_Data;
};