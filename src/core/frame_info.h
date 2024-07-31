#pragma once

#include "camera.h"
#include "game_object.h"
#include "vulkan_descriptors.h"
#include <vulkan/vulkan.h>

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
    int FrameIndex;
    float FrameTime;
    VkDescriptorSet GlobalDescriptorSet;
    GameObject::Map& GameObjects;
};
