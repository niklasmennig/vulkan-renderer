#pragma once
#include "vulkan.h"

enum BufferType {
    Uniform = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    Storage = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
};

struct Buffer
{
    size_t buffer_size;
    VkBuffer buffer_handle;
    VkDevice device_handle;
    VkDeviceMemory device_memory;
    VkDeviceAddress device_address;

    void set_data(void *data);
    void free();
};