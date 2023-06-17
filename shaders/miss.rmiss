#version 460 core
#extension GL_EXT_ray_tracing : enable

layout(push_constant) uniform PushConstants {
    float time;
    int clear_accumulated;
} push_constants;

~include "shaders/random.glsl"
~include "shaders/payload.glsl"
~include "shaders/texture_data.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    vec3 dir = gl_WorldRayDirectionEXT;

    float theta = acos(dir.y);
    float phi = sign(dir.z) * acos(dir.x / sqrt(dir.x * dir.x + dir.z * dir.z));

    float u = phi / (2.0 * PI);
    float v = theta / PI;

    payload.color += payload.contribution * sample_texture(0, vec2(u, v));
}