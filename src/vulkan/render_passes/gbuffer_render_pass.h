#pragma once

#include "render_pass.h"
#include "render_pass_resources/texture_resource.h"
#include "render_pass_resources/fence_resource.h"
#include "render_pass_resources/semaphore_resource.h"
#include "render_pass_resources/command_buffer_resource.h"
#include "render_pass_resources/framebuffer_resource.h"
#include "render_pass_resources/graphics_pipeline_resource.h"
#include "render_pass_resources/material_layout_resource.h"
#include "render_pass_resources/material_resource.h"
#include "render_pass_resources/render_pass_object_resource.h"
#include "render_pass_resources/shader_resource.h"
#include "vulkan/irenderer.h"

class GBufferPass : public RenderPass
{
public:

	GBufferPass() = default;
	~GBufferPass() override = default;

	void DeclareDependencies(const std::initializer_list<std::string>& readResources, const std::initializer_list<std::string>& writeResources) override;
	void CreateResources(RenderGraph& graph) override;
	void Record(const FrameInfo& frameInfo, RenderGraph& graph) override;
	void Submit(const FrameInfo& frameInfo, RenderGraph& graph) override;
	void OnSwapchainResize(uint32_t graphics, uint32_t height, RenderGraph& graph) override;

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

	std::vector<ResourceHandle<TextureResource>> m_PositionTextureHandles{};
	std::vector<ResourceHandle<TextureResource>> m_NormalTextureHandles{};
	std::vector<ResourceHandle<TextureResource>> m_AlbedoTextureHandles{};
	std::vector<ResourceHandle<TextureResource>> m_DepthTextureHandles{};

	std::vector<ResourceHandle<FramebufferResource>> m_FramebufferHandles{};

	ResourceHandle<GraphicsPipelineResource> m_PipelineHandle{};
	ResourceHandle<RenderPassObjectResource> m_RenderPassHandle{};
	ResourceHandle<MaterialLayoutResource> m_MaterialLayoutHandle{};
	ResourceHandle<MaterialResource> m_MaterialHandle{};
	ResourceHandle<ShaderResource> m_VertexHandle{};
	ResourceHandle<ShaderResource> m_FragmentHandle{};
};