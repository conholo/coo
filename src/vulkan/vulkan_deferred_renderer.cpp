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
    CreateLightingResources();
    CreateCompositionResources();
}

void VulkanDeferredRenderer::CreateGBufferResources()
{
    CreateGBufferTextures();
    CreateGBufferRenderPass();
    CreateGBufferPipeline();
    CreateOrInvalidateGBufferFramebuffers();
}

void VulkanDeferredRenderer::CreateGBufferTextures()
{
    TextureSpecification positionTextureSpec
    {
        .Format = ImageFormat::RGBA16F,
        .Usage = TextureUsage::Attachment,
        .Width = m_Renderer->VulkanSwapchain().Width(),
        .Height = m_Renderer->VulkanSwapchain().Height(),
        .MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .UsedInTransferOps = true,
        .DebugName = "G-Buffer Position",
    };
    for(int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
    {
        positionTextureSpec.DebugName = "G-Buffer Position " + std::to_string(i);
        m_GBufferTextures[i].push_back(VulkanTexture2D::CreateAttachment(positionTextureSpec));
    }

    TextureSpecification normalTextureSpec
    {
        .Format = ImageFormat::RGBA16F,
        .Usage = TextureUsage::Attachment,
        .Width = m_Renderer->VulkanSwapchain().Width(),
        .Height = m_Renderer->VulkanSwapchain().Height(),
        .MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .UsedInTransferOps = true,
        .DebugName = "G-Buffer Normal",
    };
    for(int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
    {
        normalTextureSpec.DebugName = "G-Buffer Normal " + std::to_string(i);
        m_GBufferTextures[i].push_back(VulkanTexture2D::CreateAttachment(normalTextureSpec));
    }

    TextureSpecification colorTextureSpec
    {
        .Format = ImageFormat::RGBA,
        .Usage = TextureUsage::Attachment,
        .Width = m_Renderer->VulkanSwapchain().Width(),
        .Height = m_Renderer->VulkanSwapchain().Height(),
        .MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .UsedInTransferOps = true,
        .DebugName = "G-Buffer Color",
    };
    for(int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
    {
        colorTextureSpec.DebugName = "G-Buffer Color " + std::to_string(i);
        m_GBufferTextures[i].push_back(VulkanTexture2D::CreateAttachment(colorTextureSpec));
    }

    TextureSpecification depthTextureSpec
    {
        .Format = ImageFormat::DEPTH32F,
        .Usage = TextureUsage::Attachment,
        .Width = m_Renderer->VulkanSwapchain().Width(),
        .Height = m_Renderer->VulkanSwapchain().Height(),
        .MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .UsedInTransferOps = true,
        .DebugName = "G-Buffer Depth",
    };

    for(int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
    {
        depthTextureSpec.DebugName = "G-Buffer Depth " + std::to_string(i);
        m_GBufferTextures[i].push_back(VulkanTexture2D::CreateAttachment(depthTextureSpec));
    }
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
                    .InitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .ClearValue = { .color = {0.0f, 0.0f, 0.0f, 0.0f} }
            });

    // Normal attachment
    m_GBufferPass->AddAttachment(
            {
                    .Type = AttachmentType::Color,
                    .Format = ImageFormat::RGBA16F,
                    .Samples = VK_SAMPLE_COUNT_1_BIT,
                    .LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .InitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .ClearValue = { .color = {0.0f, 0.0f, 0.0f, 0.0f} }
            });

    // Albedo attachment
    m_GBufferPass->AddAttachment(
            {
                    .Type = AttachmentType::Color,
                    .Format = ImageFormat::RGBA,
                    .Samples = VK_SAMPLE_COUNT_1_BIT,
                    .LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .InitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .ClearValue = { .color = {0.0f, 0.0f, 0.0f, 0.0f} }
            });

    // Depth attachment
    m_GBufferPass->AddAttachment(
            {
                    .Type = AttachmentType::Depth,
                    .Format = ImageFormat::DEPTH32F,
                    .Samples = VK_SAMPLE_COUNT_1_BIT,
                    .LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .InitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    .FinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    .ClearValue = { .depthStencil = {1.0f, 0} }
            });

    // Set up subpass
    SubpassDescription subpass;
    subpass.ColorAttachments = {0, 1, 2};  // Position, Normal, Albedo
    subpass.DepthStencilAttachment = 3;    // Depth
    m_GBufferPass->AddSubpass(subpass);

    // Dependency from outside of pass to inside G-Buffer pass where it writes to the attachments.
    m_GBufferPass->AddDependency(
            VK_SUBPASS_EXTERNAL, 0,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_DEPENDENCY_BY_REGION_BIT
    );

    // Dependency from inside G-Buffer pass to outside the pass (lighting pass) where it will read from the g-buffer attachments.
    m_GBufferPass->AddDependency(
            0, VK_SUBPASS_EXTERNAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_DEPENDENCY_BY_REGION_BIT
    );

    m_GBufferPass->Build();
}

void VulkanDeferredRenderer::CreateGBufferPipeline()
{
    m_GBufferVertexShader = std::make_shared<VulkanShader>("../assets/shaders/gbuffer.vert", ShaderType::Vertex);
    m_GBufferFragmentShader = std::make_shared<VulkanShader>("../assets/shaders/gbuffer.frag", ShaderType::Fragment);

    m_GBufferMaterialLayout = std::make_shared<VulkanMaterialLayout>(m_GBufferVertexShader, m_GBufferFragmentShader);
    m_GBufferBaseMaterial = std::make_shared<VulkanMaterial>(m_GBufferMaterialLayout);

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
                    .SetLayout(m_GBufferMaterialLayout->GetPipelineLayout());

    m_GBufferPipeline = builder.Build();
}

void VulkanDeferredRenderer::CreateOrInvalidateGBufferFramebuffers()
{
    m_GBufferFramebuffers.clear();

    VkExtent2D extent = m_Renderer->VulkanSwapchain().Extent();

    // Create G-Buffer framebuffers
    m_GBufferFramebuffers.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < m_GBufferFramebuffers.size(); i++)
    {
        std::vector <VkImageView> attachments;
        for (const auto &texture: m_GBufferTextures[i])
            attachments.push_back(texture->GetImage()->GetView()->ImageView());

        m_GBufferFramebuffers[i] = std::make_unique<VulkanFramebuffer>("G-Buffer Framebuffer " + std::to_string(i));
        m_GBufferFramebuffers[i]->Create(m_GBufferPass->RenderPass(), attachments, extent.width, extent.height);
    }
}

/*
 * Lighting
 */
void VulkanDeferredRenderer::CreateLightingResources()
{
    CreateLightingTextures();
    CreateLightingRenderPass();
    CreateLightingPipeline();
    CreateOrInvalidateLightingFramebuffers();
}

void VulkanDeferredRenderer::CreateLightingTextures()
{
    TextureSpecification textureSpec
    {
        .Format = ImageFormat::RGBA,
        .Width = m_Renderer->VulkanSwapchain().Width(),
        .Height = m_Renderer->VulkanSwapchain().Height(),
        .MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .UsedInTransferOps = true,
        .DebugName = "Lighting Color",
    };
    for(int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        textureSpec.DebugName = "Lighting Color Attachment " + std::to_string(i);
        m_LightingTextures[i] = VulkanTexture2D::CreateAttachment(textureSpec);
    }
}

void VulkanDeferredRenderer::CreateLightingRenderPass()
{
    m_LightingPass = std::make_unique<VulkanRenderPass>("Lighting Render Pass");

    // Color attachment
    m_LightingPass->AddAttachment(
            {
                    .Type = AttachmentType::Color,
                    .Format = ImageFormat::RGBA,
                    .Samples = VK_SAMPLE_COUNT_1_BIT,
                    .LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .InitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .ClearValue { .color = {0.0f, 0.0f, 0.0f, 0.0f} }
            });
    // Set up subpass
    SubpassDescription subpass;
    subpass.ColorAttachments = {0};  // Color
    m_LightingPass->AddSubpass(subpass);

    // From external pass (G-Buffer) to lighting pass
    m_LightingPass->AddDependency(
            VK_SUBPASS_EXTERNAL, 0,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,      // Wait for G-Buffer pass to *finish*,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,              // then fragment shader of lighting can then *start*.
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,              // Once the *writing* of G-Buffer attachments is finished,
            VK_ACCESS_SHADER_READ_BIT,                         // then this pass can *read* them as inputs.
            VK_DEPENDENCY_BY_REGION_BIT
    );

    // From lighting pass to external pass (composition)
    m_LightingPass->AddDependency(
            0, VK_SUBPASS_EXTERNAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,      // Wait for the writing of the lighting attachment to *finish*,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,              // then external fragment shaders that want to read the attachment can *start*.
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,              // Once the lighting pass has *written* to the lighting attachment,
            VK_ACCESS_SHADER_READ_BIT,                         // Other shaders can *read* from the attachment.
            VK_DEPENDENCY_BY_REGION_BIT
    );

    m_LightingPass->Build();
}

void VulkanDeferredRenderer::CreateLightingPipeline()
{
    m_LightingFragmentShader = std::make_shared<VulkanShader>("../assets/shaders/lighting.frag", ShaderType::Fragment);
    m_LightingMaterialLayout = std::make_shared<VulkanMaterialLayout>(m_FullScreenQuadVertexShader, m_LightingFragmentShader);
    m_LightingMaterial = std::make_shared<VulkanMaterial>(m_LightingMaterialLayout);

    auto builder =
            VulkanGraphicsPipelineBuilder("Lighting Pipeline")
                    .SetShaders(m_FullScreenQuadVertexShader, m_LightingFragmentShader)
                    .SetVertexInputDescription()
                    .SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                    .SetPolygonMode(VK_POLYGON_MODE_FILL)
                    .SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                    .SetRenderPass(m_LightingPass.get())
                    .SetLayout(m_LightingMaterialLayout->GetPipelineLayout());

    m_LightingPipeline = builder.Build();
}

void VulkanDeferredRenderer::CreateOrInvalidateLightingFramebuffers()
{
    m_LightingFramebuffers.clear();
    VkExtent2D extent = m_Renderer->VulkanSwapchain().Extent();

    m_LightingFramebuffers.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        std::vector <VkImageView> attachments { m_LightingTextures[i]->GetImage()->GetView()->ImageView() };
        m_LightingFramebuffers[i] = std::make_unique<VulkanFramebuffer>("Lighting Framebuffer " + std::to_string(i));
        m_LightingFramebuffers[i]->Create(m_LightingPass->RenderPass(), attachments, extent.width, extent.height);
    }
}


void VulkanDeferredRenderer::CreateCompositionResources()
{
    CreateCompositionRenderPass();
    CreateCompositionPipeline();
    CreateOrInvalidateCompositionFramebuffers();
}

void VulkanDeferredRenderer::CreateCompositionRenderPass()
{
    m_CompositionPass = std::make_unique<VulkanRenderPass>("Composition Render Pass");

    // Final color attachment (swapchain image)
    m_CompositionPass->AddAttachment({
                                         .Type = AttachmentType::Color,
                                         .Format = ImageUtils::VulkanFormatToImageFormat(m_Renderer->VulkanSwapchain().SwapchainImageFormat()),
                                         .Samples = VK_SAMPLE_COUNT_1_BIT,
                                         .LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                         .StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                                         .InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                         .FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                         .ClearValue = { .color = {0.0f, 0.0f, 0.0f, 1.0f} }
                                     });

    // Set up subpass
    SubpassDescription subpass;
    subpass.ColorAttachments = {0};  // Final color output
    m_CompositionPass->AddSubpass(subpass);

    // Dependency from Lighting pass to Composition pass
    m_CompositionPass->AddDependency(
        VK_SUBPASS_EXTERNAL, 0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // Wait for lighting pass to *finish*,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,          // then the composition pass can *start*.
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,          // Once the lighting pass has *written* to its attachments,
        VK_ACCESS_SHADER_READ_BIT,                     // Then the composition pass can *read* the attachment in the shader.
        VK_DEPENDENCY_BY_REGION_BIT
    );

    // Dependency from Composition pass to final output
    m_CompositionPass->AddDependency(
        0, VK_SUBPASS_EXTERNAL,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // Wait for the composition writes to *finish*,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,           // then the frame writing is complete and then display can *start*,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,          // Once the composition pass has *written* to its attachment,
        VK_ACCESS_MEMORY_READ_BIT,                     // Then the impl. can *read* the memory of the attachment (swapchain image).
        VK_DEPENDENCY_BY_REGION_BIT
    );

    m_CompositionPass->Build();
}

void VulkanDeferredRenderer::CreateCompositionPipeline()
{
    m_CompositionFragmentShader = std::make_shared<VulkanShader>("../assets/shaders/texture_display.frag", ShaderType::Fragment);
    m_CompositionMaterialLayout = std::make_shared<VulkanMaterialLayout>(m_FullScreenQuadVertexShader, m_CompositionFragmentShader);
    m_CompositionMaterial = std::make_shared<VulkanMaterial>(m_CompositionMaterialLayout);

    auto builder =
            VulkanGraphicsPipelineBuilder("Composition Pipeline")
                    .SetShaders(m_FullScreenQuadVertexShader, m_CompositionFragmentShader)
                    .SetVertexInputDescription()
                    .SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                    .SetPolygonMode(VK_POLYGON_MODE_FILL)
                    .SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                    .SetRenderPass(m_CompositionPass.get())
                    .SetLayout(m_CompositionMaterialLayout->GetPipelineLayout());

    m_CompositionPipeline = builder.Build();
}

void VulkanDeferredRenderer::CreateOrInvalidateCompositionFramebuffers()
{
    m_CompositionFramebuffers.clear();

    VkExtent2D extent = m_Renderer->VulkanSwapchain().Extent();
    uint32_t imageCount = m_Renderer->VulkanSwapchain().ImageCount();

    m_CompositionFramebuffers.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        std::vector <VkImageView> attachments { m_Renderer->VulkanSwapchain().GetImage(i)->GetView()->ImageView() };
        m_CompositionFramebuffers[i] = std::make_unique<VulkanFramebuffer>("Composition Framebuffer " + std::to_string(i));
        m_CompositionFramebuffers[i]->Create(m_CompositionPass->RenderPass(), attachments, extent.width, extent.height);
    }
}


/*
 *  A simple look at how rendering a few frames might look.
 *
 *  Frame 1:

    Acquire swapchain image (let's say index 0)
    Render G-Buffer using m_GBufferFramebuffers[0]
    Render Lighting using m_LightingFramebuffers[0]
    Render Composition to m_CompositionFramebuffers[0] (which uses swapchain image 0)
    Present swapchain image 0

    Frame 2:

    Acquire swapchain image (let's say index 1)
    Render G-Buffer using m_GBufferFramebuffers[1]
    Render Lighting using m_LightingFramebuffers[1]
    Render Composition to m_CompositionFramebuffers[1] (which uses swapchain image 1)
    Present swapchain image 1

    Frame 3:

    Acquire swapchain image (let's say index 2)
    Render G-Buffer using m_GBufferFramebuffers[0] (reusing frame in flight 0)
    Render Lighting using m_LightingFramebuffers[0] (reusing frame in flight 0)
    Render Composition to m_CompositionFramebuffers[2] (which uses swapchain image 2)
    Present swapchain image 2

    Frame 4:

    Acquire swapchain image (might be index 0 again if it's now available)
    Render G-Buffer using m_GBufferFramebuffers[1] (reusing frame in flight 1)
    Render Lighting using m_LightingFramebuffers[1] (reusing frame in flight 1)
    Render Composition to m_CompositionFramebuffers[0] (which uses swapchain image 0)
    Present swapchain image 0
 */
void VulkanDeferredRenderer::Render(FrameInfo& frameInfo)
{
    /*
     *  G-Buffer Pass
     */
    for (size_t i = 0; i < m_GBufferTextures[frameInfo.FrameIndex].size() - 1; ++i)
        m_GBufferTextures[frameInfo.FrameIndex][i]->TransitionLayout(frameInfo.CommandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    m_GBufferTextures[frameInfo.FrameIndex].back()->TransitionLayout(frameInfo.CommandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    VkRenderPassBeginInfo gBufferRenderPassInfo{};
    gBufferRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    gBufferRenderPassInfo.renderPass = m_GBufferPass->RenderPass();
    gBufferRenderPassInfo.framebuffer = m_GBufferFramebuffers[frameInfo.FrameIndex]->Framebuffer();
    gBufferRenderPassInfo.renderArea.offset = {0, 0};
    gBufferRenderPassInfo.renderArea.extent = m_Renderer->VulkanSwapchain().Extent();

    m_GBufferPass->BeginPass(frameInfo.CommandBuffer, gBufferRenderPassInfo, m_Renderer->VulkanSwapchain());
    m_GBufferPipeline->Bind(frameInfo.CommandBuffer);
    for(auto& [id, gameObject] : frameInfo.ActiveScene.GameObjects)
    {
        gameObject.Render(frameInfo);
    }
    m_GBufferPass->EndPass(frameInfo.CommandBuffer);

    // Transition G-Buffer textures to SHADER_READ_ONLY_OPTIMAL for sampling in the lighting pass
    for (size_t i = 0; i < m_GBufferTextures[frameInfo.FrameIndex].size() - 1; ++i)
        m_GBufferTextures[frameInfo.FrameIndex][i]->TransitionLayout(frameInfo.CommandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    /*
     *  Lighting Pass
     */
    m_LightingTextures[frameInfo.FrameIndex]->TransitionLayout(frameInfo.CommandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderPassBeginInfo lightingRenderPassInfo{};
    lightingRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    lightingRenderPassInfo.renderPass = m_LightingPass->RenderPass();
    lightingRenderPassInfo.framebuffer = m_LightingFramebuffers[frameInfo.FrameIndex]->Framebuffer();
    lightingRenderPassInfo.renderArea.offset = {0, 0};
    lightingRenderPassInfo.renderArea.extent = m_Renderer->VulkanSwapchain().Extent();

    m_LightingPass->BeginPass(frameInfo.CommandBuffer, lightingRenderPassInfo, m_Renderer->VulkanSwapchain());
    m_LightingPipeline->Bind(frameInfo.CommandBuffer);

    m_LightingMaterial->UpdateDescriptorSets(frameInfo.FrameIndex,
   {
       {0,
           {
               {
                   .binding = 0,
                   .type = DescriptorUpdate::Type::Buffer,
                   .bufferInfo =  frameInfo.GlobalUbo->DescriptorInfo()
               }
           }
       },
       {1,
           {
               {
                   // Position
                   .binding = 0,
                   .type = DescriptorUpdate::Type::Image,
                   .imageInfo = m_GBufferTextures[frameInfo.FrameIndex][0]->GetDescriptorInfo()
               },
               {
                   // Normal
                   .binding = 1,
                   .type = DescriptorUpdate::Type::Image,
                   .imageInfo = m_GBufferTextures[frameInfo.FrameIndex][1]->GetDescriptorInfo()
               },
               {
                   // Albedo
                   .binding = 2,
                   .type = DescriptorUpdate::Type::Image,
                   .imageInfo = m_GBufferTextures[frameInfo.FrameIndex][2]->GetDescriptorInfo()
               }
           }
       }
   });
    m_LightingMaterial->BindDescriptors(frameInfo.FrameIndex, frameInfo.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);

    // Full Screen Triangle
    vkCmdDraw(frameInfo.CommandBuffer, 3, 1, 0, 0);
    m_LightingPass->EndPass(frameInfo.CommandBuffer);
    m_LightingTextures[frameInfo.FrameIndex]->TransitionLayout(frameInfo.CommandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    /*
     *  Composition Pass
     */
    uint32_t swapchainImageIndex = m_Renderer->GetCurrentSwapchainImageIndex();
    VkRenderPassBeginInfo compositionRenderPassInfo{};
    compositionRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    compositionRenderPassInfo.renderPass = m_CompositionPass->RenderPass();
    // Note the usage of the framebuffer corresponding to the swapchain image index.
    compositionRenderPassInfo.framebuffer = m_CompositionFramebuffers[swapchainImageIndex]->Framebuffer();
    compositionRenderPassInfo.renderArea.offset = {0, 0};
    compositionRenderPassInfo.renderArea.extent = m_Renderer->VulkanSwapchain().Extent();

    m_CompositionPass->BeginPass(frameInfo.CommandBuffer, compositionRenderPassInfo, m_Renderer->VulkanSwapchain());
    m_CompositionPipeline->Bind(frameInfo.CommandBuffer);

    m_CompositionMaterial->UpdateDescriptorSets(frameInfo.FrameIndex,
     {
         {0,
             {
                 {
                     // Lighting Output
                     .binding = 0,
                     .type = DescriptorUpdate::Type::Image,
                     .imageInfo = m_LightingTextures[frameInfo.FrameIndex]->GetDescriptorInfo()
                 }
             }
         }
     });
    m_CompositionMaterial->BindDescriptors(frameInfo.FrameIndex, frameInfo.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
    vkCmdDraw(frameInfo.CommandBuffer, 3, 1, 0, 0);
    m_CompositionPass->EndPass(frameInfo.CommandBuffer);
}

void VulkanDeferredRenderer::Resize(uint32_t width, uint32_t height)
{
    for(int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
    {
        for(auto& gBufferTex: m_GBufferTextures[i])
            gBufferTex->Resize(width, height);

        m_LightingTextures[i]->Resize(width, height);
    }

    CreateOrInvalidateGBufferFramebuffers();
    CreateOrInvalidateLightingFramebuffers();
    CreateOrInvalidateCompositionFramebuffers();
}

void VulkanDeferredRenderer::RegisterGameObject(GameObject& gameObjectRef)
{
    gameObjectRef.Material = m_GBufferBaseMaterial->Clone();
}

