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
    float multiplier = 1.0f;

    vec3 get_pixel(uint32_t x, uint32_t y);
};

struct Image
{
    VkDevice device_handle;
    uint32_t width, height;
    VkImageLayout layout;
    VkFormat format;
    VkAccessFlags access;
    Buffer buffer;
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
    void cmd_transition_layout(VkCommandBuffer cmd_buffer, VkImageLayout target_layout, VkAccessFlags target_access);
    void cmd_setup_texture(VkCommandBuffer cmd_buffer);
    void cmd_update_buffer(VkCommandBuffer cmd_buffer);
    void cmd_update_image(VkCommandBuffer cmd_buffer);
    void cmd_blit_image(VkCommandBuffer cmd_buffer, Image src_image);

    VkExtent2D get_extents();
    vec3 get_pixel(uint32_t x, uint32_t y);
    ImagePixels get_pixels();
};
