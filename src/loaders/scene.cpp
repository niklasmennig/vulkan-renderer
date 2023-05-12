#include "scene.h"

#include "toml.hpp"
#include <iostream>
#include <unordered_map>

#include <glm/gtc/matrix_transform.hpp>

SceneData loaders::load_scene_description(std::string path) {
    toml::table scene_table;
    try
    {
        scene_table = toml::parse_file(path);
    }
    catch (const toml::parse_error &err)
    {
        std::stringstream msg;
        msg << "error parsing scene file: " << err.source().begin << " " << err.what();
        throw std::runtime_error(msg.str());
    }

    // meshes
    std::vector<std::tuple<std::string, std::string>> mesh_paths;
    auto meshes = scene_table["meshes"].as_table();
    for (auto m = meshes->begin(); m != meshes->end(); m++)
    {
        std::string name = m->first.str().data();
        auto mesh_data = scene_table["meshes"][name];
        std::string path = mesh_data["path"].as_string()->get();
        mesh_paths.push_back(std::make_tuple(name, path));
    }

    // textures
    std::vector<std::tuple<std::string, std::string>> texture_paths;
    auto textures = scene_table["textures"].as_table();
    for (auto m = textures->begin(); m != textures->end(); m++)
    {
        std::string name = m->first.str().data();
        auto texture_data = scene_table["textures"][name];
        std::string path = texture_data["path"].as_string()->get();
        texture_paths.push_back(std::make_tuple(name, path));
    }

    // instances
    std::vector<InstanceData> instances;
    auto instances_table = scene_table["instances"].as_table();
    for (auto i = instances_table->begin(); i != instances_table->end(); i++) {
        std::string name = i->first.str().data();
        auto data = scene_table["instances"][name];
        std::string mesh_name = data["mesh"].as_string()->get();
        InstanceData instance_data;
        instance_data.mesh_name = mesh_name;


        mat4 transformation = glm::mat4(1.0);
        auto data_table = data.as_table();
        if(data_table->contains("texture")) {
            instance_data.texture_name = data["texture"].as_string()->get();
        }
        if(data_table->contains("position")) {
            auto position_data = data["position"].as_array();
            float x = data["position"][0].as_floating_point()->get();
            float y = data["position"][1].as_floating_point()->get();
            float z = data["position"][2].as_floating_point()->get();
            transformation = glm::translate(transformation, vec3(x,y,z));
        }
        instance_data.transformation = transformation;

        instances.push_back(instance_data);
    }

    SceneData result;
    result.mesh_paths = mesh_paths;
    result.texture_paths = texture_paths;
    result.instances = instances;
    return result;
}