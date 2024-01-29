#include "pipeline/processing/pipeline_stage_oidn.h"

#include "pipeline/processing/pipeline_builder.h"
#include "pipeline/processing/compute_shader.h"

#include "pipeline/raytracing/pipeline_builder.h"

#include "core/vulkan.h"

#include <iostream>

void ProcessingPipelineStageOIDN::initialize() {
    oidn_device = oidnNewDeviceByID(0);
    oidnCommitDevice(oidn_device);

    std::cout << "OIDN Device: " << oidnGetPhysicalDeviceString(0, "name") << std::endl;
    std::cout << "OIDN Device Type: " << oidnGetPhysicalDeviceInt(0, "type") << std::endl;
}

void ProcessingPipelineStageOIDN::on_resize(VkExtent2D swapchain_extent, VkExtent2D render_extent) {

    Buffer* image_buffer = &builder->rt_pipeline->get_output_buffer("Result Image").buffer;

    Buffer* albedo_buffer = &builder->rt_pipeline->get_output_buffer("Albedo").buffer;

    Buffer* normals_buffer = &builder->rt_pipeline->get_output_buffer("Normals").buffer;

    builder->output_buffer = image_buffer;
    builder->output_extent = render_extent;

    HANDLE input_handle, albedo_handle, normals_handle;

    VkMemoryGetWin32HandleInfoKHR handle_info{};
    handle_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    handle_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;

    handle_info.memory = image_buffer->device_memory;
    builder->device->vkGetMemoryWin32HandleKHR(builder->device->vulkan_device, &handle_info, &input_handle);

    handle_info.memory = albedo_buffer->device_memory;
    builder->device->vkGetMemoryWin32HandleKHR(builder->device->vulkan_device, &handle_info, &albedo_handle);

    handle_info.memory = normals_buffer->device_memory;
    builder->device->vkGetMemoryWin32HandleKHR(builder->device->vulkan_device, &handle_info, &normals_handle);

    oidn_buffer = oidnNewSharedBufferFromWin32Handle(oidn_device, OIDN_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32, input_handle, nullptr, image_buffer->buffer_size);
    oidn_buffer_albedo = oidnNewSharedBufferFromWin32Handle(oidn_device, OIDN_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32, albedo_handle, nullptr, albedo_buffer->buffer_size);
    oidn_buffer_normal = oidnNewSharedBufferFromWin32Handle(oidn_device, OIDN_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32, normals_handle, nullptr, normals_buffer->buffer_size);

    oidn_filter = oidnNewFilter(oidn_device, "RT");
    oidnSetFilterImage(oidn_filter, "color", oidn_buffer, OIDN_FORMAT_FLOAT3, render_extent.width, render_extent.height, 0, sizeof(float) * 4, 0);
    oidnSetFilterImage(oidn_filter, "albedo", oidn_buffer_albedo, OIDN_FORMAT_FLOAT3, render_extent.width, render_extent.height, 0, sizeof(float) * 4, 0);
    oidnSetFilterImage(oidn_filter, "normals", oidn_buffer_normal, OIDN_FORMAT_FLOAT3, render_extent.width, render_extent.height, 0, sizeof(float) * 4, 0);
    oidnSetFilterImage(oidn_filter, "output", oidn_buffer, OIDN_FORMAT_FLOAT3, render_extent.width, render_extent.height, 0, sizeof(float) * 4, 0);
    oidnCommitFilter(oidn_filter);

    const char* error;
    if (oidnGetDeviceError(oidn_device, &error) != OIDN_ERROR_NONE) {
        std::cout << "OIDN Error: " << error << std::endl;
    }
}

void ProcessingPipelineStageOIDN::process(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent) {
    oidnExecuteFilter(oidn_filter);

    const char* error;
    if (oidnGetDeviceError(oidn_device, &error) != OIDN_ERROR_NONE) {
        std::cout << "OIDN Error: " << error << std::endl;
    }
}

void ProcessingPipelineStageOIDN::free() {
}