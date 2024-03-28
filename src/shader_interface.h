#pragma once

#define SHADER_STRUCTS_ONLY

#include "glm/glm.hpp"

namespace Shaders
{
    using uint = uint32_t;
    using uvec2 = glm::uvec2;
    using vec2 = glm::vec2;
    using ivec2 = glm::ivec2;
    using vec3 = glm::vec3;
    using vec4 = glm::vec4;
    using mat4 = glm::mat4;
    
    #include "../shaders/structs.glsl"
    namespace Raytracing {
        #include "../shaders/raytracing/interface.glsl"
    }
    namespace Processing {
        #include "../shaders/processing/interface.glsl"
    }
}