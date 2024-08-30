#pragma once

#include "vulkan/irenderer.h"
#include "render_pass.h"

class ResourceHandle;
class ResourceHandle<TextureResource>;

class UIRenderPass : public RenderPass
{
public:
	void CreateResources(RenderGraph& graph) override;
	void Record(const FrameInfo& frameInfo, RenderGraph& graph) override;
	void Submit(const FrameInfo& frameInfo, RenderGraph& graph) override {}
	void OnSwapchainResize(uint32_t width, uint32_t height, RenderGraph& graph) override;;
	void DeclareDependencies(const std::initializer_list<std::string>& readResources, const std::initializer_list<std::string>& writeResources) override;
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