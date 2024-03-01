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
        // diffuse rgb, opacity a
        vec4 diffuse_opacity;
        // emissive rgb, emissive strength a
        vec4 emissive_factor;
        // roughness x, metallic y, transmissive z, ior a
        vec4 roughness_metallic_transmissive_ior;
    } material_parameters;

    std::string object_name;
    mat4 transformation;
};

struct LightData {
    enum LightType {
        POINT = 0,
        AREA = 1,
        DIRECTIONAL = 2,
    };

    std::string name;
    uint32_t type;
    uint32_t instance_index;
    vec3 position;
    vec3 direction;
    vec3 intensity;
};

struct SceneData
{
    std::string environment_path = "";
    // tuples containing object name and path
    std::vector<std::tuple<std::string, std::string>> object_paths;
    // instance data
    std::vector<InstanceData> instances;
    // light data
    std::vector<LightData> lights;
};

namespace loaders {
    SceneData load_scene_description(std::string path);
}