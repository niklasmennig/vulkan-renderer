#include "pipeline/processing/pipeline_stage_upscale.h"

#include "pipeline/processing/pipeline_builder.h"
#include "pipeline/processing/compute_shader.h"

#include "pipeline/raytracing/pipeline_builder.h"

#include <iostream>

void ProcessingPipelineStageUpscale::initialize() {
    compute_shader = builder->create_compute_shader("./shaders/processing/upscale.comp");
    compute_shader->build();
}

void ProcessingPipelineStageUpscale::on_resize(VkExtent2D swapchain_extent, VkExtent2D render_extent) {
    output_buffer.free();
    output_buffer = builder->create_buffer(sizeof(float) * 4 * swapchain_extent.width * swapchain_extent.height);
}

void ProcessingPipelineStageUpscale::process(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent, Shaders::PushConstantsPacked &push_constants_packed) {
    compute_shader->set_buffer(0, builder->image_buffer);
    compute_shader->set_buffer(1, &output_buffer);
    compute_shader->dispatch(command_buffer, swapchain_extent, render_extent, push_constants_packed);

    builder->image_buffer = &output_buffer;
    builder->image_extent = swapchain_extent;
}

void ProcessingPipelineStageUpscale::free() {
    output_buffer.free();
}