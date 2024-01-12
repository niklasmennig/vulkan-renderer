#pragma once

#include "core/vulkan.h"
#include "core/device.h"

#include <vector>
#include <memory>

struct ProcessingPipelineStage;
struct ProcessingPipelineBuilder;

struct RaytracingPipeline;

struct ProcessingPipeline {
    Device* device;

    ProcessingPipelineBuilder* builder;

    void run(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent);

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

    RaytracingPipeline* rt_pipeline;
    Buffer* output_buffer = nullptr;
    VkExtent2D output_extent;

    std::vector<CreatedPipelineImage> created_images;
    std::vector<ComputeShader> created_compute_shaders;
    std::vector<Buffer> created_buffers;

    CreatedPipelineImage* create_image(unsigned int width, unsigned int height);
    ComputeShader* create_compute_shader(std::string path);
    Buffer create_buffer(uint32_t size);

    ProcessingPipelineBuilder with_stage(std::shared_ptr<ProcessingPipelineStage> stage);

    void cmd_on_resize(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent);
    ProcessingPipeline build();

    void free();
};