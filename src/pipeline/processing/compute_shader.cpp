#include "compute_shader.h"

#include <filesystem>
#include <array>
#include <iostream>
#include <sstream>

#include "core/device.h"
#include "loaders/shader_spirv.h"
#include "shader_compiler.h"

#include <glm/vec2.hpp>

namespace ShaderInterface {
    using ivec2 = glm::ivec2;
    #include "../shaders/processing/interface.glsl"
}

ComputeShader Device::create_compute_shader(std::string code_path) {
    return ComputeShader(this, code_path);
}

void ComputeShader::set_image(int index, Image* img) {
    VkDescriptorImageInfo compute_descriptor_image_info{};
    compute_descriptor_image_info.imageLayout = img->layout;
    compute_descriptor_image_info.imageView = img->view_handle;

    VkWriteDescriptorSet compute_descriptor_write{};
    compute_descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    compute_descriptor_write.dstSet = descriptor_set;
    compute_descriptor_write.dstBinding = 0;
    compute_descriptor_write.dstArrayElement = index;
    compute_descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    compute_descriptor_write.descriptorCount = 1;
    compute_descriptor_write.pImageInfo = &compute_descriptor_image_info;

    vkUpdateDescriptorSets(device->vulkan_device, 1, &compute_descriptor_write, 0, nullptr);
}

void ComputeShader::set_buffer(int index, Buffer* buffer) {
    VkDescriptorBufferInfo compute_descriptor_buffer_info{};
    compute_descriptor_buffer_info.buffer = buffer->buffer_handle;
    compute_descriptor_buffer_info.offset = 0;
    compute_descriptor_buffer_info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet compute_descriptor_write{};
    compute_descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    compute_descriptor_write.dstSet = descriptor_set;
    compute_descriptor_write.dstBinding = 0;
    compute_descriptor_write.dstArrayElement = index;
    compute_descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    compute_descriptor_write.descriptorCount = 1;
    compute_descriptor_write.pBufferInfo = &compute_descriptor_buffer_info;

    vkUpdateDescriptorSets(device->vulkan_device, 1, &compute_descriptor_write, 0, nullptr);
}

void ComputeShader::build() {
    // preprocess shader code
    std::ifstream in_file(code_path);
    std::string line;
    std::cout << "detecting shader information for compute shader at " << this->code_path << std::endl;
    while(std::getline(in_file, line)) {
        if (line.find("layout") != std::string::npos) {
            size_t size_x_pos = line.find("local_size_x");
            size_t size_y_pos = line.find("local_size_y");
            size_t size_z_pos = line.find("local_size_z");
            if (size_x_pos != std::string::npos && size_y_pos != std::string::npos && size_z_pos != std::string::npos) {
                size_t size_x_string_start = line.find("=", size_x_pos) + 1;
                size_t size_x_string_end = line.find(",", size_x_string_start);
                size_t size_y_string_start = line.find("=", size_y_pos) + 1;
                size_t size_y_string_end = line.find(",", size_y_string_start);
                size_t size_z_string_start = line.find("=", size_z_pos) + 1;
                size_t size_z_string_end = line.find(")", size_z_string_start);
                
                std::string size_x_string = line.substr(size_x_string_start, size_x_string_end - size_x_string_start);
                std::string size_y_string = line.substr(size_y_string_start, size_y_string_end - size_y_string_start);
                std::string size_z_string = line.substr(size_z_string_start, size_z_string_end - size_z_string_start);

                local_dispatch_size_x = std::stoi(size_x_string);
                local_dispatch_size_y = std::stoi(size_y_string);
                local_dispatch_size_z = std::stoi(size_z_string);

                std::cout << "shader group sizes detected: " << (int)local_dispatch_size_x << ", " << (int)local_dispatch_size_y << ", " << (int)local_dispatch_size_z << std::endl;
            }
        }

        auto define_pos = line.find("#define");
        if (define_pos != std::string::npos) {

            // NUM_BUFFERS
            auto num_buffers_pos = line.find("NUM_BUFFERS", define_pos + 7);
            if (num_buffers_pos != std::string::npos) {
                num_buffer_descriptors = std::stoi(line.substr(num_buffers_pos + 11));
                std::cout << "buffer descriptor count detected: " << (int)num_buffer_descriptors << std::endl;
            }

            // NUM_IMAGES
            auto num_images_pos = line.find("NUM_IMAGES", define_pos + 7);
            if (num_images_pos != std::string::npos) {
                num_image_descriptors = std::stoi(line.substr(num_images_pos + 10));
                std::cout << "image descriptor count detected: " << (int)num_image_descriptors << std::endl;
            }
        }
    }


    std::filesystem::path compiled_shader_path = compile_shader(code_path);

    std::cout << "before load" << code_path << std::endl;

    VkPipelineShaderStageCreateInfo stage_create_info{};
    stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_create_info.module = loaders::load_shader_module(device->vulkan_device, compiled_shader_path.string());
    stage_create_info.pName = "main";

    std::cout << "after load" << std::endl;

    std::array<VkDescriptorSetLayoutBinding, 2> layout_bindings{};

    layout_bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    layout_bindings[0].binding = 0;
    layout_bindings[0].descriptorCount = num_buffer_descriptors;
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    layout_bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    layout_bindings[1].binding = 1;
    layout_bindings[1].descriptorCount = num_image_descriptors;
    layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.bindingCount = layout_bindings.size();
    descriptor_set_layout_create_info.pBindings = layout_bindings.data();

    vkCreateDescriptorSetLayout(device->vulkan_device, &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout);

    VkDescriptorPoolSize pool_size_buffers{};
    pool_size_buffers.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_size_buffers.descriptorCount = num_buffer_descriptors;

    VkDescriptorPoolSize pool_size_images{};
    pool_size_images.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    pool_size_images.descriptorCount = num_image_descriptors;

    VkDescriptorPoolSize pool_sizes[] = {
        pool_size_buffers,
        pool_size_images
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info{};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = 2;
    descriptor_pool_create_info.pPoolSizes = pool_sizes;

    vkCreateDescriptorPool(device->vulkan_device, &descriptor_pool_create_info, nullptr, &descriptor_pool);

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.descriptorPool = descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &descriptor_set_layout;

    vkAllocateDescriptorSets(device->vulkan_device, &descriptor_set_allocate_info, &descriptor_set);

    VkPushConstantRange push_constant_range {};
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(ShaderInterface::PushConstants);
    push_constant_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkPipelineLayoutCreateInfo layout_create_info{};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.setLayoutCount = 1;
    layout_create_info.pSetLayouts = &descriptor_set_layout;
    layout_create_info.pushConstantRangeCount = 1;
    layout_create_info.pPushConstantRanges = &push_constant_range;
    vkCreatePipelineLayout(device->vulkan_device, &layout_create_info, nullptr, &layout);

    VkComputePipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_create_info.layout = layout;
    pipeline_create_info.stage = stage_create_info;

    VkPipelineCacheCreateInfo cache_create_info{};
    cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    vkCreatePipelineCache(device->vulkan_device, &cache_create_info, nullptr, &cache);

    vkCreateComputePipelines(device->vulkan_device, cache, 1, &pipeline_create_info, nullptr, &pipeline);

    vkDestroyShaderModule(device->vulkan_device, stage_create_info.module, nullptr);

    std::cout << "compute shader built" << std::endl;
}

void ComputeShader::dispatch(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent) {
    ShaderInterface::PushConstants push_constants;
    push_constants.swapchain_extent = ShaderInterface::ivec2(swapchain_extent.width, swapchain_extent.height);
    push_constants.render_extent = ShaderInterface::ivec2(render_extent.width, render_extent.height);

    vkCmdPushConstants(command_buffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ShaderInterface::PushConstants), &push_constants);

    dispatch(command_buffer, (float)swapchain_extent.width / (float)local_dispatch_size_x, swapchain_extent.height / (float)local_dispatch_size_y, 1);
}

void ComputeShader::dispatch(VkCommandBuffer command_buffer, uint32_t groups_x, uint32_t groups_y, uint32_t groups_z) {
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &descriptor_set, 0, nullptr);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdDispatch(command_buffer, groups_x, groups_y, groups_z);
}

void ComputeShader::free() {
    vkDestroyPipelineCache(device->vulkan_device, cache, nullptr);
    vkDestroyPipeline(device->vulkan_device, pipeline, nullptr);
    vkDestroyDescriptorPool(device->vulkan_device, descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(device->vulkan_device, descriptor_set_layout, nullptr);
    vkDestroyPipelineLayout(device->vulkan_device, layout, nullptr);
}

ComputeShader::ComputeShader(Device* device, std::string path) {
    this->device = device;
    this->code_path = path;
}