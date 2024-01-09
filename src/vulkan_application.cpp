#include "vulkan_application.h"

#include <set>
#include <iostream>
#include <stdexcept>
#include <filesystem>

#include "loaders/shader_spirv.h"
#include "loaders/geometry_gltf.h"

#include "glm/gtc/matrix_transform.hpp"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_glfw.h"

#include "pipeline/raytracing/pipeline_stage_simple.h"
#include "pipeline/processing/pipeline_stage_simple.h"
#include "pipeline/processing/pipeline_stage_oidn.h"

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
    indices.free();
    vertices.free();
    normals.free();
    texcoords.free();
    tangents.free();
}

MeshData VulkanApplication::create_mesh_data(std::vector<uint32_t> &indices, std::vector<vec3> &vertices, std::vector<vec3> &normals, std::vector<vec2> &texcoords, std::vector<vec3> &tangents) {
    MeshData res{};
    res.device_handle = logical_device;

    VkBufferCreateInfo index_buffer_info{};
    index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    index_buffer_info.size = sizeof(uint32_t) * indices.size();
    index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    res.indices = device.create_buffer(&index_buffer_info, 4, false);

    res.indices.set_data(indices.data());

    VkBufferCreateInfo vertex_buffer_info{};
    vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_buffer_info.size = sizeof(vec3) * vertices.size();
    vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    res.vertices = device.create_buffer(&vertex_buffer_info, 4, false);

    res.vertices.set_data(vertices.data());

    VkBufferCreateInfo normal_buffer_info{};
    normal_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    normal_buffer_info.size = sizeof(vec3) * normals.size();
    normal_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    res.normals = device.create_buffer(&normal_buffer_info, 4, false);

    res.normals.set_data(normals.data());

    VkBufferCreateInfo texcoord_buffer_info{};
    texcoord_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    texcoord_buffer_info.size = sizeof(vec2) * texcoords.size();
    texcoord_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    res.texcoords = device.create_buffer(&texcoord_buffer_info, 4, false);

    res.texcoords.set_data(texcoords.data());

    VkBufferCreateInfo tangent_buffer_info{};
    tangent_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    tangent_buffer_info.size = sizeof(vec3) * tangents.size();
    tangent_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    res.tangents = device.create_buffer(&tangent_buffer_info, 4, false);

    res.tangents.set_data(tangents.data());

    res.index_count = indices.size();
    res.vertex_count = vertices.size();
    res.normal_count = normals.size();
    res.texcoord_count = texcoords.size();
    res.tangent_count = tangents.size();

    return res;
}

AccelerationStructure VulkanApplication::build_blas(std::vector<uint32_t> &indices, std::vector<vec3> &vertices, uint32_t max_vertex) {
    std::cout << "building BLAS with " << vertices.size() << " vertices and " << indices.size() << " indices" << std::endl;

    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;

    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

    Buffer vertex_buffer = device.create_buffer(vertices.size() * sizeof(vec3), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, false);
    vertex_buffer.set_data(vertices.data());

    Buffer index_buffer = device.create_buffer(indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, false);
    index_buffer.set_data(indices.data());

    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    geometry.geometry.triangles.vertexData.deviceAddress = vertex_buffer.get_device_address();
    geometry.geometry.triangles.vertexStride = sizeof(vec3);
    geometry.geometry.triangles.maxVertex = max_vertex;
    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    geometry.geometry.triangles.indexData.deviceAddress = index_buffer.get_device_address();

    VkAccelerationStructureBuildRangeInfoKHR build_range_info{};
    build_range_info.primitiveCount = indices.size() / 3;
    build_range_info.primitiveOffset = 0;

    VkAccelerationStructureBuildGeometryInfoKHR as_info{};
    as_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    as_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    as_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    as_info.flags = 0;
    as_info.geometryCount = 1;
    as_info.pGeometries = &geometry;

    VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_size_info;
    acceleration_structure_size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    uint32_t prim_count = build_range_info.primitiveCount;

    device.vkGetAccelerationStructureBuildSizesKHR(logical_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &as_info, &prim_count, &acceleration_structure_size_info);

    AccelerationStructure result;

    VkBufferCreateInfo as_buffer_info{};
    as_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    as_buffer_info.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
    as_buffer_info.size = acceleration_structure_size_info.accelerationStructureSize;
    result.buffer = device.create_buffer(&as_buffer_info, 4);

    VkBufferCreateInfo scratch_buffer_info{};
    scratch_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    scratch_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
    scratch_buffer_info.size = acceleration_structure_size_info.buildScratchSize;
    Buffer scratch_buffer = device.create_buffer(&scratch_buffer_info, device.acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment, false);

    VkAccelerationStructureCreateInfoKHR as_create_info{};
    as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    as_create_info.buffer = result.buffer.buffer_handle;
    as_create_info.offset = 0;
    as_create_info.size = acceleration_structure_size_info.accelerationStructureSize;
    as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;


    if (device.vkCreateAccelerationStructureKHR(logical_device, &as_create_info, nullptr, &result.acceleration_structure) != VK_SUCCESS) {
        throw std::runtime_error("error creating BLAS");
    }

    as_info.dstAccelerationStructure = result.acceleration_structure;
    as_info.scratchData.deviceAddress = scratch_buffer.get_device_address();

    VkAccelerationStructureBuildRangeInfoKHR* blas_range[] = {
        &build_range_info
    };

    VkCommandBuffer cmdbuf = device.begin_single_use_command_buffer();
    device.vkCmdBuildAccelerationStructuresKHR(cmdbuf, 1, &as_info, blas_range);
    device.end_single_use_command_buffer(cmdbuf);

    scratch_buffer.free();
    vertex_buffer.free();
    index_buffer.free();

    return result;
}

AccelerationStructure VulkanApplication::build_tlas() {
    std::vector<VkAccelerationStructureInstanceKHR> instances;

    std::cout << "Building TLAS for " << loaded_scene_data.instances.size() << " instances" << std::endl;
    for (InstanceData instance : loaded_scene_data.instances) {
        int blas_offset = loaded_mesh_index[instance.object_name];
        std::cout << "Instance " << instance.object_name << " using BLAS offset " << blas_offset << std::endl;
        const auto& object = loaded_objects[instance.object_name];
        std::cout << object.nodes.size() << " nodes" << std::endl;
        for (const auto& node : object.nodes) {
            // skip nodes with no mesh
            if (node.mesh_index < 0) {
                std::cout << "Skipping node with no geometry" << std::endl;
                continue;
            }

            glm::mat4 transformation_matrix = instance.transformation * node.matrix;

            const auto& mesh = object.meshes[node.mesh_index];
            std::cout << mesh.primitives.size() << " primitives" << std::endl;
            for (int i = 0; i < mesh.primitives.size(); i++) {
                uint32_t blas_index = blas_offset + node.mesh_index + i;
                // blas_index = 90;
                std::cout << "instantiating BLAS at index " << blas_index << std::endl;
                VkAccelerationStructureKHR as = created_blas[blas_index].acceleration_structure;

                VkAccelerationStructureDeviceAddressInfoKHR blas_address_info{};
                blas_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
                blas_address_info.accelerationStructure = as;

                VkDeviceAddress blas_address = device.vkGetAccelerationStructureDeviceAddressKHR(logical_device, &blas_address_info);

                std::cout << "BLAS address: " << blas_address << std::endl;

                VkAccelerationStructureInstanceKHR structure{};
                structure.transform.matrix[0][0] = transformation_matrix[0][0];
                structure.transform.matrix[0][1] = transformation_matrix[1][0];
                structure.transform.matrix[0][2] = transformation_matrix[2][0];
                structure.transform.matrix[0][3] = transformation_matrix[3][0];
                structure.transform.matrix[1][0] = transformation_matrix[0][1];
                structure.transform.matrix[1][1] = transformation_matrix[1][1];
                structure.transform.matrix[1][2] = transformation_matrix[2][1];
                structure.transform.matrix[1][3] = transformation_matrix[3][1];
                structure.transform.matrix[2][0] = transformation_matrix[0][2];
                structure.transform.matrix[2][1] = transformation_matrix[1][2];
                structure.transform.matrix[2][2] = transformation_matrix[2][2];
                structure.transform.matrix[2][3] = transformation_matrix[3][2];

                structure.mask = 0xff;
                structure.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
                structure.accelerationStructureReference = blas_address;

                instances.push_back(structure);
            }
        }
    }

    std::cout << "TLAS instances: " << instances.size() << std::endl;

    VkBufferCreateInfo instance_buffer_info{};
    instance_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    instance_buffer_info.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    instance_buffer_info.size = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();

    Buffer instance_buffer = device.create_buffer(&instance_buffer_info, 4, false);

    void* buffer_data;
    vkMapMemory(device.vulkan_device, instance_buffer.device_memory, instance_buffer.device_memory_offset, instance_buffer.buffer_size, 0, &buffer_data);
    memcpy(buffer_data, instances.data(), sizeof(VkAccelerationStructureInstanceKHR) * instances.size());
    vkUnmapMemory(device.vulkan_device, instance_buffer.device_memory);

    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.arrayOfPointers = false;
    geometry.geometry.instances.data.deviceAddress = instance_buffer.get_device_address();

    VkAccelerationStructureBuildGeometryInfoKHR as_info{};
    as_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    as_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    as_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    as_info.flags = 0;
    as_info.geometryCount = 1;
    as_info.pGeometries = &geometry;

    VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_size_info;
    acceleration_structure_size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    uint32_t sizes = {
        (uint32_t)instances.size()
    };
    device.vkGetAccelerationStructureBuildSizesKHR(logical_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &as_info, &sizes, &acceleration_structure_size_info);

    AccelerationStructure result;

    VkBufferCreateInfo as_buffer_info{};
    as_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    as_buffer_info.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
    as_buffer_info.size = acceleration_structure_size_info.accelerationStructureSize;
    result.buffer = device.create_buffer(&as_buffer_info, 4);

    VkBufferCreateInfo scratch_buffer_info{};
    scratch_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    scratch_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
    scratch_buffer_info.size = acceleration_structure_size_info.buildScratchSize;
    Buffer scratch_buffer = device.create_buffer(&scratch_buffer_info, device.acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment, false);

    VkAccelerationStructureCreateInfoKHR as_create_info{};
    as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    as_create_info.buffer = result.buffer.buffer_handle;
    as_create_info.offset = 0;
    as_create_info.size = acceleration_structure_size_info.accelerationStructureSize;
    as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;


    if (device.vkCreateAccelerationStructureKHR(logical_device, &as_create_info, nullptr, &result.acceleration_structure) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating TLAS");
    }

    as_info.dstAccelerationStructure = result.acceleration_structure;
    as_info.scratchData.deviceAddress = scratch_buffer.get_device_address();

    VkAccelerationStructureBuildRangeInfoKHR range_info{};
    range_info.primitiveCount = instances.size();

    VkAccelerationStructureBuildRangeInfoKHR* tlas_range = {
        &range_info
    };

    VkCommandBuffer cmdbuf = device.begin_single_use_command_buffer();
    device.vkCmdBuildAccelerationStructuresKHR(cmdbuf, 1, &as_info, &tlas_range);
    device.end_single_use_command_buffer(cmdbuf);

    instance_buffer.free();
    scratch_buffer.free();

    return result;
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

    rt_pipeline.set_descriptor_acceleration_structure_binding(scene_tlas.acceleration_structure);

    rt_pipeline.set_descriptor_buffer_binding("camera_parameters", camera_buffer, BufferType::Uniform);

    // prepare mesh data for buffers
    std::vector<uint32_t> indices;
    std::vector<vec4> vertices;
    std::vector<vec4> normals;
    std::vector<vec2> texcoords;
    std::vector<vec4> tangents;
    std::vector<uint32_t> mesh_data_offsets;
    std::vector<uint32_t> mesh_offset_indices;
    std::vector<uint32_t> texture_indices;

    // push 0 offset for first instance to align offset positions with instance ids (eliminate need for 'if' in mesh_data shader functions)
    mesh_data_offsets.push_back(indices.size());
    mesh_data_offsets.push_back(vertices.size());
    mesh_data_offsets.push_back(normals.size());
    mesh_data_offsets.push_back(texcoords.size());
    mesh_data_offsets.push_back(tangents.size());

    for (const auto& object_path : loaded_scene_data.object_paths) {
        const auto& object = loaded_objects[std::get<0>(object_path)];
        for (const auto& mesh : object.meshes) {
            for (const auto& primitive : mesh.primitives) {
                indices.insert(indices.end(), primitive.indices.begin(), primitive.indices.end());
                for (auto v : primitive.vertices) { vertices.push_back(vec4(v, 1.0)); }
                for (auto n : primitive.normals) { normals.push_back(vec4(n, 0.0)); }
                texcoords.insert(texcoords.end(), primitive.uvs.begin(), primitive.uvs.end());
                for (auto t : primitive.tangents) { tangents.push_back(vec4(t, 0.0)); }

                // add all 4 offsets contiguously
                mesh_data_offsets.push_back(indices.size());
                mesh_data_offsets.push_back(vertices.size());
                mesh_data_offsets.push_back(normals.size());
                mesh_data_offsets.push_back(texcoords.size());
                mesh_data_offsets.push_back(tangents.size());
            }
        }
    }

    std::cout << "INSTANCE DATA" << std::endl;

    // index of mesh and texture used by instance
    for (auto instance : loaded_scene_data.instances) {

        // set default instance data
        const auto& data = loaded_objects[instance.object_name];
        for (const auto& node : data.nodes) {
            const auto& mesh = data.meshes[node.mesh_index];
            for (int i = 0; i < mesh.primitives.size(); i++) {
                const auto& primitive = mesh.primitives[i];
                mesh_offset_indices.push_back(loaded_mesh_index[instance.object_name] + node.mesh_index + i);
                int material_index = primitive.material_index;
                auto texture_index_offset = loaded_texture_index[instance.object_name];
                const auto& material = data.materials[material_index];

                instance.texture_indices.diffuse = material.diffuse_texture == -1 ? NULL_TEXTURE_INDEX : material.diffuse_texture + texture_index_offset;
                instance.texture_indices.normal = material.normal_texture == -1 ? NULL_TEXTURE_INDEX : material.normal_texture + texture_index_offset;
                instance.texture_indices.roughness = material.roughness_texture == -1 ? NULL_TEXTURE_INDEX : material.roughness_texture + texture_index_offset;
                instance.texture_indices.emissive = material.emission_texture == -1 ? NULL_TEXTURE_INDEX : material.emission_texture + texture_index_offset;
                instance.texture_indices.transmissive = material.transmission_texture == -1 ? NULL_TEXTURE_INDEX : material.transmission_texture + texture_index_offset;

                instance.material_parameters.diffuse_opacity = material.diffuse_factor;
                instance.material_parameters.emissive_factor = material.emission_texture == -1 ? vec4(1,1,1,0) : vec4(material.emissive_factor.r, material.emissive_factor.g, material.emissive_factor.b, 1.0);
                instance.material_parameters.roughness_metallic_transmissive_ior = vec4(material.roughness_factor, material.metallic_factor, material.transmission_factor, material.ior);

                // pairs of 5 textures: diffuse, normal, roughness, emissive, transmissive
                texture_indices.push_back(instance.texture_indices.diffuse);
                texture_indices.push_back(instance.texture_indices.normal);
                texture_indices.push_back(instance.texture_indices.roughness);
                texture_indices.push_back(instance.texture_indices.emissive);
                texture_indices.push_back(instance.texture_indices.transmissive);

                // material parameters
                material_parameters.push_back(instance.material_parameters);
            }
        }
    }

    std::cout << texture_indices.size() << "TEXTURE INDICES" << std::endl;

    index_buffer = device.create_buffer(sizeof(uint32_t) * indices.size());
    index_buffer.set_data(indices.data());
    vertex_buffer = device.create_buffer(sizeof(vec4) * vertices.size());
    vertex_buffer.set_data(vertices.data());
    normal_buffer = device.create_buffer(sizeof(vec4) * normals.size());
    normal_buffer.set_data(normals.data());
    texcoord_buffer = device.create_buffer(sizeof(vec2) * texcoords.size());
    texcoord_buffer.set_data(texcoords.data());
    tangent_buffer = device.create_buffer(sizeof(vec4) * tangents.size());
    tangent_buffer.set_data(tangents.data());
    mesh_data_offset_buffer = device.create_buffer(sizeof(uint32_t) * mesh_data_offsets.size());
    mesh_data_offset_buffer.set_data(mesh_data_offsets.data());
    mesh_offset_index_buffer = device.create_buffer(sizeof(uint32_t) * mesh_offset_indices.size());
    mesh_offset_index_buffer.set_data(mesh_offset_indices.data());
    texture_index_buffer = device.create_buffer(sizeof(uint32_t) * texture_indices.size());
    texture_index_buffer.set_data(texture_indices.data());
    material_parameter_buffer = device.create_buffer(sizeof(InstanceData::MaterialParameters) * material_parameters.size());

    rt_pipeline.set_descriptor_buffer_binding("mesh_indices", index_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("mesh_vertices", vertex_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("mesh_normals", normal_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("mesh_texcoords", texcoord_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("mesh_tangents", tangent_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("mesh_data_offsets", mesh_data_offset_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("mesh_offset_indices", mesh_offset_index_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_sampler_binding("textures", loaded_textures.data(), loaded_textures.size());
    rt_pipeline.set_descriptor_buffer_binding("texture_indices", texture_index_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("material_parameters", material_parameter_buffer, BufferType::Storage);

    int created_area_lights = 0;
    for (int instance_index = 0; instance_index < loaded_scene_data.instances.size(); instance_index++) {
        auto instance = loaded_scene_data.instances[instance_index];
        auto object = loaded_objects[instance.object_name];
        for (auto node : object.nodes) {
            auto mesh = object.meshes[node.mesh_index];
            uint32_t primitive_index = 0;
            for (auto primitive : mesh.primitives) {
                if (object.materials[primitive.material_index].emission_texture > -1) {
                    Shaders::Light light;
                    light.uint_data[0] = LightData::LightType::AREA;
                    light.uint_data[1] = instance_index + primitive_index;
                    light.uint_data[2] = primitive.vertices.size();

                    mat4 transform = instance.transformation * node.matrix;

                    light.float_data[0] = transform[0][0];
                    light.float_data[1] = transform[1][0];
                    light.float_data[2] = transform[2][0];
                    light.float_data[3] = transform[3][0];
                    light.float_data[4] = transform[0][1];
                    light.float_data[5] = transform[1][1];
                    light.float_data[6] = transform[2][1];
                    light.float_data[7] = transform[3][1];
                    light.float_data[8] = transform[0][2];
                    light.float_data[9] = transform[1][2];
                    light.float_data[10] = transform[2][2];
                    light.float_data[11] = transform[3][2];
                    light.float_data[12] = transform[0][3];
                    light.float_data[13] = transform[1][3];
                    light.float_data[14] = transform[2][3];
                    light.float_data[15] = transform[3][3];

                    lights.push_back(light);
                    created_area_lights++;
                }
                primitive_index++;
            }
        }
    }
    std::cout << "Created " << created_area_lights << " area lights from scene geometry with emissive maps" << std::endl;

    int light_buffer_size = lights.size();
    if (light_buffer_size < 1) light_buffer_size = 1;
    lights_buffer = device.create_buffer(sizeof(Shaders::Light) * light_buffer_size);
    if (lights.size() > 0) lights_buffer.set_data(lights.data(), 0, sizeof(Shaders::Light) * light_buffer_size);

    rt_pipeline.set_descriptor_buffer_binding("lights", lights_buffer, BufferType::Storage);
}

void VulkanApplication::create_synchronization() {
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateFence(logical_device, &fence_info, nullptr, &immediate_fence);
    vkCreateFence(logical_device, &fence_info, nullptr, &tlas_fence);

    if (vkCreateSemaphore(logical_device, &semaphore_info, nullptr, &image_available_semaphore) != VK_SUCCESS ||
        vkCreateSemaphore(logical_device, &semaphore_info, nullptr, &render_finished_semaphore) != VK_SUCCESS ||
        vkCreateFence(logical_device, &fence_info, nullptr, &in_flight_fence) != VK_SUCCESS) {
            throw std::runtime_error("error creating synchronization");
        }
}

void VulkanApplication::create_swapchain() {
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
                    device.surface_format = format;
                    found = true;
                    break;
                }
            }
            if (!found)
                device.surface_format = formats[0];
        }

        // select present mode (MAILBOX is preferred)
        VkPresentModeKHR present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        // {
        //     bool found = false;
        //     for (const auto &mode : present_modes)
        //     {
        //         if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        //         {
        //             present_mode = mode;
        //             found = true;
        //             break;
        //         }
        //     }
        //     if (!found)
        //         present_mode = VK_PRESENT;
        // }

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
        create_info.imageFormat = device.surface_format.format;
        create_info.imageColorSpace = device.surface_format.colorSpace;
        create_info.imageExtent = swap_chain_extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        uint32_t queue_family_indices_int[] = {queue_family_indices.graphics_compute.value(), queue_family_indices.present.value()};

        if (queue_family_indices.graphics_compute != queue_family_indices.present)
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
    

        // retrieve swap chain images
        vkGetSwapchainImagesKHR(logical_device, swap_chain, &image_count, nullptr);
        swap_chain_images.resize(image_count);
        vkGetSwapchainImagesKHR(logical_device, swap_chain, &image_count, swap_chain_images.data());
        device.image_count = image_count;
}

void VulkanApplication::create_swapchain_image_views() {
        swap_chain_image_views.resize(device.image_count);
        for (size_t i = 0; i < device.image_count; i++)
        {
            VkImageViewCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image = swap_chain_images[i];
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format = device.surface_format.format;

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
}

void VulkanApplication::create_framebuffers() {
    framebuffers.resize(swap_chain_image_views.size());
    for (size_t i = 0; i < swap_chain_image_views.size(); i++)
    {
        VkImageView attachments[] = {
            swap_chain_image_views[i]
        };

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = swap_chain_extent.width;
        framebuffer_info.height = swap_chain_extent.height;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(logical_device, &framebuffer_info, nullptr, framebuffers.data() + i) != VK_SUCCESS)
        {
            throw std::runtime_error("failure creating framebuffer");
        }
    }
}

void VulkanApplication::recreate_swapchain() {
    vkDeviceWaitIdle(device.vulkan_device);

    // clean up old swapchain
    for (size_t i = 0; i < framebuffers.size(); i++) {
        vkDestroyFramebuffer(device.vulkan_device, framebuffers[i], nullptr);
    }
    for (size_t i = 0; i < swap_chain_image_views.size(); i++) {
        vkDestroyImageView(device.vulkan_device, swap_chain_image_views[i], nullptr);
    }
    vkDestroySwapchainKHR(device.vulkan_device, swap_chain, nullptr);

    create_swapchain();
    create_swapchain_image_views();

    std::cout << "SWAPCHAIN" << std::endl;
    submit_immediate([&]() {
        for (int i = 0; i < this->swap_chain_images.size(); i++) {
            // transition output image to writeable format
            VkImageMemoryBarrier image_barrier = {};
            image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            image_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_barrier.image = swap_chain_images[i];
            image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_barrier.subresourceRange.baseMipLevel = 0;
            image_barrier.subresourceRange.levelCount = 1;
            image_barrier.subresourceRange.baseArrayLayer = 0;
            image_barrier.subresourceRange.layerCount = 1;
            image_barrier.srcAccessMask = 0;
            image_barrier.dstAccessMask = 0;

            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);
        }
    });

    create_framebuffers();
}

void VulkanApplication::recreate_render_images() {
    float render_scale = 1.0;
    switch(ui.render_scale_index) {
        case 1:
            render_scale = 0.5;
            break;
        case 2:
            render_scale = 0.25;
            break;
    }
    render_image_extent = {(uint32_t)(swap_chain_extent.width * render_scale), (uint32_t)(swap_chain_extent.height * render_scale)};
    
    VkCommandBuffer cmdbuf = device.begin_single_use_command_buffer();
    rt_pipeline.cmd_on_resize(cmdbuf, render_image_extent);
    p_pipeline_builder.cmd_on_resize(cmdbuf, render_image_extent);
    device.end_single_use_command_buffer(cmdbuf);

    if (render_transfer_image.width < 1) render_transfer_image.free();
    render_transfer_image = device.create_image(swap_chain_extent.width, swap_chain_extent.height, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, false);

}

void VulkanApplication::draw_frame() {
    uint32_t image_index;
    vkAcquireNextImageKHR(logical_device, swap_chain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);

    vkResetCommandPool(logical_device, command_pool, 0);

    if (pipeline_dirty) {
        rebuild_pipeline();
        pipeline_dirty = false;
    }

    // command buffer begin
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)    {
        throw std::runtime_error("error beginning command buffer");
    }

    if (render_images_dirty) {recreate_render_images(); render_images_dirty = false;}
    material_parameter_buffer.set_data(material_parameters.data(), 0, sizeof(InstanceData::MaterialParameters) * material_parameters.size());
    lights_buffer.set_data(lights.data(), 0, sizeof(Shaders::Light) * lights.size());

    Shaders::PushConstants push_constants;
    push_constants.sbt_stride = rt_pipeline.sbt_stride;
    push_constants.time = std::chrono::duration_cast<std::chrono::milliseconds>(last_frame_time - startup_time).count();
    push_constants.clear_accumulated = render_clear_accumulated;
    push_constants.light_count = lights.size();
    push_constants.max_depth = ui.max_ray_depth;
    push_constants.frame_samples = ui.frame_samples;
    push_constants.exposure = ui.exposure;
    push_constants.environment_cdf_dimensions = Shaders::uvec2(loaded_environment.cdf_map.width, loaded_environment.cdf_map.height);
    push_constants.image_extent = Shaders::uvec2(render_image_extent.width, render_image_extent.height);
    uint32_t flags = 0;
    if (ui.direct_lighting_enabled) flags |= 0b1;
    if (ui.indirect_lighting_enabled) flags |= 0b10;
    push_constants.flags = flags;
    vkCmdPushConstants(command_buffer, rt_pipeline.builder->pipeline_layout, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0, sizeof(Shaders::PushConstants), &push_constants);

    // raytracer draw
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rt_pipeline.builder->pipeline_layout, 0, rt_pipeline.builder->max_set + 1, rt_pipeline.builder->descriptor_sets.data(), 0, nullptr);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rt_pipeline.pipeline_handle);
    device.vkCmdTraceRaysKHR(command_buffer, &rt_pipeline.sbt.region_raygen, &rt_pipeline.sbt.region_miss, &rt_pipeline.sbt.region_hit, &rt_pipeline.sbt.region_callable, render_image_extent.width, render_image_extent.height, 1);

    // transition output image to writeable format
    VkImageMemoryBarrier image_barrier = {};
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.image = swap_chain_images[image_index];
    image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;
    image_barrier.srcAccessMask = 0;
    image_barrier.dstAccessMask = 0;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);


    OutputBuffer selected_output = rt_pipeline.get_output_buffer(ui.selected_output_image);

    // p_pipeline_builder.input_image = &displayed_image.image;
    // p_pipeline.run(command_buffer);

    VkBufferImageCopy output_buffer_copy {};
    output_buffer_copy.bufferOffset = 0;
    output_buffer_copy.imageExtent = VkExtent3D{render_transfer_image.width, render_transfer_image.height, 1};
    output_buffer_copy.imageOffset = VkOffset3D{0, 0, 0};
    output_buffer_copy.imageSubresource.layerCount = 1;
    output_buffer_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkBufferMemoryBarrier buffer_barrier {};
    buffer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    buffer_barrier.buffer = selected_output.buffer.buffer_handle;
    buffer_barrier.size = VK_WHOLE_SIZE;

    render_transfer_image.transition_layout(command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0);
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 1, &buffer_barrier, 0, nullptr);

    // std::cout << "TODO: IMPLEMENT color_under_cursor" << std::endl;
    vec2 cursor_pos = get_cursor_position();
    if (cursor_pos.x >= 0 && cursor_pos.x < swap_chain_extent.width && cursor_pos.y >= 0 && cursor_pos.y < swap_chain_extent.height) {
        uint32_t cursor_pixel_offset = cursor_pos.x + cursor_pos.y * render_image_extent.width;
        vec4* color_buffer;
        vkMapMemory(device.vulkan_device, selected_output.buffer.device_memory, selected_output.buffer.device_memory_offset, VK_WHOLE_SIZE, 0, (void**)&color_buffer);
        vec4 temp = color_buffer[cursor_pixel_offset];
        ui.color_under_cursor.r = temp.r;
        ui.color_under_cursor.g = temp.g;
        ui.color_under_cursor.b = temp.b;
        vkUnmapMemory(device.vulkan_device, selected_output.buffer.device_memory);
    }

    vkCmdCopyBufferToImage(command_buffer, selected_output.buffer.buffer_handle, render_transfer_image.image_handle, render_transfer_image.layout, 1, &output_buffer_copy);
    render_transfer_image.transition_layout(command_buffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0);

    VkImageBlit transfer_blit {};
    transfer_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    transfer_blit.srcSubresource.layerCount = 1;
    transfer_blit.srcOffsets[0] = {0,0,0};
    transfer_blit.srcOffsets[1] = {(int)swap_chain_extent.width, (int)swap_chain_extent.height, 1};
    transfer_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    transfer_blit.dstSubresource.layerCount = 1;
    transfer_blit.dstOffsets[0] = {0,0,0};
    transfer_blit.dstOffsets[1] = {(int)swap_chain_extent.width, (int)swap_chain_extent.height, 1};

    vkCmdBlitImage(command_buffer, render_transfer_image.image_handle, render_transfer_image.layout, swap_chain_images[image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &transfer_blit, VK_FILTER_LINEAR);

    // imgui draw
    VkOffset2D render_area_offset{};
    render_area_offset.x = 0;
    render_area_offset.y = 0;

    VkExtent2D render_area_extent{};
    render_area_extent.width = swap_chain_extent.width;
    render_area_extent.height = swap_chain_extent.height;

    VkRect2D render_area{};
    render_area.offset = render_area_offset;
    render_area.extent = render_area_extent;

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = framebuffers[image_index];
    render_pass_info.renderArea = render_area;
    render_pass_info.clearValueCount = 0;


    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();

    ui.draw();

    ImGui::Render();
    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("encountered an error when ending command buffer");
    }

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {image_available_semaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_available_semaphore;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    VkSemaphore signal_semaphores[] = {render_finished_semaphore};
    submit_info.signalSemaphoreCount = sizeof(signal_semaphores) / sizeof(VkSemaphore);
    submit_info.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fence) != VK_SUCCESS)
    {
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

    vkWaitForFences(logical_device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(logical_device, 1, &in_flight_fence);
}

void VulkanApplication::setup_device() {
    device.vulkan_instance = vulkan_instance;
    device.vulkan_device = logical_device;

    device.command_pool = command_pool;
    device.graphics_queue = graphics_queue;

    device.graphics_queue_family_index = queue_family_indices.graphics_compute.value();

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

void VulkanApplication::submit_immediate(std::function<void()> lambda) {
    if (vkResetFences(logical_device, 1, &immediate_fence) != VK_SUCCESS) {
        throw std::runtime_error("error resetting immediate fence");
    };

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        throw std::runtime_error("error beginning command buffer in submit_immediate");
    }

    lambda();

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("error ending command buffer in submit_immediate");
    };


    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 0;
    submit_info.signalSemaphoreCount = 0;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    auto e = vkQueueSubmit(graphics_queue, 1, &submit_info, immediate_fence);
    if (e != VK_SUCCESS) {
        std::cout << "ERROR: " << e << std::endl;
        throw std::runtime_error("error submitting command buffer in submit_immediate");
    }

    while(true) {
        if (vkWaitForFences(logical_device, 1, &immediate_fence, VK_FALSE, 10000) == VK_SUCCESS) break;
    }
}

void check_vk_result(VkResult r) {
    if (r != VK_SUCCESS) std::cout << "VKResult was not VK_SUCCESS" << std::endl;
}

void VulkanApplication::init_imgui() {
    // 1: create descriptor pool for IMGUI
    // the size of the pool is very oversize, but it's copied from imgui demo itself.
    VkDescriptorPoolSize pool_sizes[] =
        {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 10000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10000}};

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 10000;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;


    if (vkCreateDescriptorPool(device.vulkan_device, &pool_info, nullptr, &imgui_descriptor_pool) != VK_SUCCESS) {
        throw std::runtime_error("error creating imgui descriptor pool");
    }

    // initialize imgui library
    auto ctx = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx);

    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = device.vulkan_instance;
    init_info.PhysicalDevice = physical_device;
    init_info.Device = device.vulkan_device;
    init_info.Queue = graphics_queue;
    init_info.DescriptorPool = imgui_descriptor_pool;
    init_info.MinImageCount = device.image_count;
    init_info.ImageCount = device.image_count;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.ColorAttachmentFormat = device.surface_format.format;
    init_info.PipelineCache = rt_pipeline.pipeline_cache_handle;
    init_info.QueueFamily = queue_family_indices.graphics_compute.value();
    init_info.Subpass = 0;
    init_info.CheckVkResultFn = check_vk_result;

    ImGui_ImplVulkan_Init(&init_info, render_pass);

    // execute a gpu command to upload imgui font textures
    std::cout << "IMGUI upload" << std::endl;
    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

    // clear font textures from cpu data
    // ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void VulkanApplication::setup() {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1280, 720, "Vulkan window", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetWindowSizeLimits(window, 100, 100, GLFW_DONT_CARE, GLFW_DONT_CARE);

    startup_time = std::chrono::high_resolution_clock::now();

    // initialize vulkan
    // create vulkan instance
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
            if (!strcmp(available_extensions[i].extensionName, glfw_extensions[i]))
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
    

    // create window surface
    if (glfwCreateWindowSurface(vulkan_instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface");
    }
    std::cout << "window surface created" << std::endl;

    // physical device
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

        device.acceleration_structure_properties = VkPhysicalDeviceAccelerationStructurePropertiesKHR{};
        device.acceleration_structure_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
        device.ray_tracing_pipeline_properties.pNext = &device.acceleration_structure_properties;

        dev_properties.pNext = &device.ray_tracing_pipeline_properties;

        vkGetPhysicalDeviceProperties2(dev, &dev_properties);

        std::cout << "MAX INSTANCES " << device.acceleration_structure_properties.maxInstanceCount << " | MAX PRIMITIVES " << device.acceleration_structure_properties.maxPrimitiveCount << " | MAX ALLOCATIONS " << dev_properties.properties.limits.maxMemoryAllocationCount << std::endl;

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
        // find queue family that supports graphics and compute
        if ((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            queue_family_indices.graphics_compute = std::make_optional(family_index);
        }
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, family_index, surface, &present_support);
        if (present_support)
        {
            queue_family_indices.present = std::make_optional(family_index);
        }

        if (queue_family_indices.graphics_compute && queue_family_indices.present) {
            break;
        }

        family_index++;
    }
    
    std::cout << "valid physical device found" << std::endl;

    // create logical device
    {
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> unique_queue_families = {queue_family_indices.graphics_compute.value(), queue_family_indices.present.value()};
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
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            // dependencies for compute shader functionality
            VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
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
        descriptor_indexing_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        as_features.pNext = &descriptor_indexing_features;

        VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features = {};
        ray_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        ray_query_features.rayQuery = VK_TRUE;
        descriptor_indexing_features.pNext = &ray_query_features;

        VkPhysicalDeviceMaintenance4FeaturesKHR maintenance4_features = {};
        maintenance4_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES_KHR;
        maintenance4_features.maintenance4 = VK_TRUE;
        ray_query_features.pNext = &maintenance4_features;

        device_create_info.pNext = &physical_features2;

        if (vkCreateDevice(physical_device, &device_create_info, nullptr, &logical_device) != VK_SUCCESS)
        {
            throw std::runtime_error("failure to create logical device");
        }
    }

    std::cout << "QUEUE FAMILY INDICES | GRAPHICS: " << queue_family_indices.graphics_compute.value() << " | PRESENT: " << queue_family_indices.present.value() << std::endl;

    // retrieve queue handles
    vkGetDeviceQueue(logical_device, queue_family_indices.graphics_compute.value(), 0, &graphics_queue);

    vkGetDeviceQueue(logical_device, queue_family_indices.present.value(), 0, &present_queue);


    // create command pool
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_family_indices.graphics_compute.value();

    if (vkCreateCommandPool(logical_device, &pool_info, nullptr, &command_pool) != VK_SUCCESS)
    {
        throw std::runtime_error("error creating command pool");
    }
    setup_device();

    // create command buffers
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(logical_device, &alloc_info, &command_buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("error allocating command buffer");
    }

    create_synchronization();
    vkResetFences(logical_device, 1, &immediate_fence);
    vkResetFences(logical_device, 1, &in_flight_fence);

    create_swapchain();
    create_swapchain_image_views();

    // create render pass
    VkAttachmentDescription color_attachment{};
    color_attachment.format = device.surface_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.inputAttachmentCount = 0;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
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

    // create framebuffers after render pass
    create_framebuffers();

    loaded_scene_data = loaders::load_scene_description(scene_path.string());

    // BUILD BLAS
    vkResetCommandPool(logical_device, command_pool, 0);
    vkResetFences(logical_device, 1, &tlas_fence);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        throw std::runtime_error("error beginning command buffer");
    }

    // load environment map
    loaded_environment = loaders::load_environment_map(&device, (scene_path.parent_path() / std::filesystem::path(loaded_scene_data.environment_path)).string());
    loaded_textures.push_back(loaded_environment.image);
    loaded_textures.push_back(loaded_environment.cdf_map);
    loaded_textures.push_back(loaded_environment.conditional_cdf_map);

    // build blas of loaded meshes
    loaded_objects.clear();
    loaded_mesh_index.clear();
    created_meshes.clear();

    for (auto object_path : loaded_scene_data.object_paths) {
        auto full_object_path = std::filesystem::absolute(scene_path.parent_path() / std::filesystem::path(std::get<1>(object_path)));
        auto object_name = std::get<0>(object_path);
        std::cout << "Loading scene object " << object_name << std::endl;
        GLTFData gltf = loaders::load_gltf(full_object_path.string());
        // if (gltf_processor) {
        //     gltf_processor->set_data(&gltf);
        //     gltf_processor->process();
        // }
        loaded_objects[object_name] = gltf;
        loaded_mesh_index[object_name] = created_meshes.size();
        for (auto &mesh : gltf.meshes) {
            for (auto &primitive : mesh.primitives) {
                created_meshes.push_back(create_mesh_data(primitive.indices, primitive.vertices, primitive.normals, primitive.uvs, primitive.tangents));
                created_blas.push_back(build_blas(primitive.indices, primitive.vertices, primitive.max_vertex));
            }
        }
        
        full_object_path.remove_filename();
        loaded_texture_index[object_name] = loaded_textures.size();
        std::cout << "loading " << gltf.textures.size() << " textures starting at index " << loaded_texture_index[object_name] << std::endl;
        for (const auto &texture : gltf.textures) {
            auto full_path = full_object_path / texture.path;
            loaded_textures.push_back(loaders::load_image(&device, full_path.string(), 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT));
        }

        std::cout << "meshes after " << object_name << ": " << created_meshes.size() << "|" << created_blas.size() << std::endl;
    }

    std::cout << "Loaded " << loaded_objects.size() << " scene objects:" << std::endl;
    for (auto key = loaded_objects.begin(); key != loaded_objects.end(); key++) {
        std::cout << key->first.c_str() << ": " << loaded_mesh_index[key->first] << std::endl;
    }

    std::cout << "Created " << created_blas.size() << " Mesh BLASes" << std::endl;


    for (auto light_data : loaded_scene_data.lights) {
        Shaders::Light light;
        light.uint_data[0] = light_data.type;
        switch (light_data.type) {
            case LightData::LightType::POINT:
                light.float_data[0] = light_data.position.x;
                light.float_data[1] = light_data.position.y;
                light.float_data[2] = light_data.position.z;
                
                light.float_data[3] = light_data.intensity.x;
                light.float_data[4] = light_data.intensity.y;
                light.float_data[5] = light_data.intensity.z;
                break;
            case LightData::LightType::DIRECTIONAL:
                light.float_data[0] = light_data.direction.x;
                light.float_data[1] = light_data.direction.y;
                light.float_data[2] = light_data.direction.z;
                
                light.float_data[3] = light_data.intensity.x;
                light.float_data[4] = light_data.intensity.y;
                light.float_data[5] = light_data.intensity.z;
                break;
        }
        lights.push_back(light);
    }
    std::cout << "Loaded " << lights.size() << " lights" << std::endl;
    
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 0;
    submit_info.signalSemaphoreCount = 0;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    scene_tlas = build_tlas();

    std::cout << "Scene TLAS constructed" << std::endl;

    // transfer framebuffer images to correct format
    for (int i = 0; i < swap_chain_images.size(); i++) {
        VkImageMemoryBarrier texture_barrier = {};
        texture_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        texture_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        texture_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        texture_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        texture_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        texture_barrier.image = swap_chain_images[i];
        texture_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        texture_barrier.subresourceRange.baseMipLevel = 0;
        texture_barrier.subresourceRange.levelCount = 1;
        texture_barrier.subresourceRange.baseArrayLayer = 0;
        texture_barrier.subresourceRange.layerCount = 1;
        texture_barrier.srcAccessMask = 0;
        texture_barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 0, nullptr, 0, nullptr, 1, &texture_barrier);
    }


    // create raytracing pipeline
    rt_pipeline_builder = device.create_raytracing_pipeline_builder()
                   .with_default_pipeline()
                    ;

    rt_pipeline = rt_pipeline_builder.build();

    // create process pipeline
    p_pipeline_builder = device.create_processing_pipeline_builder()
                    .with_stage(std::make_shared<ProcessingPipelineStageOIDN>(ProcessingPipelineStageOIDN()))
                    // .with_stage(std::make_shared<ProcessingPipelineStageSimple>(ProcessingPipelineStageSimple()));
                    ;

    p_pipeline = p_pipeline_builder.build();
    


    init_imgui();
    std::cout << "IMGUI INITIALIZED" << std::endl;

    vkEndCommandBuffer(command_buffer);

    std::cout << "SUBMIT" << vkQueueSubmit(graphics_queue, 1, &submit_info, tlas_fence) << std::endl;

    while(vkWaitForFences(logical_device, 1, &tlas_fence, VK_FALSE, UINT64_MAX) != VK_SUCCESS);
    create_default_descriptor_writes();

    std::cout << "pipeline created" << std::endl;


    // set glfw callbacks
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        std::cout << "resized to " << width << "x" << height << std::endl;
        VulkanApplication* app = (VulkanApplication*)glfwGetWindowUserPointer(window);
        if (width > 0 && height > 0) {
            app->minimized = false;
            app->recreate_swapchain();
            app->render_images_dirty = true;
            app->render_clear_accumulated = 0;
        } else {
            app->minimized = true;
        }
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

        VulkanApplication* app = (VulkanApplication*)glfwGetWindowUserPointer(window);
        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (action == GLFW_PRESS) {
                app->mouse_look_active = true;
                if (!app->ui.is_hovered()) {
                    std::cout << "TODO: IMPLEMENT object selection on mouse press" << std::endl;
                    // vec3 hovered_instance_color = app->rt_pipeline.get_output_image("Instance Indices").image.get_pixel(app->get_cursor_position().x, app->get_cursor_position().y);
                    // int instance_index = hovered_instance_color.b + hovered_instance_color.g * (255) + hovered_instance_color.r * (255*255);
                    // if (1 - hovered_instance_color.r < FLT_EPSILON && 1 - hovered_instance_color.g < FLT_EPSILON && 1 - hovered_instance_color.b < FLT_EPSILON) instance_index = -1;
                    
                    // app->ui.selected_instance = instance_index;
                    // if (instance_index != -1) {
                    //     app->ui.selected_instance_parameters = &app->material_parameters[instance_index];
                    // } else {
                    //     app->ui.selected_instance_parameters = nullptr;
                    // }
                }
            } else {
                app->mouse_look_active = false;
            }
        }
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) {
        ImGui_ImplGlfw_CursorPosCallback(window, x, y);

        VulkanApplication* app = (VulkanApplication*)glfwGetWindowUserPointer(window);

        app->delta_cursor_x = x - app->last_cursor_x;
        app->delta_cursor_y = y - app->last_cursor_y;

        app->last_cursor_x = x;
        app->last_cursor_y = y;

        if (app->mouse_look_active && !app->ui.is_hovered()) {
            app->camera_look_x = app->delta_cursor_x * 0.01;
            app->camera_look_y = app->delta_cursor_y * 0.01;
        }
    });

    glfwSetScrollCallback(window, [](GLFWwindow* window, double x_offset, double y_offset) {
        ImGui_ImplGlfw_ScrollCallback(window, x_offset, y_offset);

        VulkanApplication* app = (VulkanApplication*)glfwGetWindowUserPointer(window);

        if (app->mouse_look_active && !app->ui.is_hovered()) {
            app->ui.camera_speed += y_offset * app->ui.camera_speed * 1e-1f;
        }
    });

    std::cout << "Setup completed" << std::endl;
}

void VulkanApplication::run() {
    // initialize UI
    ui.init(this);

    //load persisted camera data
    std::filesystem::path cam_path(camera_data_path);
    if (std::filesystem::exists(cam_path)) {
        std::cout << "found saved camera data. loading..." << std::endl;
        toml::table data = toml::parse_file(cam_path.string());
        camera_data.origin = vec4(data["origin"][0].value_or(0.0), data["origin"][1].value_or(0.0), data["origin"][2].value_or(0.0), 1.0);
        camera_data.forward = vec4(data["forward"][0].value_or(0.0), data["forward"][1].value_or(0.0), data["forward"][2].value_or(1.0), 0.0);
        camera_data.right = vec4(data["right"][0].value_or(1.0), data["right"][1].value_or(0.0), data["right"][2].value_or(0.0), 0.0);
        camera_data.up = vec4(data["up"][0].value_or(0.0), data["up"][1].value_or(1.0), data["up"][2].value_or(0.0), 0.0);
        ui.camera_fov = data["fov_x"].value_or(70.0);
    }

    delta_cursor_x = 0;
    delta_cursor_y = 0;
    camera_look_x = 0;
    camera_look_y = 0;

    // main event loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (minimized) continue;

        // camera movement
        glm::vec3 cam_up = glm::normalize(glm::vec3(camera_data.up.x, camera_data.up.y, camera_data.up.z));
        glm::vec3 cam_fwd = glm::normalize(glm::vec3(camera_data.forward.x, camera_data.forward.y, camera_data.forward.z));
        glm::vec3 cam_r = glm::normalize(glm::vec3(camera_data.right.x, camera_data.right.y, camera_data.right.z));

        vec3 camera_movement = vec3(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camera_movement += cam_fwd;
            camera_changed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            camera_movement -= cam_r;
            camera_changed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            camera_movement -= cam_fwd;
            camera_changed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            camera_movement += cam_r;
            camera_changed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            camera_movement += cam_up;
            camera_changed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        {
            camera_movement -= cam_up;
            camera_changed = true;
        }

        float camera_speed = frame_delta.count();
        if (fabsf(camera_speed) > 1) camera_speed = 0;
        glm::vec3 cam_delta = camera_movement * camera_speed * ui.camera_speed;
        camera_data.origin += glm::vec4(cam_delta.x, cam_delta.y, cam_delta.z, 0.0f);

        // camera rotation
        // horizontal rotation
        glm::mat4 rotation_matrix = glm::mat4(1.0f);
        float camera_rotation_speed = frame_delta.count();
        if (fabsf(camera_rotation_speed) > 1) camera_rotation_speed = 0;
        float angle = camera_rotation_speed;
        

        rotation_matrix = glm::rotate(rotation_matrix, camera_look_x, cam_up);

        glm::vec3 new_fwd = camera_data.forward * rotation_matrix;
        glm::vec3 new_right = glm::cross(new_fwd, cam_up);

        // vertical rotation
        rotation_matrix = glm::rotate(rotation_matrix, camera_look_y, new_right);

        if (camera_look_x != 0 || camera_look_y != 0) camera_changed = true;

        glm::vec3 new_up = camera_data.up * rotation_matrix;
        new_fwd = glm::cross(new_up, new_right);

        // roll rotation
        rotation_matrix = glm::mat4(1.0f);

        if (glfwGetKey(window, GLFW_KEY_E))
        {
            rotation_matrix = glm::rotate(rotation_matrix, -angle, new_fwd);
            camera_changed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_Q))
        {
            rotation_matrix = glm::rotate(rotation_matrix, angle, new_fwd);
            camera_changed = true;
        }

        new_up = glm::vec4(new_up.x, new_up.y, new_up.z, 1.0f) * rotation_matrix;
        new_fwd = glm::cross(new_up, new_right);

        new_fwd = glm::normalize(new_fwd);
        new_up = glm::normalize(new_up);
        new_right = glm::normalize(new_right);

        camera_data.forward = glm::vec4(new_fwd.x, new_fwd.y, new_fwd.z, 1.0f);
        camera_data.right = glm::vec4(new_right.x, new_right.y, new_right.z, 1.0f);
        camera_data.up = glm::vec4(new_up.x, new_up.y, new_up.z, 1.0f);

        // FoV
        camera_data.fov_x = ui.camera_fov;

        if (camera_changed | ui.has_changed()) render_clear_accumulated = 0;
        camera_changed = false;

        camera_buffer.set_data(&camera_data, 0, sizeof(Shaders::CameraData));

        if (render_clear_accumulated < std::numeric_limits<uint32_t>::max()) render_clear_accumulated += 1;
        
        draw_frame();

        camera_look_x = 0;
        camera_look_y = 0;

    }

    // persist camera data
    toml::table camera_data_out;
    camera_data_out.insert("origin", toml::array{camera_data.origin.x, camera_data.origin.y, camera_data.origin.z});
    camera_data_out.insert("forward", toml::array{camera_data.forward.x, camera_data.forward.y, camera_data.forward.z});
    camera_data_out.insert("right", toml::array{camera_data.right.x, camera_data.right.y, camera_data.right.z});
    camera_data_out.insert("up", toml::array{camera_data.up.x, camera_data.up.y, camera_data.up.z});
    camera_data_out.insert("fov_x", camera_data.fov_x);
    std::ofstream camera_data_out_file(camera_data_path);
    camera_data_out_file << camera_data_out;

    vkDeviceWaitIdle(logical_device);
}

void VulkanApplication::cleanup() {
    // imgui cleanup
    vkDestroyDescriptorPool(device.vulkan_device, imgui_descriptor_pool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // deinitialization
    lights_buffer.free();
    camera_buffer.free();
    for (Image i : loaded_textures) {
        i.free();
    }
    // destroy TLAS
    device.vkDestroyAccelerationStructureKHR(logical_device, scene_tlas.acceleration_structure, nullptr);
    scene_tlas.buffer.free();

    // destroy BLAS
    for (auto blas : created_blas) {
        device.vkDestroyAccelerationStructureKHR(logical_device, blas.acceleration_structure, nullptr);
        blas.buffer.free();
    }

    // free mesh data
    for (auto mesh = created_meshes.begin(); mesh != created_meshes.end(); mesh++) {
        mesh->free();
    }
    index_buffer.free();
    vertex_buffer.free();
    normal_buffer.free();
    texcoord_buffer.free();
    tangent_buffer.free();
    mesh_data_offset_buffer.free();
    mesh_offset_index_buffer.free();
    texture_index_buffer.free();
    material_parameter_buffer.free();

    render_transfer_image.free();

    rt_pipeline.free();
    rt_pipeline_builder.free();
    p_pipeline.free();
    p_pipeline_builder.free();
    vkDestroySemaphore(logical_device, image_available_semaphore, nullptr);
    vkDestroySemaphore(logical_device, render_finished_semaphore, nullptr);
    vkDestroyFence(logical_device, in_flight_fence, nullptr);
    vkDestroyFence(logical_device, immediate_fence, nullptr);
    vkDestroyFence(logical_device, tlas_fence, nullptr);
    vkDestroyCommandPool(logical_device, command_pool, nullptr);
    for (auto framebuffer : framebuffers)
        vkDestroyFramebuffer(logical_device, framebuffer, nullptr);
    // vkDestroyPipelineLayout(logical_device, pipeline_layout, nullptr);
    vkDestroyRenderPass(logical_device, render_pass, nullptr);
    for (auto image_view : swap_chain_image_views)
        vkDestroyImageView(logical_device, image_view, nullptr);
    vkDestroySwapchainKHR(logical_device, swap_chain, nullptr);
    vkDestroySurfaceKHR(vulkan_instance, surface, nullptr);
    for (auto shared_memory = device.shared_buffer_memory.begin(); shared_memory != device.shared_buffer_memory.end(); shared_memory++) {
        vkFreeMemory(logical_device, (*shared_memory).second.memory, nullptr);
    }
    vkDestroyDevice(logical_device, nullptr);
    DestroyDebugUtilsMessengerEXT(vulkan_instance, debug_messenger, nullptr);
    vkDestroyInstance(vulkan_instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}

void VulkanApplication::rebuild_pipeline() {
    RaytracingPipeline new_pipeline = rt_pipeline_builder.build();
    RaytracingPipeline old_pipeline = rt_pipeline;
    rt_pipeline = new_pipeline;
    rt_pipeline.set_descriptor_acceleration_structure_binding(scene_tlas.acceleration_structure);
    rt_pipeline.set_descriptor_buffer_binding("mesh_indices", index_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("mesh_vertices", vertex_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("mesh_normals", normal_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("mesh_texcoords", texcoord_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("mesh_tangents", tangent_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("mesh_data_offsets", mesh_data_offset_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("mesh_offset_indices", mesh_offset_index_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_sampler_binding("textures", loaded_textures.data(), loaded_textures.size());
    rt_pipeline.set_descriptor_buffer_binding("texture_indices", texture_index_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("material_parameters", material_parameter_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("lights", lights_buffer, BufferType::Storage);
    rt_pipeline.set_descriptor_buffer_binding("camera_parameters", camera_buffer, BufferType::Uniform);
    set_render_images_dirty();
    vkDeviceWaitIdle(device.vulkan_device);
    old_pipeline.free();
}

void VulkanApplication::set_scene_path(std::string path) {
    scene_path = std::filesystem::path(path);
}

void VulkanApplication::set_render_images_dirty() {
    render_images_dirty = true;
}

void VulkanApplication::set_pipeline_dirty() {
    pipeline_dirty = true;
}

void VulkanApplication::save_screenshot(std::string path, ImagePixels& pixels) {
    loaders::save_exr_image(pixels, path);
}

double VulkanApplication::get_fps() {
    return 1.0 / frame_delta.count();
}

uint32_t VulkanApplication::get_samples() {
    return render_clear_accumulated;
}

vec2 VulkanApplication::get_cursor_position() {
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    return vec2(x, y);
}

RaytracingPipeline VulkanApplication::get_pipeline() {
    return rt_pipeline;
}

SceneData& VulkanApplication::get_scene_data() {
    return loaded_scene_data;
}

std::vector<Shaders::Light>& VulkanApplication::get_lights() {
    return lights;
}