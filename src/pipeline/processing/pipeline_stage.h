#include "core/vulkan.h"

struct ProcessingPipelineBuilder;

// single processing pipeline stage
// uses compute shaders to process render data
struct ProcessingPipelineStage {
    // called when renderer is resized
    virtual void on_resize(ProcessingPipelineBuilder* builder, VkExtent2D image_extent) = 0;

    // allocate data buffers, perform processor initialization
    virtual void initialize(ProcessingPipelineBuilder* builder) = 0;

    // perform processing step
    virtual void process(VkCommandBuffer command_buffer) = 0;
};