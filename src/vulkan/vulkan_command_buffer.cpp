#include "vulkan_command_buffer.h"

#include "vulkan_context.h"
#include "vulkan_utils.h"

#include <memory>
#include <utility>

VulkanCommandBuffer::VulkanCommandBuffer(VkCommandPool commandPool, bool isPrimary, std::string debugName)
	:  m_CommandPool(commandPool), m_State(State::Initial), m_DebugName(std::move(debugName))
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	allocInfo.commandBufferCount = 1;

	VK_CHECK_RESULT(vkAllocateCommandBuffers(VulkanContext::Get().Device(), &allocInfo, &m_CommandBuffer));
	SetDebugUtilsObjectName(VulkanContext::Get().Device(), VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t )m_CommandBuffer, debugName.c_str());
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
	if (m_CommandBuffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(VulkanContext::Get().Device(), m_CommandPool, 1, &m_CommandBuffer);
		m_CommandBuffer = VK_NULL_HANDLE;
	}
}

void VulkanCommandBuffer::Begin(VkCommandBufferUsageFlags flags)
{
	if (m_State != State::Initial)
		throw std::runtime_error("Attempting to begin a command buffer in invalid state");

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = flags;

	VK_CHECK_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));
	m_State = State::Recording;
}

void VulkanCommandBuffer::End()
{
	if (m_State != State::Recording)
		throw std::runtime_error("Attempting to end a command buffer that is not in recording state");

	VK_CHECK_RESULT(vkEndCommandBuffer(m_CommandBuffer));
	m_State = State::Executable;
}

void VulkanCommandBuffer::Reset()
{
	if (m_State == State::Pending)
		throw std::runtime_error("Attempting to reset a command buffer that is pending execution");

	VK_CHECK_RESULT(vkResetCommandBuffer(m_CommandBuffer, 0));
	m_State = State::Initial;
}

void VulkanCommandBuffer::Submit(
	VkQueue queue,
	const std::vector<VulkanCommandBuffer*>& commandBuffers,
	const std::vector<VkSemaphore>& waitSemaphores,
	const std::vector<VkPipelineStageFlags>& waitStages,
	const std::vector<VkSemaphore>& signalSemaphores,
	VkFence fence)
{
	if (commandBuffers.empty())
		throw std::runtime_error("No command buffers provided for submission");

	if(fence != VK_NULL_HANDLE)
		VK_CHECK_RESULT(vkResetFences(VulkanContext::Get().Device(), 1, &fence));

	std::vector<VkCommandBuffer> vkCommandBuffers;
	vkCommandBuffers.reserve(commandBuffers.size());

	for (const auto& cb : commandBuffers)
	{
		if (cb->GetState() != State::Executable)
			throw std::runtime_error("Attempting to submit a command buffer that is not executable");
		vkCommandBuffers.push_back(cb->GetHandle());
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.data();

	submitInfo.commandBufferCount = static_cast<uint32_t>(vkCommandBuffers.size());
	submitInfo.pCommandBuffers = vkCommandBuffers.data();

	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	submitInfo.pSignalSemaphores = signalSemaphores.data();

	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));

	for (auto& cb : commandBuffers)
		cb->m_State = State::Pending;
}

void VulkanCommandBuffer::WaitForCompletion(VkFence fence)
{
	if(m_State == State::Initial)
	{
		// Already waited on.
		return;
	}

	// Wait for the fence to be signaled, indicating the GPU has finished execution
	vkWaitForFences(VulkanContext::Get().Device(), 1, &fence, VK_TRUE, UINT64_MAX);

	// After the fence is signaled, we can safely reset the state
	m_State = State::Initial;
}

void VulkanCommandBuffer::ResetCommandBuffers(const std::vector<std::unique_ptr<VulkanCommandBuffer>>& commandBuffers)
{
	for (auto& cb : commandBuffers)
	{
		if (cb->m_State == State::Pending)
			throw std::runtime_error("Attempting to reset a command buffer that is pending execution");

		VK_CHECK_RESULT(vkResetCommandBuffer(cb->m_CommandBuffer, 0));
		cb->m_State = State::Initial;
	}
}
