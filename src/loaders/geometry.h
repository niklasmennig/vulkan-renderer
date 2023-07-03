#pragma once

#include <vector>

#include "glm/vec4.hpp"
#include "glm/vec2.hpp"

struct LoadedMeshData
{
    std::vector<glm::vec4> vertices;
    std::vector<glm::vec4> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec4> tangents;

    std::vector<uint32_t> indices;

    void calculate_tangents();
};