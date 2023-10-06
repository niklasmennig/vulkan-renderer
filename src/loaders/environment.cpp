#include "environment.h"

#include "loaders/image.h"
#include "core/vulkan.h"

#include "core/color.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>

EnvironmentMap loaders::load_environment_map(Device* device, const std::string& path) {
    EnvironmentMap result {};
    int width = 100;
    int height = 100;

    width += 1;

    result.image = loaders::load_image(device, path);
    result.cdf_map = device->create_image(width, height, VK_IMAGE_USAGE_SAMPLED_BIT, 1, 1, VK_FORMAT_R32_SFLOAT, VK_FILTER_NEAREST);

    float* cdf_data = new float[width * height * 4];

    ImagePixels image_pixels = result.image.get_pixels();

    for (int v = 0; v < height; v++) {
        float conditional = 0;
        for (int u = 1; u < width; u++) {
            vec3 pixel_color = image_pixels.get_pixel((float)u / (width-1) * image_pixels.width, (float)v / height * image_pixels.height);
            float luminance = color::luminance(pixel_color);
            float sin_theta = std::sin(M_PI * (v + 0.5) / height);
            float cdf_value = luminance * sin_theta;
            cdf_data[u + v * width] = cdf_value;
            conditional += cdf_value;
        }
        // write conditional to u = 0
        cdf_data[0 + v * width] = conditional / (width - 1);
    }

    result.cdf_map.buffer.set_data(cdf_data);

    return result;
}