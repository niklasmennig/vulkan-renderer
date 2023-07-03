#include "geometry_gltf.h"

#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"


LoadedMeshData GLTFData::get_loaded_mesh_data() {
    LoadedMeshData result;
    result.vertices = this->vertices;
    result.normals = this->normals;
    result.texcoords = this->uvs;

    result.indices = this->indices;

    return result;
}

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
        for (const auto &primitive : mesh.primitives) {
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
                    result.indices.push_back(val);
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

                        result.vertices.push_back(vec4(x,y,z,1.0));
                    }
                } else if (attribute.first == "NORMAL") {
                    for (int i = 0; i < count; i++) {
                        float x = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 0);
                        float y = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 1);
                        float z = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 2);

                        result.normals.push_back(vec4(x, y, z, 0.0));
                    }
                } else if (attribute.first == "TEXCOORD_0") {
                    for (int i = 0; i < count; i++) {
                        float x = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 0);
                        float y = *(reinterpret_cast<const float*>(data_address + byte_stride * i) + 1);

                        result.uvs.push_back(vec2(x, y));
                    }
                }
            }


            // Material, Textures
            {
                const auto material = model.materials[primitive.material];
                const auto textures = model.textures;
                const auto images = model.images;
                result.texture_diffuse_path = images[textures[material.pbrMetallicRoughness.baseColorTexture.index].source].uri;
                result.texture_normal_path = images[textures[material.normalTexture.index].source].uri;
                result.texture_roughness_path = images[textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].source].uri;
                result.metallic_factor = material.pbrMetallicRoughness.metallicFactor;
            }

        }
    }

    return result;

}