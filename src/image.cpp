#include "image.h"

#include <iostream>

void Image::free()
{
    vkDestroySampler(device_handle, sampler_handle, nullptr);
    vkFreeMemory(device_handle, texture_memory, nullptr);
    vkDestroyImageView(device_handle, view_handle, nullptr);
    vkDestroyImage(device_handle, image_handle, nullptr);
    buffer.free();
}

void Image::cmd_transition_layout(VkCommandBuffer cmd_buffer, VkImageLayout target_layout, VkAccessFlags target_access) {
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

ivec3 Image::get_pixel(uint32_t x, uint32_t y) {
    if (x < 0 || x >= width || y < 0 || y >= height) return ivec3(0);
    ivec3 result;

    void *data;
    if (vkMapMemory(device_handle, texture_memory, 0, VK_WHOLE_SIZE, 0, &data) != VK_SUCCESS)
    {
        throw std::runtime_error("error mapping buffer memory");
    }
    vkUnmapMemory(device_handle, texture_memory);

    char* char_data = reinterpret_cast<char*>(data);

    switch(format) {
        case VK_FORMAT_B8G8R8A8_UNORM:
            size_t offset = (y * width + x) * 4;

            result.b = (unsigned char)char_data[offset];
            result.g = (unsigned char)char_data[offset+1];
            result.r = (unsigned char)char_data[offset+2];
            break;
    }

    return result;
}