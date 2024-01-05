#include "geometry_gltf.h"

#include <iostream>

#include "glm/gtc/type_ptr.hpp"
#include "geometry.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"


GLTFData loaders::load_gltf(const std::string path) {
    std::cout << "Loading GLTF file at: " << path << std::endl;
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;
    bool loaded = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    
    if (!warn.empty()) {
        std::cout << "GLTF Warning: " << warn << std::endl;
    }

    if (!err.empty()) {
        std::cout << "GLTF Error: " << err << std::endl;
    }

    if (!loaded) {
        std::cout << "Failure to load GLTF at " << path << std::endl;
    }

    GLTFData result;

    for (const auto &mesh : model.meshes) {
        GLTFMesh result_mesh;
        for (const auto &primitive : mesh.primitives) {
            GLTFPrimitive result_primitive;
            // Indices
            {
                const auto &index_accessor = model.accessors[primitive.indices];
                const auto &buffer_view = model.bufferViews[index_accessor.bufferView];
                const auto &buffer = model.buffers[buffer_view.buffer];
                const auto data_address = buffer.data.data() + buffer_view.byteOffset + index_accessor.byteOffset;
                const auto byte_stride = index_accessor.ByteStride(buffer_view);
                const auto count = index_accessor.count;

                for (int i = 0; i < count; i++) {
                    uint16_t val = *(reinterpret_cast<const uint16_t*>(data_address + byte_stride * i));
                    result_primitive.indices.push_back(val);
                    if (val > result_primitive.max_vertex) result_primitive.max_vertex = val;
                }
            }

            // Vertices, Normals, UVs, Tangents
            for (const auto &attribute : primitive.attributes) {
                const auto attrib_accessor = model.accessors[attribute.second];
                const auto &buffer_view = model.bufferViews[attrib_accessor.bufferView];
                const auto &buffer = model.buffers[buffer_view.buffer];
                const auto data_address = buffer.data.data() + buffer_view.byteOffset + attrib_accessor.byteOffset;
                const auto byte_stride = attrib_accessor.ByteStride(buffer_view);
                const auto count = attrib_accessor.count;

                if (attribute.first == "POSITION") {
                    for (int i = 0; i < count; i++) {
                        float x = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 0);
                        float y = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 1);
                        float z = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 2);

                        result_primitive.vertices.push_back(vec3(x,y,z));
                    }
                } else if (attribute.first == "NORMAL") {
                    for (int i = 0; i < count; i++) {
                        float x = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 0);
                        float y = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 1);
                        float z = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 2);

                        result_primitive.normals.push_back(vec3(x, y, z));
                    }
                } else if (attribute.first == "TEXCOORD_0") {
                    for (int i = 0; i < count; i++) {
                        float x = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 0);
                        float y = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 1);

                        result_primitive.uvs.push_back(vec2(x, y));
                    }
                } else if (attribute.first == "TANGENT") {
                    for (int i = 0; i < count; i++) {
                        float x = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 0);
                        float y = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 1);
                        float z = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 2);

                        result_primitive.tangents.push_back(vec3(x, y, z));
                    }
                }
            }
            
            if (result_primitive.tangents.size() == 0) {
                TangentGenerator tangent_generator;
                tangent_generator.primitive = &result_primitive;

                tangent_generator.calculate_tangents();
            }

            result_primitive.material_index = primitive.material;

            result_mesh.primitives.push_back(result_primitive);
        }

        result.meshes.push_back(result_mesh);
    }

    // Textures
    for (const auto &texture : model.textures) {
        GLTFTexture result_texture;

        result_texture.path = model.images[texture.source].uri;

        result.textures.push_back(result_texture);
    }

    // Materials
    for (const auto &material : model.materials) {
        GLTFMaterial result_material;

        // transmissions
        if (material.extensions.find("KHR_materials_transmission") != material.extensions.end()) {
            if (material.extensions.at("KHR_materials_transmission").Has("transmissionTexture")) {
                result_material.transmission_texture = material.extensions.at("KHR_materials_transmission").Get("transmissionTexture").Get("index").GetNumberAsInt();
            }
            if (material.extensions.at("KHR_materials_transmission").Has("transmissionFactor")) {
                result_material.transmission_factor = material.extensions.at("KHR_materials_transmission").Get("transmissionFactor").GetNumberAsDouble();
            }
        }

        // ior
        if (material.extensions.find("KHR_materials_ior") != material.extensions.end()) {
            result_material.ior = material.extensions.at("KHR_materials_ior").Get("ior").GetNumberAsDouble();
        }

        const auto textures = model.textures;
        const auto images = model.images;
        result_material.diffuse_texture = material.pbrMetallicRoughness.baseColorTexture.index;
        result_material.normal_texture = material.normalTexture.index;
        result_material.roughness_texture = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
        result_material.emission_texture = material.emissiveTexture.index;

        result_material.diffuse_factor.r = material.pbrMetallicRoughness.baseColorFactor[0];
        result_material.diffuse_factor.g = material.pbrMetallicRoughness.baseColorFactor[1];
        result_material.diffuse_factor.b = material.pbrMetallicRoughness.baseColorFactor[2];
        result_material.diffuse_factor.a = material.pbrMetallicRoughness.baseColorFactor[3];

        result_material.roughness_factor = material.pbrMetallicRoughness.roughnessFactor;

        result_material.metallic_factor = material.pbrMetallicRoughness.metallicFactor;

        result_material.emissive_factor.r = material.emissiveFactor[0];
        result_material.emissive_factor.g = material.emissiveFactor[1];
        result_material.emissive_factor.b = material.emissiveFactor[2];

        result.materials.push_back(result_material);
    }

    // Nodes
    result.nodes.resize(model.nodes.size());
    for (int node_index = 0; node_index < model.nodes.size(); node_index++) {
        const auto& node = model.nodes[node_index];

        GLTFNode* result_node = &result.nodes[node_index];
        //if (node.mesh < 0) continue;

        result_node->mesh_index = node.mesh;

        glm::mat4 node_matrix = glm::mat4(1.0f);
        
        if (node.translation.size() == 3) node_matrix = glm::translate(node_matrix, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
        if (node.rotation.size() == 4) {
            auto quat = glm::quat(glm::vec4(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]));
            auto axis = glm::axis(quat);
            auto angle = glm::angle(quat);
            node_matrix = glm::rotate(node_matrix, angle, axis);
        }
        if (node.scale.size() == 3) node_matrix = glm::scale(node_matrix, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));

        if (node.matrix.size() > 0) node_matrix = glm::make_mat4(node.matrix.data());

        result_node->matrix = node_matrix;

        for (const int child_index : node.children) {
            result.nodes[child_index].parent_index = node_index;
        }
    }

    // apply transformation of parent nodes to child nodes
    for (int i = 0; i < result.nodes.size(); i++) {
        GLTFNode* node = &result.nodes[i];
        if (node->parent_index >= 0) {
            // step along parent tree to reconstruct full transformation matrix
            std::vector<mat4> transformations;
            GLTFNode* parent_node = node;
            while(parent_node->parent_index >= 0) {
                parent_node = &result.nodes[parent_node->parent_index];
                transformations.push_back(parent_node->matrix);
            }

            mat4 parent_transformation = mat4(1.0f);
            for (const auto t : transformations) {
                parent_transformation = t * parent_transformation;
            }

            node->matrix = parent_transformation * node->matrix;
        }
    }

    // delete parent nodes
    result.nodes.erase(std::remove_if(result.nodes.begin(), result.nodes.end(), [](GLTFNode node) {return node.mesh_index < 0;}), result.nodes.end());

    return result;

}