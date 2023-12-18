#include "pipeline_builder.h"

#include "core/device.h"
#include "loaders/shader_spirv.h"
#include "shader_compiler.h"
#include "pipeline/processing/compute_shader.h"
#include "pipeline/processing/pipeline_stage.h"

#include <array>
#include <iostream>

ProcessingPipelineBuilder Device::create_processing_pipeline_builder() {
    ProcessingPipelineBuilder builder;
    builder.device = this;

    return builder;
}

void ProcessingPipeline::run(VkCommandBuffer command_buffer) {
    for (auto stage: builder->stages) {
        stage->process(command_buffer);
    }
}

void ProcessingPipeline::free() {
    
}

void CreatedPipelineImage::resize(unsigned int width, unsigned int height) {
    target_size.width = width;
    target_size.height = height;
}

CreatedPipelineImage* ProcessingPipelineBuilder::create_image(unsigned int width, unsigned int height) {
    created_images.push_back( CreatedPipelineImage {
        device->create_image(width, height, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 1, 1,VK_FORMAT_R8G8B8A8_UNORM, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, false),
        VkExtent2D{width, height}
    });
    return &created_images.back();
}

ComputeShader* ProcessingPipelineBuilder::create_compute_shader(std::string path) {
    created_compute_shaders.push_back(device->create_compute_shader(path));
    return &created_compute_shaders.back();
}

void ProcessingPipelineBuilder::cmd_on_resize(VkCommandBuffer command_buffer, VkExtent2D image_extent) {
    for (auto stage: stages) {
        stage->on_resize(this, image_extent);
    }

    for (auto& created_image: created_images) {
        if (created_image.target_size.width != created_image.image.width || created_image.target_size.height != created_image.image.height || created_image.image.layout == VK_IMAGE_LAYOUT_PREINITIALIZED) {
            // resize created image to target size
            std::cout << "resizing pipeline image" << std::endl;
            created_image.image.free();
            created_image.image = device->create_image(created_image.target_size.width, created_image.target_size.height, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 1, 1,VK_FORMAT_R8G8B8A8_UNORM, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, false);
            created_image.image.cmd_transition_layout(command_buffer, VK_IMAGE_LAYOUT_GENERAL, 0);
        }
    }
}

ProcessingPipelineBuilder ProcessingPipelineBuilder::with_stage(std::shared_ptr<ProcessingPipelineStage> stage) {
    std::cout << "adding stage" << std::endl;
    stages.push_back(stage);
    return *this;
}

ProcessingPipeline ProcessingPipelineBuilder::build() {
    ProcessingPipeline result;
    result.device = device;
    result.builder = this;

    std::cout << "building with " << stages.size() << " stages" << std::endl;

    for (auto stage: stages) {
        std::cout << "INITIALIZING STAGE" << std::endl;
        stage->initialize(this);
    }

    return result;
}

void ProcessingPipelineBuilder::free() {
    for (auto created_image: created_images) {
        created_image.image.free();
    }

    for (auto shader: created_compute_shaders) {
        shader.free();
    }
}