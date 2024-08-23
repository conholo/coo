#pragma once

#include "render_pass.h"
#include "ui_render_pass.h"
#include "scene_composition_pass.h"

class SwapchainPass : public RenderPass
{
public:
	SwapchainPass() = default;
	void CreateResources(RenderGraph& graph) override;
	void Record(const FrameInfo& frameInfo, RenderGraph& graph) override;
	void Submit(const FrameInfo& frameInfo, RenderGraph& graph) override;
	void OnSwapchainResize(uint32_t width, uint32_t height, RenderGraph& graph) override;

private:
	void CreateCommandBuffers(RenderGraph& graph);
	void CreateSynchronizationPrimitives(RenderGraph& graph);
	void CreateRenderPass(RenderGraph& graph);
	void CreateFramebuffers(RenderGraph& graph);

private:
	std::vector<ResourceHandle<CommandBufferResource>> m_CommandBufferHandles{};
	std::vector<ResourceHandle<FenceResource>> m_ImagesInFlightHandles{};
	std::vector<ResourceHandle<FenceResource>> m_ResourcesInFlightFenceHandles{};
	std::vector<ResourceHandle<SemaphoreResource>> m_ImageAvailableSemaphoreHandles{};
	std::vector<ResourceHandle<SemaphoreResource>> m_RenderCompleteSemaphoreHandles{};

	std::vector<ResourceHandle<FramebufferResource>> m_FramebufferHandles{};
	ResourceHandle<RenderPassObjectResource> m_RenderPassHandle{};

	UIRenderPass m_UIPass{};
};