#pragma once

#include "core/device.h"
#include "core/image.h"

#include <string>

struct EnvironmentMap {
    Image image;
    Image cdf_map;
};

namespace loaders {
    EnvironmentMap load_environment_map(Device* device, const std::string& path);
}