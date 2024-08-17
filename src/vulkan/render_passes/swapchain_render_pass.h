#pragma once

#include "vulkan/irenderer.h"

class SwapchainPass : public BaseRenderPass
{
public:
	void Initialize() override;
	void Invalidate() override;
	void Record(const FrameInfo& frameInfo) override;
	void Submit(uint32_t frameIndex) override;
	void Cleanup() override;

private:
	void CreateCommandBuffers();
	void CreateSynchronizationPrimitives();
	void CreateTextures();
	void CreateGraphicsPipeline();
	void CreateRenderPass();
	void CreateFramebuffers();

private:
	std::vector<std::unordered_map<std::string, std::shared_ptr<RenderPassResource>>> m_Inputs;
	std::vector<std::unordered_map<std::string, std::shared_ptr<RenderPassResource>>> m_Outputs;

	std::vector<std::shared_ptr<VulkanSwapchain>> m_SwapchainResource;
	std::vector<std::shared_ptr<TextureResource>> m_PositionTextures;
	std::vector<std::shared_ptr<TextureResource>> m_NormalTextures;
	std::vector<std::shared_ptr<TextureResource>> m_AlbedoTextures;
	std::vector<std::shared_ptr<TextureResource>> m_DepthTextures;

	std::vector<std::shared_ptr<CommandBufferResource>> m_CommandBuffers;

	std::vector<std::shared_ptr<SemaphoreResource>> m_PassCompleteSemaphores;
	std::vector<std::shared_ptr<FramebufferResource>> m_Framebuffers;

	std::shared_ptr<RenderPassObjectResource> m_RenderPass;
	std::shared_ptr<GraphicsPipelineObjectResource> m_Pipeline;

	std::shared_ptr<VulkanShader> m_VertexShader;
	std::shared_ptr<VulkanShader> m_FragmentShader;

	std::shared_ptr<VulkanMaterialLayout> m_MaterialLayout;
	std::shared_ptr<VulkanMaterial> m_BaseMaterial;
};