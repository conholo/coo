#pragma once

#include <string>

// Global
inline static std::string GlobalUniformBufferResourceName = "Global Uniform Buffer";
inline static std::string FullScreenQuadShaderResourceName = "Full Screen Quad Shader";

// Swapchain
inline static std::string SwapchainImagesInFlightFenceResourceName = "Swapchain Image In Flight Fence";
inline static std::string SwapchainResourcesInFlightFenceResourceName = "Swapchain Resources In Flight Fence";
inline static std::string SwapchainImageAvailableSemaphoreResourceName = "Swapchain Image Available Semaphore";
inline static std::string SwapchainRenderingCompleteSemaphoreResourceName = "Swapchain Rendering Complete Semaphore";
inline static std::string SwapchainImage2DResourceName = "Swapchain Image";
inline static std::string SwapchainFramebufferResourceName = "Swapchain Framebuffer";
inline static std::string SwapchainRenderPassResourceName = "Swapchain RenderPass";
inline static std::string SwapchainCommandBufferResourceName = "Swapchain Command Buffer";

// G-Buffer Pass
inline static std::string GBufferCommandBufferResourceName = "G-Buffer Command Buffer";
inline static std::string GBufferPositionAttachmentTextureResourceName ="G-Buffer Position Attachment";
inline static std::string GBufferNormalAttachmentTextureResourceName ="G-Buffer Normal Attachment";
inline static std::string GBufferAlbedoAttachmentTextureResourceName ="G-Buffer Albedo Attachment";
inline static std::string GBufferDepthAttachmentTextureResourceName ="G-Buffer Depth Attachment";
inline static std::string GBufferVertexShaderResourceName = "G-Buffer Vertex Shader";
inline static std::string GBufferFragmentShaderResourceName = "G-Buffer Fragment Shader";
inline static std::string GBufferMaterialLayoutResourceName = "G-Buffer Material Layout";
inline static std::string GBufferMaterialResourceName = "G-Buffer Material";
inline static std::string GBufferRenderPassResourceName = "G-Buffer Render Pass";
inline static std::string GBufferGraphicsPipelineResourceName = "G-Buffer Graphics Pipeline";
inline static std::string GBufferFramebufferResourceName = "G-Buffer Framebuffer";
inline static std::string GBufferRenderCompleteSemaphoreResourceName = "G-Buffer Render Complete Semaphore";
inline static std::string GBufferResourcesInFlightResourceName = "G-Buffer Resources In Flight Fence";

// Lighting Pass
inline static std::string LightingCommandBufferResourceName = "Lighting Command Buffer";
inline static std::string LightingColorAttachmentResourceName ="Lighting Color Attachment";
inline static std::string LightingFragmentShaderResourceName = "Lighting Fragment Shader";
inline static std::string LightingMaterialLayoutResourceName = "Lighting Material Layout";
inline static std::string LightingMaterialResourceName = "Lighting Material";
inline static std::string LightingRenderPassResourceName = "Lighting Render Pass";
inline static std::string LightingGraphicsPipelineResourceName = "Lighting Graphics Pipeline";
inline static std::string LightingFramebufferResourceName = "Lighting Framebuffer";
inline static std::string LightingRenderCompleteSemaphoreResourceName = "Lighting Render Complete Semaphore";
inline static std::string LightingResourcesInFlightResourceName = "Lighting Resources In Flight Fence";

// Swapchain Pass

// Scene Composition Sub Pass
inline static std::string SceneCompositionFragmentShaderResourceName = "Scene Composition Fragment Shader";
inline static std::string SceneCompositionMaterialLayoutResourceName = "Scene Composition Material Layout";
inline static std::string SceneCompositionMaterialResourceName = "Scene Composition Material";
inline static std::string SceneCompositionGraphicsPipelineResourceName = "Scene Composition Graphics Pipeline";

// UI Sub Pass
inline static std::string UIVertexShaderResourceName = "UI Vertex Shader";
inline static std::string UIFragmentShaderResourceName = "UI Fragment Shader";
inline static std::string UIMaterialLayoutResourceName = "UI Material Layout";
inline static std::string UIMaterialResourceName = "UI Material";
inline static std::string UIGraphicsPipelineResourceName = "UI Graphics Pipeline";
inline static std::string UIVertexBufferResourceName = "UI Vertex Buffer";
inline static std::string UIIndexBufferResourceName = "UI Index Buffer";
inline static std::string UIFontTextureResourceName = "UI Font Texture";
