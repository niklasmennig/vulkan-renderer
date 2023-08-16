#pragma once

#include "device.h"
#include "loaders/image.h"

#include <vector>
#include <string>
#include <unordered_map>

enum class BufferType
{
    Uniform = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    Storage = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
};

enum class ImageType {
    Storage = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    Sampled = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
};

struct ShaderBindingTable
{
    Buffer buffer;

    VkStridedDeviceAddressRegionKHR region_raygen;
    VkStridedDeviceAddressRegionKHR region_hit;
    VkStridedDeviceAddressRegionKHR region_miss;
    VkStridedDeviceAddressRegionKHR region_callable;
};

struct Pipeline {
    Device* device;
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
    std::unordered_map<std::string, uint32_t> named_output_image_indices;
    
    std::vector<Image> output_images;
    std::string output_image_binding_name;

    Pipeline::SetBinding get_descriptor_set_binding(std::string descriptor_name);
    void set_descriptor_image_binding(std::string name, Image image, ImageType image_type, uint32_t array_index = 0);
    void set_descriptor_buffer_binding(std::string name, Buffer& buffer, BufferType buffer_type);
    void set_descriptor_sampler_binding(std::string name, Image* images, size_t image_count = 1);

    void cmd_recreate_output_images(VkCommandBuffer command_buffer, VkExtent2D image_extent);

    Image get_output_image(std::string name);

    void free();
};

struct PipelineBuilderDescriptor
{
    std::string name;
    uint32_t set;
    uint32_t binding;
    size_t descriptor_count;
    VkDescriptorType descriptor_type;
    VkShaderStageFlags stage_flags;
};

struct PipelineBuilderShaderStage
{
    VkShaderStageFlagBits stage;
    std::string shader_code_path;
    const char *shader_entry_point = "main";
};

struct PipelineBuilderOutputImage
{
    std::string name;
};

struct PipelineBuilder
{
    Device* device;
    std::vector<PipelineBuilderShaderStage> shader_stages;
    std::vector<PipelineBuilderDescriptor> descriptors;
    std::vector<PipelineBuilderOutputImage> output_images;

    std::string output_image_name = "";
    uint32_t output_image_set, output_image_binding;

    VkShaderModule create_shader_module(const std::vector<char>& code);

    private:
    void add_descriptor(std::string name, uint32_t set, uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, size_t descriptor_count = 1);
    void add_stage(VkShaderStageFlagBits stage, std::string shader_code_path);
    void add_output_image(std::string name);

    public:
    PipelineBuilder with_output_image_descriptor(std::string name, uint32_t set, uint32_t binding);
    PipelineBuilder with_default_pipeline();

    Pipeline build();
};