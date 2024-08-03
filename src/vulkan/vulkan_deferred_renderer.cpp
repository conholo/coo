#include "vulkan_deferred_renderer.h"
#include "vulkan_model.h"
#include "vulkan_renderer.h"
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
    m_FullScreenQuadVertexShader = std::make_shared<VulkanShader>("../assets/shaders/fsq.vert", ShaderType::Vertex);
    CreateGBufferResources();
}

void VulkanDeferredRenderer::CreateGBufferResources()
{
    CreateGBufferTextures();
    CreateGBufferRenderPass();
    CreateGBufferPipeline();
    CreateGBufferFramebuffers();
}

void VulkanDeferredRenderer::CreateGBufferTextures()
{
    ImageSpecification positionImageSpec
    {
        .DebugName = "G-Buffer-Position",
        .Format = ImageFormat::RGBA16F,
        .Usage = ImageUsage::Attachment,
        .Properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .UsedInTransferOps = true,
        .Width = m_Renderer->VulkanSwapchain().Width(),
        .Height = m_Renderer->VulkanSwapchain().Height(),
        .Mips = 1,
        .Layers = 1,
        .InvalidateOnConstruction = true
    };
    m_GBufferTextures.push_back(std::make_unique<VulkanImage2D>(positionImageSpec));

    ImageSpecification normalImageSpec
    {
        .DebugName = "G-Buffer-Normal",
        .Format = ImageFormat::RGBA16F,
        .Usage = ImageUsage::Attachment,
        .Properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .UsedInTransferOps = true,
        .Width = m_Renderer->VulkanSwapchain().Width(),
        .Height = m_Renderer->VulkanSwapchain().Height(),
        .Mips = 1,
        .Layers = 1,
        .InvalidateOnConstruction = true
    };
    m_GBufferTextures.push_back(std::make_unique<VulkanImage2D>(normalImageSpec));

    ImageSpecification colorImageSpec
    {
        .DebugName = "G-Buffer-Color",
        .Format = ImageFormat::RGBA,
        .Usage = ImageUsage::Attachment,
        .Properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .UsedInTransferOps = true,
        .Width = m_Renderer->VulkanSwapchain().Width(),
        .Height = m_Renderer->VulkanSwapchain().Height(),
        .Mips = 1,
        .Layers = 1,
        .InvalidateOnConstruction = true
    };
    m_GBufferTextures.push_back(std::make_unique<VulkanImage2D>(colorImageSpec));

    ImageSpecification depthImageSpec
    {
        .DebugName = "G-Buffer-Depth",
        .Format = ImageFormat::DEPTH32F,
        .Usage = ImageUsage::Attachment,
        .Properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .UsedInTransferOps = true,
        .Width = m_Renderer->VulkanSwapchain().Width(),
        .Height = m_Renderer->VulkanSwapchain().Height(),
        .Mips = 1,
        .Layers = 1,
        .InvalidateOnConstruction = true
    };
    m_GBufferTextures.push_back(std::make_unique<VulkanImage2D>(depthImageSpec));
}

void VulkanDeferredRenderer::CreateGBufferRenderPass()
{
    m_GBufferPass = std::make_unique<VulkanRenderPass>("G-Buffer Render Pass");

    // Position attachment
    m_GBufferPass->AddAttachment(
            {
                    .Type = AttachmentType::Color,
                    .Format = ImageFormat::RGBA16F,
                    .Samples = VK_SAMPLE_COUNT_1_BIT,
                    .LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });

    // Normal attachment
    m_GBufferPass->AddAttachment(
            {
                    .Type = AttachmentType::Color,
                    .Format = ImageFormat::RGBA16F,
                    .Samples = VK_SAMPLE_COUNT_1_BIT,
                    .LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });

    // Albedo attachment
    m_GBufferPass->AddAttachment(
            {
                    .Type = AttachmentType::Color,
                    .Format = ImageFormat::RGBA,
                    .Samples = VK_SAMPLE_COUNT_1_BIT,
                    .LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });

    // Depth attachment
    m_GBufferPass->AddAttachment(
            {
                    .Type = AttachmentType::Depth,
                    .Format = ImageFormat::DEPTH32F,
                    .Samples = VK_SAMPLE_COUNT_1_BIT,
                    .LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .FinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            });

    // Set up subpass
    SubpassDescription subpass;
    subpass.ColorAttachments = {0, 1, 2};  // Position, Normal, Albedo
    subpass.DepthStencilAttachment = 3;    // Depth
    m_GBufferPass->AddSubpass(subpass);

    // Add dependency
    m_GBufferPass->AddDependency(
            VK_SUBPASS_EXTERNAL, 0,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_DEPENDENCY_BY_REGION_BIT
    );

    m_GBufferPass->Build();
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
                    .SetRenderPass(m_GBufferPass.get())
                    .SetLayout(m_GBufferMaterial->GetPipelineLayout());

    m_GBufferPipeline = builder.Build();
}

void VulkanDeferredRenderer::CreateGBufferFramebuffers()
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
}

void VulkanDeferredRenderer::Render(FrameInfo& frameInfo)
{
    // Update Global Descriptor
    m_GBufferMaterial->UpdateDescriptor
    (
        0, {
            .binding = 0,
            .bufferInfo =  frameInfo.GlobalUbo->DescriptorInfo()
        }
    );

    // G-Buffer Pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_GBufferPass->RenderPass();
    renderPassInfo.framebuffer = m_GBufferFramebuffers[frameInfo.FrameIndex]->Framebuffer();
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_Renderer->VulkanSwapchain().Extent();

    vkCmdBeginRenderPass(frameInfo.CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(frameInfo.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GBufferPipeline->GetPipeline());
    for(auto& [id, gameObject] : frameInfo.GameObjectManager.GameObjects)
    {
        std::pair<uint32_t, std::vector<DescriptorUpdate>> gameObjectSetUpdates
        {
        1,
        {
            {
                .binding = 0,
                .bufferInfo = frameInfo.GameObjectManager.GetBufferInfoForGameObject(frameInfo.FrameIndex, id)
            },
            {
                .binding = 1,
                .imageInfo = gameObject.DiffuseMap->GetDescriptorInfo()
            },
            {
                .binding = 2,
                .imageInfo = gameObject.NormalMap->GetDescriptorInfo()
              }
            }
        };

        m_GBufferMaterial->UpdateDescriptorSets({gameObjectSetUpdates});
        m_GBufferMaterial->BindDescriptors(frameInfo.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);

        gameObject.ObjectModel->BindVertexInput(frameInfo.CommandBuffer);
        gameObject.ObjectModel->Draw(frameInfo.CommandBuffer);
    }

    // Render geometry to G-Buffer
    vkCmdEndRenderPass(frameInfo.CommandBuffer);
}

void VulkanDeferredRenderer::Resize(uint32_t width, uint32_t height)
{
    for(auto& gBufferTex: m_GBufferTextures)
        gBufferTex->Resize(width, height);
}


