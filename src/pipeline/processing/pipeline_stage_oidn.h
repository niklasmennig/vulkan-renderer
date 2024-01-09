#pragma once

#include "pipeline/processing/pipeline_stage.h"

#include "core/buffer.h"
#include "core/device.h"

#include <OpenImageDenoise/oidn.hpp>

struct OutputBuffer;

struct ProcessingPipelineStageOIDN : ProcessingPipelineStage {
    OutputBuffer* output_buffer;

    oidn::DeviceRef oidn_device;
    oidn::FilterRef oidn_filter;

    oidn::BufferRef oidn_buffer_in;
    oidn::BufferRef oidn_buffer_out;

    void initialize(ProcessingPipelineBuilder* builder) override;
    void on_resize(ProcessingPipelineBuilder* builder, VkExtent2D image_extent) override;
    void process(VkCommandBuffer command_buffer) override;
    void free() override;
};