#pragma once

#include "core/vulkan.h"
#include "core/image.h"

#include <string>

struct Device;

struct ComputeShader {
    Device* device;

    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;

    VkPipeline pipeline;
    VkPipelineCache cache;

    VkPipelineLayout layout;
    uint8_t local_dispatch_size_x, local_dispatch_size_y, local_dispatch_size_z;

    std::string code_path;

    void set_image(int binding, Image* image);

    void build();
    void dispatch(VkCommandBuffer command_buffer, VkExtent2D image_extent);
    void dispatch(VkCommandBuffer command_buffer, uint32_t groups_x, uint32_t groups_y, uint32_t groups_z);
    void free();

    ComputeShader(Device* device, std::string code_path);
};