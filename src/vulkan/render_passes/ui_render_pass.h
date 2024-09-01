#pragma once

#include "vulkan/irenderer.h"
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

class UIRenderPass : public RenderPass
{
public:
	void CreateResources(RenderGraph& graph) override;
	void Record(const FrameInfo& frameInfo, RenderGraph& graph) override;
	void Submit(const FrameInfo& frameInfo, RenderGraph& graph) override {}
	void OnSwapchainResize(uint32_t width, uint32_t height, RenderGraph& graph) override;
	
private:
	void CreateShaders(RenderGraph& graph);
	void CreateMaterialLayout(RenderGraph& graph);
	void CreateMaterial(RenderGraph& graph);
	void CreateGraphicsPipeline(RenderGraph& graph);

private:
	struct DisplayTransformPushConstants
	{
		glm::vec2 Scale{};
		glm::vec2 Translate{};
	} m_TransformPushConstants{};

	ResourceHandle<TextureResource> m_FontTextureHandle{};
	ResourceHandle<GraphicsPipelineResource> m_PipelineHandle{};
	ResourceHandle<MaterialLayoutResource> m_MaterialLayoutHandle;
	ResourceHandle<MaterialResource> m_MaterialHandle;
	ResourceHandle<ShaderResource> m_VertexHandle;
	ResourceHandle<ShaderResource> m_FragmentHandle;

	ResourceHandle<BufferResource> m_VertexBufferHandle;
	ResourceHandle<BufferResource> m_IndexBufferHandle;
};