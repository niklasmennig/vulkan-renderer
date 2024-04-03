#pragma once
#include "vulkan.h"

#include "buffer.h"

#include "image.h"

#include <unordered_map>
#include <string>

struct RaytracingPipelineBuilder;

struct ComputeShader;
struct ProcessingPipelineBuilder;


struct SharedAllocation {
    VkDeviceMemory memory;
    VkDeviceSize current_offset = 0;
};

struct Device
{
    VkInstance vulkan_instance{};
    VkDevice vulkan_device{};

    VkQueue graphics_queue;
    VkCommandPool command_pool{};

    uint32_t image_count;
    VkSurfaceFormatKHR surface_format;

    uint32_t graphics_queue_family_index;

    VkPhysicalDeviceMemoryProperties memory_properties{};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties{};
    VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_properties{};

    

    uint32_t shared_allocation_size = 1048576;
    // maps memory type index to shared memory
    std::unordered_map<uint32_t, std::vector<SharedAllocation>> shared_allocations;


    VkCommandBuffer begin_single_use_command_buffer();
    void end_single_use_command_buffer(VkCommandBuffer cmd_buffer);

    Buffer create_buffer(VkBufferCreateInfo *create_info, size_t alignment = 4, bool exportable = false);
    Buffer create_buffer(VkDeviceSize size, VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, bool exportable = false);
    Image create_image(uint32_t width, uint32_t height, VkImageUsageFlags usage, uint32_t array_layers = 1, VkMemoryPropertyFlags memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VkFormat format = VK_FORMAT_UNDEFINED, VkFilter filter = VK_FILTER_LINEAR, VkSamplerAddressMode uv_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

    void allocate_memory(VkMemoryAllocateInfo alloc_info, size_t alignment, VkDeviceMemory* memory, VkDeviceSize* offset, bool* allocation_is_shared);

    RaytracingPipelineBuilder create_raytracing_pipeline_builder();
    ProcessingPipelineBuilder create_processing_pipeline_builder();

    // function pointers
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
    PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR;

    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
};