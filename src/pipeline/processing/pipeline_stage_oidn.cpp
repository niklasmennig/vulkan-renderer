#include "pipeline/processing/pipeline_stage_oidn.h"

#include "pipeline/processing/pipeline_builder.h"
#include "pipeline/processing/compute_shader.h"

#include <iostream>

void ProcessingPipelineStageOIDN::initialize(ProcessingPipelineBuilder* builder) {
    output_image = builder->input_image;

    oidn_device = oidn::newDevice();
    oidn_device.commit();
}

void ProcessingPipelineStageOIDN::on_resize(ProcessingPipelineBuilder* builder, VkExtent2D image_extent) {
    oidn_buffer_in = oidn_device.newBuffer(image_extent.width * image_extent.height * 3 * sizeof(float), oidn::Storage::Host);
    oidn_buffer_out = oidn_device.newBuffer(image_extent.width * image_extent.height * 3 * sizeof(float), oidn::Storage::Host);

    device = builder->device;

    transfer_buffer = builder->device->create_buffer(image_extent.width * image_extent.height * 3 * sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false);

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
    // output_image->copy_image_to_buffer(command_buffer, transfer_buffer);

    // void* image_data;
    // vkMapMemory(transfer_buffer.device_handle, transfer_buffer.device_memory, transfer_buffer.device_memory_offset, transfer_buffer.buffer_size, 0, &image_data);
    // oidn_buffer_in.write(0, transfer_buffer.buffer_size, image_data);
    // vkUnmapMemory(transfer_buffer.device_handle, transfer_buffer.device_memory);
    // oidn_filter.execute();
    // vkMapMemory(transfer_buffer.device_handle, transfer_buffer.device_memory, transfer_buffer.device_memory_offset, transfer_buffer.buffer_size, 0, &image_data);
    // oidn_buffer_out.read(0, transfer_buffer.buffer_size, image_data);
    // vkUnmapMemory(transfer_buffer.device_handle, transfer_buffer.device_memory);

    // output_image->copy_buffer_to_image(cmdbuf, transfer_buffer);
}

void ProcessingPipelineStageOIDN::free() {
    transfer_buffer.free();
}