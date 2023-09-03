#include "pipeline_builder.h"

#include "buffer.h"
#include "memory.h"

#include "loaders/shader_spirv.h"

#include <iostream>
#include <unordered_set>
#include <sstream>
#include <filesystem>

VkShaderModule PipelineBuilder::create_shader_module(const std::vector<char> &code)
{
    VkShaderModuleCreateInfo create_info{};
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

void PipelineBuilder::add_stage(VkShaderStageFlagBits stage, std::string code_path) {
    shader_stages.push_back(PipelineBuilderShaderStage{
        stage,
        code_path
    });
}

void PipelineBuilder::add_descriptor(std::string name, uint32_t set, uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, size_t descriptor_count) {
    descriptors.push_back(PipelineBuilderDescriptor {
        name,
        set,
        binding,
        descriptor_count,
        type,
        stage
    });
}

void PipelineBuilder::add_output_image(std::string name) {
    output_images.push_back(PipelineBuilderOutputImage {
        name
    });
}

PipelineBuilder PipelineBuilder::with_output_image_descriptor(std::string name, uint32_t set, uint32_t binding) {
    output_image_name = name;
    output_image_set = set;
    output_image_binding = binding;

    return *this;
}

PipelineBuilder PipelineBuilder::with_default_pipeline() {
    // framework descriptors (set 0)
    add_descriptor("acceleration_structure", 0, 0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    add_descriptor("camera_parameters", 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    with_output_image_descriptor("images", 0, 2);
    add_output_image("result");
    add_output_image("color_accum");
    add_output_image("instance_indices");
    add_output_image("instance_indices_colored");
    add_output_image("albedo");
    add_output_image("normals");
    add_output_image("roughness");
    // object (meshes + materials + textures) descriptors (set 1)
    add_descriptor("mesh_indices", 1, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    add_descriptor("mesh_vertices", 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    add_descriptor("mesh_normals", 1, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    add_descriptor("mesh_texcoords", 1, 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    add_descriptor("mesh_tangents", 1, 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    add_descriptor("mesh_data_offsets", 1, 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    add_descriptor("mesh_offset_indices", 1, 6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    add_descriptor("textures", 1, 7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 128);
    add_descriptor("texture_indices", 1, 8, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    add_descriptor("material_parameters", 1, 9, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    // other scene descriptors (set 2)
    add_descriptor("lights", 2, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    // shader stages
    add_stage(VK_SHADER_STAGE_RAYGEN_BIT_KHR, "./shaders/ray_gen.rgen");
    add_stage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "./shaders/closest_hit.rchit");
    add_stage(VK_SHADER_STAGE_MISS_BIT_KHR, "./shaders/miss.rmiss");

    return *this;
}

Pipeline::SetBinding Pipeline::get_descriptor_set_binding(std::string name) {
    return named_descriptors[name];
}

void Pipeline::set_descriptor_image_binding(std::string name, Image image, ImageType image_type, uint32_t array_index) {
    SetBinding set_binding = get_descriptor_set_binding(name);

    VkDescriptorImageInfo image_info{};
    image_info.imageView = image.view_handle;
    image_info.imageLayout = image.layout;

    VkWriteDescriptorSet descriptor_write_image{};
    descriptor_write_image.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write_image.dstSet = descriptor_sets[set_binding.set];
    descriptor_write_image.dstBinding = set_binding.binding;
    descriptor_write_image.dstArrayElement = array_index;
    descriptor_write_image.descriptorType = (VkDescriptorType)image_type;
    descriptor_write_image.descriptorCount = 1;
    descriptor_write_image.pImageInfo = &image_info;

    vkUpdateDescriptorSets(device->vulkan_device, 1, &descriptor_write_image, 0, nullptr);
}

void Pipeline::set_descriptor_buffer_binding(std::string name, Buffer& buffer, BufferType buffer_type) {
    SetBinding set_binding = get_descriptor_set_binding(name);

    VkDescriptorBufferInfo buffer_write_info{};
    buffer_write_info.buffer = buffer.buffer_handle;
    buffer_write_info.range = VK_WHOLE_SIZE;
    buffer_write_info.offset = 0;

    VkWriteDescriptorSet descriptor_write_buffer{};
    descriptor_write_buffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write_buffer.dstSet = descriptor_sets[set_binding.set];
    descriptor_write_buffer.dstBinding = set_binding.binding;
    descriptor_write_buffer.dstArrayElement = 0;
    descriptor_write_buffer.descriptorType = (VkDescriptorType)buffer_type;
    descriptor_write_buffer.descriptorCount = 1;
    descriptor_write_buffer.pBufferInfo = &buffer_write_info;

    vkUpdateDescriptorSets(device->vulkan_device, 1, &descriptor_write_buffer, 0, nullptr);
}

void Pipeline::set_descriptor_sampler_binding(std::string name, Image* images, size_t image_count) {
    SetBinding set_binding = get_descriptor_set_binding(name);

    for (int i = 0; i < 128; i++) {
        VkWriteDescriptorSet descriptor_write_sampler{};
        descriptor_write_sampler.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write_sampler.dstSet = descriptor_sets[set_binding.set];
        descriptor_write_sampler.dstBinding = set_binding.binding;
        descriptor_write_sampler.dstArrayElement = i;
        descriptor_write_sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_write_sampler.descriptorCount = 1;

        VkDescriptorImageInfo image_info{};
        if (i < image_count) {
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = images[i].view_handle;
            image_info.sampler = images[i].sampler_handle;
            descriptor_write_sampler.pImageInfo = &image_info;
        } else {
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = images[0].view_handle;
            image_info.sampler = images[0].sampler_handle;
            descriptor_write_sampler.pImageInfo = &image_info;
        }

        vkUpdateDescriptorSets(device->vulkan_device, 1, &descriptor_write_sampler, 0, nullptr);
    }
}

void Pipeline::cmd_recreate_output_images(VkCommandBuffer command_buffer, VkExtent2D image_extent) {
    for (int i = 0; i < output_images.size(); i++) {
        if (output_images[i].width > 0 && output_images[i].height > 0) output_images[i].free();
        output_images[i] = device->create_image(image_extent.width, image_extent.height, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, 1,
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }
}

Image Pipeline::get_output_image(std::string name) {
    uint32_t index = named_output_image_indices[name];
    return output_images[index];
}

void Pipeline::free() {
    for (int i = 0; i < output_images.size(); i++) {
        output_images[i].free();
    }

    // free sbt
    sbt.buffer.free();

    vkDestroyDescriptorPool(device->vulkan_device, descriptor_pool, nullptr);
    for (auto layout : descriptor_set_layouts) {
        vkDestroyDescriptorSetLayout(device->vulkan_device, layout, nullptr);
    }

    vkDestroyPipeline(device->vulkan_device, pipeline_handle, nullptr);
    vkDestroyPipelineLayout(device->vulkan_device, pipeline_layout_handle, nullptr);
    vkDestroyPipelineCache(device->vulkan_device, pipeline_cache_handle, nullptr);
}

Pipeline PipelineBuilder::build() {
    Pipeline result;
    result.device = device;

    // scan for highest descriptor set number
    uint32_t max_set = 0;
    for (auto desc : descriptors) {
        if (desc.set > max_set) max_set = desc.set;
    }
    result.max_set = max_set;

    // apply output image descriptor
    if (output_image_name == "") {
        // no output image descriptor is set
        throw std::runtime_error("no output image descriptor is set");
    } else {
        add_descriptor(output_image_name, output_image_set, output_image_binding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR, output_images.size());
        result.output_image_binding_name = output_image_name;
    }

    result.output_images.resize(output_images.size());
    for (int i = 0; i < output_images.size(); i++) {
        result.named_output_image_indices[output_images[i].name] = i;
    }

    #pragma region DESCRIPTOR SET LAYOUT
    result.descriptor_set_layouts.resize(max_set + 1);
    for (uint32_t current_set = 0; current_set <= max_set; current_set++) {
        std::unordered_set<uint32_t> bound_bindings;
        std::vector<VkDescriptorSetLayoutBinding> set_bindings;

        for (auto descriptor : descriptors) {
            if (descriptor.set == current_set) {
                VkDescriptorSetLayoutBinding descriptor_binding{};
                descriptor_binding.binding = descriptor.binding;
                descriptor_binding.descriptorCount = descriptor.descriptor_count;
                descriptor_binding.descriptorType = descriptor.descriptor_type;
                descriptor_binding.stageFlags = descriptor.stage_flags;
                descriptor_binding.pImmutableSamplers = nullptr;

                if (bound_bindings.find(descriptor.binding) == bound_bindings.end()) {
                    set_bindings.push_back(descriptor_binding);
                    result.named_descriptors[descriptor.name] = Pipeline::SetBinding{current_set, descriptor.binding};
                } else {
                    throw std::runtime_error("descriptor in set " + std::to_string(current_set) + ", binding " + std::to_string(descriptor.binding) + " is already bound.");
                }
            }
        }

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = set_bindings.size();
        layout_info.pBindings = set_bindings.data();

        if (vkCreateDescriptorSetLayout(device->vulkan_device, &layout_info, nullptr, &result.descriptor_set_layouts[current_set]) != VK_SUCCESS)
        {
            throw std::runtime_error("error creating descriptor set layout");
        }
    }

#pragma endregion

    #pragma region DESCRIPTOR POOL
    std::vector<VkDescriptorPoolSize> pool_sizes;

    for (auto descriptor : descriptors) {
        VkDescriptorPoolSize descriptor_pool_size{};
        descriptor_pool_size.type = descriptor.descriptor_type;
        descriptor_pool_size.descriptorCount = descriptor.descriptor_count;

        pool_sizes.push_back(descriptor_pool_size);
    }

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = pool_sizes.size();
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = max_set + 1;

    if (vkCreateDescriptorPool(device->vulkan_device, &pool_info, nullptr, &result.descriptor_pool) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating descriptor pool");
    }
#pragma endregion

    #pragma region DESCRIPTOR SETS
    result.descriptor_sets.resize(max_set + 1);

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = result.descriptor_pool;
    alloc_info.descriptorSetCount = max_set + 1;
    alloc_info.pSetLayouts = result.descriptor_set_layouts.data();

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
        // compile shader
        std::filesystem::path shader_path(stage.shader_code_path);
        std::filesystem::path shader_out_path(stage.shader_code_path);
        shader_path.make_preferred();
        shader_out_path.replace_extension(".spv");
        shader_out_path.make_preferred();
        std::stringstream compile_command;
        compile_command << GLSLC_EXE << " --target-env=vulkan1.3 -O -o " << std::filesystem::absolute(shader_out_path) << " " << std::filesystem::absolute(shader_path);
        std::cout << "COMPILING SHADER: " << compile_command.str() << std::endl;
        system(compile_command.str().c_str());

        auto code = loaders::read_spirv(shader_out_path.string());
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
        std::cout << "Stage " << stage_index << ": " << stage.stage << " with code at " << shader_out_path.string() << ". Entry point: " << stage.shader_entry_point << std::endl;
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
        case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
            group_create_info.generalShader = stage_index;
            break;

        case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
            group_create_info.closestHitShader = stage_index;
            group_create_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            break;
        }

        group_create_infos.push_back(group_create_info);
    }

    VkPushConstantRange push_constant_range{};
    push_constant_range.offset = 0;
    push_constant_range.size = 16;
    push_constant_range.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = result.descriptor_set_layouts.size();
    pipeline_layout_info.pSetLayouts = result.descriptor_set_layouts.data();
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
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
    pipeline_info.maxPipelineRayRecursionDepth = device->ray_tracing_pipeline_properties.maxRayRecursionDepth;

    VkPipelineCacheCreateInfo cache_create_info{};
    cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    if (vkCreatePipelineCache(device->vulkan_device, &cache_create_info, nullptr, &result.pipeline_cache_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating pipeline cache");
    }

    if (device->vkCreateRayTracingPipelinesKHR(device->vulkan_device, VK_NULL_HANDLE, result.pipeline_cache_handle, 1, &pipeline_info, nullptr, &result.pipeline_handle) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating raytracing pipeline");
    }

    for (auto module : generated_shader_modules)
    {
        vkDestroyShaderModule(device->vulkan_device, module, nullptr);
    }

#pragma region SBT CREATION
    uint32_t group_handle_size = device->ray_tracing_pipeline_properties.shaderGroupHandleSize;
    uint32_t handle_alignment = device->ray_tracing_pipeline_properties.shaderGroupHandleAlignment;
    uint32_t base_alignment = device->ray_tracing_pipeline_properties.shaderGroupBaseAlignment;
    uint32_t group_handle_size_aligned = memory::align_up(group_handle_size, handle_alignment);

    std::cout << "SHADER GROUPS: Handle Size: " << group_handle_size << " | Handle Alignment: " << handle_alignment << " | Aligned Size: " << group_handle_size_aligned << std::endl;

    uint32_t group_count = group_create_infos.size();
    size_t shader_binding_table_size = group_count * group_handle_size;

    result.sbt.region_raygen.stride = memory::align_up(group_handle_size_aligned, base_alignment);
    result.sbt.region_raygen.size = result.sbt.region_raygen.stride;

    result.sbt.region_hit.stride = group_handle_size_aligned;
    result.sbt.region_hit.size = memory::align_up(group_handle_size_aligned, base_alignment);

    result.sbt.region_miss.stride = group_handle_size_aligned;
    result.sbt.region_miss.size = memory::align_up(group_handle_size_aligned, base_alignment);

    result.sbt.region_callable.stride = group_handle_size_aligned;
    result.sbt.region_callable.size = memory::align_up(group_handle_size_aligned  * 1/* multiply with number of callable shaders */, base_alignment);

    std::vector<uint8_t> shader_binding_table_data(shader_binding_table_size);
    if (device->vkGetRayTracingShaderGroupHandlesKHR(device->vulkan_device, result.pipeline_handle, 0, group_count, shader_binding_table_size, shader_binding_table_data.data()) != VK_SUCCESS) {
        throw std::runtime_error("error getting shader group handles");
    }

    VkBufferCreateInfo sbt_buffer_info{};
    sbt_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    sbt_buffer_info.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
    sbt_buffer_info.size = result.sbt.region_raygen.size + result.sbt.region_hit.size + result.sbt.region_miss.size + result.sbt.region_callable.size;
    result.sbt.buffer = device->create_buffer(&sbt_buffer_info, true);


    result.sbt.region_raygen.deviceAddress = result.sbt.buffer.device_address;
    result.sbt.buffer.set_data(shader_binding_table_data.data(), 0, group_handle_size);

    size_t buffer_offset = result.sbt.region_raygen.size;
    result.sbt.region_hit.deviceAddress = result.sbt.buffer.device_address + result.sbt.region_raygen.size;
    result.sbt.buffer.set_data(shader_binding_table_data.data() + 1 * group_handle_size, buffer_offset, group_handle_size);

    buffer_offset += result.sbt.region_hit.size;
    result.sbt.region_miss.deviceAddress = result.sbt.buffer.device_address + result.sbt.region_raygen.size + result.sbt.region_hit.size;
    result.sbt.buffer.set_data(shader_binding_table_data.data() + 2 * group_handle_size, buffer_offset, group_handle_size);

    buffer_offset += result.sbt.region_miss.size;
    result.sbt.region_callable.deviceAddress = result.sbt.buffer.device_address + result.sbt.region_raygen.size + result.sbt.region_hit.size + result.sbt.region_miss.size;
    result.sbt.buffer.set_data(shader_binding_table_data.data() + 3 * group_handle_size, buffer_offset, group_handle_size);

#pragma endregion

    return result;
    
}