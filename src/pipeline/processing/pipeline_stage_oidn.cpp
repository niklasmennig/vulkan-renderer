#include "pipeline/processing/pipeline_stage_oidn.h"

#include "pipeline/processing/pipeline_builder.h"
#include "pipeline/processing/compute_shader.h"

#include <iostream>

void ProcessingPipelineStageOIDN::initialize(ProcessingPipelineBuilder* builder) {
    output_image = &builder->input_image;

    copy_in_shader = builder->create_compute_shader("shaders/processing/copy_in.comp");
    copy_out_shader = builder->create_compute_shader("shaders/processing/copy_out.comp");

    oidn_device = oidn::newDevice();
    oidn_device.commit();
}

void ProcessingPipelineStageOIDN::on_resize(ProcessingPipelineBuilder* builder, VkExtent2D image_extent) {

    oidn_color_buf = oidn_device.newBuffer(image_extent.width * image_extent.height * 3 * sizeof(float), oidn::Storage::Managed);

    oidn_filter = oidn_device.newFilter("RT");
    oidn_filter.setImage("color", oidn_color_buf, oidn::Format::Float3, image_extent.width, image_extent.height);
    oidn_filter.set("hdr", true);
    oidn_filter.commit();
}

void ProcessingPipelineStageOIDN::process(VkCommandBuffer command_buffer) {
    std::cout << "EXECUTING OIDN STAGE" << std::endl;
    float* oidn_color_ptr = (float*)oidn_color_buf.getData();
    copy_in_shader->dispatch(command_buffer, output_image->get_extents());

    // implement functionality to set buffers in compute shaders
    // use compute shader to copy image colors to OIDN buffer
    copy_in_shader.set_buffer(0, oidn_color_ptr);
    oidn_filter.execute();
    copy_out_shader->dispatch(command_buffer, output_image->get_extents());
    // use other compute shader to copy OIDN filtered color data to output image
    copy_out_shader.set_buffer(0, oidn_color_ptr);
}