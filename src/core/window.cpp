#include "window.h"
#include "core/event/window_event.h"
#include "core/event/mouse_event.h"
#include "core/event/key_event.h"

#include <stdexcept>
#include <iostream>

Window::Window(const WindowProperties& properties)
{
    Initialize(properties);
}

Window::~Window()
{
    glfwDestroyWindow(m_WindowHandle);
    glfwTerminate();
}

void Window::Initialize(const WindowProperties& properties)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_Data.Title = properties.Title;
    m_Data.Width = properties.Width;
    m_Data.Height = properties.Height;

    m_WindowHandle = glfwCreateWindow(m_Data.Width, m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(m_WindowHandle, static_cast<void*>(&m_Data));

    glfwSetWindowCloseCallback(m_WindowHandle, [](GLFWwindow* window)
    {
        WindowData& data = *(static_cast<WindowData*>(glfwGetWindowUserPointer(window)));
        WindowClosedEvent windowClosedEvent;
        data.Callback(windowClosedEvent);
    });

	glfwSetFramebufferSizeCallback(m_WindowHandle, [](GLFWwindow* window, int width, int height)
	{
		std::cout << "Received GLFW OnFramebufferSizeCallback!" << "\n";

		WindowData& data = *(static_cast<WindowData*>(glfwGetWindowUserPointer(window)));
		data.Width = width;
		data.Height = height;
		data.WasWindowResized = true;

		WindowResizedEvent windowResizedEvent(width, height);
		data.Callback(windowResizedEvent);
	});

    glfwSetWindowPosCallback(m_WindowHandle, [](GLFWwindow* window, int xPosition, int yPosition) {
        WindowData& data = *(static_cast<WindowData*>(glfwGetWindowUserPointer(window)));

        WindowMovedEvent windowMovedEvent(xPosition, yPosition);
        data.Callback(windowMovedEvent);
    });

    glfwSetKeyCallback(
            m_WindowHandle, [](GLFWwindow* window, int keyCode, int scanCode, int action, int mods) {
                WindowData& data = *(static_cast<WindowData*>(glfwGetWindowUserPointer(window)));

                switch (action)
                {
                    case GLFW_PRESS:
                    {
                        KeyPressedEvent keyPressedEvent(keyCode, 0);
                        data.Callback(keyPressedEvent);
                        break;
                    }
                    case GLFW_REPEAT:
                    {
                        KeyPressedEvent keyPressedEvent(keyCode, 1);
                        data.Callback(keyPressedEvent);
                        break;
                    }
                    case GLFW_RELEASE:
                    {
                        KeyReleasedEvent keyReleasedEvent(keyCode);
                        data.Callback(keyReleasedEvent);
                        break;
                    }
                }
            });

    glfwSetMouseButtonCallback(
            m_WindowHandle, [](GLFWwindow* window, int button, int action, int mods)
            {
                WindowData& data = *(static_cast<WindowData*>(glfwGetWindowUserPointer(window)));
                double cursorX, cursorY;
                glfwGetCursorPos(window, &cursorX, &cursorY);

                switch (action)
                {
                    case GLFW_PRESS:
                    {
                        MouseButtonPressedEvent mouseButtonPressedEvent(button, cursorX, cursorY);
                        data.Callback(mouseButtonPressedEvent);
                        break;
                    }
                    case GLFW_RELEASE:
                    {
                        MouseButtonReleasedEvent mouseButtonReleasedEvent(button, cursorX, cursorY);
                        data.Callback(mouseButtonReleasedEvent);
                        break;
                    }
                }
            });

    glfwSetScrollCallback(m_WindowHandle, [](GLFWwindow* window, double xOffset, double yOffset)
    {
        WindowData& data = *(static_cast<WindowData*>(glfwGetWindowUserPointer(window)));

        MouseScrolledEvent mouseScrolledEvent(static_cast<float>(xOffset), static_cast<float>(yOffset));
        data.Callback(mouseScrolledEvent);
    });

    glfwSetCursorPosCallback(m_WindowHandle, [](GLFWwindow* window, double xPosition, double yPosition)
    {
        WindowData& data = *(static_cast<WindowData*>(glfwGetWindowUserPointer(window)));

        MouseMovedEvent mouseMovedEvent(static_cast<float>(xPosition), static_cast<float>(yPosition));
        data.Callback(mouseMovedEvent);
    });
}

void Window::CreateWindowSurface(VkInstance instance, VkSurfaceKHR *outSurface)
{
    if(glfwCreateWindowSurface(instance, m_WindowHandle, nullptr, outSurface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface.");
}

