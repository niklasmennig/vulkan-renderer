#pragma once
#include <string>

#include "../image.h"

struct Device;

namespace loaders {
    Image load_image(Device* device, const std::string& path, bool flip_y = false);
    void save_exr_image(Image& image, const std::string& path);
}