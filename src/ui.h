#pragma once

#include "glm/vec3.hpp"
using vec3 = glm::vec3;
using ivec3 = glm::ivec3;

struct VulkanApplication;

#include "loaders/scene.h"

struct UI {
private:
    bool changed = false;
    bool hovered = false;
    bool render_scale_changed = false;
    VulkanApplication* application;

    std::vector<std::string> output_image_names;

public:
    int selected_instance = -1;
    InstanceData::MaterialParameters* selected_instance_parameters = nullptr;

    std::string selected_output_image;

    float render_scale = 1.0f;
    vec3 color_under_cursor = vec3(0);
    int max_ray_depth = 5;
    int frame_samples = 1;
    bool direct_lighting_enabled;
    bool indirect_lighting_enabled;

    bool use_processing_pipeline = false;

    float exposure = 0.0f;
    float camera_speed = 1.0f;
    float camera_fov = 70.0f;

    void init(VulkanApplication* application);
    void draw();
    bool has_changed();
    bool has_render_scale_changed();
    bool is_hovered();
};