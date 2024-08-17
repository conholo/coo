#pragma once

#include "render_pass.h"
#include "vulkan/irenderer.h"

class GBufferPass : public RenderPass
{
public:

	GBufferPass();
	void CreateResources(RenderGraph& graph) override;
	void SetDependencies(RenderGraph& graph) override;
	void Resize(RenderGraph& graph, uint32_t width, uint32_t height) override;
	void RecordCommandBuffer(RenderGraph& graph, VkCommandBuffer cmd, uint32_t frameIndex) override;
	void Cleanup(RenderGraph& graph) override;

private:
	void CreateCommandBuffers();
	void CreateSynchronizationPrimitives();
	void CreateTextures();
	void CreateGraphicsPipeline();
	void CreateRenderPass();
	void CreateFramebuffers();

private:
	Dependencies m_Dependencies;
	ResourceHandles m_Handles;
};