#include "buffer.h"

#include <cstring>
#include <stdexcept>
#include <iostream>

void Buffer::map(void* data) {
    if (vkMapMemory(device_handle, device_memory, device_memory_offset, buffer_size, 0, &data) != VK_SUCCESS)
    {
        throw std::runtime_error("error mapping buffer memory");
    }
}

void Buffer::unmap() {
    vkUnmapMemory(device_handle, device_memory);
}

void Buffer::set_data(void* data, size_t offset, size_t size) {
    if (size == 0) size = buffer_size - offset;
    void *buffer_data;
    if (vkMapMemory(device_handle, device_memory, device_memory_offset, buffer_size, 0, &buffer_data) != VK_SUCCESS)
    {
        throw std::runtime_error("error mapping buffer memory");
    }
    memcpy(((unsigned char*)buffer_data + offset), data, size);
    vkUnmapMemory(device_handle, device_memory);
}

void Buffer::get_data(void* data, size_t offset, size_t size) {
    void *buffer_data;
    if (vkMapMemory(device_handle, device_memory, 0, VK_WHOLE_SIZE, 0, (void**)&buffer_data) != VK_SUCCESS)
    {
        throw std::runtime_error("error mapping buffer memory");
    }
    memcpy(data, (unsigned char*)buffer_data + device_memory_offset + offset, size);
    vkUnmapMemory(device_handle, device_memory);
}

VkDeviceAddress Buffer::get_device_address() {
    VkBufferDeviceAddressInfo addr_info;
    addr_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addr_info.buffer = buffer_handle;
    addr_info.pNext = 0;

    return vkGetBufferDeviceAddress(device_handle, &addr_info);
}

void Buffer::free()
{
    vkDestroyBuffer(device_handle, buffer_handle, nullptr);
    if (shared) return;
    vkFreeMemory(device_handle, device_memory, nullptr);
}