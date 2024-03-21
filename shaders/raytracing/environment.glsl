#ifndef ENVIRONMENT_GLSL
#define ENVIRONMENT_GLSL

#include "texture_data.glsl"
#include "structs.glsl"
#include "random.glsl"

#define TEXTURE_ID_ENVIRONMENT_ALBEDO 0
#define TEXTURE_ID_ENVIRONMENT_MARGINAL 1
#define TEXTURE_ID_ENVIRONMENT_CONDITIONAL 2


float pdf_environment(vec3 direction, uvec2 map_dimensions) {
    vec2 thetaphi = thetaphi_from_dir(direction);

    // uv coordinates from theta and phi
    float u = mod(thetaphi.y / (2.0 * PI) + 1.0, 1.0);
    float v = thetaphi.x / PI;

    uint pixel_x = uint(floor(u * map_dimensions.x));
    uint pixel_y = uint(floor(v * map_dimensions.y));

    float conditional = fetch_texture(TEXTURE_ID_ENVIRONMENT_CONDITIONAL, ivec2(0, pixel_y)).r;
    float conditional_lo = fetch_texture(TEXTURE_ID_ENVIRONMENT_CONDITIONAL, ivec2(0, pixel_y - 1)).r;
    if (pixel_y == 0) conditional_lo = 0.0;

    float cdf = fetch_texture(TEXTURE_ID_ENVIRONMENT_MARGINAL, ivec2(pixel_x, pixel_y)).r;
    float cdf_lo = fetch_texture(TEXTURE_ID_ENVIRONMENT_MARGINAL, ivec2(pixel_x - 1, pixel_y)).r;
    if (pixel_x % map_dimensions.x == 0) cdf_lo = 0.0;

    return (cdf - cdf_lo) * (conditional - conditional_lo) * (abs(sin(thetaphi.x)) * 2.0 * PI * PI) * (map_dimensions.x * map_dimensions.y);
}

LightSample sample_environment(uint seed, uvec2 map_dimensions) {
    int width = int(map_dimensions.x);
    int height = int(map_dimensions.y);

    float conditional_target = random_float(seed);
    int conditional_y = 0;
    float conditional_low = fetch_texture(TEXTURE_ID_ENVIRONMENT_CONDITIONAL, ivec2(0, conditional_y)).r;
    float conditional_hi = fetch_texture(TEXTURE_ID_ENVIRONMENT_CONDITIONAL, ivec2(0, conditional_y + 1)).r;

    // TODO: conditional_low != conditional_hi if no gradient in cdf
    while (!(conditional_target < conditional_hi && conditional_target >= conditional_low)) {
        conditional_low = conditional_hi;
        conditional_y += 1;
        conditional_hi = fetch_texture(TEXTURE_ID_ENVIRONMENT_CONDITIONAL, ivec2(0, conditional_y)).r;

        if (conditional_y + 1 >= height) {
            conditional_y = height - 1;
            break;
        }
    }

    float cdf_target = random_float(seed);
    int cdf_x = 0;
    float cdf_low = fetch_texture(TEXTURE_ID_ENVIRONMENT_MARGINAL, ivec2(cdf_x, conditional_y)).r;
    float cdf_hi = fetch_texture(TEXTURE_ID_ENVIRONMENT_MARGINAL, ivec2(cdf_x + 1, conditional_y)).r;

    while (!(cdf_target <= cdf_hi && cdf_target > cdf_low)) {
        cdf_low = cdf_hi;
        cdf_x += 1;
        cdf_hi = fetch_texture(TEXTURE_ID_ENVIRONMENT_MARGINAL, ivec2(cdf_x, conditional_y)).r;

        if (cdf_x + 1 >= width) {
            cdf_x = width - 1;
            break;
        }
    }

    float u = ((float(cdf_x) + random_float(seed)) / width);
    float v = ((float(conditional_y) + random_float(seed)) / height);

    float theta = v * PI;
    float phi = u * 2.0 * PI;

    vec3 direction = dir_from_thetaphi(theta, phi);

    float pdf = (cdf_hi - cdf_low) * (conditional_hi - conditional_low) / (abs(sin(theta)) * 2.0 * PI * PI) * (width * height);
    if (width == 1 && height == 1) {
        pdf = 1.0 / (abs(sin(theta)) * 2.0 * PI * PI);
    }
    
    LightSample result;
    result.pdf = pdf;
    result.weight = sample_texture(TEXTURE_ID_ENVIRONMENT_ALBEDO, vec2(u, v)).rgb / pdf;
    result.distance = FLT_MAX;
    result.direction = direction;
    return result;
}


#endif