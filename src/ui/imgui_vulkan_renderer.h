#pragma once

#include <memory>
#include <vector>

#include "core/event/event.h"
#include <vulkan/vulkan.h>

class VulkanRenderPass;
class VulkanFramebuffer;
class VulkanGraphicsPipeline;
class VulkanSwapchainRenderer;
class Window;

class VulkanImGuiRenderer
{
public:
	VulkanImGuiRenderer() = default;
	~VulkanImGuiRenderer() = default;

	void Initialize(RenderGraph& graph);
	void Shutdown(RenderGraph& graph);

	void Begin();
	void End(RenderGraph& graph, uint32_t frameIndex);

	void OnEvent(Event& e) const;
	void BlockEvents(bool block) { m_BlockEvents = block; }

private:
	static void SetDarkThemeColors();

private:
	struct DisplayTransformPushConstants
	{
		glm::vec2 Scale{};
		glm::vec2 Translate{};
	} m_TransformPushConstants{};

	ResourceHandle<TextureResource> m_FontTextureHandle;
	std::unique_ptr<Buffer> m_FontMemoryBuffer;
	std::vector<ResourceHandle<BufferResource>> m_VertexBufferHandles;
	std::vector<ResourceHandle<BufferResource>> m_IndexBufferHandles;
	std::unique_ptr<VulkanDescriptorPool> m_DescriptorPool;
	std::unique_ptr<VulkanDescriptorSetLayout> m_SetLayout;
	VkDescriptorSet m_FontDescriptorSet;
	size_t m_VertexCount{};
	size_t m_IndexCount{};

	bool m_BlockEvents = true;
};