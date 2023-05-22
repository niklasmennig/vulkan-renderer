#pragma once

#include <vector>
#include <string>
#include "glm/vec3.hpp"
using vec3 = glm::vec3;
#include "glm/mat4x4.hpp"
using mat4 = glm::mat4;

struct InstanceData
{
    std::string mesh_name;
    std::string texture_name;
    mat4 transformation;
};

struct SceneData
{
    // tuples containing mesh name and path
    std::vector<std::tuple<std::string, std::string>> mesh_paths;
    // tuples containing texture name and path
    std::vector<std::tuple<std::string, std::string>> texture_paths;
    // instance data
    std::vector<InstanceData> instances;
};

namespace loaders {
    SceneData load_scene_description(std::string path);
}