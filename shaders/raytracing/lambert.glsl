#include "../common.glsl"
#include "random.glsl"
#include "sampling.glsl"
#include "material.glsl"

BSDFSample sample_lambert(vec3 ray_out, Material material, uint seed) {
        DirectionSample dir_sample = sample_uniform_hemisphere(random_float(seed), random_float(seed));

        BSDFSample result;
        result.weight = material.base_color;
        result.direction = dir_sample.direction;
        result.pdf = dir_sample.pdf;

        return result;
}

vec3 eval_lambert(vec3 ray_in, vec3 ray_out, Material material) {
        return material.base_color * material.opacity / PI;
}

float pdf_lambert(vec3 ray_in, vec3 ray_out, Material material) {
        return 1.0 / PI;
}