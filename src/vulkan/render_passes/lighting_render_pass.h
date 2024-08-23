#pragma once

#include "vulkan/irenderer.h"
#include "render_pass.h"

class LightingPass : public RenderPass
{
public:
	LightingPass() = default;
	void CreateResources(RenderGraph& graph) override;
	void Record(const FrameInfo& frameInfo, RenderGraph& graph) override;
	void Submit(const FrameInfo& frameInfo, RenderGraph& graph) override;
	void OnSwapchainResize(uint32_t width, uint32_t height, RenderGraph& graph) override;

private:
	void CreateCommandBuffers(RenderGraph& graph);
	void CreateSynchronizationPrimitives(RenderGraph& graph);
	void CreateTextures(RenderGraph& graph);
	void CreateRenderPass(RenderGraph& graph);
	void CreateShaders(RenderGraph& graph);
	void CreateMaterialLayout(RenderGraph& graph);
	void CreateMaterial(RenderGraph& graph);

	void CreateGraphicsPipeline(RenderGraph& graph);
	void CreateFramebuffers(RenderGraph& graph);

private:

	std::vector<ResourceHandle<CommandBufferResource>> m_CommandBufferHandles{};
	std::vector<ResourceHandle<SemaphoreResource>> m_RenderCompleteSemaphoreHandles{};
	std::vector<ResourceHandle<FenceResource>> m_ResourcesInFlightFenceHandles{};

	std::vector<ResourceHandle<TextureResource>> m_ColorAttachmentHandles{};
	std::vector<ResourceHandle<FramebufferResource>> m_FramebufferHandles{};

	ResourceHandle<GraphicsPipelineObjectResource> m_PipelineHandle{};
	ResourceHandle<RenderPassObjectResource> m_RenderPassHandle{};
	ResourceHandle<MaterialLayoutResource> m_MaterialLayoutHandle{};
	ResourceHandle<MaterialResource> m_MaterialHandle{};
	ResourceHandle<ShaderResource> m_FragmentHandle{};
};