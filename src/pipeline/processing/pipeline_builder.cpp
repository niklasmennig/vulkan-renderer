#include "pipeline_builder.h"

#include "core/device.h"
#include "loaders/shader_spirv.h"
#include "shader_compiler.h"

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

    std::string shader_path = "shaders/processing/test.comp";

    std::filesystem::path compiled_shader_path = compile_shader(shader_path);

    VkPipelineShaderStageCreateInfo stage_create_info{};
    stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_create_info.module = loaders::load_shader_module(device->vulkan_device, compiled_shader_path.string());
    stage_create_info.pName = "main";

    VkPipelineLayoutCreateInfo layout_create_info{};
    layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
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
    vkDestroyPipelineLayout(device->vulkan_device, layout, nullptr);
}