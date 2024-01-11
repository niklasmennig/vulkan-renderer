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

struct CreatedPipelineImage {
    Image image;
    VkExtent2D target_size;

    void resize(unsigned int width, unsigned int height);
};

struct ProcessingPipelineBuilder {
    Device* device;

    std::vector<std::shared_ptr<ProcessingPipelineStage>> stages;

    HANDLE image_memory_handle;
    HANDLE albedo_memory_handle;
    HANDLE normal_memory_handle;

    std::vector<CreatedPipelineImage> created_images;
    std::vector<ComputeShader> created_compute_shaders;

    CreatedPipelineImage* create_image(unsigned int width, unsigned int height);
    ComputeShader* create_compute_shader(std::string path);

    ProcessingPipelineBuilder with_stage(std::shared_ptr<ProcessingPipelineStage> stage);

    void cmd_on_resize(VkCommandBuffer command_buffer, VkExtent2D image_extent);
    ProcessingPipeline build();

    void free();
};