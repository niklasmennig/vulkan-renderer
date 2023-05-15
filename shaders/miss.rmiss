#version 460 core
#extension GL_EXT_ray_tracing : enable

layout(push_constant) uniform PushConstants {
    float time;
    int clear_accumulated;
} push_constants;

~include "shaders/random.glsl"
~include "shaders/payload.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    payload.contribution = vec3(0.1);
}