#include "geometry_gltf.h"

#include <iostream>

#include "glm/gtc/type_ptr.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"


GLTFData loaders::load_gltf(const std::string path) {
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
                }
            }

            // Vertices, Normals, UVs
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

                        result_primitive.vertices.push_back(vec4(x,y,z,1.0));
                    }
                } else if (attribute.first == "NORMAL") {
                    for (int i = 0; i < count; i++) {
                        float x = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 0);
                        float y = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 1);
                        float z = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 2);

                        result_primitive.normals.push_back(vec4(x, y, z, 0.0));
                    }
                } else if (attribute.first == "TEXCOORD_0") {
                    for (int i = 0; i < count; i++) {
                        float x = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 0);
                        float y = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 1);

                        result_primitive.uvs.push_back(vec2(x, y));
                    }
                }
            }
            
            // only support meshes with one primitive
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

        result_material.metallic_factor = material.pbrMetallicRoughness.metallicFactor;

        result_material.emissive_factor.r = material.emissiveFactor[0];
        result_material.emissive_factor.g = material.emissiveFactor[1];
        result_material.emissive_factor.b = material.emissiveFactor[2];

        result.materials.push_back(result_material);
    }

    // Nodes
    for (const auto &node : model.nodes) {
        GLTFNode result_node;
        if (node.mesh < 0) continue;

        result_node.mesh_index = node.mesh;
        if (node.matrix.size() > 0) result_node.matrix = glm::make_mat4(node.matrix.data());

        result.nodes.push_back(result_node);
    }

    return result;

}