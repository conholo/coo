#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include "vulkan/vulkan_context.h"
#include "core/camera.h"
#include "vulkan/vulkan_image.h"


int main()
{
    try
    {
        Window window{800, 600, "coo"};
        VulkanContext::Initialize("coo", 1.0, &window);
        Camera camera;

        ImageSpecification spec
        {
            .DebugName{"Test Image"},
            .Format = ImageFormat::RGBA,
            .Usage = ImageUsage::Storage,
            .Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            .UsedInTransferOps = true,
            .Width = 512,
            .Height = 512
        };
        auto* image = new VulkanImage2D(spec);

        auto currentTime = std::chrono::high_resolution_clock::now();
        while(!window.ShouldClose())
        {
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float>(newTime - currentTime).count();
            currentTime = newTime;

            camera.Tick(frameTime);
            glfwPollEvents();
        }

        delete image;
        VulkanContext::Shutdown();
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
