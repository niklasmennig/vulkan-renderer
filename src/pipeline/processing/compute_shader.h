#pragma once

#include "core/vulkan.h"
#include "core/image.h"
#include "shader_interface.h"

#include <string>

struct Device;

struct ComputeShader {
    Device* device;

    VkDescriptorSetLayout descriptor_set_layout_buffers, descriptor_set_layout_images, descriptor_set_layout_as;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set_buffers, descriptor_set_images, descriptor_set_as;

    VkPipeline pipeline;
    VkPipelineCache cache;

    VkPipelineLayout layout;
    uint8_t local_dispatch_size_x, local_dispatch_size_y, local_dispatch_size_z;
    std::vector<uint8_t> buffer_descriptor_counts, image_descriptor_counts;
    uint8_t as_descriptor_count;

    std::string code_path;

    void set_image(int index, Image* image, int array_index = 0);
    void set_images(int index, std::vector<Image>* images);
    void set_buffer(int index, Buffer* buffer, int array_index = 0);
    void set_acceleration_structure(int index, VkAccelerationStructureKHR acceleration_structure);

    void build();
    void dispatch(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent, Shaders::PushConstantsPacked &push_constants_packed);
    void dispatch(VkCommandBuffer command_buffer, uint32_t groups_x, uint32_t groups_y, uint32_t groups_z, Shaders::PushConstantsPacked &push_constants_packed);
    void free();

    ComputeShader(Device* device, std::string code_path);
};