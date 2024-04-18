#include "../common.glsl"
#include "../random.glsl"
#include "sampling.glsl"
#include "material.glsl"

#include "lambert.glsl"
#include "principled.glsl"

BSDFSample sample_bsdf(vec3 ray_out, Material material, uint seed) {
    // return sample_lambert(ray_out, material, seed);
    return sample_principled(ray_out, material, seed);
}

vec3 eval_bsdf(vec3 ray_dir, vec3 light_dir, Material material) {
    // return eval_lambert(ray_dir, light_dir, material);
    return eval_principled(ray_dir, light_dir, material);
}

float pdf_bsdf(vec3 ray_dir, vec3 light_dir, Material material) {
    // return pdf_lambert(ray_dir, light_dir, material);
    return pdf_principled(ray_dir, light_dir, material);
}