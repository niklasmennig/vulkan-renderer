#include "image.h"
#include "../device.h"

#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Image loaders::load_image(Device* device, std::string path) {
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = width * height * 4;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    Image result;
    result.device_handle = device->vulkan_device;
    result.buffer = device->create_buffer(&buffer_info);
    result.buffer.set_data(data);
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
    image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(device->vulkan_device, &image_info, nullptr, &result.image_handle) != VK_SUCCESS) {
        throw std::runtime_error("error creating image");
    }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device->vulkan_device, result.image_handle, &memory_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = device->find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device->vulkan_device, &alloc_info, nullptr, &result.texture_memory) != VK_SUCCESS) {
        throw std::runtime_error("error allocating texture device memory");
    }

    vkBindImageMemory(device->vulkan_device, result.image_handle, result.texture_memory, 0);

    VkImageViewCreateInfo image_view_info{};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = result.image_handle;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device->vulkan_device, &image_view_info, nullptr, &result.view_handle) != VK_SUCCESS) {
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

    if (vkCreateSampler(device->vulkan_device, &sampler_info, nullptr, &result.sampler_handle) != VK_SUCCESS) {
        throw std::runtime_error("error creating image sampler");
    }

    stbi_image_free(data);

    return result;
}
