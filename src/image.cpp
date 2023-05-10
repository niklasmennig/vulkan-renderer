#include "image.h"

void Image::free()
{
    vkDestroySampler(device_handle, sampler_handle, nullptr);
    vkFreeMemory(device_handle, texture_memory, nullptr);
    vkDestroyImageView(device_handle, view_handle, nullptr);
    vkDestroyImage(device_handle, image_handle, nullptr);
    buffer.free();
}

void Image::cmd_setup_texture(VkCommandBuffer cmd_buffer) {
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

    // transition texture to readable format
    VkImageMemoryBarrier texture_barrier = {};
    texture_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    texture_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    texture_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texture_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    texture_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    texture_barrier.image = image_handle;
    texture_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    texture_barrier.subresourceRange.baseMipLevel = 0;
    texture_barrier.subresourceRange.levelCount = 1;
    texture_barrier.subresourceRange.baseArrayLayer = 0;
    texture_barrier.subresourceRange.layerCount = 1;
    texture_barrier.srcAccessMask = 0;
    texture_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 0, nullptr, 0, nullptr, 1, &texture_barrier);
}