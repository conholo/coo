#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <chrono>
#include "vulkan/vulkan_context.h"
#include "core/camera.h"
#include "vulkan/vulkan_renderer.h"


int main()
{
    try
    {
        Window window{800, 600, "coo"};
        VulkanContext::Initialize("coo", 1.0, &window);
        Camera camera;

        VulkanRenderer renderer{window};
        renderer.Initialize();

        auto currentTime = std::chrono::high_resolution_clock::now();
        while(!window.ShouldClose())
        {
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float>(newTime - currentTime).count();
            currentTime = newTime;

            renderer.Render();

            camera.Tick(frameTime);
            glfwPollEvents();
        }

        renderer.Shutdown();
        VulkanContext::Shutdown();
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
