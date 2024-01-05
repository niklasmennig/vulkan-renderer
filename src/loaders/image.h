#pragma once
#include <string>

#include "core/image.h"

struct Device;

namespace loaders {
    Image load_image(Device* device, const std::string& path, VkMemoryPropertyFlags additional_memory_properties = 0, VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL, VkAccessFlags access = 0);
    void save_exr_image(ImagePixels& pixels, const std::string& path);
}