#pragma once

#include "core/frame_info.h"
#include "irenderer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_graphics_pipeline.h"
#include "vulkan_image.h"
#include "vulkan_material.h"
#include "vulkan_render_pass.h"
#include "vulkan_texture.h"

class VulkanRenderer;
class VulkanDeferredRenderer : public IRenderer
{
public:
    explicit VulkanDeferredRenderer(VulkanRenderer* renderer);
    ~VulkanDeferredRenderer() override;

    void Initialize() override;
    void Shutdown() override;
    void Render(FrameInfo& frameInfo) override;
    void Resize(uint32_t width, uint32_t height) override;
    void RegisterGameObject(GameObject& gameObjectRef) override;

private:

	void RecordGBufferCommandBuffer(FrameInfo& frameInfo);
	void RecordLightingPassCommandBuffer(FrameInfo& frameInfo);
	void RecordCompositionPassCommandBuffer(FrameInfo& frameInfo);
	void SubmitRenderPasses(uint32_t frameIndex);

    void CreateGBufferResources();
    void CreateGBufferTextures();
    void CreateGBufferRenderPass();
    void CreateGBufferPipeline();
    void CreateGBufferFramebuffers();

    void CreateLightingResources();
    void CreateLightingTextures();
    void CreateLightingRenderPass();
    void CreateLightingPipeline();
    void CreateLightingFramebuffers();

    void CreateCompositionResources();
    void CreateCompositionRenderPass();
    void CreateCompositionPipeline();
    void CreateCompositionFramebuffers();

	void CreateCommandBuffers();
	void CreateSynchronizationPrimitives();

private:
    VulkanRenderer *m_Renderer;

    /*
     * Per Frame Resources - Double/Triple Buffered
     */
    std::vector<std::vector<std::shared_ptr<VulkanTexture2D>>> m_GBufferTextures{VulkanSwapchain::MAX_FRAMES_IN_FLIGHT};
    std::vector<std::shared_ptr<VulkanTexture2D>> m_LightingTextures{VulkanSwapchain::MAX_FRAMES_IN_FLIGHT};

	std::vector<VkCommandBuffer> m_GBufferCommandBuffers;
	std::vector<VkCommandBuffer> m_LightingCommandBuffers;

	std::vector<VkSemaphore> m_GBufferCompleteSemaphores;
	std::vector<VkSemaphore> m_LightingCompleteSemaphores;
	std::vector<VkSemaphore> m_CompositionRenderCompleteSemaphores;
	std::vector<VkFence> m_GBufferCompleteFence;

    // These framebuffers will be resized on creation or resize.
    std::vector<std::unique_ptr<VulkanFramebuffer>> m_GBufferFramebuffers;
    std::vector<std::unique_ptr<VulkanFramebuffer>> m_LightingFramebuffers;
    /*
     * Needs to be allocated based on the number swapchain images - NOT the number of frames in flight
     */
    std::vector<std::unique_ptr<VulkanFramebuffer>> m_CompositionFramebuffers;

    /*
     * Single Use Resources
    */
    std::unique_ptr<VulkanRenderPass> m_GBufferPass;
    std::unique_ptr<VulkanRenderPass> m_LightingPass;
    std::unique_ptr<VulkanRenderPass> m_CompositionPass;

    std::unique_ptr<VulkanGraphicsPipeline> m_GBufferPipeline;
    std::unique_ptr<VulkanGraphicsPipeline> m_LightingPipeline;
    std::unique_ptr<VulkanGraphicsPipeline> m_CompositionPipeline;

    std::shared_ptr<VulkanShader> m_GBufferVertexShader;
    std::shared_ptr<VulkanShader> m_GBufferFragmentShader;

    std::shared_ptr<VulkanShader> m_FullScreenQuadVertexShader;
    std::shared_ptr<VulkanShader> m_LightingFragmentShader;
    std::shared_ptr<VulkanShader> m_CompositionFragmentShader;

    std::shared_ptr<VulkanMaterialLayout> m_GBufferMaterialLayout;
    std::shared_ptr<VulkanMaterial> m_GBufferBaseMaterial;

    std::shared_ptr<VulkanMaterialLayout> m_LightingMaterialLayout;
    std::shared_ptr<VulkanMaterial> m_LightingMaterial;

    std::shared_ptr<VulkanMaterialLayout> m_CompositionMaterialLayout;
    std::shared_ptr<VulkanMaterial> m_CompositionMaterial;

	std::shared_ptr<VulkanTexture2D> m_SimpleTextureA;
};