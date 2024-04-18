#include "../bsdf/lambertian.glsl"
#include "../bsdf/spec.glsl"

BSDFEvaluation eval_bsdf(vec3 ray_out, vec3 ray_in, Material material) {
    // return eval_lambertian(ray_out, ray_in, material);
    return eval_spec(ray_out, ray_in, material);
}

BSDFSample sample_bsdf(vec3 ray_out, Material material, inout uint seed) {
    // return sample_lambertian(ray_out, material, seed);
    return sample_spec(ray_out, material, seed);
}