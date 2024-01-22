#pragma once

#include "pipeline/processing/pipeline_stage.h"

#include "core/buffer.h"
#include "core/device.h"

#include <OpenImageDenoise/oidn.hpp>

struct OutputBuffer;

struct ProcessingPipelineStageOIDN : ProcessingPipelineStage {
    Buffer* output_buffer, test_buffer;

    OIDNDevice oidn_device;
    OIDNFilter oidn_filter;
    OIDNBuffer oidn_buffer, oidn_buffer_albedo, oidn_buffer_normal, oidn_buffer_output;

    void initialize() override;
    void on_resize(VkExtent2D swapchain_extent, VkExtent2D render_extent) override;
    void process(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent) override;
    void free() override;
};