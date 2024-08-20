#pragma once

#include "vulkan/irenderer.h"
#include "render_pass.h"

class SceneCompositionPass : public RenderPass
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
	std::vector<ResourceHandle<SemaphoreResource>> m_RenderCompleteSemaphoreHandles;
	std::vector<ResourceHandle<FenceResource>> m_FenceHandles;

	ResourceHandle<GraphicsPipelineObjectResource> m_PipelineHandle{};
	ResourceHandle<MaterialLayoutResource> m_MaterialLayoutHandle;
	ResourceHandle<MaterialResource> m_MaterialHandle;
	ResourceHandle<ShaderResource> m_FragmentHandle;
};