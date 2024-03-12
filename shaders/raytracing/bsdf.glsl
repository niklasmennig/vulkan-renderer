#include "common.glsl"
#include "random.glsl"
#include "sampling.glsl"
#include "material.glsl"

#include "lambert.glsl"
#include "principled.glsl"

BSDFSample sample_bsdf(vec3 ray_out, Material material, inout uint seed) {
    return sample_lambert(ray_out, material, seed);
    // return sample_principled(ray_out, material, seed);
}

vec3 eval_bsdf(vec3 ray_in, vec3 ray_out, Material material) {
    return eval_lambert(ray_in, ray_out, material);
    // return eval_principled(ray_in, ray_out, material);
}

float pdf_bsdf(vec3 ray_in, vec3 ray_out, Material material) {
    return pdf_lambert(ray_in, ray_out, material);
    // return pdf_principled(ray_in, ray_out, material);
}