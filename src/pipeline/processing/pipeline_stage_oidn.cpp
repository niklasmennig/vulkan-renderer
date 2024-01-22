#include "pipeline/processing/pipeline_stage_oidn.h"

#include "pipeline/processing/pipeline_builder.h"
#include "pipeline/processing/compute_shader.h"

#include "pipeline/raytracing/pipeline_builder.h"

#include "core/vulkan.h"

#include <iostream>

void ProcessingPipelineStageOIDN::initialize() {
    oidn_device = oidnNewDeviceByID(0);
    oidnCommitDevice(oidn_device);

    std::cout << "OIDN Device:" << oidnGetPhysicalDeviceString(0, "name") << std::endl;
    std::cout << "OIDN Device Type:" << oidnGetPhysicalDeviceInt(0, "type") << std::endl;
}

void ProcessingPipelineStageOIDN::on_resize(VkExtent2D swapchain_extent, VkExtent2D render_extent) {
    test_buffer = builder->create_buffer(sizeof(float) * 4 * 100000);

    VkMemoryGetWin32HandleInfoKHR handle_info{};
    handle_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    handle_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    Buffer* image_buffer = &builder->rt_pipeline->get_output_buffer("Result Image").buffer;
    handle_info.memory = image_buffer->device_memory;
    HANDLE image_handle;
    builder->device->vkGetMemoryWin32HandleKHR(builder->device->vulkan_device, &handle_info, &image_handle);
    oidn_buffer = oidnNewSharedBufferFromWin32Handle(oidn_device, OIDN_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32, &image_handle, nullptr, render_extent.width * render_extent.height * 4 * sizeof(float));

    Buffer* albedo_buffer = &builder->rt_pipeline->get_output_buffer("Albedo").buffer;
    handle_info.memory = albedo_buffer->device_memory;
    HANDLE albedo_handle;
    builder->device->vkGetMemoryWin32HandleKHR(builder->device->vulkan_device, &handle_info, &albedo_handle);
    oidn_buffer_albedo = oidnNewSharedBufferFromWin32Handle(oidn_device, OIDN_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32, &albedo_handle, nullptr, render_extent.width * render_extent.height * 4 * sizeof(float));

    Buffer* normal_buffer = &builder->rt_pipeline->get_output_buffer("Normals").buffer;
    handle_info.memory = normal_buffer->device_memory;
    HANDLE normals_handle;
    builder->device->vkGetMemoryWin32HandleKHR(builder->device->vulkan_device, &handle_info, &normals_handle);
    oidn_buffer_normal = oidnNewSharedBufferFromWin32Handle(oidn_device, OIDN_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32, &normals_handle, nullptr, render_extent.width * render_extent.height * 4 * sizeof(float));

    output_buffer = &builder->rt_pipeline->get_output_buffer("Ray Depth").buffer;
    handle_info.memory = output_buffer->device_memory;
    HANDLE output_handle;
    builder->device->vkGetMemoryWin32HandleKHR(builder->device->vulkan_device, &handle_info, &output_handle);
    oidn_buffer_output = oidnNewSharedBufferFromWin32Handle(oidn_device, OIDN_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32, &output_handle, nullptr, render_extent.width * render_extent.height * 4 * sizeof(float));

    std::cout << "HANDLES: " << image_handle << " " << albedo_handle << " " << normals_handle << std::endl;

    oidn_filter = oidnNewFilter(oidn_device, "RT");
    oidnSetFilterImage(oidn_filter, "color", oidn_buffer, OIDN_FORMAT_FLOAT3, render_extent.width, render_extent.height, 0, 4 * sizeof(float), 0);
    oidnSetFilterImage(oidn_filter, "albedo", oidn_buffer_albedo, OIDN_FORMAT_FLOAT3, render_extent.width, render_extent.height, 0, 4 * sizeof(float), 0);
    oidnSetFilterImage(oidn_filter, "normal", oidn_buffer_normal, OIDN_FORMAT_FLOAT3, render_extent.width, render_extent.height, 0, 4 * sizeof(float), 0);
    oidnSetFilterImage(oidn_filter, "output", oidn_buffer_output, OIDN_FORMAT_FLOAT3, render_extent.width, render_extent.height, 0, 4 * sizeof(float), 0);
    oidnSetFilterBool(oidn_filter, "hdr", true);
    oidnCommitFilter(oidn_filter);

    builder->output_buffer = output_buffer;
    builder->output_extent = render_extent;

    const char* error;
    if (oidnGetDeviceError(oidn_device, &error) != OIDN_ERROR_NONE) {
        std::cout << "OIDN Error: " << error << std::endl;
    }
}

void ProcessingPipelineStageOIDN::process(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent) {
    oidnExecuteFilter(oidn_filter);

    VkCommandBuffer cmdbuf = builder->device->begin_single_use_command_buffer();

    VkBufferCopy copy{};
    copy.srcOffset = 0;
    copy.dstOffset = 0;
    copy.size = sizeof(float) * 4 * 100000;

    vkCmdCopyBuffer(cmdbuf, test_buffer.buffer_handle, output_buffer->buffer_handle, 1, &copy);

    builder->device->end_single_use_command_buffer(cmdbuf);

    const char* error;
    if (oidnGetDeviceError(oidn_device, &error) != OIDN_ERROR_NONE) {
        std::cout << "OIDN Error: " << error << std::endl;
    }

}

void ProcessingPipelineStageOIDN::free() {
}