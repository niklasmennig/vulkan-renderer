#include "pipeline/processing/pipeline_stage_restir.h"

#include "pipeline/processing/pipeline_builder.h"
#include "pipeline/processing/compute_shader.h"

#include "pipeline/raytracing/pipeline_builder.h"

#include <iostream>

void ProcessingPipelineStageRestir::initialize() {
    compute_shader = builder->create_compute_shader("./shaders/processing/restir_spatial.comp");
    compute_shader->build();
}

void ProcessingPipelineStageRestir::on_resize(VkExtent2D swapchain_extent, VkExtent2D render_extent) {
    compute_shader->set_buffer(1, &builder->rt_pipeline->get_output_buffer("Position").buffer, 0);
    compute_shader->set_buffer(1, &builder->rt_pipeline->get_output_buffer("Normals").buffer, 1);
}

void ProcessingPipelineStageRestir::process(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent, Shaders::PushConstants &push_constants) {
    Buffer* image_buffer = &builder->rt_pipeline->get_output_buffer("Result Image").buffer;
    compute_shader->set_buffer(0, image_buffer);
    
    // compute_shader->set_buffer(1, &output_buffer);
    compute_shader->dispatch(command_buffer, swapchain_extent, render_extent, push_constants);
}

void ProcessingPipelineStageRestir::free() {
    
}