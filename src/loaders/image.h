#pragma once
#include <string>

#include "../image.h"

struct Device;

namespace loaders {
    Image load_image(Device* device, std::string path);
}