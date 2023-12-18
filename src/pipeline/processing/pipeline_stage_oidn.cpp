#include "pipeline/processing/pipeline_stage_oidn.h"

#include "pipeline/processing/pipeline_builder.h"
#include "pipeline/processing/compute_shader.h"

#include <iostream>

void ProcessingPipelineStageOIDN::initialize(ProcessingPipelineBuilder* builder) {
    std::cout << "CREATING PIPELINE IMAGE" << std::endl;
    image = builder->create_image(100, 100);
    output_image = &builder->input_image;

    oidn_device = oidn::newDevice();
    oidn_device.commit();
}

void ProcessingPipelineStageOIDN::on_resize(ProcessingPipelineBuilder* builder, VkExtent2D image_extent) {
    image->resize(image_extent.width / 2, 100);

    oidn_color_buf = oidn_device.newBuffer(image_extent.width * image_extent.height * 3 * sizeof(float), oidn::Storage::Managed);

    oidn_filter = oidn_device.newFilter("RT");
    oidn_filter.setImage("color", oidn_color_buf, oidn::Format::Float3, image_extent.width, image_extent.height);
    oidn_filter.set("hdr", true);
    oidn_filter.commit();
}

void ProcessingPipelineStageOIDN::process(VkCommandBuffer command_buffer) {
    std::cout << "EXECUTING OIDN STAGE" << std::endl;
    float* oidn_color_ptr = (float*)oidn_color_buf.getData();
    output_image->cmd_update_buffer(command_buffer);
    output_image->buffer.get_data(oidn_color_ptr, 0, output_image->width * output_image->height * 3 * sizeof(float));
    oidn_filter.execute();
    output_image->buffer.set_data(oidn_color_ptr);
    output_image->cmd_update_image(command_buffer);
}