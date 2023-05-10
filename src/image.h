#pragma once

#include "vulkan.h"
#include "buffer.h"

struct Image
{
    VkDevice device_handle;
    uint32_t width, height;
    Buffer buffer;
    VkDeviceMemory texture_memory;
    VkImage image_handle;
    VkImageView view_handle;
    VkSampler sampler_handle;

    void free();
    void cmd_setup_texture(VkCommandBuffer cmd_buffer);
};
