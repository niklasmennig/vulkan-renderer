#pragma once
#include <string>

#include "../buffer.h"

struct Device;

struct Image {
    VkDevice device_handle;
    uint32_t width, height;
    Buffer buffer;
    VkDeviceMemory texture_memory;
    VkImage image_handle;
    VkImageView view_handle;
    VkSampler sampler_handle;

    void free();
};

namespace loaders {
    Image load_image(Device* device, std::string path);
}