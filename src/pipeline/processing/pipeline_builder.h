struct Device;

#include "core/vulkan.h"

struct ProcessingPipeline {
    Device* device;

    VkPipeline pipeline;
    VkPipelineCache cache;

    void free();
};

struct ProcessingPipelineBuilder {
    Device* device;

    VkPipelineLayout layout;

    ProcessingPipeline build();

    void free();
};