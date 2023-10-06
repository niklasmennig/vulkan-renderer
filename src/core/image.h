#pragma once

#include <vector>

#include "vulkan.h"
#include "buffer.h"

#include "glm/vec3.hpp"
using vec3 = glm::vec3;

struct ImagePixels {
    std::vector<unsigned char> data;
    VkFormat format;
    uint32_t width, height;

    vec3 get_pixel(uint32_t x, uint32_t y);
};

struct Image
{
    VkDevice device_handle;
    uint32_t width, height, channels;
    VkImageLayout layout;
    VkFormat format;
    VkAccessFlags access;
    Buffer buffer;
    VkDeviceMemory texture_memory;
    VkImage image_handle;
    VkImageView view_handle;
    VkSampler sampler_handle;

    static uint32_t bytes_per_channel(VkFormat format);
    static uint32_t pixel_byte_offset(VkFormat format, uint32_t x, uint32_t y, uint32_t widht, uint32_t height);

    void free();
    VkImageMemoryBarrier get_layout_transition(VkImageLayout target_layout, VkAccessFlags target_access);
    void cmd_transition_layout(VkCommandBuffer cmd_buffer, VkImageLayout target_layout, VkAccessFlags target_access);
    void cmd_setup_texture(VkCommandBuffer cmd_buffer);
    void cmd_update_buffer(VkCommandBuffer cmd_buffer);
    vec3 get_pixel(uint32_t x, uint32_t y);
    ImagePixels get_pixels();
};
