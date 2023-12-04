#include "compute_shader.h"

#include <string>
#include <filesystem>
#include <array>
#include <iostream>

#include "core/device.h"
#include "loaders/shader_spirv.h"
#include "shader_compiler.h"

void ComputeShader::set_image(int binding, Image& img) {
    VkDescriptorImageInfo compute_descriptor_image_info{};
    compute_descriptor_image_info.imageLayout = img.layout;
    compute_descriptor_image_info.imageView = img.view_handle;

    VkWriteDescriptorSet compute_descriptor_write{};
    compute_descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    compute_descriptor_write.dstSet = descriptor_set;
    compute_descriptor_write.dstBinding = binding;
    compute_descriptor_write.dstArrayElement = 0;
    compute_descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    compute_descriptor_write.descriptorCount = 1;
    compute_descriptor_write.pImageInfo = &compute_descriptor_image_info;

    vkUpdateDescriptorSets(device->vulkan_device, 1, &compute_descriptor_write, 0, nullptr);
}

void ComputeShader::build() {
    std::string shader_path = "shaders/processing/test.comp";

    std::filesystem::path compiled_shader_path = compile_shader(shader_path);

    std::cout << "before load" << std::endl;

    VkPipelineShaderStageCreateInfo stage_create_info{};
    stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_create_info.module = loaders::load_shader_module(device->vulkan_device, compiled_shader_path.string());
    stage_create_info.pName = "main";

    std::cout << "after load" << std::endl;

    std::array<VkDescriptorSetLayoutBinding, 1> layout_bindings{};

    layout_bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    layout_bindings[0].binding = 0;
    layout_bindings[0].descriptorCount = 1;
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.bindingCount = layout_bindings.size();
    descriptor_set_layout_create_info.pBindings = layout_bindings.data();

    vkCreateDescriptorSetLayout(device->vulkan_device, &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout);

    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    pool_size.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info{};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = 1;
    descriptor_pool_create_info.pPoolSizes = &pool_size;

    vkCreateDescriptorPool(device->vulkan_device, &descriptor_pool_create_info, nullptr, &descriptor_pool);

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.descriptorPool = descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &descriptor_set_layout;

    vkAllocateDescriptorSets(device->vulkan_device, &descriptor_set_allocate_info, &descriptor_set);

    VkPipelineLayoutCreateInfo layout_create_info{};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.setLayoutCount = 1;
    layout_create_info.pSetLayouts = &descriptor_set_layout;
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

    std::cout << "Comput Shader built" << std::endl;
}

void ComputeShader::dispatch(VkCommandBuffer command_buffer, VkExtent2D image_extent) {
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &descriptor_set, 0, nullptr);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdDispatch(command_buffer, image_extent.width / local_dispatch_size_x, image_extent.height / local_dispatch_size_y, 1);
}

void ComputeShader::free() {
    vkDestroyPipelineCache(device->vulkan_device, cache, nullptr);
    vkDestroyPipeline(device->vulkan_device, pipeline, nullptr);
    vkDestroyDescriptorPool(device->vulkan_device, descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(device->vulkan_device, descriptor_set_layout, nullptr);
    vkDestroyPipelineLayout(device->vulkan_device, layout, nullptr);
}