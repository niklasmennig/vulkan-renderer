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
            auto allocation_error = vkAllocateMemory(vulkan_device, &shared_alloc_info, nullptr, &shared.memory);
            if (allocation_error != VK_SUCCESS) {
                std::cout << "error code " << (int)allocation_error << std::endl;
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

VkCommandBuffer Device::begin_single_use_command_buffer() {
    VkCommandBufferAllocateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer_info.commandPool = command_pool;
    buffer_info.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vulkan_device, &buffer_info, &commandBuffer);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &begin_info);

    return commandBuffer;
}

void Device::end_single_use_command_buffer(VkCommandBuffer cmd_buffer) {
    vkEndCommandBuffer(cmd_buffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd_buffer;

    vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(vulkan_device, command_pool, 1, &cmd_buffer);
}

Buffer Device::create_buffer(VkBufferCreateInfo *create_info, size_t alignment, bool shared, bool exportable)
{
    Buffer result{};
    result.device_handle = vulkan_device;

    create_info->usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    create_info->sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (exportable) {
        VkExternalMemoryBufferCreateInfo ext_buffer_info{};
        ext_buffer_info.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
        ext_buffer_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

        create_info->pNext = &ext_buffer_info;
    }

    if (vkCreateBuffer(vulkan_device, create_info, nullptr, &result.buffer_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating buffer");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(vulkan_device, result.buffer_handle, &mem_requirements);
    result.buffer_size = mem_requirements.size;

    // find correct memory type
    uint32_t type_filter = mem_requirements.memoryTypeBits;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    if (exportable) properties = 0;

    uint32_t memtype_index = find_memory_type(type_filter, properties);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = memtype_index;

    VkMemoryAllocateFlagsInfo alloc_flags{};
    alloc_flags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    alloc_flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    alloc_info.pNext = &alloc_flags;

    if (exportable) {
        VkExportMemoryAllocateInfo export_info {};
        export_info.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
        export_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

        alloc_flags.pNext = &export_info;
    }

    size_t memory_alignment = std::max(mem_requirements.alignment, alignment);

    allocate_memory(alloc_info, memory_alignment, &result.device_memory, &result.device_memory_offset, shared);
    if (shared) std::cout << "Binding Buffer to shared memory at heap " << memtype_index << " in range "  << result.device_memory_offset << " - " << result.device_memory_offset + result.buffer_size << std::endl;
    vkBindBufferMemory(vulkan_device, result.buffer_handle, result.device_memory, result.device_memory_offset);

    result.shared = shared;

    return result;
}

Buffer Device::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, bool shared, bool exportable) {
    VkBufferCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;

    return create_buffer(&create_info, 4, shared, exportable);
}

Image Device::create_image(uint32_t width, uint32_t height, VkImageUsageFlags usage, uint32_t array_layers, VkMemoryPropertyFlags memory_properties, VkFormat format, VkFilter filter, VkSamplerAddressMode uv_mode, bool shared) {
    if (format == VK_FORMAT_UNDEFINED) format = surface_format.format;

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = width * height * Image::num_channels(format) * Image::bytes_per_channel(format) * array_layers;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    Image result;
    result.format = format;
    result.device = this;
    result.width = width;
    result.height = height;

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
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

    vkGetImageMemoryRequirements(vulkan_device, result.image_handle, &result.memory_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = result.memory_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(result.memory_requirements.memoryTypeBits, memory_properties);

    allocate_memory(alloc_info, result.memory_requirements.alignment, &result.texture_memory, &result.texture_memory_offset, shared);

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
    sampler_info.addressModeU = uv_mode;
    sampler_info.addressModeV = uv_mode;
    sampler_info.addressModeW = uv_mode;
    sampler_info.unnormalizedCoordinates = VK_FALSE;

    if (vkCreateSampler(vulkan_device, &sampler_info, nullptr, &result.sampler_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating image sampler");
    }

    return result;
}