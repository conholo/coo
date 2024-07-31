#include "vulkan_buffer.h"
#include "vulkan_context.h"

#include <cassert>
#include <ostream>

void VulkanBuffer::CreateVkBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(VulkanContext::Get().Device(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to create vertex buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(VulkanContext::Get().Device(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = VulkanContext::Get().FindDeviceMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(VulkanContext::Get().Device(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate vertex buffer memory!");

    vkBindBufferMemory(VulkanContext::Get().Device(), buffer, bufferMemory, 0);
}

void VulkanBuffer::CopyBufferToImage(
        VkBuffer buffer,
        VkImage image,
        uint32_t width, uint32_t height,
        uint32_t layerCount)
{
    VkCommandBuffer commandBuffer = VulkanContext::Get().BeginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layerCount;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

    VulkanContext::Get().EndSingleTimeCommand(commandBuffer);
}


void VulkanBuffer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = VulkanContext::Get().BeginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    VulkanContext::Get().EndSingleTimeCommand(commandBuffer);
}

VkDeviceSize VulkanBuffer::GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment)
{
    if (minOffsetAlignment > 0)
    {
        return instanceSize + minOffsetAlignment - 1 & ~(minOffsetAlignment - 1);
    }
    return instanceSize;
}

VulkanBuffer::VulkanBuffer(
    VkDeviceSize instanceSize,
    uint32_t instanceCount,
    VkBufferUsageFlags usageFlags,
    VkMemoryPropertyFlags memoryPropertyFlags,
    VkDeviceSize minOffsetAlignment)
    : m_InstanceCount{instanceCount},
      m_InstanceSize{instanceSize},
      m_UsageFlags{usageFlags},
      m_MemoryPropertyFlags{memoryPropertyFlags}
{
    m_AlignmentSize = GetAlignment(instanceSize, minOffsetAlignment);
    m_BufferSize = m_AlignmentSize * instanceCount;
    CreateVkBuffer(m_BufferSize, usageFlags, memoryPropertyFlags, m_Buffer, m_Memory);
}

VulkanBuffer::~VulkanBuffer()
{
    Unmap();
    vkDestroyBuffer(VulkanContext::Get().Device(), m_Buffer, nullptr);
    vkFreeMemory(VulkanContext::Get().Device(), m_Memory, nullptr);
}

/**
 * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
 *
 * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
 * buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the buffer mapping call
 */
VkResult VulkanBuffer::Map(VkDeviceSize size, VkDeviceSize offset)
{
    assert(m_Buffer && m_Memory && "Called map on buffer before create");
    return vkMapMemory(VulkanContext::Get().Device(), m_Memory, offset, size, 0, &m_Mapped);
}

/**
 * Unmap a mapped memory range
 *
 * @note Does not return a result as vkUnmapMemory can't fail
 */
void VulkanBuffer::Unmap()
{
    if (m_Mapped)
    {
        vkUnmapMemory(VulkanContext::Get().Device(), m_Memory);
        m_Mapped = nullptr;
    }
}

/**
 * Copies the specified data to the mapped buffer. Default value writes whole buffer range
 *
 * @param data Pointer to the data to copy
 * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
 * range.
 * @param offset (Optional) Byte offset from beginning of mapped region
 *
 */
void VulkanBuffer::WriteToBuffer(const void* data, VkDeviceSize size, VkDeviceSize offset) const
{
    assert(m_Mapped && "Cannot copy to unmapped buffer");

    if (size == VK_WHOLE_SIZE)
    {
        memcpy(m_Mapped, data, m_BufferSize);
    }
    else
    {
        auto memOffset = static_cast<char *>(m_Mapped);
        memOffset += offset;
        memcpy(memOffset, data, size);
    }
}

/**
 * Flush a memory range of the buffer to make it visible to the device
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
 * complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the flush call
 */
VkResult VulkanBuffer::Flush(VkDeviceSize size, VkDeviceSize offset) const
{
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = m_Memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    return vkFlushMappedMemoryRanges(VulkanContext::Get().Device(), 1, &mappedRange);
}

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
 * the complete buffer range.
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkResult of the invalidate call
 */
VkResult VulkanBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset)
{
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = m_Memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    return vkInvalidateMappedMemoryRanges(VulkanContext::Get().Device(), 1, &mappedRange);
}

/**
 * Create a buffer info descriptor
 *
 * @param size (Optional) Size of the memory range of the descriptor
 * @param offset (Optional) Byte offset from beginning
 *
 * @return VkDescriptorBufferInfo of specified offset and range
 */
VkDescriptorBufferInfo VulkanBuffer::DescriptorInfo(VkDeviceSize size, VkDeviceSize offset) const
{
    return VkDescriptorBufferInfo{m_Buffer, offset, size};
}

/**
 * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
 *
 * @param data Pointer to the data to copy
 * @param index Used in offset calculation
 *
 */
void VulkanBuffer::WriteToIndex(void* data, int index) const
{
    WriteToBuffer(data, m_InstanceSize, index * m_AlignmentSize);
}

/**
 *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
 *
 * @param index Used in offset calculation
 *
 */
VkResult VulkanBuffer::FlushIndex(int index) const
{
    assert(m_AlignmentSize % VulkanContext::Get().PhysicalDeviceProperties().limits.nonCoherentAtomSize == 0 &&
        "Cannot use VulkanBuffer::FlushIndex if alignmentSize isn't a multiple of Device Limits "
        "nonCoherentAtomSize");
    return Flush(m_AlignmentSize, index * m_AlignmentSize);
}

/**
 * Create a buffer info descriptor
 *
 * @param index Specifies the region given by index * alignmentSize
 *
 * @return VkDescriptorBufferInfo for instance at index
 */
VkDescriptorBufferInfo VulkanBuffer::DescriptorInfoForIndex(int index) const
{
    return DescriptorInfo(m_AlignmentSize, index * m_AlignmentSize);
}

/**
 * Invalidate a memory range of the buffer to make it visible to the host
 *
 * @note Only required for non-coherent memory
 *
 * @param index Specifies the region to invalidate: index * alignmentSize
 *
 * @return VkResult of the invalidate call
 */
VkResult VulkanBuffer::InvalidateIndex(int index)
{
    return Invalidate(m_AlignmentSize, index * m_AlignmentSize);
}
