#pragma once

#include "core/device.h"
#include "loaders/image.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#include "pipeline/pipeline_stage.h"

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

struct OutputImage
{
    Image image;
    std::string name;
    bool hidden;
    bool update_buffer;
    VkFormat format;
};

struct DescriptorSetBinding {
    uint32_t set;
    uint32_t binding;
};

struct Pipeline {
    Device* device;
    PipelineBuilder* builder;

    VkPipeline pipeline_handle;
    VkPipelineCache pipeline_cache_handle;

    ShaderBindingTable sbt;
    VkDeviceSize sbt_stride;

    std::string output_image_binding_name;

    DescriptorSetBinding get_descriptor_set_binding(std::string descriptor_name);
    void set_descriptor_acceleration_structure_binding(VkAccelerationStructureKHR acceleration_structure);
    void set_descriptor_image_binding(std::string name, Image image, ImageType image_type, uint32_t array_index = 0);
    void set_descriptor_buffer_binding(std::string name, Buffer& buffer, BufferType buffer_type);
    void set_descriptor_sampler_binding(std::string name, Image* images, size_t image_count = 1);

    void cmd_recreate_output_images(VkCommandBuffer command_buffer, VkExtent2D image_extent);
    void cmd_update_output_image_buffers(VkCommandBuffer command_buffer);

    OutputImage get_output_image(std::string name);

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

struct PipelineBuilderOutputImage
{
    std::string name;
    VkFormat format;
    bool hidden;
    bool update_buffers;
};

struct PipelineBuilder
{
    Device* device;
    std::vector<std::shared_ptr<PipelineStage>> shader_stages;
    std::vector<PipelineBuilderDescriptor> descriptors;
    std::vector<PipelineBuilderOutputImage> output_images;

    std::string output_image_name = "";
    uint32_t output_image_set, output_image_binding;

    VkShaderModule create_shader_module(const std::vector<char>& code);

    private:
    uint32_t hit_stages = 0;
    uint32_t miss_stages = 0;
    uint32_t callable_stages = 0;

    void add_descriptor(std::string name, uint32_t set, uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, size_t descriptor_count = 1);
    void add_stage(std::shared_ptr<PipelineStage> stage);
    void add_output_image(std::string name, VkFormat format = VK_FORMAT_UNDEFINED, bool hidden = false, bool update_buffer = true);


    public:
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    uint8_t max_set = 0;
    std::vector<OutputImage> created_output_images;
    std::unordered_map<std::string, uint32_t> named_output_image_indices;
    std::unordered_map<std::string, DescriptorSetBinding> named_descriptors;
    std::vector<VkDescriptorSet> descriptor_sets;
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
    VkDescriptorPool descriptor_pool;
    PipelineBuilder with_output_image_descriptor(std::string name, uint32_t set, uint32_t binding);
    PipelineBuilder with_default_pipeline();

    PipelineBuilder with_buffer_descriptor(std::string name, uint32_t binding, VkShaderStageFlags stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR);

    Pipeline build();
    void free();
};