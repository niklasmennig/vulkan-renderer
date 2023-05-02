#include "device.h"

#include "pipeline_builder.h"

#include <stdexcept>

Buffer Device::create_buffer(VkBufferCreateInfo *create_info)
{
    Buffer result{};
    result.device_handle = vulkan_device;

    create_info->usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (vkCreateBuffer(vulkan_device, create_info, nullptr, &result.buffer_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating buffer");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(vulkan_device, result.buffer_handle, &mem_requirements);

    // find correct memory type
    uint32_t type_filter = mem_requirements.memoryTypeBits;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    uint32_t memtype_index = 0;
    for (; memtype_index < memory_properties.memoryTypeCount; memtype_index++)
    {
        if ((type_filter & (1 << memtype_index)) && (memory_properties.memoryTypes[memtype_index].propertyFlags & properties) == properties)
        {
            break;
        }
    }

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = memtype_index;

    VkMemoryAllocateFlagsInfo alloc_flags{};
    alloc_flags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    alloc_flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    alloc_info.pNext = &alloc_flags;

    if (vkAllocateMemory(vulkan_device, &alloc_info, nullptr, &result.device_memory) != VK_SUCCESS)
    {
        throw std::runtime_error("error allocating buffer memory");
    }

    vkBindBufferMemory(vulkan_device, result.buffer_handle, result.device_memory, 0);

    result.buffer_size = create_info->size;

    VkBufferDeviceAddressInfo addr_info;
    addr_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addr_info.buffer = result.buffer_handle;
    addr_info.pNext = 0;

    result.device_address = vkGetBufferDeviceAddress(vulkan_device, &addr_info);

    return result;
}

PipelineBuilder Device::create_pipeline_builder() {
    PipelineBuilder res;
    res.device = this;

    return res;
}