#pragma once

#include "vulkan_device.h"
#include <memory>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <vulkan_buffer.h>
#include <glm/glm.hpp>

class Model
{
public:
    struct Vertex
    {
        glm::vec3 Position{};
        glm::vec3 Color{};
        glm::vec3 Normal{};
        glm::vec3 Tangent{};
        glm::vec2 UV{};

        static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

        bool operator==(const Vertex& other) const
        {
            return
                Position == other.Position &&
                Color == other.Color &&
                Normal == other.Normal &&
                Tangent == other.Tangent &&
                UV == other.UV;
        }
    };

    struct Builder
    {
        std::vector<Vertex> Vertices{};
        std::vector<uint32_t> Indices{};

        void LoadModel(const std::string& filePath);

    private:
        void ComputeTangentBasis(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    };

    Model(VulkanDevice& deviceRef, const Builder& builder);
    ~Model();

    Model(const Model &) = delete;
    Model& operator=(const Model &) = delete;

    static std::shared_ptr<Model> CreateModelFromFile(VulkanDevice& deviceRef, const std::string& filePath);
    void Bind(VkCommandBuffer commandBuffer);
    void Draw(VkCommandBuffer commandBuffer) const;

private:
    void CreateVertexBuffer(const std::vector<Vertex>& vertices);
    void CreateIndexBuffer(const std::vector<uint32_t>& indices);

private:
    VulkanDevice& m_DeviceRef;

    std::unique_ptr<VulkanBuffer> m_VertexBuffer;
    uint32_t m_VertexCount{};

    bool m_HasIndexBuffer{false};
    std::unique_ptr<VulkanBuffer> m_IndexBuffer;
    uint32_t m_IndexCount{};
};