#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "payload.glsl"
#include "../push_constants.glsl"
#include "../common.glsl"
#include "../texture_data.glsl"
#include "../mis.glsl"
#include "../environment.glsl"
#include "../output.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    // calculate theta and phi for environment map
    vec2 thetaphi = thetaphi_from_dir(gl_WorldRayDirectionEXT);

    // uv coordinates from theta and phi
    float u = thetaphi.y / (2.0 * PI);
    float v = thetaphi.x / PI;

    // query environment map color
    vec3 env_contribution = sample_texture(TEXTURE_ID_ENVIRONMENT_ALBEDO, vec2(u, v)).rgb;

    if (payload.depth == 1) {
        write_output(OUTPUT_BUFFER_INSTANCE, payload.pixel_index, vec4(encode_uint(NULL_INSTANCE), 0));
        write_output(OUTPUT_BUFFER_ALBEDO, payload.pixel_index, vec4(env_contribution, 1.0));
        write_output(OUTPUT_BUFFER_NORMAL, payload.pixel_index, vec4(0.0));

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