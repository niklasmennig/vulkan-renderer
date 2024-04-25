#ifndef BSDF_LOBE_SPECULAR_GLSL
#define BSDF_LOBE_SPECULAR_GLSL

#include "../../common.glsl"
#include "../../random.glsl"
#include "../sampling.glsl"
#include "../bsdf.glsl"
#include "../fresnel.glsl"
#include "../ggx.glsl"

BSDFEvaluation eval_lobe_specular(vec3 ray_out, vec3 ray_in, Material material) {
    BSDFEvaluation result;
    float alpha = max(material.roughness * material.roughness, 0.001);

    bool is_transmission = ray_out.y * ray_in.y < 0;
    if (is_transmission) {
        result.color = vec3(0.0);
        result.pdf = 0.0;
        return result;
    }

    vec3 h = normalize(ray_out + ray_in);

    float pdf = pdf_ggx(ray_out, h, alpha);
    pdf *= det_reflection(ray_out, h);

    float g = g_smith(ray_in, h, alpha);

    result.color = material.base_color * g * pdf;
    result.pdf = pdf;

    return result;
}

BSDFSample sample_lobe_specular(vec3 ray_out, Material material, inout uint seed) {
    BSDFSample result;
    float alpha = max(material.roughness * material.roughness, 0.001);

    vec3 h = sample_ggx(ray_out, alpha, seed);

    vec3 ray_in = reflect(-ray_out, h);

    // no contribution for transmission
    if (ray_out.y * ray_in.y < 0.0) {
        result.weight = vec3(0.0);
        result.pdf = 0.0;
        return result;
    }

    float pdf = pdf_ggx(ray_out, h, alpha);
    pdf *= det_reflection(ray_out, h);

    float g = g_smith(ray_in, h, alpha);

    result.weight = material.base_color * g;
    result.direction = ray_in;
    result.pdf = pdf;

    return result;
}

#endif