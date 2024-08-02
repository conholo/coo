#pragma once

#include "vulkan_render_pass.h"
#include "vulkan_framebuffer.h"
#include "vulkan_graphics_pipeline.h"
#include "vulkan_image.h"
#include "vulkan_material.h"

class VulkanRenderer;
class VulkanDeferredRenderer
{
public:
    explicit VulkanDeferredRenderer(VulkanRenderer* renderer);
    ~VulkanDeferredRenderer();

    void Initialize();

    void Render(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t imageIndex);
    void Resize(uint32_t width, uint32_t height);

private:

    void CreateGBufferResources();
    void CreateGBufferTextures();
    void CreateGBufferRenderPass();
    void CreateGBufferPipeline();

    void CreatePipelines();

    void CreateFramebuffers();

private:
    VulkanRenderer *m_Renderer;

    std::unique_ptr<VulkanRenderPass> m_GBufferPass;
    std::unique_ptr<VulkanRenderPass> m_LightingPass;
    std::unique_ptr<VulkanRenderPass> m_CompositionPass;

    std::vector<std::unique_ptr<VulkanImage2D>> m_GBufferTextures;
    std::unique_ptr<VulkanImage2D> m_LightingTexture;

    std::vector<std::unique_ptr<VulkanFramebuffer>> m_GBufferFramebuffers;
    std::vector<std::unique_ptr<VulkanFramebuffer>> m_LightingFramebuffers;
    std::vector<std::unique_ptr<VulkanFramebuffer>> m_CompositionFramebuffers;

    std::unique_ptr<VulkanGraphicsPipeline> m_GBufferPipeline;
    std::unique_ptr<VulkanGraphicsPipeline> m_LightingPipeline;
    std::unique_ptr<VulkanGraphicsPipeline> m_CompositionPipeline;

    std::shared_ptr<VulkanShader> m_GBufferVertexShader;
    std::shared_ptr<VulkanShader> m_GBufferFragmentShader;

    std::shared_ptr<VulkanShader> m_FullScreenQuadVertexShader;
    std::shared_ptr<VulkanShader> m_LightingFragmentShader;
    std::shared_ptr<VulkanShader> m_CompositionFragmentShader;

    std::shared_ptr<VulkanMaterial> m_GBufferMaterial;
    std::shared_ptr<VulkanMaterial> m_LightingMaterial;
    std::shared_ptr<VulkanMaterial> m_CompositionMaterial;
};