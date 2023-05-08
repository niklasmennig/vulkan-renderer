#version 460 core
#extension GL_EXT_ray_tracing : enable

~include "shaders/payload.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    payload.hit_t = -1;
}