#pragma once

#include <vector>

#include "vulkan.h"
#include "buffer.h"

#include "glm/vec3.hpp"
using vec3 = glm::vec3;

struct Device;

struct Image
{
    Device* device;

    uint32_t width, height;
    VkImageLayout layout;
    VkFormat format;
    VkAccessFlags access;
    VkMemoryRequirements memory_requirements;
    VkDeviceMemory texture_memory;
    VkDeviceSize texture_memory_offset;
    VkImage image_handle;
    VkImageView view_handle;
    VkSampler sampler_handle;

    bool shared_memory;

    static uint32_t bytes_per_channel(VkFormat format);
    static uint32_t num_channels(VkFormat format);
    static uint32_t pixel_byte_offset(VkFormat format, uint32_t x, uint32_t y, uint32_t widht, uint32_t height);
    static vec3 color_from_packed_data(VkFormat, unsigned char* data);

    void free();
    VkImageMemoryBarrier get_layout_transition(VkImageLayout target_layout, VkAccessFlags target_access);
    void transition_layout(VkCommandBuffer cmd_buffer, VkImageLayout target_layout, VkAccessFlags target_access = 0);
    void transition_layout(VkImageLayout target_layout, VkAccessFlags target_access = 0);
    void cmd_blit_image(VkCommandBuffer cmd_buffer, Image src_image);
    void copy_buffer_to_image(VkCommandBuffer cmd_buffer, Buffer buffer);
    void copy_buffer_to_image(Buffer buffer);
    void copy_image_to_buffer(VkCommandBuffer cmd_buffer, Buffer buffer);
    void copy_image_to_buffer(Buffer buffer);

    VkExtent2D get_extents();

    Image();
};
