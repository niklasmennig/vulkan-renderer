#pragma once

#include "core/vulkan.h"
#include "core/device.h"

#include <vector>
#include <memory>

struct ProcessingPipelineStage;
struct ProcessingPipelineBuilder;

struct ProcessingPipeline {
    Device* device;

    ProcessingPipelineBuilder* builder;

    void run(VkCommandBuffer command_buffer);

    void free();
};

struct ProcessingPipelineBuilder {
    Device* device;

    std::vector<std::shared_ptr<ProcessingPipelineStage>> stages;

    Image* input_image;
    std::vector<Image> created_images;
    std::vector<ComputeShader> created_compute_shaders;

    Image* create_image(int width, int height);
    ComputeShader* create_compute_shader(std::string path);

    ProcessingPipelineBuilder with_stage(std::shared_ptr<ProcessingPipelineStage> stage);

    void on_resize(VkExtent2D image_extent);
    ProcessingPipeline build();

    void free();
};