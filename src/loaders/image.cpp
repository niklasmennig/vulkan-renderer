#include "image.h"
#include "../core/device.h"

#include <stdexcept>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 1
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

Image loaders::load_image(Device* device, const std::string& path, bool flip_y) {
    if (flip_y) stbi_set_flip_vertically_on_load(1);
    stbi_set_unpremultiply_on_load(1);
    stbi_ldr_to_hdr_gamma(1.0);
    stbi_ldr_to_hdr_scale(1.0);

    bool is_hdr = stbi_is_hdr(path.c_str());

    Image result;
    int width, height, channels;
    VkFormat format;

    if (!is_hdr) {
        uint8_t* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        std::cout << "loading non-HDR image at " << path << "| Channels: " << channels << std::endl;
        format = VK_FORMAT_R8G8B8A8_UNORM;

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = width * height * 4;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        result.device_handle = device->vulkan_device;
        result.buffer = device->create_buffer(&buffer_info);
        result.buffer.set_data(data);
        result.width = width;
        result.height = height;
        stbi_image_free(data);
    } else {
        float* data = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        std::cout << "loading HDR image at " << path << "| Channels: " << channels << std::endl;
        format = VK_FORMAT_R32G32B32A32_SFLOAT;

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = width * height * 16;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        result.device_handle = device->vulkan_device;
        result.buffer = device->create_buffer(&buffer_info);
        result.buffer.set_data(data);
        result.width = width;
        result.height = height;
        stbi_image_free(data);
    }

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = VK_IMAGE_TILING_LINEAR;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(device->vulkan_device, &image_info, nullptr, &result.image_handle) != VK_SUCCESS) {
        throw std::runtime_error("error creating image");
    }

    result.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    result.access = 0;

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device->vulkan_device, result.image_handle, &memory_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = device->find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    if (vkAllocateMemory(device->vulkan_device, &alloc_info, nullptr, &result.texture_memory) != VK_SUCCESS) {
        throw std::runtime_error("error allocating texture device memory");
    }

    vkBindImageMemory(device->vulkan_device, result.image_handle, result.texture_memory, 0);

    VkImageViewCreateInfo image_view_info{};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = result.image_handle;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = format;
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

    return result;
}

void loaders::save_exr_image(Image& img, const std::string& path) {
    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage exr_image;
    InitEXRImage(&exr_image);

    uint32_t width = img.width;
    uint32_t height = img.height;

    exr_image.num_channels = 3;

    std::vector<float> channels[3];
    channels[0].resize(width * height);
    channels[1].resize(width * height);
    channels[2].resize(width * height);

    for (int i = 0; i < width * height; i++) {
        uint32_t x = i % width;
        uint32_t y = std::floor(i / width);
        ivec3 color = img.get_pixel(x, y);

        channels[0][i] = (float)color.r / 255.0;
        channels[1][i] = (float)color.g / 255.0;
        channels[2][i] = (float)color.b / 255.0;
    }

    float* channel_ptrs[3];
    // exr stored in BGR format
    channel_ptrs[0] = channels[2].data();
    channel_ptrs[1] = channels[1].data();
    channel_ptrs[2] = channels[0].data();

    exr_image.images = (unsigned char**) channel_ptrs;
    exr_image.width = width;
    exr_image.height = height;

    header.num_channels = 3;
    header.channels = (EXRChannelInfo*) malloc(sizeof(EXRChannelInfo) * header.num_channels);

    strncpy(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
    strncpy(header.channels[1].name, "G", 255); header.channels[1].name[strlen("G")] = '\0';
    strncpy(header.channels[2].name, "R", 255); header.channels[2].name[strlen("R")] = '\0';

    header.pixel_types = (int*) malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types = (int*) malloc(sizeof(int) * header.num_channels);

    for (int i = 0; i < header.num_channels; i++) {
        header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
    }

    const char* err = NULL;

    std::cout << "saving" << std::endl;

    int ret = SaveEXRImageToFile(&exr_image, &header, path.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        std::cout << "Error saving exr image: " << err << std::endl;
        FreeEXRErrorMessage(err);
    }

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);
}