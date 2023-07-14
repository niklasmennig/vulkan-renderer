#pragma once

#include <string>
#include <vector>

#include "geometry.h"

#include "glm/glm.hpp"
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

struct GLTFData {
    std::vector<vec4> vertices;
    std::vector<vec4> normals;
    std::vector<vec2> uvs;
    std::vector<uint32_t> indices;

    std::string texture_diffuse_path = "";
    std::string texture_normal_path = "";
    std::string texture_roughness_path = "";
    std::string texture_emissive_path = "";

    vec4 diffuse_factor = vec4(0);
    float metallic_factor = 0;
    vec3 emissive_factor = vec3(0);

    LoadedMeshData get_loaded_mesh_data();
};

namespace loaders
{
    GLTFData load_gltf(const std::string path);
}
