#pragma once

#include "vulkan_render_pass.h"
#include "vulkan_framebuffer.h"
#include "vulkan_graphics_pipeline.h"
#include "vulkan_image.h"
#include "vulkan_material.h"
#include "core/frame_info.h"
#include "vulkan_texture.h"

class VulkanRenderer;
class VulkanSimpleRenderer
{
public:
    explicit VulkanSimpleRenderer(VulkanRenderer* renderer);
    ~VulkanSimpleRenderer();

    void Initialize();

    void Render(FrameInfo& frameInfo);
    void Resize(uint32_t width, uint32_t height);

private:
    void CreateSimpleResources();
    void CreateSimpleTexture();
    void CreateSimpleRenderPass();
    void CreateSimplePipeline();
    void CreateOrInvalidateSimpleFramebuffers();

private:
    VulkanRenderer *m_Renderer;

    /*
     * Needs to be allocated based on the number swapchain images - NOT the number of frames in flight
     */
    std::vector<std::unique_ptr<VulkanFramebuffer>> m_SimpleFramebuffers;

    /*
     * Single Use Resources
    */
    std::shared_ptr<VulkanTexture2D> m_SimpleTexture;
    std::unique_ptr<VulkanRenderPass> m_SimplePass;

    std::unique_ptr<VulkanGraphicsPipeline> m_SimplePipeline;

    std::shared_ptr<VulkanShader> m_SimpleVertexShader;
    std::shared_ptr<VulkanShader> m_SimpleFragmentShader;

    std::shared_ptr<VulkanMaterialLayout> m_SimpleMaterialLayout;
    std::shared_ptr<VulkanMaterial> m_SimpleBaseMaterial;
};