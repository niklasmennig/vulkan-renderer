#pragma once
#include <string>

#include "../image.h"

struct Device;

enum ImageFormat {
    SRGB = VK_FORMAT_R8G8B8A8_SRGB,
    LINEAR = VK_FORMAT_R16G16B16A16_SFLOAT
};

namespace loaders {
    Image load_image(Device* device, std::string path, ImageFormat format = SRGB);
}