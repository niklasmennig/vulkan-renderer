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

Image* ProcessingPipelineBuilder::create_image(int width, int height) {
    created_images.push_back(device->create_image(width, height, VK_IMAGE_USAGE_STORAGE_BIT, 1, 1,VK_FORMAT_R8G8B8A8_UNORM, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, false));
    return &created_images.back();
}

ComputeShader* ProcessingPipelineBuilder::create_compute_shader(std::string path) {
    created_compute_shaders.push_back(device->create_compute_shader(path));
    return &created_compute_shaders.back();
}

void ProcessingPipelineBuilder::on_resize(VkExtent2D image_extent) {
    for (auto stage: stages) {
        stage->on_resize(this, image_extent);
    }
}

ProcessingPipelineBuilder ProcessingPipelineBuilder::with_stage(std::shared_ptr<ProcessingPipelineStage> stage) {
    stages.push_back(stage);
    return *this;
}

ProcessingPipeline ProcessingPipelineBuilder::build() {
    ProcessingPipeline result;
    result.device = device;
    result.builder = this;

    std::cout << "building with " << stages.size() << " stages" << std::endl;

    for (auto stage: stages) {
        stage->initialize(this);
    }

    return result;
}

void ProcessingPipelineBuilder::free() {
    for (auto image: created_images) {
        image.free();
    }

    for (auto shader: created_compute_shaders) {
        shader.free();
    }
}