#include "bsdf.glsl"
#include "lobes/diffuse.glsl"

BSDFEvaluation eval_lambertian(vec3 ray_out, vec3 ray_in, Material material) {
    return eval_lobe_diffuse(ray_out, ray_in, material);
}

BSDFSample sample_lambertian(vec3 ray_out, Material material, inout uint seed) {
    return sample_lobe_diffuse(ray_out, material, seed);
}