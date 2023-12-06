#include "pipeline/processing/pipeline_stage_simple.h"

#include "pipeline/processing/pipeline_builder.h"
#include "pipeline/processing/compute_shader.h"

#include <iostream>

void ProcessingPipelineStageSimple::initialize(ProcessingPipelineBuilder* builder) {
    std::cout << "CREATING PIPELINE IMAGE" << std::endl;
    image = builder->create_image(100, 100);

    compute_shader = builder->create_compute_shader("./shaders/processing/test.comp");
    compute_shader->build();
}

void ProcessingPipelineStageSimple::on_resize(ProcessingPipelineBuilder* builder, VkExtent2D image_extent) {
    output_image = builder->input_image;

    image->resize(image_extent.width / 2, image_extent.height / 2);
}

void ProcessingPipelineStageSimple::process(VkCommandBuffer command_buffer) {
    compute_shader->set_image(0, *output_image);
    compute_shader->set_image(1, image->image);
    compute_shader->dispatch(command_buffer, image->image.get_extents());
}