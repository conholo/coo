#pragma once

#include "vulkan/vulkan_model.h"
#include "vulkan/vulkan_swapchain.h"
#include "vulkan/vulkan_material.h"
#include "vulkan/vulkan_texture.h"

#include <memory>
#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>

struct FrameInfo;
class Scene;

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

    void Render(VkCommandBuffer cmd, uint32_t frameIndex, VkDescriptorBufferInfo globalUboInfo);

    id_t GetId() const { return m_Id; }

    VkDescriptorBufferInfo GetBufferInfo(int frameIndex);

    glm::vec3 Color{};
    TransformComponent ObjectTransform{};

    std::shared_ptr<VulkanMaterial> Material = nullptr;
    std::shared_ptr<VulkanTexture2D> DiffuseMap = nullptr;
    std::shared_ptr<VulkanTexture2D> NormalMap = nullptr;
    std::shared_ptr<VulkanModel> ObjectModel = nullptr;
    std::unique_ptr<PointLightComponent> PointLightComp = nullptr;

private:

    explicit GameObject(id_t objectId, const Scene& gameObjectManager )
        : m_Id(objectId), m_Scene(gameObjectManager) { }

    id_t m_Id;
    const Scene& m_Scene;

    friend class Scene;
};
