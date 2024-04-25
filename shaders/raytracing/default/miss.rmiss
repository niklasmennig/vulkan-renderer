#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "payload.glsl"
#include "../../push_constants.glsl"
#include "../../common.glsl"
#include "../texture_data.glsl"
#include "../mis.glsl"
#include "../environment.glsl"
#include "../output.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    PushConstants constants = get_push_constants();

    // calculate theta and phi for environment map
    vec2 thetaphi = thetaphi_from_dir(gl_WorldRayDirectionEXT);

    // uv coordinates from theta and phi
    float u = thetaphi.y / (2.0 * PI);
    float v = thetaphi.x / PI;

    // query environment map color
    vec3 env_color = sample_texture(TEXTURE_ID_ENVIRONMENT_ALBEDO, vec2(u, v)).rgb;

    if (payload.depth == 1) {
        write_output(OUTPUT_BUFFER_INSTANCE, payload.pixel_index, vec4(encode_uint(NULL_INSTANCE), 0));
        write_output(OUTPUT_BUFFER_ALBEDO, payload.pixel_index, vec4(env_color, 1.0));
        write_output(OUTPUT_BUFFER_NORMAL, payload.pixel_index, vec4(0.0));
        write_output(OUTPUT_BUFFER_POSITION, payload.pixel_index, vec4(payload.origin, 1.0));

        write_output(OUTPUT_BUFFER_ENVIRONMENT_CONDITIONAL, payload.pixel_index, vec4(sample_texture(1, vec2(u,v)).r));
        write_output(OUTPUT_BUFFER_ENVIRONMENT_MARGINAL, payload.pixel_index, vec4(sample_texture(2, vec2(u,v)).r));

        write_output(OUTPUT_BUFFER_INSTANCE, payload.pixel_index, vec4(encode_uint(NULL_INSTANCE), 0.0));
        write_output(OUTPUT_BUFFER_INSTANCE_COLOR, payload.pixel_index, vec4(vec3(0.0), 1.0));
    
        payload.color = env_color;
    } else {
        if ((constants.flags & ENABLE_INDIRECT_LIGHTING) == ENABLE_INDIRECT_LIGHTING) {
            float mis = 1.0;
            if ((constants.flags & ENABLE_DIRECT_LIGHTING) == ENABLE_DIRECT_LIGHTING) {
                float env_pdf = pdf_environment(gl_WorldRayDirectionEXT, constants.environment_cdf_dimensions);
                mis = 1.0 / (1.0 + payload.last_bsdf_pdf_inv * env_pdf);
                write_output(OUTPUT_BUFFER_NORMAL, payload.pixel_index, vec4(mis));
            }
            payload.color += max(vec3(0.0), payload.contribution * env_color * mis);
        }
    }


    return;
}