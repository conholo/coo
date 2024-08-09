#include "scene.h"
#include "game_object.h"
#include "vulkan/vulkan_renderer.h"
#include <numeric>

void Scene::UpdateGameObjectUboBuffers(int frameIndex)
{
    // Copy model matrix and normal matrix for each game object into buffer for this frame.
    for(auto& entry: GameObjects)
    {
        auto& obj = entry.second;
        GameObjectBufferData data{};
        data.ModelMatrix = obj.ObjectTransform.Mat4();
        data.NormalMatrix = glm::mat4(obj.ObjectTransform.NormalMatrix());
        m_GameObjectUboBuffers[frameIndex]->WriteToIndex(&data, entry.first);
    }

    m_GameObjectUboBuffers[frameIndex]->Flush();
}

GameObject &Scene::CreateGameObject(VulkanRenderer &renderer)
{
    assert(m_CurrentId < MAX_GAME_OBJECTS && "Max game object count exceeded!");
    auto gameObject = GameObject{m_CurrentId++, *this};
    auto gameObjectId = gameObject.GetId();
    renderer.PrepareGameObjectForRendering(gameObject);
    GameObjects.emplace(gameObjectId, std::move(gameObject));
    return GameObjects.at(gameObjectId);
}

Scene::Scene()
{
    // including nonCoherentAtomSize allows us to flush a specific index at once
    auto alignment = std::lcm(
            VulkanContext::Get().PhysicalDeviceProperties().limits.nonCoherentAtomSize,
            VulkanContext::Get().PhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment);

    for (auto& uboBuffer : m_GameObjectUboBuffers)
    {
        uboBuffer = std::make_unique<VulkanBuffer>(
                sizeof(GameObjectBufferData),
                Scene::MAX_GAME_OBJECTS,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                alignment);
        uboBuffer->Map();
    }
}

