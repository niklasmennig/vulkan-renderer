#ifndef PUSH_CONSTANTS_GLSL
#define PUSH_CONSTANTS_GLSL

#include "../structs.glsl"
layout(std430, push_constant) uniform PConstants {PushConstantsRT constants;} push_constants;

#endif