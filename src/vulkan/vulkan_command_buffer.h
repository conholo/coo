#pragma once

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>
#include <memory>

class VulkanCommandBuffer
{
public:
	enum class State
	{
		Initial,
		Recording,
		Executable,
		Pending,
		Invalid
	};

	explicit VulkanCommandBuffer(VkCommandPool commandPool, bool isPrimary = true, std::string debugName = "Command Buffer");
	~VulkanCommandBuffer();

	// Disable copying
	VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
	VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;

	void Begin(VkCommandBufferUsageFlags flags = 0);
	void End();

	void Reset();
	void WaitForCompletion(VkFence fence);

	static void Submit(VkQueue queue,
		const std::vector<VulkanCommandBuffer*>& commandBuffers,
		const std::vector<VkSemaphore>& waitSemaphores,
		const std::vector<VkPipelineStageFlags>& waitStages,
		const std::vector<VkSemaphore>& signalSemaphores,
		VkFence fence = VK_NULL_HANDLE);
	static void ResetCommandBuffers(const std::vector<std::unique_ptr<VulkanCommandBuffer>>& commandBuffers);
	VkResult InterruptAndReset(VkFence fence, bool recreate = false);

	bool IsRecording() const { return m_State == State::Recording; }
	VkCommandBuffer GetHandle() const { return m_CommandBuffer; }
	State GetState() const { return m_State; }

private:
	VkCommandPool m_CommandPool{};
	VkCommandBuffer m_CommandBuffer{};
	State m_State;
	std::string m_DebugName;
};