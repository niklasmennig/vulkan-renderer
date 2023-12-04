#pragma once

#include "core/vulkan.h"
#include "core/image.h"

struct Device;

struct ComputeShader {
    Device* device;

    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;

    VkPipeline pipeline;
    VkPipelineCache cache;

    VkPipelineLayout layout;
    uint8_t local_dispatch_size_x = 8, local_dispatch_size_y = 8;

    void set_image(int binding, Image& image);

    void build();
    void dispatch(VkCommandBuffer command_buffer, VkExtent2D image_extent);
    void free();
};