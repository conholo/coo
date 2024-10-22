#pragma once

#include <vulkan/vulkan.h>

#include <cassert>
#include <iostream>

inline const char* VKResultToString(VkResult result)
{
	switch (result)
	{
		case VK_SUCCESS:
			return "VK_SUCCESS";
		case VK_NOT_READY:
			return "VK_NOT_READY";
		case VK_TIMEOUT:
			return "VK_TIMEOUT";
		case VK_EVENT_SET:
			return "VK_EVENT_SET";
		case VK_EVENT_RESET:
			return "VK_EVENT_RESET";
		case VK_INCOMPLETE:
			return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:
			return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST:
			return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:
			return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:
			return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:
			return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:
			return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL:
			return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_UNKNOWN:
			return "VK_ERROR_UNKNOWN";
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:
			return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_FRAGMENTATION:
			return "VK_ERROR_FRAGMENTATION";
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
			return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		case VK_ERROR_SURFACE_LOST_KHR:
			return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR:
			return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:
			return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT:
			return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV:
			return "VK_ERROR_INVALID_SHADER_NV";
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
			return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_NOT_PERMITTED_EXT:
			return "VK_ERROR_NOT_PERMITTED_EXT";
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
			return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		case VK_THREAD_IDLE_KHR:
			return "VK_THREAD_IDLE_KHR";
		case VK_THREAD_DONE_KHR:
			return "VK_THREAD_DONE_KHR";
		case VK_OPERATION_DEFERRED_KHR:
			return "VK_OPERATION_DEFERRED_KHR";
		case VK_OPERATION_NOT_DEFERRED_KHR:
			return "VK_OPERATION_NOT_DEFERRED_KHR";
		case VK_PIPELINE_COMPILE_REQUIRED_EXT:
			return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
		case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
			return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
			return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
			return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
			return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
			return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
			return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
		case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
			return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
		case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:
			return "VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT";
		case VK_RESULT_MAX_ENUM:
			return "VK_RESULT_MAX_ENUM";
	}
	return nullptr;
}

inline const char* VkObjectTypeToString(const VkObjectType type)
{
	switch (type)
	{
		case VK_OBJECT_TYPE_COMMAND_BUFFER:
			return "VK_OBJECT_TYPE_COMMAND_BUFFER";
		case VK_OBJECT_TYPE_PIPELINE:
			return "VK_OBJECT_TYPE_PIPELINE";
		case VK_OBJECT_TYPE_FRAMEBUFFER:
			return "VK_OBJECT_TYPE_FRAMEBUFFER";
		case VK_OBJECT_TYPE_IMAGE:
			return "VK_OBJECT_TYPE_IMAGE";
		case VK_OBJECT_TYPE_QUERY_POOL:
			return "VK_OBJECT_TYPE_QUERY_POOL";
		case VK_OBJECT_TYPE_RENDER_PASS:
			return "VK_OBJECT_TYPE_RENDER_PASS";
		case VK_OBJECT_TYPE_COMMAND_POOL:
			return "VK_OBJECT_TYPE_COMMAND_POOL";
		case VK_OBJECT_TYPE_PIPELINE_CACHE:
			return "VK_OBJECT_TYPE_PIPELINE_CACHE";
		case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:
			return "VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR";
		case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV:
			return "VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV";
		case VK_OBJECT_TYPE_BUFFER:
			return "VK_OBJECT_TYPE_BUFFER";
		case VK_OBJECT_TYPE_BUFFER_VIEW:
			return "VK_OBJECT_TYPE_BUFFER_VIEW";
		case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
			return "VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT";
		case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
			return "VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT";
		case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR:
			return "VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR";
		case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
			return "VK_OBJECT_TYPE_DESCRIPTOR_POOL";
		case VK_OBJECT_TYPE_DESCRIPTOR_SET:
			return "VK_OBJECT_TYPE_DESCRIPTOR_SET";
		case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
			return "VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT";
		case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE:
			return "VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE";
		case VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT:
			return "VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT";
		case VK_OBJECT_TYPE_DEVICE:
			return "VK_OBJECT_TYPE_DEVICE";
		case VK_OBJECT_TYPE_DEVICE_MEMORY:
			return "VK_OBJECT_TYPE_DEVICE_MEMORY";
		case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
			return "VK_OBJECT_TYPE_PIPELINE_LAYOUT";
		case VK_OBJECT_TYPE_DISPLAY_KHR:
			return "VK_OBJECT_TYPE_DISPLAY_KHR";
		case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:
			return "VK_OBJECT_TYPE_DISPLAY_MODE_KHR";
		case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
			return "VK_OBJECT_TYPE_PHYSICAL_DEVICE";
		case VK_OBJECT_TYPE_EVENT:
			return "VK_OBJECT_TYPE_EVENT";
		case VK_OBJECT_TYPE_FENCE:
			return "VK_OBJECT_TYPE_FENCE";
		case VK_OBJECT_TYPE_IMAGE_VIEW:
			return "VK_OBJECT_TYPE_IMAGE_VIEW";
		case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV:
			return "VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV";
		case VK_OBJECT_TYPE_INSTANCE:
			return "VK_OBJECT_TYPE_INSTANCE";
		case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL:
			return "VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL";
		case VK_OBJECT_TYPE_QUEUE:
			return "VK_OBJECT_TYPE_QUEUE";
		case VK_OBJECT_TYPE_SAMPLER:
			return "VK_OBJECT_TYPE_SAMPLER";
		case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION:
			return "VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION";
		case VK_OBJECT_TYPE_SEMAPHORE:
			return "VK_OBJECT_TYPE_SEMAPHORE";
		case VK_OBJECT_TYPE_SHADER_MODULE:
			return "VK_OBJECT_TYPE_SHADER_MODULE";
		case VK_OBJECT_TYPE_SURFACE_KHR:
			return "VK_OBJECT_TYPE_SURFACE_KHR";
		case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
			return "VK_OBJECT_TYPE_SWAPCHAIN_KHR";
		case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:
			return "VK_OBJECT_TYPE_VALIDATION_CACHE_EXT";
		case VK_OBJECT_TYPE_UNKNOWN:
			return "VK_OBJECT_TYPE_UNKNOWN";
		case VK_OBJECT_TYPE_MAX_ENUM:
			return "VK_OBJECT_TYPE_MAX_ENUM";
		case VK_OBJECT_TYPE_VIDEO_SESSION_KHR:
			return "VK_OBJECT_TYPE_VIDEO_SESSION_KHR";
		case VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR:
			return "VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR";
		case VK_OBJECT_TYPE_CU_MODULE_NVX:
			return "VK_OBJECT_TYPE_CU_MODULE_NVX";
		case VK_OBJECT_TYPE_CU_FUNCTION_NVX:
			return "VK_OBJECT_TYPE_CU_FUNCTION_NVX";
		case VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA:
			return "VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA";
		case VK_OBJECT_TYPE_MICROMAP_EXT:
			return "VK_OBJECT_TYPE_MICROMAP_EXT";
		case VK_OBJECT_TYPE_OPTICAL_FLOW_SESSION_NV:
			return "VK_OBJECT_TYPE_OPTICAL_FLOW_SESSION_NV";
		case VK_OBJECT_TYPE_SHADER_EXT:
			return "VK_OBJECT_TYPE_SHADER_EXT";
	}
	return "Unknown VK_OBJECT_TYPE";
}

#define VK_CHECK_RESULT(f)                                                                                                    \
	{                                                                                                                         \
		VkResult res = (f);                                                                                                   \
		if (res != VK_SUCCESS)                                                                                                \
		{                                                                                                                     \
			std::cout << "Fatal : VkResult is \"" << VKResultToString(res) << "\" in " << __FILE__ << " at line " << __LINE__ \
					  << "\n";                                                                                                \
			assert(res == VK_SUCCESS);                                                                                        \
		}                                                                                                                     \
	}

inline void SetDebugUtilsObjectName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const char* name)
{
	VkDebugUtilsObjectNameInfoEXT nameInfo = {};
	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.objectType = objectType;
	nameInfo.objectHandle = objectHandle;
	nameInfo.pObjectName = name;

	auto func = (PFN_vkSetDebugUtilsObjectNameEXT) vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
	if (func != nullptr)
	{
		func(device, &nameInfo);
	}
}

inline void BeginDebugMarker(VkCommandBuffer cmdBuffer, const char* markerName, const float color[4])
{
	auto func = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(VulkanContext::Get().Instance(), "vkCmdBeginDebugUtilsLabelEXT");
	if (func != nullptr)
	{
		VkDebugUtilsLabelEXT markerInfo = {};
		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		markerInfo.pLabelName = markerName;
		memcpy(markerInfo.color, color, sizeof(float) * 4);
		func(cmdBuffer, &markerInfo);
	}
}

inline void EndDebugMarker(VkCommandBuffer cmdBuffer)
{
	auto func = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(VulkanContext::Get().Instance(), "vkCmdEndDebugUtilsLabelEXT");
	if (func != nullptr)
	{
		func(cmdBuffer);
	}
}