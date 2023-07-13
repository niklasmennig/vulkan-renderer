#pragma once
#include "vulkan.h"

struct Buffer
{
    size_t buffer_size;
    VkBuffer buffer_handle;
    VkDevice device_handle;
    VkDeviceMemory device_memory;
    VkDeviceAddress device_address;

    void set_data(void *data, size_t offset = 0, size_t size = VK_WHOLE_SIZE);
    void get_data(void *data, size_t offset = 0, size_t size = VK_WHOLE_SIZE);
    void free();
};