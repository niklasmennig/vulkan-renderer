#include "pipeline_builder.h"

#include "core/device.h"
#include "loaders/shader_spirv.h"
#include "shader_compiler.h"

#include <array>

ProcessingPipelineBuilder Device::create_processing_pipeline_builder() {
    ProcessingPipelineBuilder builder;
    builder.device = this;

    return builder;
}

void ProcessingPipeline::free() {
    vkDestroyPipelineCache(device->vulkan_device, cache, nullptr);
    vkDestroyPipeline(device->vulkan_device, pipeline, nullptr);
}

ProcessingPipeline ProcessingPipelineBuilder::build() {
    ProcessingPipeline result;
    result.device = device;
    result.builder = this;

    std::string shader_path = "shaders/processing/test.comp";

    std::filesystem::path compiled_shader_path = compile_shader(shader_path);

    VkPipelineShaderStageCreateInfo stage_create_info{};
    stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_create_info.module = loaders::load_shader_module(device->vulkan_device, compiled_shader_path.string());
    stage_create_info.pName = "main";

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
    vkCreatePipelineCache(device->vulkan_device, &cache_create_info, nullptr, &result.cache);

    vkCreateComputePipelines(device->vulkan_device, result.cache, 1, &pipeline_create_info, nullptr, &result.pipeline);

    vkDestroyShaderModule(device->vulkan_device, stage_create_info.module, nullptr);

    return result;
}

void ProcessingPipelineBuilder::free() {
    vkDestroyDescriptorPool(device->vulkan_device, descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(device->vulkan_device, descriptor_set_layout, nullptr);
    vkDestroyPipelineLayout(device->vulkan_device, layout, nullptr);
}