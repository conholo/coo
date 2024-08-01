#include "vulkan_deferred_renderer.h"
#include "vulkan_model.h"
#include <vulkan/vulkan.h>

VulkanDeferredRenderer::VulkanDeferredRenderer(VulkanRenderer* renderer)
    :m_Renderer(renderer)
{

}

VulkanDeferredRenderer::~VulkanDeferredRenderer()
{

}

void VulkanDeferredRenderer::Initialize()
{
    m_FullScreenQuadVertexShader = std::make_shared<VulkanShader>("", ShaderType::Vertex);
    CreateRenderPasses();
    CreateFramebuffers();
}

void VulkanDeferredRenderer::CreateRenderPasses()
{
    m_GBufferPass = std::make_unique<VulkanRenderPass>("G-Buffer Render Pass");
    m_GBufferPass->AddAttachment({})
}

void VulkanDeferredRenderer::CreateGBufferPipeline()
{
    m_GBufferVertexShader = std::make_shared<VulkanShader>("../assets/shaders/gbuffer.vert", ShaderType::Vertex);
    m_GBufferFragmentShader = std::make_shared<VulkanShader>("../assets/shaders/gbuffer.frag", ShaderType::Fragment);
    m_GBufferMaterial = std::make_shared<VulkanMaterial>(m_GBufferVertexShader, m_GBufferFragmentShader);

    auto builder =
            VulkanGraphicsPipelineBuilder("G-Buffer Pipeline")
            .SetShaders(m_GBufferVertexShader, m_GBufferFragmentShader)
            .SetVertexInputDescription({VulkanModel::Vertex::GetBindingDescriptions(), VulkanModel::Vertex::GetAttributeDescriptions()})
            .SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .SetPolygonMode(VK_POLYGON_MODE_FILL)
            .SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
            .SetDepthTesting(true, true, VK_COMPARE_OP_LESS_OR_EQUAL)
            .SetColorBlendAttachment(false)
            .SetDynamicStates({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
            .SetLayout(m_GBufferMaterial->GetPipelineLayout())
            .SetRenderPass(m_GBufferPass->RenderPass());

    m_GBufferPipeline = builder.Build();
}


void VulkanDeferredRenderer::CreatePipelines()
{
    CreateGBufferPipeline();

    // Lighting Pipeline
    m_LightingPipeline = std::make_unique<VulkanGraphicsPipeline>("Lighting Pipeline");
    // Set up Lighting pipeline states...
    m_LightingPipeline->SetRenderPass(m_LightingPass->RenderPass());
    m_LightingPipeline->Build();

    // Composition Pipeline
    m_CompositionPipeline = std::make_unique<VulkanGraphicsPipeline>("Composition Pipeline");
    // Set up Composition pipeline states...
    m_CompositionPipeline->SetRenderPass(m_CompositionPass->RenderPass());
    m_CompositionPipeline->Build();
}

void VulkanDeferredRenderer::CreateFramebuffers()
{
    VkExtent2D extent = m_Renderer->VulkanSwapchain().Extent();
    uint32_t imageCount = m_Renderer->VulkanSwapchain().ImageCount();

    // Create G-Buffer framebuffers
    m_GBufferFramebuffers.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        std::vector <VkImageView> attachments;
        for (const auto &texture: m_GBufferTextures)
            attachments.push_back(texture->GetView()->ImageView());

        m_GBufferFramebuffers[i] = std::make_unique<VulkanFramebuffer>("G-Buffer Framebuffer " + std::to_string(i));
        m_GBufferFramebuffers[i]->Create(m_GBufferPass->RenderPass(), attachments, extent.width, extent.height);
    }

    // Create Lighting framebuffers
    m_LightingFramebuffers.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        std::vector <VkImageView> attachments = {m_LightingTexture->GetView()->ImageView()};

        m_LightingFramebuffers[i] = std::make_unique<VulkanFramebuffer>("Lighting Framebuffer " + std::to_string(i));
        m_LightingFramebuffers[i]->Create(m_LightingPass->RenderPass(), attachments, extent.width, extent.height);
    }

    // Create Composition framebuffers
    m_CompositionFramebuffers.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        VkImageView view = m_Renderer->VulkanSwapchain().GetImage(i)->GetView()->ImageView();
        std::vector <VkImageView> attachments = {view};

        m_CompositionFramebuffers[i] = std::make_unique<VulkanFramebuffer>("Composition Framebuffer " + std::to_string(i));
        m_CompositionFramebuffers[i]->Create(m_CompositionPass->RenderPass(), attachments, extent.width, extent.height);
    }
}

void VulkanDeferredRenderer::Render(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t imageIndex)
{
    // G-Buffer Pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_GBufferPass->RenderPass();
    renderPassInfo.framebuffer = m_GBufferFramebuffers[frameIndex]->Framebuffer();
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_Renderer->VulkanSwapchain().Extent();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GBufferPipeline->GraphicsPipeline());
    // Render geometry to G-Buffer
    vkCmdEndRenderPass(commandBuffer);

    // Lighting Pass
    renderPassInfo.renderPass = m_LightingPass->RenderPass();
    renderPassInfo.framebuffer = m_LightingFramebuffers[frameIndex]->Framebuffer();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightingPipeline->GraphicsPipeline());
    // Perform lighting calculations
    vkCmdEndRenderPass(commandBuffer);

    // Composition Pass (Final output to swapchain image)
    renderPassInfo.renderPass = m_CompositionPass->RenderPass();
    renderPassInfo.framebuffer = m_CompositionFramebuffers[imageIndex]->Framebuffer();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CompositionPipeline->GraphicsPipeline());
    // Perform final composition
    vkCmdEndRenderPass(commandBuffer);
}