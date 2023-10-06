#include "environment.h"

#include "loaders/image.h"
#include "core/vulkan.h"

#include "core/color.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>

EnvironmentMap loaders::load_environment_map(Device* device, const std::string& path) {
    EnvironmentMap result {};
    int width = 20;
    int height = 20;

    result.image = loaders::load_image(device, path);
    result.cdf_map = device->create_image(width, height, VK_IMAGE_USAGE_SAMPLED_BIT, 1, 1, VK_FORMAT_R32_SFLOAT);

    float* cdf_data = new float[width * height];

    ImagePixels image_pixels = result.image.get_pixels();

    for (int v = 0; v < height; v++) {
        for (int u = 0; u < width; u++) {
            vec3 pixel_color = image_pixels.get_pixel((float)u / width * image_pixels.width, (float)v / height * image_pixels.height);
            float luminance = color::luminance(pixel_color);
            float cdf_value = luminance;
            float sin_theta = std::sin(M_PI * (v + 0.5) / height);
            cdf_data[u + v * width] = cdf_value;
        }
    }

    result.cdf_map.buffer.set_data(cdf_data);

    return result;
}