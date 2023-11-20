#pragma once
#include "vulkan.h"

struct Buffer
{
    size_t buffer_size;
    VkBuffer buffer_handle = VK_NULL_HANDLE;
    VkDevice device_handle;
    VkDeviceMemory device_memory;
    VkDeviceSize device_memory_offset;
    VkDeviceAddress device_address;

    bool shared;

    void set_data(void *data, size_t offset = 0, size_t size = 0);
    void get_data(void *data, size_t offset, size_t size);
    void free();
};