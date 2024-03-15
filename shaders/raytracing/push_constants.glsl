#ifndef PUSH_CONSTANTS_GLSL
#define PUSH_CONSTANTS_GLSL

#include "interface.glsl"
layout(std430, push_constant) uniform PConstants {PushConstants constants;} push_constants;

#endif