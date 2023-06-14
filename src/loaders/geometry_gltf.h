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

    std::string texture_diffuse_path;

    LoadedMeshData get_loaded_mesh_data();
};

namespace loaders
{
    GLTFData load_gltf(const std::string path);
}
