#include "environment.h"

#include "loaders/image.h"
#include "core/vulkan.h"

#define _USE_MATH_DEFINES
#include <math.h>

EnvironmentMap loaders::load_environment_map(Device* device, const std::string& path) {
    EnvironmentMap result {};
    int width = 32;
    int height = 32;

    result.image = loaders::load_image(device, path);
    result.cdf_map = device->create_image(width, height, VK_IMAGE_USAGE_SAMPLED_BIT, 1, 1, VK_FORMAT_R32_SFLOAT);

    float* cdf_data = new float[width * height];

    // for (int v = 0; v < height; v++) {
    //     for (int u = 0; u < width; u++) {
            
    //         float cdf_value = result.image.get_pixel(u / width * result.image.width, v / height * result.image.height).b;
    //         float sin_theta = std::sin(M_PI * (v + 0.5) / height);
    //         cdf_data[u + v * width] = cdf_value;
    //     }
    // }

    // result.cdf_map.buffer.set_data(cdf_data);

    return result;
}