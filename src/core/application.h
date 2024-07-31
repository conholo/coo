#pragma once

#include <camera.h>
#include <vector>
#include "vulkan_device.h"
#include "vulkan_descriptors.h"
#include "game_object.h"
#include "window.h"

class Application
{
public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void Run();

    static Application* GetInstance() { return s_ApplicationInstance; }
    [[nodiscard]] const Camera& GetCamera() const { return m_Camera; }
    Camera& GetCamera() { return m_Camera; }

private:
    void LoadGameObjects();

private:

    Window m_Window { WIDTH, HEIGHT, "Vulkan Window" };
    VulkanDevice m_VulkanDevice { m_Window };

    std::unique_ptr<VulkanDescriptorPool> m_GlobalPool;
    GameObjectManager m_GameObjectManager {m_VulkanDevice};
    inline static Application* s_ApplicationInstance = nullptr;
    Camera m_Camera {};
};