#pragma once
#include "device.h"

#include <vector>
#include <string>
#include <unordered_map>

struct ShaderBindingTable
{
    Buffer raygen;
    Buffer hit;
    Buffer miss;
};

struct Pipeline {
    VkDevice device_handle;
    uint32_t max_set;

    VkPipeline pipeline_handle;
    VkPipelineLayout pipeline_layout_handle;
    VkPipelineCache pipeline_cache_handle;

    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;

    ShaderBindingTable sbt;

    struct SetBinding {
        uint32_t set;
        uint32_t binding;
    };
    std::unordered_map<std::string, SetBinding> named_descriptors;


    Pipeline::SetBinding get_descriptor_set_binding(std::string descriptor_name);
    void set_descriptor_image_binding(std::string name, VkImageView image_view);
    void set_descriptor_buffer_binding(std::string name, Buffer buffer, BufferType buffer_type);

    void free();
};

struct PipelineBuilderDescriptor
{
    std::string name;
    uint32_t set;
    uint32_t binding;
    VkDescriptorType descriptor_type;
    VkShaderStageFlags stage_flags;
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
    std::vector<PipelineBuilderDescriptor> descriptors;

    VkShaderModule create_shader_module(const std::vector<char>& code);

    public:
    PipelineBuilder add_descriptor(std::string name, uint32_t set, uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage);
    PipelineBuilder add_stage(VkShaderStageFlagBits stage, std::string shader_code_path);
    Pipeline build();
};