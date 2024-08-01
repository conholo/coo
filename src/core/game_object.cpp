#include <numeric>
#include "game_object.h"
#include "vulkan/vulkan_context.h"


// Matrix corresponds to Translate * Ry * Rx * Rz * Scale
// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
glm::mat4 TransformComponent::Mat4() const
{
    const float c3 = glm::cos(glm::radians(Rotation.z));
    const float s3 = glm::sin(glm::radians(Rotation.z));
    const float c2 = glm::cos(glm::radians(Rotation.x));
    const float s2 = glm::sin(glm::radians(Rotation.x));
    const float c1 = glm::cos(glm::radians(Rotation.y));
    const float s1 = glm::sin(glm::radians(Rotation.y));

    return glm::mat4
    {
        {
            Scale.x * (c1 * c3 + s1 * s2 * s3),
            Scale.x * (c2 * s3),
            Scale.x * (c1 * s2 * s3 - c3 * s1),
            0.0f,
        },
        {
            Scale.y * (c3 * s1 * s2 - c1 * s3),
            Scale.y * (c2 * c3),
            Scale.y * (c1 * c3 * s2 + s1 * s3),
            0.0f,
        },
        {
            Scale.z * (c2 * s1),
            Scale.z * (-s2),
            Scale.z * (c1 * c2),
            0.0f,
        },
        {Translation.x, Translation.y, Translation.z, 1.0f}
    };
}

glm::mat3 TransformComponent::NormalMatrix() const
{
    const float c3 = glm::cos(glm::radians(Rotation.z));
    const float s3 = glm::sin(glm::radians(Rotation.z));
    const float c2 = glm::cos(glm::radians(Rotation.x));
    const float s2 = glm::sin(glm::radians(Rotation.x));
    const float c1 = glm::cos(glm::radians(Rotation.y));
    const float s1 = glm::sin(glm::radians(Rotation.y));
    const glm::vec3 invScale = 1.0f / Scale;

    return glm::mat3{
        {
            invScale.x * (c1 * c3 + s1 * s2 * s3),
            invScale.x * (c2 * s3),
            invScale.x * (c1 * s2 * s3 - c3 * s1),
        },
        {
            invScale.y * (c3 * s1 * s2 - c1 * s3),
            invScale.y * (c2 * c3),
            invScale.y * (c1 * c3 * s2 + s1 * s3),
        },
        {
            invScale.z * (c2 * s1),
            invScale.z * (-s2),
            invScale.z * (c1 * c2),
      },
  };
}

VkDescriptorBufferInfo GameObject::GetBufferInfo(int frameIndex)
{
    return m_GameObjectManger.GetBufferInfoForGameObject(frameIndex, m_Id);
}

GameObject& GameObjectManager::MakePointLight(float intensity, float radius, glm::vec3 color)
{
    GameObject& obj = CreateGameObject();
    obj.Color = color;
    obj.ObjectTransform.Scale.x = radius;
    obj.PointLightComp = std::make_unique<PointLightComponent>();
    obj.PointLightComp->LightIntensity = intensity;
    return obj;
}

void GameObjectManager::UpdateBuffer(int frameIndex)
{
    // copy model matrix and normal matrix for each gameObj into
    // buffer for this frame
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

GameObjectManager::GameObjectManager()
{
    // including nonCoherentAtomSize allows us to flush a specific index at once
    int alignment = std::lcm(
        VulkanContext::Get().PhysicalDeviceProperties().limits.nonCoherentAtomSize,
        VulkanContext::Get().PhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment);

    for (auto& uboBuffer : m_GameObjectUboBuffers)
    {
        uboBuffer = std::make_unique<VulkanBuffer>(
            sizeof(GameObjectBufferData),
            GameObjectManager::MAX_GAME_OBJECTS,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            alignment);
        uboBuffer->Map();
    }
}
