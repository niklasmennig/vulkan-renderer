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

Image loaders::load_image(Device* device, const std::string& path, VkMemoryPropertyFlags additional_memory_properties, VkImageLayout layout, VkAccessFlags access) {
    stbi_set_unpremultiply_on_load(1);
    stbi_ldr_to_hdr_gamma(1.0);
    stbi_ldr_to_hdr_scale(1.0);

    bool is_hdr = stbi_is_hdr(path.c_str());

    int width, height, channels;
    VkFormat format;

    unsigned char* image_data;

    if (!is_hdr) {
        image_data = (stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha));
        format = VK_FORMAT_R8G8B8A8_UNORM;

        std::cout << "loading non-HDR image at " << path << "| Channels: " << channels << std::endl;
    } else {
        image_data = reinterpret_cast<unsigned char*>(stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha));
        format = VK_FORMAT_R32G32B32A32_SFLOAT;

        std::cout << "loading HDR image at " << path << "| Channels: " << channels << std::endl;
    }

    Image result = device->create_image(width, height, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 1, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | additional_memory_properties, format);

    Buffer image_data_buffer = device->create_buffer(result.memory_requirements.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    void* buffer_data;
    vkMapMemory(device->vulkan_device, image_data_buffer.device_memory, image_data_buffer.device_memory_offset, image_data_buffer.buffer_size, 0, &buffer_data);
    memcpy(buffer_data, image_data, image_data_buffer.buffer_size);
    vkUnmapMemory(device->vulkan_device, image_data_buffer.device_memory);

    VkCommandBuffer cmd_buffer = device->begin_single_use_command_buffer();

    result.transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    result.copy_buffer_to_image(cmd_buffer, image_data_buffer);
    result.transition_layout(cmd_buffer, layout, access);

    device->end_single_use_command_buffer(cmd_buffer);

    image_data_buffer.free();
    stbi_image_free(image_data);

    return result;
}

void loaders::save_exr_image(ImagePixels& pixels, const std::string& path) {
    std::cout << "TODO: IMPLEMENT loaders::save_exr_image" << std::endl;
    // EXRHeader header;
    // InitEXRHeader(&header);

    // EXRImage exr_image;
    // InitEXRImage(&exr_image);

    // uint32_t width = pixels.width;
    // uint32_t height = pixels.height;

    // exr_image.num_channels = 3;

    // std::vector<float> channels[3];
    // channels[0].resize(width * height);
    // channels[1].resize(width * height);
    // channels[2].resize(width * height);

    // for (int i = 0; i < width * height; i++) {
    //     uint32_t x = i % width;
    //     uint32_t y = std::floor(i / width);
    //     vec3 color = pixels.get_pixel(x, y);

    //     channels[0][i] = (float)color.r;
    //     channels[1][i] = (float)color.g;
    //     channels[2][i] = (float)color.b;
    // }

    // float* channel_ptrs[3];
    // // exr stored in BGR format
    // channel_ptrs[0] = channels[2].data();
    // channel_ptrs[1] = channels[1].data();
    // channel_ptrs[2] = channels[0].data();

    // exr_image.images = (unsigned char**) channel_ptrs;
    // exr_image.width = width;
    // exr_image.height = height;

    // header.num_channels = 3;
    // header.channels = (EXRChannelInfo*) malloc(sizeof(EXRChannelInfo) * header.num_channels);

    // strncpy(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
    // strncpy(header.channels[1].name, "G", 255); header.channels[1].name[strlen("G")] = '\0';
    // strncpy(header.channels[2].name, "R", 255); header.channels[2].name[strlen("R")] = '\0';

    // header.pixel_types = (int*) malloc(sizeof(int) * header.num_channels);
    // header.requested_pixel_types = (int*) malloc(sizeof(int) * header.num_channels);

    // for (int i = 0; i < header.num_channels; i++) {
    //     header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
    //     header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
    // }

    // const char* err = NULL;

    // std::cout << "saving EXR image" << std::endl;

    // int ret = SaveEXRImageToFile(&exr_image, &header, path.c_str(), &err);
    // if (ret != TINYEXR_SUCCESS) {
    //     std::cout << "Error saving exr image: " << err << std::endl;
    //     FreeEXRErrorMessage(err);
    // }

    // free(header.channels);
    // free(header.pixel_types);
    // free(header.requested_pixel_types);
}