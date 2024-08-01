#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <chrono>
#include "vulkan/vulkan_context.h"
#include "core/camera.h"
#include "vulkan/vulkan_renderer.h"
#include "core/game_object.h"
#include "vulkan/vulkan_model.h"

void CreateGameObjects(GameObjectManager& gameObjectManager)
{
    auto bunnyModel = VulkanModel::CreateModelFromFile("../models/bunny.obj");
    auto suzanneModel = VulkanModel::CreateModelFromFile("../models/suzanne.obj");
    auto quadModel = VulkanModel::CreateModelFromFile("../models/quad.obj");

    auto &bunny = gameObjectManager.CreateGameObject();
    bunny.ObjectModel = bunnyModel;
    bunny.ObjectTransform.Translation = {-0.5f, 0.0f, 0.085f};
    bunny.ObjectTransform.Scale = {0.25, 0.25, 0.25};
    bunny.ObjectTransform.Rotation = {90.0f, 0.0f, 0.0f};

    auto &suzanne = gameObjectManager.CreateGameObject();
    suzanne.ObjectModel = suzanneModel;
    suzanne.ObjectTransform.Translation = {.5f, 0.0f, 0.35f};
    suzanne.ObjectTransform.Scale = {0.35, 0.35, 0.35};
    suzanne.ObjectTransform.Rotation = {90.0f, 0.0f, 0.0f};

    auto &floor = gameObjectManager.CreateGameObject();
    floor.ObjectModel = quadModel;
    floor.ObjectTransform.Translation = {0.0f, 0.0f, 0.0};
    floor.ObjectTransform.Scale = {3.f, 1.0f, 3.f};
    floor.ObjectTransform.Rotation = {270.0, 0.0, 0.0};
}

int main()
{
    try
    {
        Window window{800, 600, "coo"};
        VulkanContext::Initialize("coo", 1.0, &window);
        Camera camera;

        VulkanRenderer renderer{window};
        renderer.Initialize();

        GameObjectManager gameObjectManager;
        CreateGameObjects(gameObjectManager);

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
