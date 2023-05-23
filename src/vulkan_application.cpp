#include "vulkan_application.h"

#include <set>
#include <iostream>
#include <stdexcept>
#include <filesystem>

#include "loaders/shader_spirv.h"
#include "loaders/geometry_obj.h"

#include "glm/gtc/matrix_transform.hpp"

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

void MeshData::free() {
    vertices.free();
    vertex_indices.free();
    normals.free();
    normal_indices.free();
    texcoords.free();
    texcoord_indices.free();
}

MeshData VulkanApplication::create_mesh_data(std::vector<vec4> &vertices, std::vector<uint32_t> &vertex_indices, std::vector<vec4> &normals, std::vector<uint32_t> &normal_indices, std::vector<vec2> &texcoords, std::vector<uint32_t> &texcoord_indices) {
    MeshData res{};
    res.device_handle = logical_device;

    VkBufferCreateInfo vertex_buffer_info{};
    vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_buffer_info.size = sizeof(vec4) * vertices.size();
    vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    res.vertices = device.create_buffer(&vertex_buffer_info);

    res.vertices.set_data(vertices.data());

    VkBufferCreateInfo vertex_index_buffer_info{};
    vertex_index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_index_buffer_info.size = sizeof(uint32_t) * vertex_indices.size();
    vertex_index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    res.vertex_indices = device.create_buffer(&vertex_index_buffer_info);

    res.vertex_indices.set_data(vertex_indices.data());

    VkBufferCreateInfo normal_buffer_info{};
    normal_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    normal_buffer_info.size = sizeof(vec4) * normals.size();
    normal_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    res.normals = device.create_buffer(&normal_buffer_info);

    res.normals.set_data(normals.data());

    VkBufferCreateInfo normal_index_buffer_info{};
    normal_index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    normal_index_buffer_info.size = sizeof(uint32_t) * normal_indices.size();
    normal_index_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    res.normal_indices = device.create_buffer(&normal_index_buffer_info);

    res.normal_indices.set_data(normal_indices.data());

    VkBufferCreateInfo texcoord_buffer_info{};
    texcoord_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    texcoord_buffer_info.size = sizeof(vec2) * texcoords.size();
    texcoord_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    res.texcoords = device.create_buffer(&texcoord_buffer_info);

    res.texcoords.set_data(texcoords.data());

    VkBufferCreateInfo texcoord_index_buffer_info{};
    texcoord_index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    texcoord_index_buffer_info.size = sizeof(uint32_t) * texcoord_indices.size();
    texcoord_index_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    res.texcoord_indices = device.create_buffer(&texcoord_index_buffer_info);

    res.texcoord_indices.set_data(texcoord_indices.data());

    res.vertex_count = vertices.size();
    res.normal_count = normals.size();
    res.texcoord_count = texcoords.size();
    res.vertex_index_count = vertex_indices.size();
    res.normal_index_count = normal_indices.size();
    res.texcoord_index_count = texcoord_indices.size();

    return res;
}

MeshData VulkanApplication::create_mesh_data(LoadedMeshData loaded_mesh_data) {
    return create_mesh_data(
        loaded_mesh_data.vertices,
        loaded_mesh_data.vertex_indices,
        loaded_mesh_data.normals,
        loaded_mesh_data.normal_indices,
        loaded_mesh_data.texcoords,
        loaded_mesh_data.texcoord_indices
        );
}

BLAS VulkanApplication::build_blas(MeshData &mesh_data) {
    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;

    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags = 0;

    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometry.geometry.triangles.vertexData.deviceAddress = mesh_data.vertices.device_address;
    geometry.geometry.triangles.vertexStride = sizeof(float) * 4;
    geometry.geometry.triangles.maxVertex = mesh_data.vertex_count;
    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    geometry.geometry.triangles.indexData.deviceAddress = mesh_data.vertex_indices.device_address;

    VkAccelerationStructureBuildRangeInfoKHR build_range_info{};
    build_range_info.primitiveCount = mesh_data.vertex_index_count / 3;
    build_range_info.primitiveOffset = 0;

    VkAccelerationStructureBuildGeometryInfoKHR as_info{};
    as_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    as_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    as_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    as_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    as_info.geometryCount = 1;
    as_info.pGeometries = &geometry;

    acceleration_structure_size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    uint32_t prim_count = build_range_info.primitiveCount;

    device.vkGetAccelerationStructureBuildSizesKHR(logical_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &as_info, &prim_count, &acceleration_structure_size_info);

    BLAS result{};

    VkBufferCreateInfo as_buffer_info{};
    as_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    as_buffer_info.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
    as_buffer_info.size = acceleration_structure_size_info.accelerationStructureSize;
    result.as_buffer = device.create_buffer(&as_buffer_info);

    VkBufferCreateInfo scratch_buffer_info{};
    scratch_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    scratch_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
    scratch_buffer_info.size = acceleration_structure_size_info.buildScratchSize;
    result.scratch_buffer = device.create_buffer(&scratch_buffer_info);

    VkAccelerationStructureCreateInfoKHR as_create_info{};
    as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    as_create_info.buffer = result.as_buffer.buffer_handle;
    as_create_info.offset = 0;
    as_create_info.size = acceleration_structure_size_info.accelerationStructureSize;
    as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    if (device.vkCreateAccelerationStructureKHR(logical_device, &as_create_info, nullptr, &result.acceleration_structure) != VK_SUCCESS) {
        throw std::runtime_error("error creating BLAS");
    }

    as_info.dstAccelerationStructure = result.acceleration_structure;
    as_info.scratchData.deviceAddress = result.scratch_buffer.device_address;

    VkAccelerationStructureBuildRangeInfoKHR* blas_range[] = {
        &build_range_info
    };

    device.vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &as_info, blas_range);

    return result;
}

void VulkanApplication::free_blas(BLAS &blas) {
    device.vkDestroyAccelerationStructureKHR(logical_device, blas.acceleration_structure, nullptr);
    blas.as_buffer.free();
    blas.scratch_buffer.free();
}

TLAS VulkanApplication::build_tlas() {
    std::vector<VkAccelerationStructureInstanceKHR> instances;

    int count = 0;
    for (InstanceData instance : loaded_scene_data.instances) {
        BLAS& as = loaded_blas[instance.mesh_name];

        VkAccelerationStructureDeviceAddressInfoKHR blas_address_info{};
        blas_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        blas_address_info.accelerationStructure = as.acceleration_structure;

        VkDeviceAddress blas_address = device.vkGetAccelerationStructureDeviceAddressKHR(logical_device, &blas_address_info);

        VkAccelerationStructureInstanceKHR structure{};
        structure.transform.matrix[0][0] = instance.transformation[0][0];
        structure.transform.matrix[0][1] = instance.transformation[1][0];
        structure.transform.matrix[0][2] = instance.transformation[2][0];
        structure.transform.matrix[0][3] = instance.transformation[3][0];
        structure.transform.matrix[1][0] = instance.transformation[0][1];
        structure.transform.matrix[1][1] = instance.transformation[1][1];
        structure.transform.matrix[1][2] = instance.transformation[2][1];
        structure.transform.matrix[1][3] = instance.transformation[3][1];
        structure.transform.matrix[2][0] = instance.transformation[0][2];
        structure.transform.matrix[2][1] = instance.transformation[1][2];
        structure.transform.matrix[2][2] = instance.transformation[2][2];
        structure.transform.matrix[2][3] = instance.transformation[3][2];

        structure.mask = 0xff;
        structure.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        structure.accelerationStructureReference = blas_address;

        instances.push_back(structure);
        count++;
    }

    std::cout << "TLAS instances: " << instances.size() << std::endl;

    VkBufferCreateInfo instance_buffer_info{};
    instance_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    instance_buffer_info.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    instance_buffer_info.size = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();

    TLAS result{};
    result.instance_buffer = device.create_buffer(&instance_buffer_info);
    result.instance_buffer.set_data(instances.data());

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
    result.as_buffer = device.create_buffer(&as_buffer_info);

    VkBufferCreateInfo scratch_buffer_info{};
    scratch_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    scratch_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
    scratch_buffer_info.size = acceleration_structure_size_info.buildScratchSize;
    result.scratch_buffer = device.create_buffer(&scratch_buffer_info);

    VkAccelerationStructureCreateInfoKHR as_create_info{};
    as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    as_create_info.buffer = result.as_buffer.buffer_handle;
    as_create_info.offset = 0;
    as_create_info.size = acceleration_structure_size_info.accelerationStructureSize;
    as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    if (device.vkCreateAccelerationStructureKHR(logical_device, &as_create_info, nullptr, &result.acceleration_structure) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating TLAS");
    }

    as_info.dstAccelerationStructure = result.acceleration_structure;
    as_info.scratchData.deviceAddress = result.scratch_buffer.device_address;

    VkAccelerationStructureBuildRangeInfoKHR range_info{};
    range_info.primitiveCount = instances.size();

    VkAccelerationStructureBuildRangeInfoKHR* tlas_range = {
        &range_info
    };

    device.vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &as_info, &tlas_range);

    return result;
}

void VulkanApplication::free_tlas(TLAS &tlas) {
    device.vkDestroyAccelerationStructureKHR(logical_device, tlas.acceleration_structure, nullptr);
    tlas.instance_buffer.free();
    tlas.as_buffer.free();
    tlas.scratch_buffer.free();
}

void VulkanApplication::create_default_descriptor_writes() {
    VkBufferCreateInfo cam_buffer_info{};
    cam_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    cam_buffer_info.size = sizeof(Shaders::CameraData);
    cam_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    camera_buffer = device.create_buffer(&cam_buffer_info);

    camera_data.fov_x = 90.0f;
    camera_data.origin = vec4(0.0f, 1.0f, -3.0f, 0.0f);
    camera_data.forward = vec4(0.0f, 0.0f, 1.0f, 0.0f);
    camera_data.right = vec4(1.0f, 0.0f, 0.0f, 0.0f);
    camera_data.up = vec4(0.0f, 1.0f, 0.0f, 0.0f);

    camera_buffer.set_data(&camera_data);

    VkWriteDescriptorSet descriptor_write_as{};
    descriptor_write_as.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write_as.dstSet = pipeline.descriptor_sets[0];
    descriptor_write_as.dstBinding = 1;
    descriptor_write_as.dstArrayElement = 0;
    descriptor_write_as.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    descriptor_write_as.descriptorCount = 1;

    VkWriteDescriptorSetAccelerationStructureKHR descriptor_write_as_data{};
    descriptor_write_as_data.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    descriptor_write_as_data.accelerationStructureCount = 1;
    descriptor_write_as_data.pAccelerationStructures = &scene_tlas.acceleration_structure;

    descriptor_write_as.pNext = &descriptor_write_as_data;
    vkUpdateDescriptorSets(logical_device, 1, &descriptor_write_as, 0, nullptr);

    pipeline.set_descriptor_buffer_binding("camera_parameters", camera_buffer, BufferType::Uniform);

    // prepare mesh data for buffers
    std::vector<vec4> vertices;
    std::vector<uint32_t> vertex_indices;
    std::vector<vec4> normals;
    std::vector<uint32_t> normal_indices;
    std::vector<vec2> texcoords;
    std::vector<uint32_t> texcoord_indices;
    std::vector<uint32_t> mesh_data_offsets;
    std::vector<uint32_t> mesh_offset_indices;
    std::vector<uint32_t> texture_indices;

    // push 0 offset for first instance to align offset positions with instance ids (eliminate need for 'if' in mesh_data shader functions)
    mesh_data_offsets.push_back(vertices.size());
    mesh_data_offsets.push_back(vertex_indices.size());
    mesh_data_offsets.push_back(normals.size());
    mesh_data_offsets.push_back(normal_indices.size());
    mesh_data_offsets.push_back(texcoords.size());
    mesh_data_offsets.push_back(texcoord_indices.size());

    for (int i = 0; i < loaded_mesh_data.size(); i++) {
        vertices.insert(vertices.end(), loaded_mesh_data[i].vertices.begin(), loaded_mesh_data[i].vertices.end());
        vertex_indices.insert(vertex_indices.end(), loaded_mesh_data[i].vertex_indices.begin(), loaded_mesh_data[i].vertex_indices.end());
        normals.insert(normals.end(), loaded_mesh_data[i].normals.begin(), loaded_mesh_data[i].normals.end());
        normal_indices.insert(normal_indices.end(), loaded_mesh_data[i].normal_indices.begin(), loaded_mesh_data[i].normal_indices.end());
        texcoords.insert(texcoords.end(), loaded_mesh_data[i].texcoords.begin(), loaded_mesh_data[i].texcoords.end());
        texcoord_indices.insert(texcoord_indices.end(), loaded_mesh_data[i].texcoord_indices.begin(), loaded_mesh_data[i].texcoord_indices.end());

        // add all 6 offsets contiguously
        mesh_data_offsets.push_back(vertices.size());
        mesh_data_offsets.push_back(vertex_indices.size());
        mesh_data_offsets.push_back(normals.size());
        mesh_data_offsets.push_back(normal_indices.size());
        mesh_data_offsets.push_back(texcoords.size());
        mesh_data_offsets.push_back(texcoord_indices.size());
    }

    // index of mesh and texture used by instance
    for (auto instance : loaded_scene_data.instances) {
        mesh_offset_indices.push_back(loaded_mesh_index[instance.mesh_name]);
        texture_indices.push_back(loaded_texture_index[instance.texture_name]);
    }

    vertex_buffer = device.create_buffer(sizeof(vec4) * vertices.size());
    vertex_buffer.set_data(vertices.data());
    vertex_index_buffer = device.create_buffer(sizeof(uint32_t) * vertex_indices.size());
    vertex_index_buffer.set_data(vertex_indices.data());
    normal_buffer = device.create_buffer(sizeof(vec4) * normals.size());
    normal_buffer.set_data(normals.data());
    normal_index_buffer = device.create_buffer(sizeof(uint32_t) * normal_indices.size());
    normal_index_buffer.set_data(normal_indices.data());
    texcoord_buffer = device.create_buffer(sizeof(vec2) * texcoords.size());
    texcoord_buffer.set_data(texcoords.data());
    texcoord_index_buffer = device.create_buffer(sizeof(uint32_t) * texcoord_indices.size());
    texcoord_index_buffer.set_data(texcoord_indices.data());
    mesh_data_offset_buffer = device.create_buffer(sizeof(uint32_t) * mesh_data_offsets.size());
    mesh_data_offset_buffer.set_data(mesh_data_offsets.data());
    mesh_offset_index_buffer = device.create_buffer(sizeof(uint32_t) * mesh_offset_indices.size());
    mesh_offset_index_buffer.set_data(mesh_offset_indices.data());
    texture_index_buffer = device.create_buffer(sizeof(uint32_t) * texture_indices.size());
    texture_index_buffer.set_data(texture_indices.data());

    pipeline.set_descriptor_buffer_binding("mesh_vertices", vertex_buffer, BufferType::Storage);
    pipeline.set_descriptor_buffer_binding("mesh_vertex_indices", vertex_index_buffer, BufferType::Storage);
    pipeline.set_descriptor_buffer_binding("mesh_normals", normal_buffer, BufferType::Storage);
    pipeline.set_descriptor_buffer_binding("mesh_normal_indices", normal_index_buffer, BufferType::Storage);
    pipeline.set_descriptor_buffer_binding("mesh_texcoords", texcoord_buffer, BufferType::Storage);
    pipeline.set_descriptor_buffer_binding("mesh_texcoord_indices", texcoord_index_buffer, BufferType::Storage);
    pipeline.set_descriptor_buffer_binding("mesh_data_offsets", mesh_data_offset_buffer, BufferType::Storage);
    pipeline.set_descriptor_buffer_binding("mesh_offset_indices", mesh_offset_index_buffer, BufferType::Storage);
    pipeline.set_descriptor_sampler_binding("textures", loaded_textures.data(), loaded_textures.size());
    pipeline.set_descriptor_buffer_binding("texture_indices", texture_index_buffer, BufferType::Storage);
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


    float time = std::chrono::duration_cast<std::chrono::milliseconds>(last_frame_time - startup_time).count();
    if (render_clear_accumulated > -6000) render_clear_accumulated -= 1;

    vkCmdPushConstants(command_buffer, pipeline.pipeline_layout_handle, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0, 4, &time);
    vkCmdPushConstants(command_buffer, pipeline.pipeline_layout_handle, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 4, 4, &render_clear_accumulated);

    // transition output image to writeable format
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

    // set current image as output image for raytracing pipeline
    pipeline.set_descriptor_image_binding("out_image", swap_chain_image_views[image_index], ImageType::Storage);
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline.pipeline_handle);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline.pipeline_layout_handle, 0, pipeline.max_set + 1, pipeline.descriptor_sets.data(), 0, nullptr);

    uint32_t shader_group_handle_size = device.ray_tracing_pipeline_properties.shaderGroupHandleSize;

    device.vkCmdTraceRaysKHR(command_buffer, &pipeline.sbt.region_raygen, &pipeline.sbt.region_miss, &pipeline.sbt.region_hit, &pipeline.sbt.region_callable, swap_chain_extent.width, swap_chain_extent.height, 1);

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
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    VkSemaphore signal_semaphores[] = {render_finished_semaphore};
    submit_info.signalSemaphoreCount = sizeof(signal_semaphores) / sizeof(VkSemaphore);
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

    auto time = std::chrono::high_resolution_clock::now();
    frame_delta = time - last_frame_time;
    last_frame_time = time;

    glfwSetWindowTitle(window, ("Vulkan Renderer | FPS: " + std::to_string(1.0 / frame_delta.count())).c_str());
}

void VulkanApplication::setup_device() {
    device.vulkan_instance = vulkan_instance;
    device.vulkan_device = logical_device;

    // load function pointers
    device.vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkGetAccelerationStructureBuildSizesKHR");
    device.vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkCreateAccelerationStructureKHR");
    device.vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkCmdBuildAccelerationStructuresKHR");
    device.vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkDestroyAccelerationStructureKHR");
    device.vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkGetAccelerationStructureDeviceAddressKHR");
    device.vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkCreateRayTracingPipelinesKHR");
    device.vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkGetRayTracingShaderGroupHandlesKHR");
    device.vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)glfwGetInstanceProcAddress(vulkan_instance, "vkCmdTraceRaysKHR");
}

void VulkanApplication::setup() {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1280, 720, "Vulkan window", nullptr, nullptr);

    startup_time = std::chrono::high_resolution_clock::now();

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
            
            device.ray_tracing_pipeline_properties = VkPhysicalDeviceRayTracingPipelinePropertiesKHR{};
            device.ray_tracing_pipeline_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
            dev_properties.pNext = &device.ray_tracing_pipeline_properties;

            vkGetPhysicalDeviceProperties2(dev, &dev_properties);

            VkPhysicalDeviceFeatures dev_features;
            vkGetPhysicalDeviceFeatures(dev, &dev_features);

            if (dev_properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                physical_device = dev;
                break;
            }
        }

        std::cout << "SBT STRIDE: " << device.ray_tracing_pipeline_properties.shaderGroupHandleSize << std::endl;
        std::cout << "MAX DEPTH: " << device.ray_tracing_pipeline_properties.maxRayRecursionDepth << std::endl;

        vkGetPhysicalDeviceMemoryProperties(physical_device, &device.memory_properties);

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

        VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features = {};
        descriptor_indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        descriptor_indexing_features.runtimeDescriptorArray = VK_TRUE;
        as_features.pNext = &descriptor_indexing_features;

        device_create_info.pNext = &physical_features2;

        if (vkCreateDevice(physical_device, &device_create_info, nullptr, &logical_device) != VK_SUCCESS)
        {
            throw std::runtime_error("failure to create logical device");
        }
    }

    std::cout << "QUEUE FAMILY INDICES | GRAPHICS: " << queue_family_indices.graphics.value() << " | PRESENT: " << queue_family_indices.present.value() << std::endl;

    // retrieve queue handles
    vkGetDeviceQueue(logical_device, queue_family_indices.graphics.value(), 0, &graphics_queue);

    vkGetDeviceQueue(logical_device, queue_family_indices.present.value(), 0, &present_queue);

    setup_device();

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
    device.image_count = image_count;

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

    std::filesystem::path scene_path("../../../scenes/test.toml");
    loaded_scene_data = loaders::load_scene_description(scene_path.string());

    for (auto tex_path : loaded_scene_data.texture_paths) {
        auto full_texture_path = scene_path.parent_path() / std::filesystem::path(std::get<1>(tex_path));
        loaded_textures.push_back(loaders::load_image(&device, full_texture_path.string()));
        loaded_texture_index[std::get<0>(tex_path)] = loaded_textures.size() - 1;
    }

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

    // build blas of loaded meshes
    for (auto tup : loaded_scene_data.mesh_paths) {
        auto full_mesh_path = scene_path.parent_path() / std::filesystem::path(std::get<1>(tup));
        LoadedMeshData loaded_mesh = loaders::load_obj(full_mesh_path.string());
        MeshData mesh_data = create_mesh_data(loaded_mesh);
        loaded_mesh_data.push_back(loaded_mesh);
        loaded_mesh_index[std::get<0>(tup)] = loaded_mesh_data.size() - 1;
        created_meshes.push_back(mesh_data);
        loaded_blas[std::get<0>(tup)] = build_blas(mesh_data);
    }

    for (Image i : loaded_textures) {
        i.cmd_setup_texture(command_buffer);
    }

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

    scene_tlas = build_tlas();

    vkEndCommandBuffer(command_buffer);

    vkResetFences(logical_device, 1, &immediate_fence);

    vkQueueSubmit(graphics_queue, 1, &submit_info, immediate_fence);

    vkWaitForFences(logical_device, 1, &immediate_fence, VK_TRUE, UINT64_MAX);

    std::cout << "TLAS CREATED" << std::endl;

    PipelineBuilder builder = device.create_pipeline_builder();

    // create pipeline
    pipeline = device.create_pipeline_builder()
                   // framework descriptors (set 0)
                   .add_descriptor("out_image", 0, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                   .add_descriptor("acceleration_structure", 0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CALLABLE_BIT_KHR)
                   .add_descriptor("camera_parameters", 0, 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
                   // mesh data descriptors (set 1)
                   .add_descriptor("mesh_vertices", 1, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
                   .add_descriptor("mesh_vertex_indices", 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
                   .add_descriptor("mesh_normals", 1, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
                   .add_descriptor("mesh_normal_indices", 1, 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
                   .add_descriptor("mesh_texcoords", 1, 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
                   .add_descriptor("mesh_texcoord_indices", 1, 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
                   .add_descriptor("mesh_data_offsets", 1, 6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
                   .add_descriptor("mesh_offset_indices", 1, 7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
                   .add_descriptor("textures", 1, 8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CALLABLE_BIT_KHR, 16)
                   .add_descriptor("texture_indices", 1, 9, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CALLABLE_BIT_KHR)
                   // shader stages
                   .add_stage(VK_SHADER_STAGE_RAYGEN_BIT_KHR, "../../../shaders/spirv/ray_gen.spv")
                   .add_stage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "../../../shaders/spirv/closest_hit.spv")
                   .add_stage(VK_SHADER_STAGE_MISS_BIT_KHR, "../../../shaders/spirv/miss.spv")
                   .add_stage(VK_SHADER_STAGE_CALLABLE_BIT_KHR, "../../../shaders/spirv/material.spv")
                   .build();
    create_default_descriptor_writes();

    std::cout << "pipeline created" << std::endl;
    

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

        // camera movement
        vec4 camera_movement = vec4(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camera_movement += camera_data.forward;
            render_clear_accumulated = 4;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            camera_movement -= camera_data.right;
            render_clear_accumulated = 4;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            camera_movement -= camera_data.forward;
            render_clear_accumulated = 4;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            camera_movement += camera_data.right;
            render_clear_accumulated = 4;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            camera_movement += camera_data.up;
            render_clear_accumulated = 4;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        {
            camera_movement -= camera_data.up;
            render_clear_accumulated = 4;
        }
        camera_data.origin += camera_movement * 0.1f;

        // camera rotation
        // horizontal rotation
        glm::mat4 rotation_matrix = glm::mat4(1.0f);
        float angle = 0.05f;
        glm::vec3 cam_up = glm::vec3(camera_data.up.x, camera_data.up.y, camera_data.up.z);
        glm::vec3 cam_fwd = glm::vec3(camera_data.forward.x, camera_data.forward.y, camera_data.forward.z);
        glm::vec3 cam_r = glm::vec3(camera_data.right.x, camera_data.right.y, camera_data.right.z);

        if (glfwGetKey(window, GLFW_KEY_RIGHT)) 
        {
            rotation_matrix = glm::rotate(rotation_matrix, angle, cam_up);
            render_clear_accumulated = 4;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT))
        {
            rotation_matrix = glm::rotate(rotation_matrix, -angle, cam_up);
            render_clear_accumulated = 4;
        }

        glm::vec3 new_fwd = camera_data.forward * rotation_matrix;
        glm::vec3 new_right = glm::cross(new_fwd, cam_up);

        // vertical rotation
        rotation_matrix = glm::mat4(1.0f);

        if (glfwGetKey(window, GLFW_KEY_UP))
        {
            rotation_matrix = glm::rotate(rotation_matrix, -angle, new_right);
            render_clear_accumulated = 4;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN))
        {
            rotation_matrix = glm::rotate(rotation_matrix, angle, new_right);
            render_clear_accumulated = 4;
        }

        glm::vec3 new_up = camera_data.up * rotation_matrix;
        new_fwd = glm::cross(new_up, new_right);

        // roll rotation
        rotation_matrix = glm::mat4(1.0f);

        if (glfwGetKey(window, GLFW_KEY_E))
        {
            rotation_matrix = glm::rotate(rotation_matrix, -angle, new_fwd);
            render_clear_accumulated = 4;
        }
        if (glfwGetKey(window, GLFW_KEY_Q))
        {
            rotation_matrix = glm::rotate(rotation_matrix, angle, new_fwd);
            render_clear_accumulated = 4;
        }

        new_up = glm::vec4(new_up.x, new_up.y, new_up.z, 1.0f) * rotation_matrix;
        new_fwd = glm::cross(new_up, new_right);

        camera_data.forward = glm::vec4(new_fwd.x, new_fwd.y, new_fwd.z, 1.0f);
        camera_data.right = glm::vec4(new_right.x, new_right.y, new_right.z, 1.0f);
        camera_data.up = glm::vec4(new_up.x, new_up.y, new_up.z, 1.0f);

        // FoV
        camera_data.fov_x = 70.0f;

        camera_buffer.set_data(&camera_data);

        draw_frame();
    }

    vkDeviceWaitIdle(logical_device);
}

void VulkanApplication::cleanup() {
    // deinitialization
    camera_buffer.free();
    for (Image i : loaded_textures) {
        i.free();
    }
    free_tlas(scene_tlas);
    for (auto blas = loaded_blas.begin(); blas != loaded_blas.end(); blas++) {
        free_blas(blas->second);
    };
    for (auto mesh = created_meshes.begin(); mesh != created_meshes.end(); mesh++) {
        mesh->free();
    }
    vertex_buffer.free();
    vertex_index_buffer.free();
    normal_buffer.free();
    normal_index_buffer.free();
    texcoord_buffer.free();
    texcoord_index_buffer.free();
    mesh_data_offset_buffer.free();
    mesh_offset_index_buffer.free();
    texture_index_buffer.free();
    pipeline.free();
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