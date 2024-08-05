#pragma once

#include "vulkan_render_pass.h"
#include "vulkan_framebuffer.h"
#include "vulkan_graphics_pipeline.h"
#include "vulkan_image.h"
#include "vulkan_material.h"
#include "core/frame_info.h"
#include "vulkan_texture.h"

class VulkanRenderer;
class VulkanDeferredRenderer
{
public:
    explicit VulkanDeferredRenderer(VulkanRenderer* renderer);
    ~VulkanDeferredRenderer();

    void Initialize();

    void Render(FrameInfo& frameInfo);
    void Resize(uint32_t width, uint32_t height);

    void RegisterGameObject(GameObject& gameObjectRef);

private:

    void CreateGBufferResources();
    void CreateGBufferTextures();
    void CreateGBufferRenderPass();
    void CreateGBufferPipeline();
    void CreateOrInvalidateGBufferFramebuffers();

    void CreateLightingResources();
    void CreateLightingTextures();
    void CreateLightingRenderPass();
    void CreateLightingPipeline();
    void CreateOrInvalidateLightingFramebuffers();

    void CreateCompositionResources();
    void CreateCompositionRenderPass();
    void CreateCompositionPipeline();
    void CreateOrInvalidateCompositionFramebuffers();

private:
    VulkanRenderer *m_Renderer;

    /*
     * Per Frame Resources - Double/Triple Buffered
     */
    std::vector<std::vector<std::shared_ptr<VulkanTexture2D>>> m_GBufferTextures{VulkanSwapchain::MAX_FRAMES_IN_FLIGHT};
    std::vector<std::shared_ptr<VulkanTexture2D>> m_LightingTextures{VulkanSwapchain::MAX_FRAMES_IN_FLIGHT};

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