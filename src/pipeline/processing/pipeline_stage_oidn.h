#pragma once

#include "pipeline/processing/pipeline_stage.h"

#include "core/buffer.h"
#include "core/device.h"

#include <OpenImageDenoise/oidn.hpp>

struct Image;
struct ComputeShader;
struct CreatedPipelineImage;

struct ProcessingPipelineStageOIDN : ProcessingPipelineStage {
    ProcessingPipelineBuilder* builder;

    OIDNDevice oidn_device;
    OIDNFilter oidn_filter;
    OIDNBuffer oidn_buffer, oidn_buffer_albedo, oidn_buffer_normal;

    void initialize(ProcessingPipelineBuilder* builder) override;
    void on_resize(ProcessingPipelineBuilder* builder, VkExtent2D image_extent) override;
    void process(VkCommandBuffer command_buffer) override;
    void free() override;
};