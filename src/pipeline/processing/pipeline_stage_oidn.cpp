#include "pipeline/processing/pipeline_stage_oidn.h"

#include "pipeline/processing/pipeline_builder.h"
#include "pipeline/processing/compute_shader.h"

#include <iostream>

void ProcessingPipelineStageOIDN::initialize(ProcessingPipelineBuilder* builder) {
    output_image = &builder->input_image;

    oidn_device = oidn::newDevice();
    oidn_device.commit();
}

void ProcessingPipelineStageOIDN::on_resize(ProcessingPipelineBuilder* builder, VkExtent2D image_extent) {
    oidn_buffer_in = oidn_device.newBuffer(image_extent.width * image_extent.height * 3 * sizeof(float), oidn::Storage::Host);
    oidn_buffer_out = oidn_device.newBuffer(image_extent.width * image_extent.height * 3 * sizeof(float), oidn::Storage::Host);

    oidn_filter = oidn_device.newFilter("RT");
    oidn_filter.setImage("color", oidn_buffer_in, oidn::Format::Float3, image_extent.width, image_extent.height);
    oidn_filter.setImage("output", oidn_buffer_out, oidn::Format::Float3, image_extent.width, image_extent.height);
    oidn_filter.commit();

    const char* error;
    if (oidn_device.getError(error) != oidn::Error::None) {
        std::cout << "OIDN Error: " << error << std::endl;
    }
}

void ProcessingPipelineStageOIDN::process(VkCommandBuffer command_buffer) {
    // output_image->cmd_update_buffer(command_buffer);
    // output_image->cmd_transition_layout(command_buffer, output_image->layout, output_image->access);
    // float* oidn_color_ptr = (float*)oidn_buffer_in.getData();
    // vkMapMemory(output_image->device_handle, output_image->buffer.device_memory, output_image->buffer.device_memory_offset, output_image->buffer.buffer_size, 0, (void**)&oidn_color_ptr);
    // oidn_filter.execute();
    // vkUnmapMemory(output_image->device_handle, output_image->buffer.device_memory);
    // float* image_data;
    // float* oidn_denoised_ptr = (float*)oidn_buffer_out.getData();
    // vkMapMemory(output_image->device_handle, output_image->buffer.device_memory, output_image->buffer.device_memory_offset, output_image->buffer.buffer_size, 0, (void**)&image_data);
    // memcpy(image_data, oidn_denoised_ptr, output_image->buffer.buffer_size);
    // vkUnmapMemory(output_image->device_handle, output_image->buffer.device_memory);
    // output_image->cmd_update_image(command_buffer);
}