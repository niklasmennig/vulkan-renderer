#pragma once

#include <vector>

#include "glm/vec4.hpp"
#include "glm/vec2.hpp"

#include "geometry_gltf.h"

struct TangentGenerator
{
    GLTFPrimitive* primitive;

    void calculate_tangents();
};