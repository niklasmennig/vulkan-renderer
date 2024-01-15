#include "pipeline/processing/pipeline_stage_oidn.h"

#include "pipeline/processing/pipeline_builder.h"
#include "pipeline/processing/compute_shader.h"

#include "pipeline/raytracing/pipeline_builder.h"

#include "core/vulkan.h"

#include <iostream>

void ProcessingPipelineStageOIDN::initialize() {
    oidn_device = oidnNewDevice(OIDN_DEVICE_TYPE_DEFAULT);
    oidnCommitDevice(oidn_device);
}

void ProcessingPipelineStageOIDN::on_resize(VkExtent2D swapchain_extent, VkExtent2D render_extent) {
    this->builder = builder;

    VkMemoryGetWin32HandleInfoKHR handle_info{};
    handle_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    handle_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    handle_info.memory = builder->rt_pipeline->get_output_buffer("Result Image").buffer.device_memory;
    HANDLE image_handle;
    builder->device->vkGetMemoryWin32HandleKHR(builder->device->vulkan_device, &handle_info, &image_handle);

    
    handle_info.memory = builder->rt_pipeline->get_output_buffer("Albedo").buffer.device_memory;
    HANDLE albedo_handle;
    builder->device->vkGetMemoryWin32HandleKHR(builder->device->vulkan_device, &handle_info, &albedo_handle);

    handle_info.memory = builder->rt_pipeline->get_output_buffer("Normals").buffer.device_memory;
    HANDLE normals_handle;
    builder->device->vkGetMemoryWin32HandleKHR(builder->device->vulkan_device, &handle_info, &normals_handle);

    std::cout << "HANDLES: " << image_handle << " " << albedo_handle << " " << normals_handle << std::endl;

    // oidn_buffer = oidnNewSharedBufferFromWin32Handle(oidn_device, OIDN_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32, image_handle, nullptr, render_extent.width * render_extent.height * 4 * sizeof(float));
    // oidn_buffer_albedo = oidnNewSharedBufferFromWin32Handle(oidn_device, OIDN_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32, albedo_handle, nullptr, render_extent.width * render_extent.height * 4 * sizeof(float));
    // oidn_buffer_normal = oidnNewSharedBufferFromWin32Handle(oidn_device, OIDN_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32, normals_handle, nullptr, render_extent.width * render_extent.height * 4 * sizeof(float));

    // oidn_filter = oidnNewFilter(oidn_device, "RT");
    // oidnSetFilterImage(oidn_filter, "color", oidn_buffer, OIDN_FORMAT_FLOAT3, render_extent.width, render_extent.height, 0, 0, 0);
    // oidnSetFilterImage(oidn_filter, "albedo", oidn_buffer_albedo, OIDN_FORMAT_FLOAT3, render_extent.width, render_extent.height, 0, 0, 0);
    // oidnSetFilterImage(oidn_filter, "normal", oidn_buffer_normal, OIDN_FORMAT_FLOAT3, render_extent.width, render_extent.height, 0, 0, 0);
    // oidnSetFilterImage(oidn_filter, "output", oidn_buffer, OIDN_FORMAT_FLOAT3, render_extent.width, render_extent.height, 0, 0, 0);
    // oidnSetFilterBool(oidn_filter, "hdr", true);
    // oidnCommitFilter(oidn_filter);

    const char* error;
    if (oidnGetDeviceError(oidn_device, &error) != OIDN_ERROR_NONE) {
        std::cout << "OIDN Error: " << error << std::endl;
    }
}

void ProcessingPipelineStageOIDN::process(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent) {
    oidnExecuteFilter(oidn_filter);
}

void ProcessingPipelineStageOIDN::free() {
}