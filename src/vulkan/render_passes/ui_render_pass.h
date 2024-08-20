#pragma once

#include "vulkan/irenderer.h"
#include "render_pass.h"

class UIRenderPass : public RenderPass
{
public:
	void CreateResources(RenderGraph& graph) override;
	void Record(const FrameInfo& frameInfo, RenderGraph& graph) override;
	void Submit(const FrameInfo& frameInfo, RenderGraph& graph) override {}

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
	ResourceHandle<GraphicsPipelineObjectResource> m_PipelineHandle{};
	ResourceHandle<MaterialLayoutResource> m_MaterialLayoutHandle;
	ResourceHandle<MaterialResource> m_MaterialHandle;
	ResourceHandle<ShaderResource> m_VertexHandle;
	ResourceHandle<ShaderResource> m_FragmentHandle;

	ResourceHandle<BufferResource> m_VertexBufferHandle;
	ResourceHandle<BufferResource> m_IndexBufferHandle;
};