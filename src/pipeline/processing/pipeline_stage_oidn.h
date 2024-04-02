#pragma once

#include "pipeline/processing/pipeline_stage.h"

#include "core/buffer.h"
#include "core/device.h"

#include <OpenImageDenoise/oidn.hpp>

struct ProcessingPipelineStageOIDN : ProcessingPipelineStage {
    OIDNDevice oidn_device;
    OIDNFilter oidn_filter;
    OIDNBuffer oidn_buffer, oidn_buffer_albedo, oidn_buffer_normal;

    void initialize() override;
    void on_resize(VkExtent2D swapchain_extent, VkExtent2D render_extent) override;
    void process(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent, Shaders::PushConstantsPacked &push_constants_packed) override;
    void free() override;
};