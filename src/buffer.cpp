#include "buffer.h"

#include <cstring>
#include <stdexcept>

void Buffer::set_data(void* data) {
    set_data(data, 0, VK_WHOLE_SIZE);
}

void Buffer::set_data(void* data, size_t offset, size_t size) {
    void *buffer_data;
    if (vkMapMemory(device_handle, device_memory, offset, size, 0, &buffer_data) != VK_SUCCESS)
    {
        throw std::runtime_error("error mapping buffer memory");
    }
    memcpy(buffer_data, data, (size_t)buffer_size);
    vkUnmapMemory(device_handle, device_memory);
}

void Buffer::free()
{
    vkDestroyBuffer(device_handle, buffer_handle, nullptr);
    vkFreeMemory(device_handle, device_memory, nullptr);
}