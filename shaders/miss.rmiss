#version 460 core
#extension GL_EXT_ray_tracing : enable

~include "shaders/payload.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    payload.direct_light = vec3(0,0,0);
    payload.shadow_miss = true;
    payload.next_origin = vec3(0,0,0);
    payload.next_direction = vec3(0,0,0);
}