#include "bsdf.glsl"
#include "lobes/diffuse.glsl"
#include "lobes/specular.glsl"
#include "lobes/transmission.glsl"

void principled_lobe_weights(Material material, out float diffuse, out float specular, out float transmissive) {
    diffuse = (1.0 - material.metallic) * (1.0 - material.transmission);
    transmissive = material.transmission * (1.0 - material.metallic);
    specular = material.metallic * (1.0 - material.transmission);

    float sum = diffuse + specular + transmissive;

    diffuse /= sum;
    specular /= sum;
    transmissive /= sum;
}

BSDFEvaluation eval_principled(vec3 ray_out, vec3 ray_in, Material material) {
    BSDFEvaluation diff = eval_lobe_diffuse(ray_out, ray_in, material);
    BSDFEvaluation spec = eval_lobe_specular(ray_out, ray_in, material);
    BSDFEvaluation trans = eval_lobe_transmission(ray_out, ray_in, material);

    float diff_weight, spec_weight, trans_weight;
    principled_lobe_weights(material, diff_weight, spec_weight, trans_weight);

    BSDFEvaluation result;
    result.color = (diff.color * diff_weight + spec.color * spec_weight + trans.color * trans_weight) * material.opacity;
    result.pdf = diff.pdf * diff_weight + spec.pdf * spec_weight + trans.pdf * trans_weight;

    return result;
}

BSDFSample sample_principled(vec3 ray_out, Material material, inout uint seed) {

    BSDFSample bsdf_sample;
    if (random_float(seed) >= material.opacity) {
        bsdf_sample.weight = vec3(1.0);
        bsdf_sample.direction = -ray_out;
        bsdf_sample.pdf = 1.0;
        return bsdf_sample;
    }

    float diff_weight, spec_weight, trans_weight;
    principled_lobe_weights(material, diff_weight, spec_weight, trans_weight);

    float lobe_sel = random_float(seed);
    float lobe_prob = 0.0;

    if (lobe_sel < diff_weight) {
        bsdf_sample = sample_lobe_diffuse(ray_out, material, seed);
        lobe_prob = diff_weight;
    } else if (lobe_sel < diff_weight + spec_weight) {
        bsdf_sample = sample_lobe_specular(ray_out, material, seed);
        lobe_prob = spec_weight;
    } else {
        bsdf_sample = sample_lobe_transmission(ray_out, material, seed);
        lobe_prob = trans_weight;
    }

    if (lobe_prob < 1.0) {
        BSDFEvaluation eval = eval_principled(ray_out, bsdf_sample.direction, material);
        // bsdf_sample.weight = eval.color;
    } else {
        bsdf_sample.pdf *= material.opacity;
    }

    return bsdf_sample;
}