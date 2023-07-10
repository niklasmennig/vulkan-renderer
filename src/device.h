#pragma once
#include "vulkan.h"

#include "buffer.h"

#include "image.h"

struct PipelineBuilder;

struct Device
{
    VkInstance vulkan_instance{};
    VkDevice vulkan_device{};

    uint32_t image_count;
    VkSurfaceFormatKHR surface_format;

    VkPhysicalDeviceMemoryProperties memory_properties{};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties{};

    Buffer create_buffer(VkBufferCreateInfo *create_info);
    Buffer create_buffer(VkDeviceSize size, VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    Image create_image(uint32_t width, uint32_t height, VkImageUsageFlags usage, uint32_t array_layers = 1);
    PipelineBuilder create_pipeline_builder();

    // function pointers
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
};