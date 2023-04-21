#include "vulkan_application.h"
#include "loaders/shader_spirv.h"

#include <set>
#include <iostream>

#include "loaders/geometry_obj.h"

#pragma region VULKAN DEBUGGING
const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"
};

VkResult VulkanApplication::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanApplication::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}
#pragma endregion


Buffer VulkanApplication::create_buffer(VkBufferCreateInfo* create_info) {
    Buffer result{};

    create_info->usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (vkCreateBuffer(logical_device, create_info, nullptr, &result.buffer_handle) != VK_SUCCESS) {
        throw std::runtime_error("error creating buffer");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(logical_device, result.buffer_handle, &mem_requirements);

    // find correct memory type
    uint32_t type_filter = mem_requirements.memoryTypeBits;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    uint32_t memtype_index = 0;
    for (; memtype_index < memory_properties.memoryTypeCount; memtype_index++) {
        if ((type_filter & (1 << memtype_index)) && (memory_properties.memoryTypes[memtype_index].propertyFlags & properties) == properties) {
            break;
        }
    }

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = memtype_index;

    VkMemoryAllocateFlagsInfo alloc_flags{};
    alloc_flags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    alloc_flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    alloc_info.pNext = &alloc_flags;

    if (vkAllocateMemory(logical_device, &alloc_info, nullptr, &result.device_memory) != VK_SUCCESS) {
        throw std::runtime_error("error allocating buffer memory");
    }

    vkBindBufferMemory(logical_device, result.buffer_handle, result.device_memory, 0);

    result.buffer_size = create_info->size;

    VkBufferDeviceAddressInfo addr_info;
    addr_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addr_info.buffer = result.buffer_handle;
    addr_info.pNext = 0;

    result.device_address = vkGetBufferDeviceAddress(logical_device, &addr_info);

    return result;
}

void VulkanApplication::set_buffer_data(Buffer &buffer, void* data) {
    void* buffer_data;
    if (vkMapMemory(logical_device, buffer.device_memory, 0, buffer.buffer_size, 0, &buffer_data) != VK_SUCCESS) {
        throw std::runtime_error("error mapping buffer memory");
    }
    memcpy(buffer_data, data, (size_t) buffer.buffer_size);
    vkUnmapMemory(logical_device, buffer.device_memory);
}

void VulkanApplication::free_buffer(Buffer &buffer) {
    vkDestroyBuffer(logical_device, buffer.buffer_handle, nullptr);
    vkFreeMemory(logical_device, buffer.device_memory, nullptr);
}

void VulkanApplication::free_shader_binding_table(ShaderBindingTable &sbt) {
    free_buffer(sbt.raygen);
    free_buffer(sbt.hit);
    free_buffer(sbt.miss);
}

MeshData VulkanApplication::create_mesh_data(std::vector<float> &vertices, std::vector<uint32_t> &indices) {
    MeshData res{};

    VkBufferCreateInfo vertex_buffer_info{};
    vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_buffer_info.size = sizeof(float) * vertices.size();
    vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    res.vertices = create_buffer(&vertex_buffer_info);

    set_buffer_data(res.vertices, vertices.data());

    VkBufferCreateInfo index_buffer_info{};
    index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    index_buffer_info.size = sizeof(uint32_t) * indices.size();
    index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    res.indices = create_buffer(&index_buffer_info);

    set_buffer_data(res.indices, indices.data());

    return res;
}

void VulkanApplication::free_mesh_data(MeshData &mesh_data) {
    free_buffer(mesh_data.vertices);
    free_buffer(mesh_data.indices);
}

BLAS VulkanApplication::build_blas(MeshData &mesh_data) {
    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;

    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometry.geometry.triangles.vertexData.deviceAddress = mesh_data.vertices.device_address;
    geometry.geometry.triangles.vertexStride = sizeof(float) * 3;
    geometry.geometry.triangles.maxVertex = mesh_data.vertices.buffer_size / 3;
    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    geometry.geometry.triangles.indexData.deviceAddress = mesh_data.indices.device_address;

    VkAccelerationStructureBuildRangeInfoKHR build_range_info{};
    build_range_info.primitiveCount = mesh_data.vertices.buffer_size / 3;
    build_range_info.primitiveOffset = 0;

    VkAccelerationStructureBuildGeometryInfoKHR as_info{};
    as_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    as_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    as_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    as_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    as_info.geometryCount = 1;
    as_info.pGeometries = &geometry;

    acceleration_structure_size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    uint32_t prim_count = mesh_data.vertices.buffer_size;

    PFN_vkGetAccelerationStructureBuildSizesKHR loaded_vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkGetAccelerationStructureBuildSizesKHR");
    loaded_vkGetAccelerationStructureBuildSizesKHR(logical_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &as_info, &prim_count, &acceleration_structure_size_info);

    BLAS result{};

    VkBufferCreateInfo as_buffer_info{};
    as_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    as_buffer_info.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
    as_buffer_info.size = acceleration_structure_size_info.accelerationStructureSize;
    result.as_buffer = create_buffer(&as_buffer_info);

    VkBufferCreateInfo scratch_buffer_info{};
    scratch_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    scratch_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
    scratch_buffer_info.size = acceleration_structure_size_info.buildScratchSize;
    result.scratch_buffer = create_buffer(&scratch_buffer_info);

    VkAccelerationStructureCreateInfoKHR as_create_info{};
    as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    as_create_info.buffer = result.as_buffer.buffer_handle;
    as_create_info.offset = 0;
    as_create_info.size = acceleration_structure_size_info.accelerationStructureSize;
    as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    PFN_vkCreateAccelerationStructureKHR loaded_vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR) glfwGetInstanceProcAddress(vulkan_instance, "vkCreateAccelerationStructureKHR");
    if (loaded_vkCreateAccelerationStructureKHR(logical_device, &as_create_info, nullptr, &result.acceleration_structure) != VK_SUCCESS) {
        throw std::runtime_error("error creating BLAS");
    }

    as_info.dstAccelerationStructure = result.acceleration_structure;
    as_info.scratchData.deviceAddress = result.scratch_buffer.device_address;

    VkAccelerationStructureBuildRangeInfoKHR* blas_range[] = {
        &build_range_info
    };

    PFN_vkCmdBuildAccelerationStructuresKHR loaded_vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkCmdBuildAccelerationStructuresKHR");
    loaded_vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &as_info, blas_range);

    return result;
}

void VulkanApplication::free_blas(BLAS &blas) {
    PFN_vkDestroyAccelerationStructureKHR loaded_vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR) glfwGetInstanceProcAddress(vulkan_instance, "vkDestroyAccelerationStructureKHR");
    loaded_vkDestroyAccelerationStructureKHR(logical_device, blas.acceleration_structure, nullptr);
    free_buffer(blas.as_buffer);
    free_buffer(blas.scratch_buffer);
}

TLAS VulkanApplication::build_tlas(BLAS &as) {
    VkAccelerationStructureDeviceAddressInfoKHR blas_address_info{};
    blas_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    blas_address_info.accelerationStructure = as.acceleration_structure;

    PFN_vkGetAccelerationStructureDeviceAddressKHR loaded_vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkGetAccelerationStructureDeviceAddressKHR");
    VkDeviceAddress blas_address = loaded_vkGetAccelerationStructureDeviceAddressKHR(logical_device, &blas_address_info);

    VkAccelerationStructureInstanceKHR structure{};
    structure.transform.matrix[0][0] = 1.0f;
    structure.transform.matrix[1][1] = 1.0f;
    structure.transform.matrix[2][2] = 1.0f;
    structure.mask = 0xff;
    structure.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    structure.accelerationStructureReference = blas_address;

    VkBufferCreateInfo instance_buffer_info{};
    instance_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    instance_buffer_info.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    instance_buffer_info.size = sizeof(VkAccelerationStructureInstanceKHR);

    TLAS result{};
    result.instance_buffer = create_buffer(&instance_buffer_info);
    set_buffer_data(result.instance_buffer, &structure);

    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.arrayOfPointers = false;
    geometry.geometry.instances.data.deviceAddress = result.instance_buffer.device_address;

    VkAccelerationStructureBuildGeometryInfoKHR as_info{};
    as_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    as_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    as_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    as_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    as_info.geometryCount = 1;
    as_info.pGeometries = &geometry;


    VkBufferCreateInfo as_buffer_info{};
    as_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    as_buffer_info.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
    as_buffer_info.size = acceleration_structure_size_info.accelerationStructureSize;
    result.as_buffer = create_buffer(&as_buffer_info);

    VkBufferCreateInfo scratch_buffer_info{};
    scratch_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    scratch_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
    scratch_buffer_info.size = acceleration_structure_size_info.buildScratchSize;
    result.scratch_buffer = create_buffer(&scratch_buffer_info);

    VkAccelerationStructureCreateInfoKHR as_create_info{};
    as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    as_create_info.buffer = result.as_buffer.buffer_handle;
    as_create_info.offset = 0;
    as_create_info.size = acceleration_structure_size_info.accelerationStructureSize;
    as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    PFN_vkCreateAccelerationStructureKHR loaded_vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkCreateAccelerationStructureKHR");
    if (loaded_vkCreateAccelerationStructureKHR(logical_device, &as_create_info, nullptr, &result.acceleration_structure) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating TLAS");
    }

    as_info.dstAccelerationStructure = result.acceleration_structure;
    as_info.scratchData.deviceAddress = result.scratch_buffer.device_address;

    VkAccelerationStructureBuildRangeInfoKHR range_info{};
    range_info.primitiveCount = 1;

    VkAccelerationStructureBuildRangeInfoKHR* tlas_range = {
        &range_info
    };

    PFN_vkCmdBuildAccelerationStructuresKHR loaded_vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkCmdBuildAccelerationStructuresKHR");
    loaded_vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &as_info, &tlas_range);

    return result;
}

void VulkanApplication::free_tlas(TLAS &tlas) {
    PFN_vkDestroyAccelerationStructureKHR loaded_vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkDestroyAccelerationStructureKHR");
    loaded_vkDestroyAccelerationStructureKHR(logical_device, tlas.acceleration_structure, nullptr);
    free_buffer(tlas.instance_buffer);
    free_buffer(tlas.as_buffer);
    free_buffer(tlas.scratch_buffer);
}

VkShaderModule VulkanApplication::create_shader_module(const std::vector<char>& code) {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(logical_device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        throw std::runtime_error("error creating shader module");
    }

    return shader_module;
}

void VulkanApplication::create_descriptor_set_layout() {
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

    VkDescriptorSetLayoutBinding bindings[] = {
        image_layout_binding,
        as_layout_binding
    };

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 2;
    layout_info.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(logical_device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
        throw std::runtime_error("error creating descriptor set layout");
    }
}

void VulkanApplication::create_descriptor_pool() {
    VkDescriptorPoolSize image_pool_size{};
    image_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    image_pool_size.descriptorCount = swap_chain_images.size();

    VkDescriptorPoolSize as_pool_size{};
    as_pool_size.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    as_pool_size.descriptorCount = swap_chain_images.size();

    VkDescriptorPoolSize pool_sizes[] = {
        image_pool_size,
        as_pool_size
    };

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 2;
    pool_info.pPoolSizes = pool_sizes;
    pool_info.maxSets = swap_chain_images.size();

    if (vkCreateDescriptorPool(logical_device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS) {
        throw std::runtime_error("error creating descriptor pool");
    }
}

void VulkanApplication::create_descriptor_sets() {
    std::vector<VkDescriptorSetLayout> layouts(swap_chain_images.size(), descriptor_set_layout);
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = swap_chain_images.size();
    alloc_info.pSetLayouts = layouts.data();

    descriptor_sets.resize(swap_chain_images.size());
    if (vkAllocateDescriptorSets(logical_device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS) {
        throw std::runtime_error("error allocating descriptor sets");
    }


    for (int i = 0; i < swap_chain_images.size(); i++) {
        VkDescriptorImageInfo image_info{};
        image_info.imageView = swap_chain_image_views[i];
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet descriptor_write_image{};
        descriptor_write_image.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write_image.dstSet = descriptor_sets[i];
        descriptor_write_image.dstBinding = 0;
        descriptor_write_image.dstArrayElement = 0;
        descriptor_write_image.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptor_write_image.descriptorCount = 1;
        descriptor_write_image.pImageInfo = &image_info;

        VkWriteDescriptorSet descriptor_write_as{};
        descriptor_write_as.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write_as.dstSet = descriptor_sets[i];
        descriptor_write_as.dstBinding = 1;
        descriptor_write_as.dstArrayElement = 0;
        descriptor_write_as.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        descriptor_write_as.descriptorCount = 1;

        VkWriteDescriptorSetAccelerationStructureKHR descriptor_write_as_data{};
        descriptor_write_as_data.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        descriptor_write_as_data.accelerationStructureCount = 1;
        descriptor_write_as_data.pAccelerationStructures = &scene_tlas.acceleration_structure;

        descriptor_write_as.pNext = &descriptor_write_as_data;

        VkWriteDescriptorSet descriptor_writes[] = {
            descriptor_write_image,
            descriptor_write_as
        };

        vkUpdateDescriptorSets(logical_device, 2, descriptor_writes, 0, nullptr);
    }
}

/* Deprecated: Graphics Pipeline
// void VulkanApplication::create_graphics_pipeline() {
//     auto vert_shader_code = loaders::read_spirv("shaders/test_vert.spv");
//     auto frag_shader_code = loaders::read_spirv("shaders/test_frag.spv");

//     VkShaderModule vert_shader_module = create_shader_module(vert_shader_code);
//     VkShaderModule frag_shader_module = create_shader_module(frag_shader_code);

//     VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
//     vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//     vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
//     vert_shader_stage_info.module = vert_shader_module;
//     vert_shader_stage_info.pName = "main";

//     VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
//     frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//     frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//     frag_shader_stage_info.module = frag_shader_module;
//     frag_shader_stage_info.pName = "main";

//     VkPipelineShaderStageCreateInfo shader_stages[] = {
//         vert_shader_stage_info,
//         frag_shader_stage_info
//     };

//     VkPipelineVertexInputStateCreateInfo vertex_input_info{};
//     vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//     vertex_input_info.vertexBindingDescriptionCount = 0;
//     vertex_input_info.pVertexBindingDescriptions = nullptr;
//     vertex_input_info.vertexAttributeDescriptionCount = 0;
//     vertex_input_info.pVertexAttributeDescriptions = nullptr;

//     VkPipelineInputAssemblyStateCreateInfo input_assembly{};
//     input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//     input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//     input_assembly.primitiveRestartEnable = false;

//     VkPipelineViewportStateCreateInfo viewport_state{};
//     viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//     viewport_state.viewportCount = 1;
//     viewport_state.scissorCount = 1;

//     VkPipelineRasterizationStateCreateInfo rasterizer{};
//     rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//     rasterizer.depthClampEnable = VK_FALSE;
//     rasterizer.rasterizerDiscardEnable = VK_FALSE;
//     rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//     rasterizer.lineWidth = 1.0f;
//     rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
//     rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
//     rasterizer.depthBiasEnable = VK_FALSE;

//     VkPipelineMultisampleStateCreateInfo multisampling{};
//     multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//     multisampling.sampleShadingEnable = VK_FALSE;
//     multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

//     VkPipelineColorBlendAttachmentState color_blend_attachment{};
//     color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//     color_blend_attachment.blendEnable = VK_FALSE;

//     VkPipelineColorBlendStateCreateInfo color_blending{};
//     color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//     color_blending.logicOpEnable = VK_FALSE;
//     color_blending.logicOp = VK_LOGIC_OP_COPY;
//     color_blending.attachmentCount = 1;
//     color_blending.pAttachments = &color_blend_attachment;
//     color_blending.blendConstants[0] = 0.0f;
//     color_blending.blendConstants[1] = 0.0f;
//     color_blending.blendConstants[2] = 0.0f;
//     color_blending.blendConstants[3] = 0.0f;

//     std::vector<VkDynamicState> dynamic_states = {
//         VK_DYNAMIC_STATE_VIEWPORT,
//         VK_DYNAMIC_STATE_SCISSOR
//     };

//     VkPipelineDynamicStateCreateInfo dynamic_state{};
//     dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//     dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
//     dynamic_state.pDynamicStates = dynamic_states.data();

//     VkPipelineLayoutCreateInfo pipeline_layout_info{};
//     pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//     pipeline_layout_info.setLayoutCount = 0;
//     pipeline_layout_info.pushConstantRangeCount = 0;

//     if (vkCreatePipelineLayout(logical_device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
//         throw std::runtime_error("error creating pipeline layout");
//     }

//     VkGraphicsPipelineCreateInfo pipeline_info{};
//     pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//     pipeline_info.stageCount = 2;
//     pipeline_info.pStages = shader_stages;
//     pipeline_info.pVertexInputState = &vertex_input_info;
//     pipeline_info.pInputAssemblyState = &input_assembly;
//     pipeline_info.pViewportState = &viewport_state;
//     pipeline_info.pRasterizationState = &rasterizer;
//     pipeline_info.pMultisampleState = &multisampling;
//     pipeline_info.pColorBlendState = &color_blending;
//     pipeline_info.pDynamicState = &dynamic_state;
//     pipeline_info.layout = pipeline_layout;
//     pipeline_info.renderPass = render_pass;
//     pipeline_info.subpass = 0;
//     pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

//     if (vkCreateGraphicsPipelines(logical_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS) {
//         throw std::runtime_error("error creating graphics pipeline");
//     }

//     vkDestroyShaderModule(logical_device, vert_shader_module, nullptr);
//     vkDestroyShaderModule(logical_device, frag_shader_module, nullptr);
// }
*/

void VulkanApplication::create_raytracing_pipeline()
{
    auto ray_gen_shader_code = loaders::read_spirv("shaders/ray_gen.spv");
    auto closest_hit_shader_code = loaders::read_spirv("shaders/closest_hit.spv");
    auto miss_shader_code = loaders::read_spirv("shaders/miss.spv");

    VkShaderModule ray_gen_shader_module = create_shader_module(ray_gen_shader_code);
    VkShaderModule closest_hit_shader_module = create_shader_module(closest_hit_shader_code);
    VkShaderModule miss_shader_module = create_shader_module(miss_shader_code);

    VkPipelineShaderStageCreateInfo ray_gen_shader_stage_info{};
    ray_gen_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ray_gen_shader_stage_info.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    ray_gen_shader_stage_info.module = ray_gen_shader_module;
    ray_gen_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo closest_hit_shader_stage_info{};
    closest_hit_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    closest_hit_shader_stage_info.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    closest_hit_shader_stage_info.module = closest_hit_shader_module;
    closest_hit_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo miss_shader_stage_info{};
    miss_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    miss_shader_stage_info.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    miss_shader_stage_info.module = miss_shader_module;
    miss_shader_stage_info.pName = "main";

    shader_stages.push_back(ray_gen_shader_stage_info);
    shader_stages.push_back(closest_hit_shader_stage_info);
    shader_stages.push_back(miss_shader_stage_info);

    VkRayTracingShaderGroupCreateInfoKHR raygen_shader_group_info{};
    raygen_shader_group_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    raygen_shader_group_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    raygen_shader_group_info.generalShader = 0;
    raygen_shader_group_info.closestHitShader = VK_SHADER_UNUSED_KHR;
    raygen_shader_group_info.anyHitShader = VK_SHADER_UNUSED_KHR;
    raygen_shader_group_info.intersectionShader = VK_SHADER_UNUSED_KHR;

    VkRayTracingShaderGroupCreateInfoKHR closest_hit_shader_group_info{};
    closest_hit_shader_group_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    closest_hit_shader_group_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    closest_hit_shader_group_info.closestHitShader = 1;
    closest_hit_shader_group_info.intersectionShader = VK_SHADER_UNUSED_KHR;
    closest_hit_shader_group_info.anyHitShader = VK_SHADER_UNUSED_KHR;
    closest_hit_shader_group_info.generalShader = VK_SHADER_UNUSED_KHR;

    VkRayTracingShaderGroupCreateInfoKHR miss_shader_group_info{};
    miss_shader_group_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    miss_shader_group_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    miss_shader_group_info.generalShader = 2;
    miss_shader_group_info.closestHitShader = VK_SHADER_UNUSED_KHR;
    miss_shader_group_info.anyHitShader = VK_SHADER_UNUSED_KHR;
    miss_shader_group_info.intersectionShader = VK_SHADER_UNUSED_KHR;

    VkRayTracingShaderGroupCreateInfoKHR shader_groups[] = {
        raygen_shader_group_info,
        closest_hit_shader_group_info,
        miss_shader_group_info
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
    pipeline_layout_info.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(logical_device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating pipeline layout");
    }

    VkRayTracingPipelineCreateInfoKHR pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipeline_info.stageCount = shader_stages.size();
    pipeline_info.pStages = shader_stages.data();
    pipeline_info.groupCount = 3;
    pipeline_info.pGroups = shader_groups;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.maxPipelineRayRecursionDepth = 1;

    VkPipelineCacheCreateInfo cache_create_info{};
    cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    if (vkCreatePipelineCache(logical_device, &cache_create_info, nullptr, &pipeline_cache) != VK_SUCCESS) {
        throw std::runtime_error("error creating pipeline cache");
    }

    PFN_vkCreateRayTracingPipelinesKHR loaded_vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkCreateRayTracingPipelinesKHR");

    if (loaded_vkCreateRayTracingPipelinesKHR(logical_device, VK_NULL_HANDLE, pipeline_cache, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating graphics pipeline");
    }

    shader_binding_table = create_shader_binding_table();

    vkDestroyShaderModule(logical_device, ray_gen_shader_module, nullptr);
    vkDestroyShaderModule(logical_device, closest_hit_shader_module, nullptr);
    vkDestroyShaderModule(logical_device, miss_shader_module, nullptr);
}

ShaderBindingTable VulkanApplication::create_shader_binding_table() {
    int group_handle_size = ray_tracing_pipeline_properties.shaderGroupHandleSize;
    size_t shader_binding_table_size = group_handle_size * shader_stages.size();

    uint8_t* shader_binding_table_data = new uint8_t[shader_binding_table_size];

    PFN_vkGetRayTracingShaderGroupHandlesKHR loaded_vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR) glfwGetInstanceProcAddress(vulkan_instance, "vkGetRayTracingShaderGroupHandlesKHR");
    loaded_vkGetRayTracingShaderGroupHandlesKHR(logical_device, pipeline, 0, shader_stages.size(), shader_binding_table_size, shader_binding_table_data);

    VkBufferCreateInfo sbt_buffer_info{};
    sbt_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    sbt_buffer_info.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
    sbt_buffer_info.size = group_handle_size;

    Buffer raygen_buffer = create_buffer(&sbt_buffer_info);
    set_buffer_data(raygen_buffer, shader_binding_table_data);

    Buffer hit_buffer = create_buffer(&sbt_buffer_info);
    set_buffer_data(hit_buffer, shader_binding_table_data + group_handle_size);

    Buffer miss_buffer = create_buffer(&sbt_buffer_info);
    set_buffer_data(miss_buffer, shader_binding_table_data + group_handle_size * 2);

    return ShaderBindingTable{
        raygen_buffer,
        hit_buffer,
        miss_buffer
    };
}

void VulkanApplication::record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index) {
    // command buffer begin
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        throw std::runtime_error("error beginning command buffer");
    }

    /*
    // VkRenderPassBeginInfo render_pass_info{};
    // render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    // render_pass_info.renderPass = render_pass;
    // render_pass_info.framebuffer = framebuffers[image_index];
    // render_pass_info.renderArea.offset = {0, 0};
    // render_pass_info.renderArea.extent = swap_chain_extent;

    // VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    // render_pass_info.clearValueCount = 1;
    // render_pass_info.pClearValues = &clear_color;

    // vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    // VkViewport viewport{};
    // viewport.x = 0.0f;
    // viewport.y = 0.0f;
    // viewport.width = static_cast<float>(swap_chain_extent.width);
    // viewport.height = static_cast<float>(swap_chain_extent.width);
    // viewport.minDepth = 0.0f;
    // viewport.maxDepth = 1.0f;
    // vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    // VkRect2D scissor{};
    // scissor.offset = {0,0};
    // scissor.extent = swap_chain_extent;
    // vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    // vkCmdEndRenderPass(command_buffer);
    */

    // transition image to writeable format
    VkImageMemoryBarrier image_barrier = {};
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.image = swap_chain_images[image_index];
    image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;
    image_barrier.srcAccessMask = 0;
    image_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_layout, 0, 1, &descriptor_sets[image_index], 0, nullptr);

    uint32_t shader_group_handle_size = ray_tracing_pipeline_properties.shaderGroupHandleSize;

    VkStridedDeviceAddressRegionKHR ar_raygen{};
    ar_raygen.deviceAddress = shader_binding_table.raygen.device_address;
    ar_raygen.stride = shader_group_handle_size;
    ar_raygen.size = shader_group_handle_size;

    VkStridedDeviceAddressRegionKHR ar_hit{};
    ar_hit.deviceAddress = shader_binding_table.hit.device_address;

    VkStridedDeviceAddressRegionKHR ar_miss{};
    ar_miss.deviceAddress = shader_binding_table.miss.device_address;

    VkStridedDeviceAddressRegionKHR ar_callable{};

    auto loaded_vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkCmdTraceRaysKHR");
    loaded_vkCmdTraceRaysKHR(command_buffer, &ar_raygen, &ar_miss, &ar_hit, &ar_callable, swap_chain_extent.width, swap_chain_extent.height, 1);

    // retransition image layout
    image_barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    image_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("encountered an error when ending command buffer");
    }
}

void VulkanApplication::create_synchronization() {
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateFence(logical_device, &fence_info, nullptr, &immediate_fence);

    if (vkCreateSemaphore(logical_device, &semaphore_info, nullptr, &image_available_semaphore) != VK_SUCCESS ||
        vkCreateSemaphore(logical_device, &semaphore_info, nullptr, &render_finished_semaphore) != VK_SUCCESS ||
        vkCreateFence(logical_device, &fence_info, nullptr, &in_flight_fence) != VK_SUCCESS) {
            throw std::runtime_error("error creating synchronization");
        }
}

void VulkanApplication::draw_frame() {
    vkWaitForFences(logical_device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(logical_device, 1, &in_flight_fence);

    uint32_t image_index;
    vkAcquireNextImageKHR(logical_device, swap_chain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);

    vkResetCommandBuffer(command_buffer, 0);
    record_command_buffer(command_buffer, image_index);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {image_available_semaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    VkSemaphore signal_semaphores[] = {render_finished_semaphore};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fence) != VK_SUCCESS) {
        throw std::runtime_error("error submitting draw command buffer");
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swap_chains[] = {swap_chain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;

    vkQueuePresentKHR(present_queue, &present_info);
}

void VulkanApplication::setup() {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    // initialize vulkan
    // create vulkan instance
    {
        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Vulkan Renderer";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        uint32_t glfw_extension_count = 0;
        const char **glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        std::vector<const char*> required_extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
        // extension needed for vulkan debugging
        required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        uint32_t available_extension_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr);
        std::vector<VkExtensionProperties> available_extensions(available_extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, available_extensions.data());

        std::cout << "vulkan extensions needed by glfw" << std::endl;
        for (uint32_t i = 0; i < glfw_extension_count; i++)
        {
            bool found = false;
            for (uint32_t j = 0; j < available_extension_count; j++)
            {
                if (strcmp(available_extensions[i].extensionName, glfw_extensions[i]))
                {
                    found = true;
                    break;
                }
            }
            std::cout << "\t" << (found ? "\033[1;32m" : "\033[1;31m") << glfw_extensions[i] << "\n";
        }
        std::cout << "\033[0m";

        create_info.enabledExtensionCount = (uint32_t)required_extensions.size();
        create_info.ppEnabledExtensionNames = required_extensions.data();
        create_info.flags = 0;


        // Debug validation layers
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();

        debug_create_info = {};
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = debugCallback;


        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;

        if (vkCreateInstance(&create_info, nullptr, &vulkan_instance) != VK_SUCCESS)
        {
            throw std::runtime_error("error when creating vulkan instance");
        }
        std::cout << "vulkan instance created" << std::endl;

        if (CreateDebugUtilsMessengerEXT(vulkan_instance, &debug_create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
            throw std::runtime_error("error setting up debug messenger");
        }
    }

    // create window surface
    if (glfwCreateWindowSurface(vulkan_instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface");
    }
    std::cout << "window surface created" << std::endl;

    // physical device
    {
        uint32_t physical_device_count = 0;
        vkEnumeratePhysicalDevices(vulkan_instance, &physical_device_count, nullptr);

        if (physical_device_count == 0)
            throw std::runtime_error("found no physical devices supporting Vulkan");

        std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
        vkEnumeratePhysicalDevices(vulkan_instance, &physical_device_count, physical_devices.data());

        // pick suitable physical device
        for (const auto &dev : physical_devices)
        {
            // check device suitability
            VkPhysicalDeviceProperties2 dev_properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
            
            ray_tracing_pipeline_properties = VkPhysicalDeviceRayTracingPipelinePropertiesKHR{};
            ray_tracing_pipeline_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
            dev_properties.pNext = &ray_tracing_pipeline_properties;

            vkGetPhysicalDeviceProperties2(dev, &dev_properties);

            VkPhysicalDeviceFeatures dev_features;
            vkGetPhysicalDeviceFeatures(dev, &dev_features);

            if (dev_properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                physical_device = dev;
                break;
            }
        }

        std::cout << "SBT STRIDE: " << ray_tracing_pipeline_properties.shaderGroupHandleSize << std::endl;

        vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

        // find queue families
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

        int family_index = 0;
        for (const auto &queue_family : queue_families)
        {
            // check for graphics family
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                queue_family_indices.graphics = std::make_optional(family_index);
            }
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, family_index, surface, &present_support);
            if (present_support)
            {
                queue_family_indices.present = std::make_optional(family_index);
            }

            if (queue_family_indices.graphics && queue_family_indices.present) {
                break;
            }

            family_index++;
        }
    }
    std::cout << "valid physical device found" << std::endl;

    // create logical device
    {
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> unique_queue_families = {queue_family_indices.graphics.value(), queue_family_indices.present.value()};
        float queue_priority = 1.0f;

        for (uint32_t unique_family : unique_queue_families)
        {
            std::cout << "CREATING QUEUE: " << unique_family << std::endl;
            VkDeviceQueueCreateInfo dev_queue_create_info{};
            dev_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            dev_queue_create_info.queueFamilyIndex = unique_family;
            dev_queue_create_info.queueCount = 1;
            dev_queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(dev_queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features{};

        // check for device extension support
        const std::vector<const char *> device_extensions = {
            // needed for window display
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            // needed for vulkan raytracing functionality
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_RAY_QUERY_EXTENSION_NAME,
            // dependencies for raytracing functionality
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
            VK_KHR_SPIRV_1_4_EXTENSION_NAME,
            VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
        };

        uint32_t extension_count = 0;
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());

        std::cout << "extensions needed in physical device" << std::endl;
        std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());
        for (const auto &ext : available_extensions)
        {
            if (required_extensions.find(ext.extensionName) != required_extensions.end())
                std::cout << "\t"
                          << "\033[1;32m" << ext.extensionName << "\n";
            required_extensions.erase(ext.extensionName);
        }
        for (std::string not_found : required_extensions)
        {
            std::cout << "\t"
                      << "\033[1;31m" << not_found << "\n";
        }
        std::cout << "\033[0m";
        // throw error if not all required extensions are found
        if (required_extensions.size() > 0)
            throw std::runtime_error("extension requirements not satisfied by physical device");

        VkDeviceCreateInfo device_create_info{};
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
        device_create_info.pQueueCreateInfos = queue_create_infos.data();
        device_create_info.queueCreateInfoCount = 1;

        //device_create_info.pEnabledFeatures = &device_features;
        device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
        device_create_info.ppEnabledExtensionNames = device_extensions.data();

        // validation layers
        device_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        device_create_info.ppEnabledLayerNames = validation_layers.data();

        VkPhysicalDeviceFeatures2 physical_features2 = {};
        physical_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        vkGetPhysicalDeviceFeatures2(physical_device, &physical_features2);

        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipeline_features = {};
        rt_pipeline_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        rt_pipeline_features.rayTracingPipeline = VK_TRUE;
        physical_features2.pNext = &rt_pipeline_features;

        VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features = {};
        buffer_device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        buffer_device_address_features.bufferDeviceAddress = VK_TRUE;
        rt_pipeline_features.pNext = &buffer_device_address_features;

        VkPhysicalDeviceAccelerationStructureFeaturesKHR as_features = {};
        as_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        as_features.accelerationStructure = VK_TRUE;
        buffer_device_address_features.pNext = &as_features;

        device_create_info.pNext = &physical_features2;

        if (vkCreateDevice(physical_device, &device_create_info, nullptr, &logical_device) != VK_SUCCESS)
        {
            throw std::runtime_error("failure to create logical device");
        }
    }

    std::cout << "QUEUE FAMILIES | GRAPHICS: " << queue_family_indices.graphics.value() << " | PRESENT: " << queue_family_indices.present.value() << std::endl;

    // retrieve queue handles
    vkGetDeviceQueue(logical_device, queue_family_indices.graphics.value(), 0, &graphics_queue);

    vkGetDeviceQueue(logical_device, queue_family_indices.present.value(), 0, &present_queue);

    // create command pool
    {
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = queue_family_indices.graphics.value();

        if (vkCreateCommandPool(logical_device, &pool_info, nullptr, &command_pool) != VK_SUCCESS)
        {
            throw std::runtime_error("error creating command pool");
        }
    }

    // create command buffer
    {
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(logical_device, &alloc_info, &command_buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("error allocating command buffer");
        }
    }

    create_synchronization();

    // swap chain
    VkSurfaceFormatKHR surface_format;
    {
        uint32_t format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats.data());

        uint32_t present_mode_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
        std::vector<VkPresentModeKHR> present_modes(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes.data());

        // select surface format (SRGB is preferred)
        {
            bool found = false;
            for (const auto &format : formats)
            {
                if (format.format == VK_FORMAT_B8G8R8A8_SRGB | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    surface_format = format;
                    found = true;
                    break;
                }
            }
            if (!found)
                surface_format = formats[0];
        }

        // select present mode (MAILBOX is preferred)
        VkPresentModeKHR present_mode;
        {
            bool found = false;
            for (const auto &mode : present_modes)
            {
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    present_mode = mode;
                    found = true;
                    break;
                }
            }
            if (!found)
                present_mode = VK_PRESENT_MODE_FIFO_KHR;
        }

        // select swap extent
        VkSurfaceCapabilitiesKHR surface_capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);

        if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            swap_chain_extent = surface_capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actual_extent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)};

            swap_chain_extent = actual_extent;
        }

        uint32_t image_count = surface_capabilities.minImageCount + 1;

        VkSwapchainCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface;

        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = swap_chain_extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

        uint32_t queue_family_indices_int[] = {queue_family_indices.graphics.value(), queue_family_indices.present.value()};

        if (queue_family_indices.graphics != queue_family_indices.present)
        {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = queue_family_indices_int;
        }
        else
        {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices = nullptr;
        }

        create_info.preTransform = surface_capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(logical_device, &create_info, nullptr, &swap_chain) != VK_SUCCESS)
        {
            throw std::runtime_error("failure to create swapchain");
        }
    }

    // retrieve swap chain images
    uint32_t image_count = 0;
    vkGetSwapchainImagesKHR(logical_device, swap_chain, &image_count, nullptr);
    swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(logical_device, swap_chain, &image_count, swap_chain_images.data());

    // create image views
    swap_chain_image_views.resize(image_count);
    for (size_t i = 0; i < image_count; i++)
    {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swap_chain_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = surface_format.format;

        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(logical_device, &create_info, nullptr, &swap_chain_image_views[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failure to create image view");
        }
    }

    // create render pass
    VkAttachmentDescription color_attachment{};
    color_attachment.format = surface_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(logical_device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS)
    {
        throw std::runtime_error("failure to create render pass");
    }

    // // create scene
    // std::vector<float> vertices = {
    //     // floor plane
    //     -10, 0, -10,
    //     10, 0, -10,
    //     10, 0, 10,
    //     -10, 0, 10,

    //     // upper plane
    //     -1,3,-1,
    //     1,3,-1,
    //     1,3,1,
    //     -1,3,1
    //     };

    // std::vector<uint32_t> indices = {
    //     // floor plane
    //     0, 1, 2,
    //     3, 0, 2,

    //     // upper plane
    //     4, 5, 6,
    //     7, 4, 6
    // };

    LoadedMeshData loaded_scene = loaders::load_obj("scenes/obj/test.obj");

    scene_mesh_data = create_mesh_data(loaded_scene.vertices, loaded_scene.indices);

    // BUILD BLAS
    vkResetCommandBuffer(command_buffer, 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        throw std::runtime_error("error beginning command buffer");
    }

    scene_blas = build_blas(scene_mesh_data);

    vkEndCommandBuffer(command_buffer);

    vkResetFences(logical_device, 1, &immediate_fence);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 0;
    submit_info.signalSemaphoreCount = 0;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(graphics_queue, 1, &submit_info, immediate_fence);

    vkWaitForFences(logical_device, 1, &immediate_fence, VK_TRUE, UINT64_MAX);

    // BUILD TLAS
    vkResetCommandBuffer(command_buffer, 0);

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        throw std::runtime_error("error beginning command buffer");
    }

    scene_tlas = build_tlas(scene_blas);

    vkEndCommandBuffer(command_buffer);

    vkResetFences(logical_device, 1, &immediate_fence);

    vkQueueSubmit(graphics_queue, 1, &submit_info, immediate_fence);

    vkWaitForFences(logical_device, 1, &immediate_fence, VK_TRUE, UINT64_MAX);

    std::cout << "TLAS CREATED" << std::endl;

    // create descriptor sets
    create_descriptor_set_layout();
    create_descriptor_pool();
    create_descriptor_sets();
    std::cout << "DESCRIPTORS CREATED" << std::endl;

    // create graphics pipeline
    //create_graphics_pipeline();
    create_raytracing_pipeline();

    // create framebuffers
    framebuffers.resize(swap_chain_image_views.size());
    for (size_t i = 0; i < swap_chain_image_views.size(); i++)
    {
        VkImageView attachments[] = {
            swap_chain_image_views[i]};

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = swap_chain_extent.width;
        framebuffer_info.height = swap_chain_extent.height;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(logical_device, &framebuffer_info, nullptr, &framebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failure creating framebuffer");
        }
    }

}

void VulkanApplication::run() {
    // main event loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        draw_frame();
    }

    vkDeviceWaitIdle(logical_device);
}

void VulkanApplication::cleanup() {
    // deinitialization
    free_tlas(scene_tlas);
    free_blas(scene_blas);
    free_mesh_data(scene_mesh_data);
    free_shader_binding_table(shader_binding_table);
    vkDestroyDescriptorSetLayout(logical_device, descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(logical_device, descriptor_pool, nullptr);
    vkDestroyPipelineCache(logical_device, pipeline_cache, nullptr);
    vkDestroyPipeline(logical_device, pipeline, nullptr);
    vkDestroyPipelineLayout(logical_device, pipeline_layout, nullptr);
    vkDestroySemaphore(logical_device, image_available_semaphore, nullptr);
    vkDestroySemaphore(logical_device, render_finished_semaphore, nullptr);
    vkDestroyFence(logical_device, in_flight_fence, nullptr);
    vkDestroyFence(logical_device, immediate_fence, nullptr);
    vkDestroyCommandPool(logical_device, command_pool, nullptr);
    for (auto framebuffer : framebuffers)
        vkDestroyFramebuffer(logical_device, framebuffer, nullptr);
    // vkDestroyPipelineLayout(logical_device, pipeline_layout, nullptr);
    vkDestroyRenderPass(logical_device, render_pass, nullptr);
    for (auto image_view : swap_chain_image_views)
        vkDestroyImageView(logical_device, image_view, nullptr);
    vkDestroySwapchainKHR(logical_device, swap_chain, nullptr);
    vkDestroySurfaceKHR(vulkan_instance, surface, nullptr);
    vkDestroyDevice(logical_device, nullptr);
    DestroyDebugUtilsMessengerEXT(vulkan_instance, debug_messenger, nullptr);
    vkDestroyInstance(vulkan_instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}