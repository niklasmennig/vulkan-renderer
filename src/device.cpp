#include "device.h"

#include "pipeline_builder.h"

#include <stdexcept>

uint32_t Device::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    uint32_t memtype_index = 0;
    for (; memtype_index < memory_properties.memoryTypeCount; memtype_index++)
    {
        if ((type_filter & (1 << memtype_index)) && (memory_properties.memoryTypes[memtype_index].propertyFlags & properties) == properties)
        {
            break;
        }
    }

    return memtype_index;
}

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

    uint32_t memtype_index = find_memory_type(type_filter, properties);

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

Buffer Device::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage) {
    VkBufferCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;

    return create_buffer(&create_info);
}

Image Device::create_image(uint32_t width, uint32_t height, VkImageUsageFlags usage) {
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = width * height * 4;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    Image result;
    result.device_handle = vulkan_device;
    result.buffer = create_buffer(&buffer_info);
    result.width = width;
    result.height = height;

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = surface_format.format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(vulkan_device, &image_info, nullptr, &result.image_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating image");
    }

    result.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    result.access = 0;

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vulkan_device, result.image_handle, &memory_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(vulkan_device, &alloc_info, nullptr, &result.texture_memory) != VK_SUCCESS)
    {
        throw std::runtime_error("error allocating texture device memory");
    }

    vkBindImageMemory(vulkan_device, result.image_handle, result.texture_memory, 0);

    VkImageViewCreateInfo image_view_info{};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = result.image_handle;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = surface_format.format;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(vulkan_device, &image_view_info, nullptr, &result.view_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating image view");
    }

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.unnormalizedCoordinates = VK_FALSE;

    if (vkCreateSampler(vulkan_device, &sampler_info, nullptr, &result.sampler_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating image sampler");
    }

    return result;
} 

PipelineBuilder Device::create_pipeline_builder() {
    PipelineBuilder res;
    res.device = this;

    return res;
}