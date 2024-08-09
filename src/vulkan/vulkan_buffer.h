#pragma once

#include <vulkan/vulkan.h>

class VulkanBuffer
{
public:
    VulkanBuffer(
        VkDeviceSize instanceSize,
        uint32_t instanceCount,
        VkBufferUsageFlags usageFlags,
        VkMemoryPropertyFlags memoryPropertyFlags,
        VkDeviceSize minOffsetAlignment = 1);

    ~VulkanBuffer();

    static void CreateVkBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer& buffer,
            VkDeviceMemory& bufferMemory);

    static void CopyBufferToImage(
            VkBuffer buffer,
            VkImage image,
            uint32_t width, uint32_t height,
            uint32_t layerCount);

    static void CopyBuffer(
            VkBuffer srcBuffer,
            VkBuffer dstBuffer,
            VkDeviceSize size);

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void Unmap();

    void WriteToBuffer(const void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
    VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
    VkDescriptorBufferInfo DescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
    VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    void WriteToIndex(void* data, int index) const;
    VkResult FlushIndex(int index) const;
    VkDescriptorBufferInfo DescriptorInfoForIndex(int index) const;
    VkResult InvalidateIndex(int index);

    VkBuffer GetBuffer() const { return m_Buffer; }
    void* GetMappedMemory() const { return m_Mapped; }
    uint32_t GetInstanceCount() const { return m_InstanceCount; }
    VkDeviceSize GetInstanceSize() const { return m_InstanceSize; }
    VkDeviceSize GetAlignmentSize() const { return m_InstanceSize; }
    VkBufferUsageFlags GetUsageFlags() const { return m_UsageFlags; }
    VkMemoryPropertyFlags GetMemoryPropertyFlags() const { return m_MemoryPropertyFlags; }
    VkDeviceSize GetBufferSize() const { return m_BufferSize; }

private:
    static VkDeviceSize GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

    void* m_Mapped = nullptr;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;

    VkDeviceSize m_BufferSize;
    uint32_t m_InstanceCount;
    VkDeviceSize m_InstanceSize;
    VkDeviceSize m_AlignmentSize;
    VkBufferUsageFlags m_UsageFlags;
    VkMemoryPropertyFlags m_MemoryPropertyFlags;
};
