#pragma once

#include "vulkan.h"
#include "buffer.h"

#include "glm/vec3.hpp"
using ivec3 = glm::ivec3;

struct Image
{
    VkDevice device_handle;
    uint32_t width, height;
    VkImageLayout layout;
    VkFormat format;
    VkAccessFlags access;
    Buffer buffer;
    VkDeviceMemory texture_memory;
    VkImage image_handle;
    VkImageView view_handle;
    VkSampler sampler_handle;

    void free();
    void cmd_transition_layout(VkCommandBuffer cmd_buffer, VkImageLayout target_layout, VkAccessFlags target_access);
    void cmd_setup_texture(VkCommandBuffer cmd_buffer);
    void cmd_update_buffer(VkCommandBuffer cmd_buffer);
    ivec3 get_pixel(uint32_t x, uint32_t y);
};
