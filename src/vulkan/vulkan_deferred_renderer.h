#pragma once

#include "core/frame_info.h"
#include "irenderer.h"

class VulkanMaterial;
class VulkanRenderPass;
class VulkanTexture2D;
class VulkanImage2D;
class VulkanRenderer;
class VulkanGraphicsPipeline;
class VulkanFramebuffer;
class VulkanCommandBuffer;

class VulkanDeferredRenderer : public IRenderer
{
public:
    explicit VulkanDeferredRenderer(VulkanRenderer* renderer);
    ~VulkanDeferredRenderer() override = default;

    void Initialize() override;
    void Shutdown() override;
    void Render(FrameInfo& frameInfo) override;
    void Resize(uint32_t width, uint32_t height) override;
    void RegisterGameObject(GameObject& gameObjectRef) override;
	void OnEvent(Event& event) override;
	VulkanRenderPass& GetRenderFinishedRenderPass() override { return *m_CompositionPass; }
	VulkanFramebuffer& GetRenderFinishedFramebuffer(uint32_t frameIndex) override { return *m_CompositionFramebuffers[frameIndex]; }
	VkSemaphore GetRendererFinishedSemaphore(uint32_t frameIndex) override { return m_LightingCompleteSemaphores[frameIndex]; }

private:
	void RecordGBufferCommandBuffer(FrameInfo& frameInfo);
	void RecordLightingPassCommandBuffer(FrameInfo& frameInfo);
	void RecordCompositionPassCommandBuffer(FrameInfo& frameInfo);
	void SubmitRenderPasses(uint32_t frameIndex);

    void InvalidateGBufferPass();
    void CreateGBufferTextures();
    void CreateGBufferRenderPass();
    void CreateGBufferPipeline();
    void CreateGBufferFramebuffers();

    void InvalidateLightingPass();
    void CreateLightingTextures();
    void CreateLightingRenderPass();
    void CreateLightingPipeline();
    void CreateLightingFramebuffers();

    void InvalidateCompositionPass();
    void CreateCompositionRenderPass();
    void CreateCompositionPipeline();
    void CreateCompositionFramebuffers();

	void CreateCommandBuffers();
	void CreateSynchronizationPrimitives();
	void CreateShaders();
	void CreateMaterials();
	void CreateAttachmentTextures();

private:
    VulkanRenderer *m_Renderer;

    /*
     * Per Frame Resources - Double/Triple Buffered
     */
    std::vector<std::vector<std::shared_ptr<VulkanTexture2D>>> m_GBufferTextures{VulkanSwapchain::MAX_FRAMES_IN_FLIGHT};
    std::vector<std::shared_ptr<VulkanTexture2D>> m_LightingTextures{VulkanSwapchain::MAX_FRAMES_IN_FLIGHT};

	std::vector<std::unique_ptr<VulkanCommandBuffer>> m_GBufferCommandBuffers;
	std::vector<std::unique_ptr<VulkanCommandBuffer>> m_LightingCommandBuffers;

	std::vector<VkSemaphore> m_GBufferCompleteSemaphores;
	std::vector<VkSemaphore> m_LightingCompleteSemaphores;

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
};