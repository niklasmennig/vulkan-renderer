#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "common.glsl"
#include "random.glsl"
#include "payload.glsl"
#include "texture_data.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    payload.hit = false;
    payload.hit_instance = NULL_INSTANCE;
}