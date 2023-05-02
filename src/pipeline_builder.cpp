#include "pipeline_builder.h"

#include "buffer.h"

#include "loaders/shader_spirv.h"

#include <iostream>

VkShaderModule PipelineBuilder::create_shader_module(const std::vector<char> &code)
{
    VkShaderModuleCreateInfo create_info{};
    memset(&create_info, 0, sizeof(VkShaderModuleCreateInfo));
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());
    create_info.flags = 0;
    create_info.pNext = nullptr;

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device->vulkan_device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating shader module");
    }

    return shader_module;
}

PipelineBuilder PipelineBuilder::add_stage(VkShaderStageFlagBits stage, std::string code_path) {
    shader_stages.push_back(PipelineBuilderShaderStage{
        stage,
        code_path
    });

    return *this;
}

void Pipeline::free() {
    // free sbt
    sbt.raygen.free();
    sbt.hit.free();
    sbt.miss.free();

    vkDestroyDescriptorPool(device_handle, descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(device_handle, descriptor_set_layout, nullptr);

    vkDestroyPipeline(device_handle, pipeline_handle, nullptr);
    vkDestroyPipelineLayout(device_handle, pipeline_layout_handle, nullptr);
    vkDestroyPipelineCache(device_handle, pipeline_cache_handle, nullptr);
}

Pipeline PipelineBuilder::build() {
    Pipeline result;
    result.device_handle = device->vulkan_device;

    #pragma region DESCRIPTOR SET LAYOUT

    VkDescriptorSetLayoutBinding image_layout_binding{};
    image_layout_binding.binding = 0;
    image_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    image_layout_binding.descriptorCount = 1;
    image_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding as_layout_binding{};
    as_layout_binding.binding = 1;
    as_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    as_layout_binding.descriptorCount = 1;
    as_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutBinding cam_layout_binding{};
    cam_layout_binding.binding = 2;
    cam_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cam_layout_binding.descriptorCount = 1;
    cam_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding normal_index_layout_binding{};
    normal_index_layout_binding.binding = 3;
    normal_index_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    normal_index_layout_binding.descriptorCount = 1;
    normal_index_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutBinding normal_layout_binding{};
    normal_layout_binding.binding = 4;
    normal_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    normal_layout_binding.descriptorCount = 1;
    normal_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutBinding bindings[] = {
        image_layout_binding,
        as_layout_binding,
        cam_layout_binding,
        normal_index_layout_binding,
        normal_layout_binding};

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding);
    layout_info.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device->vulkan_device, &layout_info, nullptr, &result.descriptor_set_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating descriptor set layout");
    }
#pragma endregion

    #pragma region DESCRIPTOR POOL
    VkDescriptorPoolSize image_pool_size{};
    image_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    image_pool_size.descriptorCount = device->image_count;

    VkDescriptorPoolSize as_pool_size{};
    as_pool_size.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    as_pool_size.descriptorCount = device->image_count;

    VkDescriptorPoolSize cam_pool_size{};
    cam_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cam_pool_size.descriptorCount = device->image_count;

    VkDescriptorPoolSize normal_index_pool_size{};
    normal_index_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    normal_index_pool_size.descriptorCount = device->image_count;

    VkDescriptorPoolSize normal_pool_size{};
    normal_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    normal_pool_size.descriptorCount = device->image_count;

    VkDescriptorPoolSize pool_sizes[] = {
        image_pool_size,
        as_pool_size,
        cam_pool_size,
        normal_index_pool_size,
        normal_pool_size
    };

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize);
    pool_info.pPoolSizes = pool_sizes;
    pool_info.maxSets = device->image_count;

    if (vkCreateDescriptorPool(device->vulkan_device, &pool_info, nullptr, &result.descriptor_pool) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating descriptor pool");
    }
#pragma endregion

    #pragma region DESCRIPTOR SETS
    result.descriptor_sets.resize(device->image_count);

    std::vector<VkDescriptorSetLayout> layouts(device->image_count, result.descriptor_set_layout);
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = result.descriptor_pool;
    alloc_info.descriptorSetCount = device->image_count;
    alloc_info.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(device->vulkan_device, &alloc_info, result.descriptor_sets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("error allocating descriptor sets");
    }
    #pragma endregion


    std::vector<VkShaderModule> generated_shader_modules;
    std::vector<VkPipelineShaderStageCreateInfo> stage_create_infos;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> group_create_infos;

    std::cout << "Building pipeline with " << shader_stages.size() << " stages" << std::endl;

    for (auto stage : shader_stages)
    {
        auto code = loaders::read_spirv(stage.shader_code_path);
        VkShaderModule module = create_shader_module(code);
        generated_shader_modules.push_back(module);

        VkPipelineShaderStageCreateInfo stage_create_info{};
        stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage_create_info.stage = stage.stage;
        stage_create_info.module = module;
        stage_create_info.pName = stage.shader_entry_point;
        stage_create_info.pSpecializationInfo = nullptr;
        stage_create_info.flags = 0;
        stage_create_info.pNext = nullptr;

        uint32_t stage_index = (uint32_t)stage_create_infos.size();
        std::cout << "Stage " << stage_index << ": " << stage.stage << " with code at " << stage.shader_code_path << ". Entry point: " << stage.shader_entry_point << std::endl;
        stage_create_infos.push_back(stage_create_info);

        VkRayTracingShaderGroupCreateInfoKHR group_create_info{};
        group_create_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        group_create_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        group_create_info.generalShader = VK_SHADER_UNUSED_KHR;
        group_create_info.closestHitShader = VK_SHADER_UNUSED_KHR;
        group_create_info.anyHitShader = VK_SHADER_UNUSED_KHR;
        group_create_info.intersectionShader = VK_SHADER_UNUSED_KHR;
        group_create_info.pNext = nullptr;

        switch (stage.stage)
        {
        case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
        case VK_SHADER_STAGE_MISS_BIT_KHR:
            group_create_info.generalShader = stage_index;
            break;

        case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
            group_create_info.closestHitShader = stage_index;
            group_create_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            break;
        }

        group_create_infos.push_back(group_create_info);
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &result.descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pNext = 0;
    pipeline_layout_info.flags = 0;

    if (vkCreatePipelineLayout(device->vulkan_device, &pipeline_layout_info, nullptr, &result.pipeline_layout_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating pipeline layout");
    }

    VkRayTracingPipelineCreateInfoKHR pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipeline_info.layout = result.pipeline_layout_handle;
    pipeline_info.stageCount = (uint32_t)stage_create_infos.size();
    pipeline_info.pStages = stage_create_infos.data();
    pipeline_info.groupCount = (uint32_t)group_create_infos.size();
    pipeline_info.pGroups = group_create_infos.data();

    VkPipelineCacheCreateInfo cache_create_info{};
    cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    if (vkCreatePipelineCache(device->vulkan_device, &cache_create_info, nullptr, &result.pipeline_cache_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating pipeline cache");
    }

    std::cout << "created pipeline cache" << std::endl;

    if (device->vkCreateRayTracingPipelinesKHR(device->vulkan_device, VK_NULL_HANDLE, result.pipeline_cache_handle, 1, &pipeline_info, nullptr, &result.pipeline_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating raytracing pipeline");
    }

    std::cout << "created pipeline" << std::endl;

    for (auto module : generated_shader_modules)
    {
        vkDestroyShaderModule(device->vulkan_device, module, nullptr);
    }

#pragma region SBT CREATION

    int group_handle_size = device->ray_tracing_pipeline_properties.shaderGroupHandleSize;
    size_t shader_binding_table_size = group_handle_size * shader_stages.size();

    uint8_t *shader_binding_table_data = new uint8_t[shader_binding_table_size];

    device->vkGetRayTracingShaderGroupHandlesKHR(device->vulkan_device, result.pipeline_handle, 0, (uint32_t)shader_stages.size(), shader_binding_table_size, shader_binding_table_data);

    VkBufferCreateInfo sbt_buffer_info{};
    sbt_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    sbt_buffer_info.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
    sbt_buffer_info.size = group_handle_size;

    result.sbt.raygen = device->create_buffer(&sbt_buffer_info);
    result.sbt.raygen.set_data(shader_binding_table_data);

    result.sbt.hit = device->create_buffer(&sbt_buffer_info);
    result.sbt.hit.set_data(shader_binding_table_data + group_handle_size);

    result.sbt.miss = device->create_buffer(&sbt_buffer_info);
    result.sbt.miss.set_data(shader_binding_table_data + group_handle_size * 2);

    // Buffer callable_buffer = device->create_buffer(&sbt_buffer_info);
    // callable_buffer.set_data(shader_binding_table_data + group_handle_size * 3);

#pragma endregion

    return result;
}