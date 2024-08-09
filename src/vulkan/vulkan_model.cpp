#include "vulkan_model.h"
#include "core/engine_utils.h"
#include "vulkan_buffer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std
{
    template<>
    struct hash<VulkanModel::Vertex>
    {
        size_t operator()(const VulkanModel::Vertex& vertex) const noexcept
        {
            size_t seed = 0;
            EngineUtils::HashCombine(seed, vertex.Position, vertex.Color, vertex.Normal, vertex.UV);
            return seed;
        }
    };
}

void VulkanModel::Builder::LoadModel(const std::string &filePath)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str()))
    {
        throw std::runtime_error(warn + err);
    }

    Vertices.clear();
    Indices.clear();
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto &shape: shapes)
    {
        for (const auto &index: shape.mesh.indices)
        {
            Vertex vertex{};

            if (index.vertex_index >= 0)
            {
                vertex.Position =
                {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2],
                };

                vertex.Color = {
                    attrib.colors[3 * index.vertex_index + 0],
                    attrib.colors[3 * index.vertex_index + 1],
                    attrib.colors[3 * index.vertex_index + 2],
                };
            }

            if (index.normal_index >= 0)
            {
                vertex.Normal =
                {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2],
                };
            }

            if (index.texcoord_index >= 0)
            {
                vertex.UV =
                {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1],
                };
            }

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(Vertices.size());
                Vertices.push_back(vertex);
            }
            Indices.push_back(uniqueVertices[vertex]);
        }
    }

    ComputeTangentBasis(Vertices, Indices);
}

void VulkanModel::Builder::ComputeTangentBasis(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        Vertex& v0 = vertices[indices[i]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        // Edge vectors
        glm::vec3 edge1 = v1.Position - v0.Position;
        glm::vec3 edge2 = v2.Position - v0.Position;

        // UV delta
        glm::vec2 deltaUV1 = v1.UV - v0.UV;
        glm::vec2 deltaUV2 = v2.UV - v0.UV;

        float f = 1.0f / glm::max(0.001f, (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y));

        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        v0.Tangent += tangent;
        v1.Tangent += tangent;
        v2.Tangent += tangent;
    }

    // Normalize and orthogonalize
    for (auto& vertex : vertices)
    {
        vertex.Tangent = glm::normalize(vertex.Tangent - vertex.Normal * glm::dot(vertex.Normal, vertex.Tangent));
    }
}

VulkanModel::VulkanModel(const Builder& builder)
{
    CreateVertexBuffer(builder.Vertices);
    CreateIndexBuffer(builder.Indices);
}

std::shared_ptr<VulkanModel> VulkanModel::CreateModelFromFile(const std::string &filePath)
{
    Builder builder{};
    builder.LoadModel(filePath);
    return std::make_shared<VulkanModel>(builder);
}


void VulkanModel::CreateVertexBuffer(const std::vector<Vertex> &vertices)
{
    m_VertexCount = static_cast<uint32_t>(vertices.size());

    assert(m_VertexCount >= 3 && "vertex count must be at least 3");

    VkDeviceSize bufferSize = sizeof(vertices[0]) * m_VertexCount;
    uint32_t vertexSize = sizeof(vertices[0]);

    VulkanBuffer stagingBuffer{
        vertexSize,
        m_VertexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    stagingBuffer.Map();
    stagingBuffer.WriteToBuffer(vertices.data());

    m_VertexBuffer = std::make_unique<VulkanBuffer>(
        vertexSize,
        m_VertexCount,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

    VulkanBuffer::CopyBuffer(
            stagingBuffer.GetBuffer(),
            m_VertexBuffer->GetBuffer(),
            bufferSize);
}

void VulkanModel::CreateIndexBuffer(const std::vector<uint32_t>& indices)
{
    m_IndexCount = static_cast<uint32_t>(indices.size());
    m_HasIndexBuffer = m_IndexCount > 0;
    if(!m_HasIndexBuffer) return;

    VkDeviceSize bufferSize = sizeof(indices[0]) * m_IndexCount;
    uint32_t indexSize = sizeof(indices[0]);

    VulkanBuffer stagingBuffer{
        indexSize,
        m_IndexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    stagingBuffer.Map();
    stagingBuffer.WriteToBuffer(indices.data());

    m_IndexBuffer = std::make_unique<VulkanBuffer>(
        indexSize,
        m_IndexCount,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

    VulkanBuffer::CopyBuffer(
            stagingBuffer.GetBuffer(),
            m_IndexBuffer->GetBuffer(),
            bufferSize);
}

void VulkanModel::Draw(VkCommandBuffer commandBuffer) const
{
    if(m_HasIndexBuffer)
    {
        vkCmdDrawIndexed(commandBuffer, m_IndexCount, 1, 0, 0, 0);
    }
    else
    {
        vkCmdDraw(commandBuffer,  m_VertexCount, 1, 0, 0);
    }
}

void VulkanModel::BindVertexInput(VkCommandBuffer commandBuffer)
{
    VkBuffer buffers[] = {m_VertexBuffer->GetBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
    if(m_HasIndexBuffer)
    {
        vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }
}

std::vector<VkVertexInputBindingDescription> VulkanModel::Vertex::GetBindingDescriptions()
{
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> VulkanModel::Vertex::GetAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

    attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Position)});
    attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Color)});
    attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Normal)});
    attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Tangent)});
    attributeDescriptions.push_back({4, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV)});

    return attributeDescriptions;
}
