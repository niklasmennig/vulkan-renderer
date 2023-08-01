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
    VulkanApplication* application;

public:
    int selected_instance = -1;
    InstanceData::MaterialParameters* selected_instance_parameters = nullptr;

    int displayed_image_index = 0;
    int render_scale_index = 0;
    ivec3 color_under_cursor = ivec3(0);

    float camera_speed = 1.0f;
    float camera_fov = 70.0f;

    void init(VulkanApplication* application);
    void draw();
    bool has_changed();
    bool is_hovered();
};