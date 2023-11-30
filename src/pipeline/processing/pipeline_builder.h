#pragma once

struct Device;
struct ProcessingPipelineBuilder;

#include "core/vulkan.h"

struct ProcessingPipeline {
    Device* device;

    ProcessingPipelineBuilder* builder;

    VkPipeline pipeline;
    VkPipelineCache cache;

    void free();
};

struct ProcessingPipelineBuilder {
    Device* device;

    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;

    VkPipelineLayout layout;

    ProcessingPipeline build();

    void free();
};