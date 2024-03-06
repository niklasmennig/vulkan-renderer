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
    int height = 50;

    result.image = loaders::load_image(device, path, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    Buffer image_data_buffer = device->create_buffer(result.image.memory_requirements.size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    // copy on separate command buffer to make sure copy is finished when processing starts
    result.image.copy_image_to_buffer(image_data_buffer);

    // generate cdf texture from image
    result.cdf_map = device->create_image(width, height, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 1, 1, VK_FORMAT_R32_SFLOAT, VK_FILTER_NEAREST);
    float* cdf_data = new float[width * height];
    Buffer cdf_data_buffer = device->create_buffer(sizeof(float) * width * height, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    result.conditional_cdf_map = device->create_image(1, height, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 1, 1, VK_FORMAT_R32_SFLOAT, VK_FILTER_NEAREST);
    float* conditional_cdf_data = new float[height];
    Buffer conditional_cdf_data_buffer = device->create_buffer(sizeof(float) * height, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    int scan_pixels_x = std::floor(result.image.width / width);
    int scan_pixels_y = std::floor(result.image.height / height);

    VkCommandBuffer cmd_buffer = device->begin_single_use_command_buffer();
    result.image.transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_GENERAL);
    result.cdf_map.transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_GENERAL);
    result.conditional_cdf_map.transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_GENERAL);


    float* image_data;
    vkMapMemory(device->vulkan_device, image_data_buffer.device_memory, image_data_buffer.device_memory_offset, image_data_buffer.buffer_size, 0, (void**)&image_data);

    float conditional_sum = 0;
    for (int v = 0; v < height; v++) {
        float conditional = 0;
        for (int u = 0; u < width; u++) {
            float luminance_sum = 0;
            for (int scan_x = 0; scan_x < scan_pixels_x; scan_x++) {
                for (int scan_y = 0; scan_y < scan_pixels_y; scan_y++) {
                    int pixel_x = (float)u / width * result.image.width + scan_x;
                    int pixel_y = (float)v / height * result.image.height + scan_y;
                    
                    int pixel_offset = (pixel_x + pixel_y * width) * 4;
                    vec3 pixel_color = vec3(image_data[pixel_offset + 0], image_data[pixel_offset + 1], image_data[pixel_offset + 2]);

                    luminance_sum += color::luminance(pixel_color);
                }   
            }
            float luminance = luminance_sum / (scan_pixels_x * scan_pixels_y);
            float sin_theta = std::sin(M_PI * (v + 0.5) / height);
            float cdf_value = luminance * sin_theta;
            conditional += cdf_value;
            cdf_data[u + v * width] = conditional;
        }
        if (conditional < FLT_EPSILON) {
            for (int u = 0; u < width; u++) {
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

    vkUnmapMemory(device->vulkan_device, image_data_buffer.device_memory);

    cdf_data_buffer.set_data(cdf_data);
    result.cdf_map.copy_buffer_to_image(cmd_buffer, cdf_data_buffer);

    conditional_cdf_data_buffer.set_data(conditional_cdf_data);
    result.conditional_cdf_map.copy_buffer_to_image(cmd_buffer, conditional_cdf_data_buffer);

    result.image.transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
    result.cdf_map.transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
    result.conditional_cdf_map.transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);

    device->end_single_use_command_buffer(cmd_buffer);

    image_data_buffer.free();
    cdf_data_buffer.free();
    conditional_cdf_data_buffer.free();

    return result;
}

EnvironmentMap loaders::load_default_environment_map(Device* device, vec3 color) {
    EnvironmentMap result {};
    int width = 1;
    int height = 1;

    result.image = device->create_image(1, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FILTER_NEAREST);
    result.cdf_map = device->create_image(1, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    result.conditional_cdf_map = device->create_image(1, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    auto cmd_buffer = device->begin_single_use_command_buffer();
    Buffer image_buffer = device->create_buffer(result.image.memory_requirements.size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    float* image_data;
    vkMapMemory(image_buffer.device_handle, image_buffer.device_memory, image_buffer.device_memory_offset, VK_WHOLE_SIZE, 0, (void**)&image_data);
    image_data[0] = color.r;
    image_data[1] = color.g;
    image_data[2] = color.b;
    vkUnmapMemory(image_buffer.device_handle, image_buffer.device_memory);

    result.image.copy_buffer_to_image(cmd_buffer, image_buffer);

    result.image.transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
    result.cdf_map.transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
    result.conditional_cdf_map.transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);

    device->end_single_use_command_buffer(cmd_buffer);

    image_buffer.free();

    return result;
}