#pragma once

#include "vulkan/vulkan_buffer.h"
#include "game_object.h"

class VulkanRenderer;

class Scene
{
public:
    static constexpr int MAX_GAME_OBJECTS = 1000;

    explicit Scene();
    Scene(const Scene&) = delete;

    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;

    GameObject& CreateGameObject(VulkanRenderer& renderer);
    VkDescriptorBufferInfo GetBufferInfoForGameObject(size_t frameIndex, GameObject::id_t gameObjectId) const
    {
        return m_GameObjectUboBuffers[frameIndex]->DescriptorInfoForIndex(gameObjectId);
    }

    void UpdateGameObjectUboBuffers(int frameIndex);

    GameObject::Map GameObjects{};
    std::vector<std::unique_ptr<VulkanBuffer>> m_GameObjectUboBuffers{VulkanSwapchain::MAX_FRAMES_IN_FLIGHT};

private:
    GameObject::id_t m_CurrentId = 0;
};
