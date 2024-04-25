#pragma once

#include <string>
#include <vector>

#include "glm/glm.hpp"
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;

struct GLTFTexture {
    std::string path;
};

struct GLTFMaterial {
    int diffuse_texture = -1;
    int roughness_texture = -1;
    int normal_texture = -1;
    int emission_texture = -1;
    int transmission_texture = -1;

    vec4 diffuse_factor = vec4(1.0);
    float roughness_factor = 0;
    float metallic_factor = 0;
    vec3 emissive_factor = vec3(0);
    float transmission_factor = 0;
    float ior = 1.1;
};

struct GLTFPrimitive {
    std::vector<vec3> vertices;
    std::vector<vec3> normals;
    std::vector<vec2> uvs;
    std::vector<uint32_t> indices;
    std::vector<vec3> tangents;

    int material_index;
    uint32_t max_vertex;
};

struct GLTFMesh {
    std::vector<GLTFPrimitive> primitives;
};

struct GLTFNode {
    int parent_index = -1;
    int mesh_index = -1;
    mat4 matrix;
};

struct GLTFData {
    std::vector<GLTFNode> nodes;
    std::vector<GLTFMesh> meshes;
    std::vector<GLTFMaterial> materials;
    std::vector<GLTFTexture> textures; 
};

namespace loaders
{
    GLTFData load_gltf(const std::string path);
}
