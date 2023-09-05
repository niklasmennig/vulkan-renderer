#version 460 core
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : require

layout(push_constant) uniform PushConstants {
    float time;
    int clear_accumulated;
} push_constants;

#include "common.glsl"
#include "random.glsl"
#include "payload.glsl"
#include "texture_data.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 1) rayPayloadInEXT ShadowRayPayload shadow_payload;

void main() {
    if (shadow_payload.query_shadow) {
        shadow_payload.occluded = false;
        shadow_payload.query_shadow = false;
        return;
    }

    vec3 dir = gl_WorldRayDirectionEXT;

    float theta = acos(dir.y);
    float phi = sign(dir.z) * acos(dir.x / sqrt(dir.x * dir.x + dir.z * dir.z));

    float u = phi / (2.0 * PI);
    float v = theta / PI;

    vec3 base_color = sample_texture(0, vec2(u, v)).rgb;
    //base_color = vec3(0,0,0);

    // payload.color += base_color * payload.contribution;

    if (payload.depth == 1) {
        payload.primary_hit_instance = NULL_INSTANCE;
        payload.primary_hit_albedo = base_color;
        payload.primary_hit_normal = vec3(0);
        payload.primary_hit_roughness = vec3(0);
    }
}