#pragma once

#include "core/vulkan.h"
#include "core/image.h"

#include <string>

struct Device;

struct ComputeShader {
    Device* device;

    VkDescriptorSetLayout descriptor_set_layout_buffers, descriptor_set_layout_images;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set_buffers, descriptor_set_images;

    VkPipeline pipeline;
    VkPipelineCache cache;

    VkPipelineLayout layout;
    uint8_t local_dispatch_size_x, local_dispatch_size_y, local_dispatch_size_z;
    std::vector<uint8_t> buffer_descriptor_counts, image_descriptor_counts;

    std::string code_path;

    void set_image(int index, Image* image, int array_index = 0);
    void set_buffer(int index, Buffer* buffer, int array_index = 0);

    void build();
    void dispatch(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent);
    void dispatch(VkCommandBuffer command_buffer, uint32_t groups_x, uint32_t groups_y, uint32_t groups_z);
    void free();

    ComputeShader(Device* device, std::string code_path);
};