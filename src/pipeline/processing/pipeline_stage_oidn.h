#pragma once

#include "pipeline/processing/pipeline_stage.h"

#include <OpenImageDenoise/oidn.hpp>

struct Image;
struct ComputeShader;
struct CreatedPipelineImage;

struct ProcessingPipelineStageOIDN : ProcessingPipelineStage {
    Image* output_image;

    ComputeShader* copy_in_shader;
    ComputeShader* copy_out_shader;

    oidn::DeviceRef oidn_device;
    oidn::BufferRef oidn_color_buf;
    oidn::FilterRef oidn_filter;

    void initialize(ProcessingPipelineBuilder* builder) override;
    void on_resize(ProcessingPipelineBuilder* builder, VkExtent2D image_extent) override;
    void process(VkCommandBuffer command_buffer) override;
};