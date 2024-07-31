#pragma once

#include "vulkan_renderer.h"
#include "vulkan_render_pass.h"
#include "vulkan_framebuffer.h"
#include "vulkan_graphics_pipeline.h"

class VulkanDeferredRenderer
{
public:
    explicit VulkanDeferredRenderer(VulkanRenderer* renderer);
    ~VulkanDeferredRenderer();

    void Initialize();

    void Render(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t imageIndex);
    void Resize(uint32_t width, uint32_t height);

private:
    VulkanRenderer *m_Renderer;

    std::unique_ptr<VulkanRenderPass> m_GBufferPass;
    std::unique_ptr<VulkanRenderPass> m_LightingPass;
    std::unique_ptr<VulkanRenderPass> m_CompositionPass;

    std::vector <std::unique_ptr<VulkanImage2D>> m_GBufferTextures;
    std::unique_ptr<VulkanImage2D> m_LightingTexture;

    std::vector<std::unique_ptr<VulkanFramebuffer>> m_GBufferFramebuffers;
    std::vector<std::unique_ptr<VulkanFramebuffer>> m_LightingFramebuffers;
    std::vector<std::unique_ptr<VulkanFramebuffer>> m_CompositionFramebuffers;

    std::unique_ptr<VulkanGraphicsPipeline> m_GBufferPipeline;
    std::unique_ptr<VulkanGraphicsPipeline> m_LightingPipeline;
    std::unique_ptr<VulkanGraphicsPipeline> m_CompositionPipeline;

    void CreatePipelines();
    void CreateRenderPasses();
    void CreateGBufferTextures();
    void CreateFramebuffers();
};