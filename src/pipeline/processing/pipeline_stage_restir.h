#pragma once

#include "pipeline/processing/pipeline_stage.h"
#include "pipeline/processing/compute_shader.h"

struct ProcessingPipelineStageRestir : ProcessingPipelineStage {
    ComputeShader* compute_shader;

    void initialize();
    void on_resize(VkExtent2D swapchain_extent, VkExtent2D render_extent);
    void process(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent);
    void free();
};