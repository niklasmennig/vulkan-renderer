#pragma once

#include <vector>
#include <string>
#include "glm/vec3.hpp"
using vec3 = glm::vec3;
#include "glm/vec4.hpp"
using vec4 = glm::vec4;
#include "glm/mat4x4.hpp"
using mat4 = glm::mat4;

struct InstanceData
{
    struct TextureIndices {
        uint32_t diffuse, normal, roughness, emissive, transmissive;
    } texture_indices;

    struct MaterialParameters {
        // diffuse rgba
        vec4 diffuse_factor;
        // emissive rgb, metallic a
        vec4 emissive_metallic_factor;
        // transmissive
        vec4 transmissive;
    } material_parameters;

    std::string object_name;
    mat4 transformation;
};

struct SceneData
{
    std::string environment_path;
    // tuples containing object name and path
    std::vector<std::tuple<std::string, std::string>> object_paths;
    // instance data
    std::vector<InstanceData> instances;
};

namespace loaders {
    SceneData load_scene_description(std::string path);
}