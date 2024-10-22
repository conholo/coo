#pragma once

#include "event/event.h"
#include "event/window_event.h"
#include "vulkan/vulkan_renderer.h"

#include <memory>

class Application
{
public:
    Application();
    ~Application();

    void Run();
    void OnEvent(Event& event);

private:
    bool OnWindowClose(WindowClosedEvent& event);
    bool OnWindowResize(WindowResizedEvent& event);

	void CreateGameObjects(Scene& scene, VulkanRenderer& renderer);

    std::shared_ptr<Window> m_Window;
    std::unique_ptr<VulkanRenderer> m_Renderer;
    std::unique_ptr<Scene> m_Scene;
    Camera m_Camera;

	bool m_ApplicationIsRunning = true;
};