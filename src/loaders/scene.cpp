#include "scene.h"

#include "toml.hpp"
#include <iostream>
#include <unordered_map>

#include "glm/gtc/matrix_transform.hpp"

SceneData loaders::load_scene_description(std::string path) {
    toml::table scene_table = toml::parse_file(path);

    // objects
    std::vector<std::tuple<std::string, std::string>> object_paths;
    auto meshes = scene_table["objects"].as_table();
    for (auto m = meshes->begin(); m != meshes->end(); m++)
    {
        std::string name = m->first.str().data();
        auto mesh_data = scene_table["objects"][name];
        std::string path = mesh_data["path"].as_string()->get();
        object_paths.push_back(std::make_tuple(name, path));
    }

    // instances
    std::vector<InstanceData> instances;
    auto instances_table = scene_table["instances"].as_table();
    for (auto i = instances_table->begin(); i != instances_table->end(); i++) {
        std::string name = i->first.str().data();
        auto data = scene_table["instances"][name];
        std::string object_name = data["object"].as_string()->get();
        InstanceData instance_data;
        instance_data.object_name = object_name;


        auto data_table = data.as_table();
        // if(data_table->contains("texture")) {
        //     instance_data.texture_name = data["texture"].as_string()->get();
        // }
        // if (data_table->contains("material")) {
        //     instance_data.material_name = data["material"].as_string()->get();
        // }

        mat4 transformation = glm::mat4(1.0);
        if(data_table->contains("position")) {
            auto position_data = data["position"].as_array();
            float x = data["position"][0].as_floating_point()->get();
            float y = data["position"][1].as_floating_point()->get();
            float z = data["position"][2].as_floating_point()->get();
            transformation = glm::translate(transformation, vec3(x,y,z));
        }
        if (data_table->contains("rotation")) {
            auto rotation_data = data["rotation"].as_array();
            float x = data["rotation"][0].as_floating_point()->get();
            float y = data["rotation"][1].as_floating_point()->get();
            float z = data["rotation"][2].as_floating_point()->get();
            transformation = glm::rotate(transformation, glm::radians(x), vec3(1, 0, 0));
            transformation = glm::rotate(transformation, glm::radians(y), vec3(0, 1, 0));
            transformation = glm::rotate(transformation, glm::radians(z), vec3(0, 0, 1));
        }
        if (data_table->contains("scale")) {
            auto scale_data = data["scale"].as_array();
            float x = data["scale"][0].as_floating_point()->get();
            float y = data["scale"][1].as_floating_point()->get();
            float z = data["scale"][2].as_floating_point()->get();
            transformation = glm::scale(transformation, vec3(x,y,z));
        }
        instance_data.transformation = transformation;

        instances.push_back(instance_data);
    }

    SceneData result;
    result.object_paths = object_paths;
    result.instances = instances;
    return result;
}