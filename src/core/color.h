#pragma once

#include "glm/vec3.hpp"
using vec3 = glm::vec3;

namespace color {
    float luminance(vec3 color) {
        return (0.299*color.r + 0.587*color.g + 0.114*color.b);
    }
}
