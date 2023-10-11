#include "device.h"

#include <iostream>

#include <stdexcept>
#include "memory.h"

#include <algorithm>

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

void Device::allocate_memory(VkMemoryAllocateInfo alloc_info, size_t alignment, VkDeviceMemory* memory, VkDeviceSize* offset, bool shared) {
    if (alignment == 0) alignment = 1;
    if (shared) {
        if (shared_buffer_memory.find(alloc_info.memoryTypeIndex) == shared_buffer_memory.end()) {
            // allocate shared memory
            VkMemoryAllocateInfo shared_alloc_info = alloc_info;
            VkDeviceSize alloc_size = std::min(shared_buffer_size, (uint32_t)memory_properties.memoryHeaps[memory_properties.memoryTypes[alloc_info.memoryTypeIndex].heapIndex].size);
            shared_alloc_info.allocationSize = alloc_size;
            SharedDeviceMemory shared;
            shared.offset = 0;
            shared.memory = 0;
            std::cout << "allocating shared memory" << std::endl;
            if (vkAllocateMemory(vulkan_device, &shared_alloc_info, nullptr, &shared.memory) != VK_SUCCESS) {
                throw std::runtime_error("error allocating shared memory");
            }
            shared.size = alloc_size;
            shared_buffer_memory.emplace(alloc_info.memoryTypeIndex, shared);
        }
        SharedDeviceMemory* shared_memory = &shared_buffer_memory[alloc_info.memoryTypeIndex];
        // align memory
        shared_memory->offset = memory::align_up(shared_memory->offset, alignment);
        *memory = shared_memory->memory;
        *offset = shared_memory->offset;
        if (shared_memory->offset + alloc_info.allocationSize > shared_memory->size) std::cerr << "shared memory allocation oversteps heap size" << std::endl;
        shared_memory->offset += alloc_info.allocationSize;
    } else {
        VkResult res = vkAllocateMemory(vulkan_device, &alloc_info, nullptr, memory);
        if (res != VK_SUCCESS) {
            std::cout << "ERROR " << res << std::endl;
            throw std::runtime_error("error allocating unshared memory");
        }
        *offset = 0;
    }
}

Buffer Device::create_buffer(VkBufferCreateInfo *create_info, size_t alignment, bool shared)
{
    Buffer result{};
    result.device_handle = vulkan_device;
    result.buffer_size = create_info->size;

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

    size_t memory_alignment = std::max(mem_requirements.alignment, alignment);

    allocate_memory(alloc_info, memory_alignment, &result.device_memory, &result.device_memory_offset, shared);
    if (shared) std::cout << "Binding Buffer to shared memory at heap " << memtype_index << " in range "  << result.device_memory_offset << " - " << result.device_memory_offset + result.buffer_size << std::endl;
    vkBindBufferMemory(vulkan_device, result.buffer_handle, result.device_memory, result.device_memory_offset);



    VkBufferDeviceAddressInfo addr_info;
    addr_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addr_info.buffer = result.buffer_handle;
    addr_info.pNext = 0;

    result.device_address = vkGetBufferDeviceAddress(vulkan_device, &addr_info);
    result.shared = shared;

    return result;
}

Buffer Device::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage) {
    VkBufferCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;

    return create_buffer(&create_info);
}

Image Device::create_image(uint32_t width, uint32_t height, VkImageUsageFlags usage, uint32_t array_layers, VkMemoryPropertyFlags memory_properties, VkFormat format, VkFilter filter, bool shared) {
    if (format == VK_FORMAT_UNDEFINED) format = surface_format.format;

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = width * height * Image::num_channels(format) * Image::bytes_per_channel(format) * array_layers;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    Image result;
    result.format = format;
    result.device_handle = vulkan_device;
    result.buffer = create_buffer(&buffer_info, 0, shared);
    result.width = width;
    result.height = height;

    VkImageType image_type = VK_IMAGE_TYPE_2D;

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = image_type;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = array_layers;
    image_info.format = format;
    image_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.queueFamilyIndexCount = 1;
    image_info.pQueueFamilyIndices = &graphics_queue_family_index;

    if (vkCreateImage(vulkan_device, &image_info, nullptr, &result.image_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating image");
    }

    result.layout = image_info.initialLayout;
    result.access = 0;

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vulkan_device, result.image_handle, &memory_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, memory_properties);

    allocate_memory(alloc_info, memory_requirements.alignment, &result.texture_memory, &result.texture_memory_offset, shared);

    if(shared) std::cout << "Binding Image to shared memory at " << alloc_info.memoryTypeIndex << " in range " << result.texture_memory_offset << " - " << result.texture_memory_offset + alloc_info.allocationSize << std::endl;
    vkBindImageMemory(vulkan_device, result.image_handle, result.texture_memory, result.texture_memory_offset);

    result.shared_memory = shared;

    VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
    if (array_layers > 1) view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;

    VkImageViewCreateInfo image_view_info{};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = result.image_handle;
    image_view_info.viewType = view_type;
    image_view_info.format = format;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = array_layers;

    if (vkCreateImageView(vulkan_device, &image_view_info, nullptr, &result.view_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating image view");
    }

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = filter;
    sampler_info.minFilter = filter;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.unnormalizedCoordinates = VK_FALSE;

    if (vkCreateSampler(vulkan_device, &sampler_info, nullptr, &result.sampler_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating image sampler");
    }

    return result;
} 