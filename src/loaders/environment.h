#pragma once

#include "core/device.h"
#include "core/image.h"

#include <string>

struct EnvironmentMap {
    Image image;
    Image cdf_map;
    Image conditional_cdf_map;
};

namespace loaders {
    EnvironmentMap load_environment_map(Device* device, const std::string& path);
    EnvironmentMap load_default_environment_map(Device* device, vec3 color = vec3(0.0));
}