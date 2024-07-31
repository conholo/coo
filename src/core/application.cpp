#include "application.h"
#include "frame_info.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <chrono>
#include <iostream>

Application::Application()
{
    assert(!s_ApplicationInstance && "Application instance already exists.");
    s_ApplicationInstance = this;
}

Application::~Application() = default;

void Application::LoadGameObjects()
{
    TextureSpecification fileSpec;

    auto eratoModel = Model::CreateModelFromFile(m_VulkanDevice, std::string("../assets/models/erato.obj"));
    auto quadModel = Model::CreateModelFromFile(m_VulkanDevice, std::string("../assets/models/quad.obj"));
    auto teapotModel = Model::CreateModelFromFile(m_VulkanDevice, std::string("../assets/models/teapot.obj"));

    auto& erato = m_GameObjectManager.CreateGameObject();
    //erato.DiffuseMap = VulkanTexture::CreateTextureFromFile(m_VulkanDevice, "../assets/textures/marble/marble_albedo.jpg");
    //erato.NormalMap = VulkanTexture::CreateTextureFromFile(m_VulkanDevice, "../assets/textures/marble/marble_normal.jpg");

    erato.ObjectModel = eratoModel;
    erato.DiffuseMap = std::make_shared<VulkanTexture2D>(m_VulkanDevice, TextureSpecification(), std::string("../assets/textures/marble/marble_albedo.jpg"));
    erato.NormalMap = std::make_shared<VulkanTexture2D>(m_VulkanDevice, TextureSpecification(), std::string("../assets/textures/marble/marble_normal.jpg"));

    erato.ObjectTransform.Translation = {-0.5f , 0.0f, 0.085f};
    erato.ObjectTransform.Scale = {0.01, 0.01, 0.01};
    erato.ObjectTransform.Rotation = {90.0f, 0.0f, 0.0f};

    auto& teapot = m_GameObjectManager.CreateGameObject();
    /*
    teapot.DiffuseMap = VulkanTexture::CreateTextureFromFile(m_VulkanDevice, "../assets/textures/moss/moss_albedo.jpg");
    teapot.NormalMap = VulkanTexture::CreateTextureFromFile(m_VulkanDevice, "../assets/textures/moss/moss_normal.jpg");
    */

    teapot.ObjectModel = teapotModel;
    teapot.DiffuseMap = std::make_shared<VulkanTexture2D>(m_VulkanDevice, TextureSpecification(), std::string("../assets/textures/moss/moss_albedo.jpg"));
    teapot.NormalMap = std::make_shared<VulkanTexture2D>(m_VulkanDevice, TextureSpecification(), std::string("../assets/textures/moss/moss_normal.jpg"));
    teapot.ObjectTransform.Translation = {.5f, 0.0f, 0.35f};
    teapot.ObjectTransform.Scale = {0.001, 0.001, 0.001};
    teapot.ObjectTransform.Rotation = {90.0f, 0.0f, 0.0f};

    auto& floor = m_GameObjectManager.CreateGameObject();
/*
    floor.DiffuseMap = VulkanTexture::CreateTextureFromFile(m_VulkanDevice, "../assets/textures/ground/ground_albedo.jpg");
    floor.NormalMap = VulkanTexture::CreateTextureFromFile(m_VulkanDevice, "../assets/textures/ground/ground_normal.jpg");
*/

    floor.ObjectModel = quadModel;
    floor.DiffuseMap = std::make_shared<VulkanTexture2D>(m_VulkanDevice, TextureSpecification(), std::string("../assets/textures/ground/ground_albedo.jpg"));
    floor.NormalMap = std::make_shared<VulkanTexture2D>(m_VulkanDevice, TextureSpecification(), std::string("../assets/textures/ground/ground_normal.jpg"));

    floor.ObjectTransform.Translation = {0.0f, 0.0f,  0.0};
    floor.ObjectTransform.Scale = {3.f, 1.0f, 3.f};
    floor.ObjectTransform.Rotation = {270.0, 0.0, 0.0};

    std::vector<glm::vec3> lightColors
    {
        {1.f, .1f, .1f},
        {.1f, .1f, 1.f},
        {.1f, 1.f, .1f},
        {1.f, 1.f, .1f},
        {.1f, 1.f, 1.f},
        {1.f, 1.f, 1.f}
    };

    for(int i = 0; i < lightColors.size(); i++)
    {
        auto& pointLight = m_GameObjectManager.MakePointLight(0.2, 1.0, lightColors[i]);
        float theta = i * glm::two_pi<float>() / lightColors.size();
        auto rotationMatrix = glm::rotate(glm::mat4(1.0f), theta, {0.f, 0.0f, 1.0f});
        pointLight.ObjectTransform.Translation = glm::vec3(rotationMatrix * glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
}

void Application::Run()
{
    DeferredRenderer renderer(m_VulkanDevice);

    auto currentTime = std::chrono::high_resolution_clock::now();
    m_Camera.SetPerspectiveProjection(glm::radians(50.0f), swapchainRenderer.GetAspectRatio(), 0.1f, 100.0f);

    while(!m_Window.ShouldClose())
    {
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float>(newTime - currentTime).count();
        currentTime = newTime;

        m_Camera.Tick(frameTime);
        voxelRenderer.Render(m_Camera);
    }

    vkDeviceWaitIdle(m_VulkanDevice.GetDevice());
}