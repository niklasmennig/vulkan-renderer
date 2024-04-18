//https://github.com/iRath96/raymond/tree/main/raymond/device/bsdf/lobes

#include "../../common.glsl"
#include "../../random.glsl"
#include "../sampling.glsl"
#include "../bsdf.glsl"

BSDFEvaluation eval_lobe_diffuse(vec3 ray_out, vec3 ray_in, Material material) {
    BSDFEvaluation result;
    result.color = material.base_color / PI;
    result.pdf = 1.0 / PI;

    return result;
}

BSDFSample sample_lobe_diffuse(vec3 ray_out, Material material, inout uint seed) {
    BSDFSample result;

    DirectionSample dir_sample = sample_cosine_hemisphere(random_float(seed), random_float(seed));

    result.weight = material.base_color;
    result.direction = dir_sample.direction;
    result.pdf = dir_sample.pdf;

    return result;
}