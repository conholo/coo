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
class VulkanSimpleRenderer : public IRenderer
{
public:
    explicit VulkanSimpleRenderer(VulkanRenderer* renderer);
    ~VulkanSimpleRenderer() override;

    void Initialize() override;
    void Shutdown() override;
    void Render(FrameInfo& frameInfo) override;
    void Resize(uint32_t width, uint32_t height) override;
	void RegisterGameObject(GameObject& gameObject) override { }


private:
    void CreateSimpleResources();
    void CreateSimpleTexture();
    void CreateSimpleRenderPass();
    void CreateSimplePipeline();
    void CreateSimpleFramebuffers();

private:
    VulkanRenderer *m_Renderer;

    /*
     * Needs to be allocated based on the number swapchain images - NOT the number of frames in flight
     */
    std::vector<std::unique_ptr<VulkanFramebuffer>> m_SimpleFramebuffers;

    /*
     * Single Use Resources
    */
    std::shared_ptr<VulkanTexture2D> m_SimpleTextureA;
    std::shared_ptr<VulkanTexture2D> m_SimpleTextureB;
    std::unique_ptr<VulkanRenderPass> m_SimplePass;

    std::unique_ptr<VulkanGraphicsPipeline> m_SimplePipeline;

    std::shared_ptr<VulkanShader> m_SimpleVertexShader;
    std::shared_ptr<VulkanShader> m_SimpleFragmentShader;

    std::shared_ptr<VulkanMaterialLayout> m_SimpleMaterialLayout;
    std::shared_ptr<VulkanMaterial> m_SimpleBaseMaterial;

	std::vector<VkSemaphore> m_SimpleRenderFinishedSemaphores;
};