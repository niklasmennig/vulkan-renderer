#version 460
#extension GL_EXT_ray_tracing : enable

#include "payload.glsl"
#include "../common.glsl"
#include "../texture_data.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(std430, push_constant) uniform PConstants {PushConstants constants;} push_constants;

void main() {
    // calculate theta and phi for environment map
    vec2 thetaphi = thetaphi_from_dir(gl_WorldRayDirectionEXT);

    // uv coordinates from theta and phi
    float u = thetaphi.y / (2.0 * PI);
    float v = thetaphi.x / PI;

    // query environment map color
    vec3 env_contribution = sample_texture(0, vec2(u, v)).rgb;

    if (payload.depth == 1) {
        payload.primary_hit_instance = NULL_INSTANCE;
        payload.primary_hit_albedo = env_contribution;
        payload.primary_hit_normal = vec3(0.0);
    }

    if ((push_constants.constants.flags & ENABLE_INDIRECT_LIGHTING) == ENABLE_INDIRECT_LIGHTING) {
        payload.color += payload.contribution * env_contribution;
    }

    payload.end_ray = true;
}