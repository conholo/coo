#pragma once

#include "camera.h"
#include "game_object.h"
#include "scene.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_command_buffer.h>

struct GlobalUbo
{
    glm::mat4 Projection{1.0f};
    glm::mat4 View{1.0f};
    glm::mat4 InvView{1.0f};
    glm::mat4 InvProjection{1.0f};
    glm::vec4 CameraPosition{0.0f};
};

struct FrameInfo
{
    size_t FrameIndex;
	uint32_t ImageIndex;
    float DeltaTime;
    Scene& ActiveScene;
    std::weak_ptr<VulkanBuffer> GlobalUbo;
	std::weak_ptr<VulkanCommandBuffer> SwapchainSubmitCommandBuffer;
	VkSemaphore RendererCompleteSemaphore;
    Camera& Cam;
};
