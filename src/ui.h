#pragma once

#include "glm/vec3.hpp"
using vec3 = glm::vec3;
using ivec3 = glm::ivec3;

struct VulkanApplication;

struct UI {
private:
    VulkanApplication* application;

public:
    int displayed_image_index = 0;
    ivec3 color_under_cursor = ivec3(0);

    void init(VulkanApplication* application);
    void draw();
};