#pragma once

#include <vector>
#include <string>

#include "glm/vec4.hpp"
#include "glm/vec2.hpp"

#include "geometry.h"

namespace loaders {
    LoadedMeshData load_obj(const std::string path); 
}