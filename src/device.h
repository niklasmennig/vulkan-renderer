#pragma once
#include "vulkan.h"

#include "buffer.h"

#include "image.h"

#include <unordered_map>

struct PipelineBuilder;

struct SharedDeviceMemory {
    VkDeviceMemory memory;
    VkDeviceSize offset = 0;
};

struct Device
{
    VkInstance vulkan_instance{};
    VkDevice vulkan_device{};

    uint32_t image_count;
    VkSurfaceFormatKHR surface_format;

    uint32_t graphics_queue_family_index;

    VkPhysicalDeviceMemoryProperties memory_properties{};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties{};

    

    // maps memory type index to shared memory
    std::unordered_map<uint32_t, SharedDeviceMemory> shared_buffer_memory;


    Buffer create_buffer(VkBufferCreateInfo *create_info);
    Buffer create_buffer(VkDeviceSize size, VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    Image create_image(uint32_t width, uint32_t height, VkImageUsageFlags usage, uint32_t array_layers = 1, VkMemoryPropertyFlags memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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