#version 460
#extension GL_EXT_ray_tracing : enable

#include "payload.glsl"
#include "../common.glsl"
#include "../texture_data.glsl"
#include "../mis.glsl"
#include "../environment.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(std430, push_constant) uniform PConstants {PushConstants constants;} push_constants;

void main() {
    // calculate theta and phi for environment map
    vec2 thetaphi = thetaphi_from_dir(gl_WorldRayDirectionEXT);

    // uv coordinates from theta and phi
    float u = thetaphi.y / (2.0 * PI);
    float v = thetaphi.x / PI;

    // query environment map color
    vec3 env_contribution = sample_texture(TEXTURE_ID_ENVIRONMENT_ALBEDO, vec2(u, v)).rgb;

    if (payload.depth == 1) {
        payload.primary_hit_instance = NULL_INSTANCE;
        payload.primary_hit_albedo = env_contribution;
        payload.primary_hit_normal = vec3(0.0);

        payload.environment_cdf = sample_texture(1, vec2(u,v)).r;
        payload.environment_conditional = sample_texture(2, vec2(u,v)).r;
    }

    if ((push_constants.constants.flags & ENABLE_INDIRECT_LIGHTING) == ENABLE_INDIRECT_LIGHTING) {
        // MIS!!!
        float mis = 1.0;
        if ((push_constants.constants.flags & ENABLE_DIRECT_LIGHTING) == ENABLE_DIRECT_LIGHTING) {
            float env_pdf = pdf_environment(gl_WorldRayDirectionEXT, push_constants.constants.environment_cdf_dimensions);
            mis = balance_heuristic(1.0, 1.0, 1.0, env_pdf * payload.last_bsdf_pdf_inv);
        }
        payload.color += payload.contribution * env_contribution * mis;
    }

    return;
}