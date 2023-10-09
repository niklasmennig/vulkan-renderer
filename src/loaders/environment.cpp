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

    result.image = loaders::load_image(device, path);
    ImagePixels image_pixels = result.image.get_pixels();

    // generate cdf texture from image
    result.cdf_map = device->create_image(width, height, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 1, 1, VK_FORMAT_R32_SFLOAT, VK_FILTER_NEAREST);
    float* cdf_data = new float[width * height];

    result.conditional_cdf_map = device->create_image(1, height, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 1, 1, VK_FORMAT_R32_SFLOAT, VK_FILTER_NEAREST);
    float* conditional_cdf_data = new float[height];

    float conditional_sum = 0;
    for (int v = 0; v < height; v++) {
        float conditional = 0;
        for (int u = 0; u < width; u++) {
            vec3 pixel_color = image_pixels.get_pixel((float)u / width * image_pixels.width, (float)v / height * image_pixels.height);
            float luminance = color::luminance(pixel_color);
            float sin_theta = std::sin(M_PI * (v + 0.5) / height);
            float cdf_value = luminance * sin_theta;
            conditional += cdf_value;
            cdf_data[u + v * width] = conditional;
        }
        if (conditional < FLT_EPSILON) {
            for (int u = 0; u < width; u++) {
                // normalize cdf along row to 1
                cdf_data[u + v * width] = (float)u / (width-1);
            }
        } else {
            for (int u = 0; u < width; u++) {
                // normalize cdf along row to 1
                cdf_data[u + v * width] /= conditional;
            }
        }
        conditional_sum += conditional;
        conditional_cdf_data[v] = conditional_sum;
    }
    if (conditional_sum < FLT_EPSILON) {
        for (int v = 0; v < height; v++) {
            conditional_cdf_data[v] = (float)v / (height-1);
        }
    } else {
        for (int v = 0; v < height; v++) {
            // normalize conditional pdf along column to 1
            conditional_cdf_data[v] /= conditional_sum;
        }
    }

    result.cdf_map.buffer.set_data(cdf_data);
    result.conditional_cdf_map.buffer.set_data(conditional_cdf_data);

    return result;
}