#include "image.h"

#include "device.h"

#include <iostream>

uint32_t Image::bytes_per_channel(VkFormat format) {
    switch (format) {
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_SRGB:
            return 1;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        case VK_FORMAT_R32_SFLOAT:
            return 4;
        default:
            std::cerr << "bytes_per_channel not implemented for this format (" << format << ")" << std::endl;
    }
}

uint32_t Image::num_channels(VkFormat format) {
    switch (format) {
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 4;
        case VK_FORMAT_R32_SFLOAT:
            return 1;
        default:
            std::cerr << "num_channels not implemented for this format (" << format << ")" << std::endl;
    }
}

uint32_t Image::pixel_byte_offset(VkFormat format, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    int pixel_index = (y * width + x);
    return pixel_index * bytes_per_channel(format) * num_channels(format);
}

vec3 Image::color_from_packed_data(VkFormat format, unsigned char* data) {
    vec3 result;

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
            result.r = *reinterpret_cast<float*>(data + 0 * Image::bytes_per_channel(format));
            result.g = *reinterpret_cast<float*>(data + 1 * Image::bytes_per_channel(format));
            result.b = *reinterpret_cast<float*>(data + 2 * Image::bytes_per_channel(format));
            break;
        default:
            std::cerr << "color_from_packed_data not implemented for this image format (" << format << ")" << std::endl;
            break;
    }
    return result;
}

void Image::free()
{
    vkDestroySampler(device->vulkan_device, sampler_handle, nullptr);
    vkDestroyImageView(device->vulkan_device, view_handle, nullptr); 
    vkDestroyImage(device->vulkan_device, image_handle, nullptr);
    if (!shared_memory) {
        vkFreeMemory(device->vulkan_device, texture_memory, nullptr);
    }
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

void Image::transition_layout(VkCommandBuffer cmd_buffer, VkImageLayout target_layout, VkAccessFlags target_access) {
    auto texture_barrier = get_layout_transition(target_layout, target_access);

    layout = target_layout;
    access = target_access;

    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &texture_barrier);
}

void Image::transition_layout(VkImageLayout target_layout, VkAccessFlags target_access) {
    VkCommandBuffer cmd_buffer = device->begin_single_use_command_buffer();

    transition_layout(cmd_buffer, target_layout, target_access);

    device->end_single_use_command_buffer(cmd_buffer);
}

void Image::cmd_blit_image(VkCommandBuffer cmd_buffer, Image src_image) {
    VkImageBlit blit{};
    blit.srcSubresource.layerCount = 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.layerCount = 1;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcOffsets[0] = {0,0,0};
    blit.srcOffsets[1] = {(int32_t)src_image.width, (int32_t)src_image.height, 1};
    blit.dstOffsets[0] = {0,0,0};
    blit.dstOffsets[1] = {(int32_t)width, (int32_t)height, 1};

    vkCmdBlitImage(cmd_buffer, src_image.image_handle, src_image.layout, image_handle, layout, 1, &blit, VK_FILTER_LINEAR);
}

void Image::copy_buffer_to_image(VkCommandBuffer cmd_buffer, Buffer buffer) {
    VkBufferImageCopy copy{};
    copy.imageOffset = {0, 0, 0};
    copy.imageExtent = {width, height, 1};
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.layerCount = 1;
    copy.imageSubresource.mipLevel = 0;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.bufferOffset = 0;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;

    vkCmdCopyBufferToImage(cmd_buffer, buffer.buffer_handle, image_handle, layout, 1, &copy);
}

void Image::copy_buffer_to_image(Buffer buffer) {
    VkCommandBuffer cmd_buffer = device->begin_single_use_command_buffer();

    copy_buffer_to_image(cmd_buffer, buffer);

    device->end_single_use_command_buffer(cmd_buffer);
}

void Image::copy_image_to_buffer(VkCommandBuffer cmd_buffer, Buffer buffer) {
    VkBufferImageCopy copy{};
    copy.imageOffset = {0, 0, 0};
    copy.imageExtent = {width, height, 1};
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.layerCount = 1;
    copy.imageSubresource.mipLevel = 0;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.bufferOffset = 0;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;

    vkCmdCopyImageToBuffer(cmd_buffer, image_handle, layout, buffer.buffer_handle, 1, &copy);
}

void Image::copy_image_to_buffer(Buffer buffer) {
    VkCommandBuffer cmd_buffer = device->begin_single_use_command_buffer();

    copy_image_to_buffer(cmd_buffer, buffer);

    device->end_single_use_command_buffer(cmd_buffer);
}

VkExtent2D Image::get_extents() {
    return VkExtent2D{
        width,
        height
    };
}

Image::Image() {
    width = 0;
    height = 0;
}