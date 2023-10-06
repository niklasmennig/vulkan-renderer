#include "image.h"

#include <iostream>

uint32_t Image::bytes_per_channel(VkFormat format) {
    switch (format) {
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
            return 1;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        case VK_FORMAT_R32_SFLOAT:
            return 4;
        default:
            std::cerr << "bytes_per_channel not implemented for this format (" << format << ")" << std::endl;
    }
}

uint32_t Image::pixel_byte_offset(VkFormat format, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    int pixel_index = (y * width + x) * 4;
    return pixel_index * bytes_per_channel(format);
}

vec3 Image::color_from_packed_data(VkFormat format, unsigned char* data) {
    vec3 result;

    float* float_data = reinterpret_cast<float*>(data);

    switch(format) {
        case VK_FORMAT_B8G8R8A8_UNORM:
            result.b = data[0 * Image::bytes_per_channel(format)];
            result.g = data[1 * Image::bytes_per_channel(format)];
            result.r = data[2 * Image::bytes_per_channel(format)];
            break;
        case VK_FORMAT_R8G8B8A8_UNORM:
            result.r = data[0 * Image::bytes_per_channel(format)];
            result.g = data[1 * Image::bytes_per_channel(format)];
            result.b = data[2 * Image::bytes_per_channel(format)];
            break;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            result.r = float_data[0 * Image::bytes_per_channel(format)];
            result.g = float_data[1 * Image::bytes_per_channel(format)];
            result.b = float_data[2 * Image::bytes_per_channel(format)];
            break;
        default:
            std::cerr << "color_from_packed_data not implemented for this image format (" << format << ")" << std::endl;
            break;
    }
    return result;
}

void Image::free()
{
    vkDestroySampler(device_handle, sampler_handle, nullptr);
    vkDestroyImageView(device_handle, view_handle, nullptr); 
    vkDestroyImage(device_handle, image_handle, nullptr);
    vkFreeMemory(device_handle, texture_memory, nullptr);
    buffer.free();
}

VkImageMemoryBarrier Image::get_layout_transition(VkImageLayout target_layout, VkAccessFlags target_access) {
    VkImageMemoryBarrier texture_barrier = {};
    texture_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    texture_barrier.oldLayout = layout;
    texture_barrier.newLayout = target_layout;
    texture_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    texture_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    texture_barrier.image = image_handle;
    texture_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    texture_barrier.subresourceRange.baseMipLevel = 0;
    texture_barrier.subresourceRange.levelCount = 1;
    texture_barrier.subresourceRange.baseArrayLayer = 0;
    texture_barrier.subresourceRange.layerCount = 1;
    texture_barrier.srcAccessMask = access;
    texture_barrier.dstAccessMask = target_access;

    return texture_barrier;
}

void Image::cmd_transition_layout(VkCommandBuffer cmd_buffer, VkImageLayout target_layout, VkAccessFlags target_access) {
    auto texture_barrier = get_layout_transition(target_layout, target_access);

    layout = target_layout;
    access = target_access;

    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 0, nullptr, 0, nullptr, 1, &texture_barrier);
}

void Image::cmd_setup_texture(VkCommandBuffer cmd_buffer) {
    cmd_transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0);

    VkBufferImageCopy texture_copy{};
    texture_copy.bufferOffset = 0;
    texture_copy.bufferRowLength = 0;
    texture_copy.bufferImageHeight = 0;
    texture_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    texture_copy.imageSubresource.mipLevel = 0;
    texture_copy.imageSubresource.baseArrayLayer = 0;
    texture_copy.imageSubresource.layerCount = 1;
    texture_copy.imageOffset = {0, 0, 0};
    texture_copy.imageExtent = {
        width,
        height,
        1};

    vkCmdCopyBufferToImage(cmd_buffer, buffer.buffer_handle, image_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &texture_copy);

    cmd_transition_layout(cmd_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);    
}

void Image::cmd_update_buffer(VkCommandBuffer cmd_buffer) {
    VkBufferImageCopy texture_copy{};
    texture_copy.bufferOffset = 0;
    texture_copy.bufferRowLength = 0;
    texture_copy.bufferImageHeight = 0;
    texture_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    texture_copy.imageSubresource.mipLevel = 0;
    texture_copy.imageSubresource.baseArrayLayer = 0;
    texture_copy.imageSubresource.layerCount = 1;
    texture_copy.imageOffset = {0, 0, 0};
    texture_copy.imageExtent = {
        width,
        height,
        1};

    vkCmdCopyImageToBuffer(cmd_buffer, image_handle, layout, buffer.buffer_handle, 1, &texture_copy);
}

ImagePixels Image::get_pixels() {
    ImagePixels result;
    result.data.resize(buffer.buffer_size);
    buffer.get_data(result.data.data(), 0, buffer.buffer_size);
    result.format = format;
    result.width = width;
    result.height = height;


    return result;
}

vec3 Image::get_pixel(uint32_t x, uint32_t y) {
    if (x < 0 || x >= width || y < 0 || y >= height) return vec3(0);

    size_t offset = Image::pixel_byte_offset(format, x, y, width, height);

    unsigned char* data = new unsigned char[16];
    buffer.get_data(data, offset, 16);

    vec3 result = Image::color_from_packed_data(format, data);
    return result;
}

vec3 ImagePixels::get_pixel(uint32_t x, uint32_t y) {
    if (x < 0 || x >= width || y < 0 || y >= height) return vec3(0);


    size_t offset = Image::pixel_byte_offset(format, x, y, width, height);

    vec3 result = Image::color_from_packed_data(format, data.data() + offset);

    return result;
}