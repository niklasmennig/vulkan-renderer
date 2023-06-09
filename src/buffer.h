#pragma once
#include "vulkan.h"

struct Buffer
{
    size_t buffer_size;
    VkBuffer buffer_handle;
    VkDevice device_handle;
    VkDeviceMemory device_memory;
    VkDeviceAddress device_address;

    void set_data(void *data);
    void set_data(void *data, size_t offset, size_t size);
    void free();
};