#ifndef ENVIRONMENT_GLSL
#define ENVIRONMENT_GLSL

#include "lights.glsl"
#include "texture_data.glsl"

#define TEXTURE_ID_ENVIRONMENT_ALBEDO 0
#define TEXTURE_ID_ENVIRONMENT_CDF 1
#define TEXTURE_ID_ENVIRONMENT_CONDITIONAL 2

LightSample sample_environment(vec4 random_values, uvec2 map_dimensions) {
    uint width = map_dimensions.x;
    uint height = map_dimensions.y;

    float x_step = 1.0 / width;
    float y_step = 1.0 / height;

    float conditional_target = random_values.y;
    float conditional_y = 0;
    float conditional_low = sample_texture(TEXTURE_ID_ENVIRONMENT_CONDITIONAL, vec2(0, conditional_y)).r;
    float conditional_hi = sample_texture(TEXTURE_ID_ENVIRONMENT_CONDITIONAL, vec2(0, conditional_y + y_step)).r;

    while (!(conditional_target < conditional_hi && conditional_target >= conditional_low)) {
        conditional_low = conditional_hi;
        conditional_y += y_step;
        conditional_hi = sample_texture(TEXTURE_ID_ENVIRONMENT_CONDITIONAL, vec2(0, conditional_y)).r;

        if (conditional_y + y_step >= 1) {
            conditional_y = 1;
            break;
        }
    }

    float cdf_target = random_values.x;
    float cdf_x = 0;
    float cdf_low = sample_texture(TEXTURE_ID_ENVIRONMENT_CDF, vec2(cdf_x, conditional_y)).r;
    float cdf_hi = sample_texture(TEXTURE_ID_ENVIRONMENT_CDF, vec2(cdf_x + x_step, conditional_y)).r;

    while (!(cdf_target <= cdf_hi && cdf_target > cdf_low)) {
        cdf_low = cdf_hi;
        cdf_x += x_step;
        cdf_hi = sample_texture(TEXTURE_ID_ENVIRONMENT_CDF, vec2(cdf_x, conditional_y)).r;

        if (cdf_x + x_step >= 1) {
            cdf_x = 1;
            break;
        }
    }

    float sample_pdf_cdf = cdf_hi - cdf_low;
    float sample_pdf_conditional = conditional_hi - conditional_low;

    // sample pixel offset for environment map intensity sample
    vec2 offset = random_values.zw * vec2(x_step, y_step);
    vec2 sample_uv = vec2(cdf_x, conditional_y) + offset;

    float theta = sample_uv.y * PI;
    float phi = sample_uv.x * 2.0 * PI;

    float sample_pdf = sample_pdf_cdf * sample_pdf_conditional / (4.0 * PI) * sin(theta);
    
    LightSample result;
    result.pdf = sample_pdf;
    result.intensity = sample_texture(TEXTURE_ID_ENVIRONMENT_ALBEDO, sample_uv).rgb / sample_pdf;
    result.distance = FLT_MAX;
    result.direction = dir_from_thetaphi(theta, phi);
    return result;
}

#endif