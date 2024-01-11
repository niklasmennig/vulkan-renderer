#include "pipeline/processing/pipeline_stage_oidn.h"

#include "pipeline/processing/pipeline_builder.h"
#include "pipeline/processing/compute_shader.h"

#include <iostream>

void ProcessingPipelineStageOIDN::initialize(ProcessingPipelineBuilder* builder) {
    oidn_device = oidnNewDevice(OIDN_DEVICE_TYPE_DEFAULT);
    oidnCommitDevice(oidn_device);
}

void ProcessingPipelineStageOIDN::on_resize(ProcessingPipelineBuilder* builder, VkExtent2D image_extent) {
    std::cout << "Processing Pipeline resized to " << image_extent.width << ", " << image_extent.height << std::endl;
    this->builder = builder;

    oidn_buffer = oidnNewSharedBufferFromWin32Handle(oidn_device, OIDN_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32, builder->image_memory_handle, nullptr, image_extent.width * image_extent.height * 4 * sizeof(float));
    oidn_buffer_albedo = oidnNewSharedBufferFromWin32Handle(oidn_device, OIDN_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32, builder->albedo_memory_handle, nullptr, image_extent.width * image_extent.height * 4 * sizeof(float));
    oidn_buffer_normal = oidnNewSharedBufferFromWin32Handle(oidn_device, OIDN_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32, builder->normal_memory_handle, nullptr, image_extent.width * image_extent.height * 4 * sizeof(float));

    oidn_filter = oidnNewFilter(oidn_device, "RT");
    oidnSetFilterImage(oidn_filter, "color", oidn_buffer, OIDN_FORMAT_FLOAT3, image_extent.width, image_extent.height, 0, 0, 0);
    oidnSetFilterImage(oidn_filter, "albedo", oidn_buffer_albedo, OIDN_FORMAT_FLOAT3, image_extent.width, image_extent.height, 0, 0, 0);
    oidnSetFilterImage(oidn_filter, "normal", oidn_buffer_normal, OIDN_FORMAT_FLOAT3, image_extent.width, image_extent.height, 0, 0, 0);
    oidnSetFilterImage(oidn_filter, "output", oidn_buffer, OIDN_FORMAT_FLOAT3, image_extent.width, image_extent.height, 0, 0, 0);
    oidnSetFilterBool(oidn_filter, "hdr", true);
    oidnCommitFilter(oidn_filter);

    const char* error;
    if (oidnGetDeviceError(oidn_device, &error) != OIDN_ERROR_NONE) {
        std::cout << "OIDN Error: " << error << std::endl;
    }
}

void ProcessingPipelineStageOIDN::process(VkCommandBuffer command_buffer) {
    oidnExecuteFilter(oidn_filter);
}

void ProcessingPipelineStageOIDN::free() {
}