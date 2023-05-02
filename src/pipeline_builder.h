#pragma once
#include "device.h"

#include <vector>
#include <string>

struct ShaderBindingTable
{
    Buffer raygen;
    Buffer hit;
    Buffer miss;
};

struct Pipeline {
    VkDevice device_handle;

    VkPipeline pipeline_handle;
    VkPipelineLayout pipeline_layout_handle;
    VkPipelineCache pipeline_cache_handle;

    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;

    ShaderBindingTable sbt;

    void free();
};

struct PipelineBuilderDescriptor
{
};

struct PipelineBuilderShaderStage
{
    VkShaderStageFlagBits stage;
    std::string shader_code_path;
    const char *shader_entry_point = "main";
};

struct PipelineBuilder
{
    Device* device;
    std::vector<PipelineBuilderShaderStage> shader_stages;

    VkShaderModule create_shader_module(const std::vector<char>& code);

    public:
    PipelineBuilder add_descriptor();
    PipelineBuilder add_stage(VkShaderStageFlagBits stage, std::string shader_code_path);
    Pipeline build();
};