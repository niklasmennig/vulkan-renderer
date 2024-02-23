#include "image.h"
#include "../core/device.h"

#include <stdexcept>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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