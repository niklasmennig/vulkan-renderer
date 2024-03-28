#pragma once

#include "core/vulkan.h"
#include "shader_interface.h"

struct ProcessingPipelineBuilder;

// single processing pipeline stage
// uses compute shaders to process render data
struct ProcessingPipelineStage {
    ProcessingPipelineBuilder* builder;

    // called when renderer is resized
    virtual void on_resize(VkExtent2D swapchain_extent, VkExtent2D render_extent) = 0;

    // allocate data buffers, perform processor initialization
    virtual void initialize() = 0;

    // perform processing step
    virtual void process(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent, Shaders::PushConstants &push_constants) = 0;

    virtual void free() = 0;
};