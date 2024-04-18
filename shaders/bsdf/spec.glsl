#include "bsdf.glsl"
#include "lobes/specular.glsl"

BSDFEvaluation eval_spec(vec3 ray_out, vec3 ray_in, Material material) {
    return eval_lobe_specular(ray_out, ray_in, material);
}

BSDFSample sample_spec(vec3 ray_out, Material material, inout uint seed) {
    return sample_lobe_specular(ray_out, material, seed);
}