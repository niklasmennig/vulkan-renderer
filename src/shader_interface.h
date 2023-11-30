#pragma once

#include "glm/glm.hpp"

namespace Shaders
{
    using uint = uint32_t;
    using uvec2 = glm::uvec2;
    using vec2 = glm::vec2;
    using vec4 = glm::vec4;
    #include "../shaders/raytracing/interface.glsl"
}