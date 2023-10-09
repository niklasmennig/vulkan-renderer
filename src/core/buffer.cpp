#include "buffer.h"

#include <cstring>
#include <stdexcept>
#include <iostream>

void Buffer::set_data(void* data, size_t offset, size_t size) {
    if (size == 0) size = buffer_size - offset;
    unsigned char *buffer_data;
    if (vkMapMemory(device_handle, device_memory, 0, VK_WHOLE_SIZE, 0, (void**)&buffer_data) != VK_SUCCESS)
    {
        throw std::runtime_error("error mapping buffer memory");
    }
    memcpy(buffer_data + device_memory_offset + offset, data, size);
    vkUnmapMemory(device_handle, device_memory);
}

void Buffer::get_data(void* data, size_t offset, size_t size) {
    unsigned char *buffer_data;
    if (vkMapMemory(device_handle, device_memory, 0, VK_WHOLE_SIZE, 0, (void**)&buffer_data) != VK_SUCCESS)
    {
        throw std::runtime_error("error mapping buffer memory");
    }
    memcpy(data, buffer_data + device_memory_offset + offset, size);
    vkUnmapMemory(device_handle, device_memory);
}

void Buffer::free()
{
    vkDestroyBuffer(device_handle, buffer_handle, nullptr);
    if (shared) return;
    vkFreeMemory(device_handle, device_memory, nullptr);
}