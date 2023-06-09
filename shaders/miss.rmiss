#version 460 core
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : require

layout(push_constant) uniform PushConstants {
    float time;
    int clear_accumulated;
} push_constants;

~include "shaders/common.glsl"
~include "shaders/random.glsl"
~include "shaders/payload.glsl"
~include "shaders/texture_data.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    if (payload.depth == 0) payload.primary_hit_instance = NULL_INSTANCE;

    vec3 dir = gl_WorldRayDirectionEXT;

    float theta = acos(dir.y);
    float phi = sign(dir.z) * acos(dir.x / sqrt(dir.x * dir.x + dir.z * dir.z));

    float u = phi / (2.0 * PI);
    float v = theta / PI;

    payload.color += payload.contribution * sample_texture(0, vec2(u, v));
}