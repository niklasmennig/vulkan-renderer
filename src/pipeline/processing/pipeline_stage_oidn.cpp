#include "pipeline/processing/pipeline_stage_oidn.h"

#include "pipeline/processing/pipeline_builder.h"
#include "pipeline/processing/compute_shader.h"

#include "pipeline/raytracing/pipeline_builder.h"

#include <iostream>

void ProcessingPipelineStageOIDN::initialize(ProcessingPipelineBuilder* builder) {
    oidn_device = oidn::newDevice();
    oidn_device.commit();
}

void ProcessingPipelineStageOIDN::on_resize(ProcessingPipelineBuilder* builder, VkExtent2D image_extent) {
    output_buffer = builder->output_buffer;

    oidn_buffer_in = oidn_device.newBuffer(image_extent.width * image_extent.height * 4 * sizeof(float), oidn::Storage::Host);
    oidn_buffer_out = oidn_device.newBuffer(image_extent.width * image_extent.height * 4 * sizeof(float), oidn::Storage::Host);

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
    void* image_data;
    vkMapMemory(output_buffer->buffer.device_handle, output_buffer->buffer.device_memory, output_buffer->buffer.device_memory_offset, output_buffer->buffer.buffer_size, 0, &image_data);
    oidn_buffer_in.write(0, output_buffer->buffer.buffer_size, image_data);
    vkUnmapMemory(output_buffer->buffer.device_handle, output_buffer->buffer.device_memory);
    oidn_filter.execute();
    vkMapMemory(output_buffer->buffer.device_handle, output_buffer->buffer.device_memory, output_buffer->buffer.device_memory_offset, output_buffer->buffer.buffer_size, 0, &image_data);
    oidn_buffer_out.read(0, output_buffer->buffer.buffer_size, image_data);
    vkUnmapMemory(output_buffer->buffer.device_handle, output_buffer->buffer.device_memory);
}

void ProcessingPipelineStageOIDN::free() {
    
}