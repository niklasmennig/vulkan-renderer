#pragma once

#include "vulkan.h"
#include "buffer.h"

struct Image
{
    VkDevice device_handle;
    uint32_t width, height;
    VkImageLayout layout;
    VkAccessFlags access;
    Buffer buffer;
    VkDeviceMemory texture_memory;
    VkImage image_handle;
    VkImageView view_handle;
    VkSampler sampler_handle;

    void free();
    void cmd_transition_layout(VkCommandBuffer cmd_buffer, VkImageLayout target_layout, VkAccessFlags target_access);
    void cmd_setup_texture(VkCommandBuffer cmd_buffer);
};
