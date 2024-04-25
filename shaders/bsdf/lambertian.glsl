#include "bsdf.glsl"
#include "lobes/diffuse.glsl"

BSDFEvaluation eval_lambertian(vec3 ray_out, vec3 ray_in, Material material) {
    BSDFEvaluation eval = eval_lobe_diffuse(ray_out, ray_in, material);

    eval.color *= material.opacity;

    return eval;
}

BSDFSample sample_lambertian(vec3 ray_out, Material material, inout uint seed) {
    BSDFSample bsdf_sample;

    if (random_float(seed) >= material.opacity) {
        bsdf_sample.weight = vec3(1.0);
        bsdf_sample.direction = -ray_out;
        bsdf_sample.pdf = 1.0;
        return bsdf_sample;
    }

    bsdf_sample = sample_lobe_diffuse(ray_out, material, seed);
    return bsdf_sample;
}