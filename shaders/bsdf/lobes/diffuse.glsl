#ifndef BSDF_LOBE_DIFFUSE_GLSL
#define BSDF_LOBE_DIFFUSE_GLSL

#include "../../common.glsl"
#include "../../random.glsl"
#include "../sampling.glsl"
#include "../bsdf.glsl"

BSDFEvaluation eval_lobe_diffuse(vec3 ray_out, vec3 ray_in, Material material) {
    BSDFEvaluation result;

    if (ray_out.y * ray_in.y < 0.0) {
        result.color = vec3(0.0);
        result.pdf = 0.0;
        return result;
    }

    float cos_theta = abs(ray_in.y);
 
    result.color = material.base_color * cos_theta / PI;
    result.pdf = cos_theta / PI;

    return result;
}

BSDFSample sample_lobe_diffuse(vec3 ray_out, Material material, inout uint seed) {
    BSDFSample result;

    DirectionSample dir_sample = sample_cosine_hemisphere(random_float(seed), random_float(seed));

    result.weight = material.base_color;
    result.direction = dir_sample.direction;
    result.pdf = abs(result.direction.y) / PI;

    return result;
}

#endif