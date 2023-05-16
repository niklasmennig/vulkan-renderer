#include "buffer.h"

#include <cstring>
#include <stdexcept>

void* Buffer::map() {
    void *buffer_data;
    if (vkMapMemory(device_handle, device_memory, 0, VK_WHOLE_SIZE, 0, &buffer_data) != VK_SUCCESS)
    {
        throw std::runtime_error("error mapping buffer memory");
    }
    return buffer_data;
}

void Buffer::unmap() {
    vkUnmapMemory(device_handle, device_memory);
}

void Buffer::set_data(void* data) {
    void* buffer_data = map();
    memcpy(buffer_data, data, (size_t)buffer_size);
    unmap();
}

void Buffer::free()
{
    vkDestroyBuffer(device_handle, buffer_handle, nullptr);
    vkFreeMemory(device_handle, device_memory, nullptr);
}