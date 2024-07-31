#pragma once

#include "model.h"
#include <memory>
#include <unordered_map>
#include <vulkan_swapchain.h>
#include <vulkan_texture.h>
#include <glm/gtc/matrix_transform.hpp>

class GameObjectManager;

struct TransformComponent
{
    glm::vec3 Translation{};
    glm::vec3 Scale{1.0f, 1.0f, 1.0f};
    glm::vec3 Rotation{};

    [[nodiscard]] glm::mat4 Mat4() const;
    [[nodiscard]] glm::mat3 NormalMatrix() const;
};

struct PointLightComponent
{
    float LightIntensity = 1.0f;
};

struct GameObjectBufferData
{
    glm::mat4 ModelMatrix{1.0f};
    glm::mat4 NormalMatrix{1.0f};
};

class GameObject
{
public:
    using id_t = unsigned int;
    using Map = std::unordered_map<id_t, GameObject>;

    GameObject(const GameObject&) = delete;
    GameObject&operator=(const GameObject &) = delete;
    GameObject(GameObject&&) = default;
    GameObject&operator=(GameObject &&) = delete;

    [[nodiscard]] id_t GetId() const { return m_Id; }

    VkDescriptorBufferInfo GetBufferInfo(int frameIndex);

    glm::vec3 Color{};
    TransformComponent ObjectTransform{};

    std::shared_ptr<VulkanTexture2D> DiffuseMap = nullptr;
    std::shared_ptr<VulkanTexture2D> NormalMap = nullptr;
    std::shared_ptr<Model> ObjectModel{};
    std::unique_ptr<PointLightComponent> PointLightComp = nullptr;

private:

    explicit GameObject(id_t objectId, const GameObjectManager& gameObjectManager )
        :m_Id(objectId), m_GameObjectManger(gameObjectManager) { }

    id_t m_Id;
    const GameObjectManager& m_GameObjectManger;

    friend class GameObjectManager;
};

class GameObjectManager
{
public:
    static constexpr int MAX_GAME_OBJECTS = 1000;

    explicit GameObjectManager(VulkanDevice& device);
    GameObjectManager(const GameObjectManager&) = delete;

    GameObjectManager& operator=(const GameObjectManager&) = delete;
    GameObjectManager(GameObjectManager&&) = delete;
    GameObjectManager& operator=(GameObjectManager&&) = delete;

    GameObject& CreateGameObject()
    {
        assert(m_CurrentId < MAX_GAME_OBJECTS && "Max game object count exceeded!");
        auto gameObject = GameObject{m_CurrentId++, *this};
        auto gameObjectId = gameObject.GetId();
        gameObject.DiffuseMap = m_DefaultTexture;
        GameObjects.emplace(gameObjectId, std::move(gameObject));
        return GameObjects.at(gameObjectId);
    }

    GameObject& MakePointLight(float intensity = 10.f, float radius = 0.1f, glm::vec3 color = glm::vec3(1.0f));

    VkDescriptorBufferInfo GetBufferInfoForGameObject(int frameIndex, GameObject::id_t gameObjectId) const
    {
        return m_GameObjectUboBuffers[frameIndex]->DescriptorInfoForIndex(gameObjectId);
    }

    void UpdateBuffer(int frameIndex);

    GameObject::Map GameObjects{};
    std::vector<std::unique_ptr<VulkanBuffer>> m_GameObjectUboBuffers{VulkanSwapchain::MAX_FRAMES_IN_FLIGHT};

private:
    GameObject::id_t m_CurrentId = 0;
    std::shared_ptr<VulkanTexture2D> m_DefaultTexture;
};
