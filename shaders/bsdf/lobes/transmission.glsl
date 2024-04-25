#ifndef BSDF_LOBE_TRANSMISSION_GLSL
#define BSDF_LOBE_TRANSMISSION_GLSL

#include "../../common.glsl"
#include "../../random.glsl"
#include "../sampling.glsl"
#include "../bsdf.glsl"
#include "../fresnel.glsl"
#include "../ggx.glsl"

BSDFEvaluation eval_lobe_transmission(vec3 ray_out, vec3 ray_in, Material material) {
    BSDFEvaluation result;
    float alpha = max(material.roughness * material.roughness, 0.001);

    bool is_transmission = ray_out.y * ray_in.y < 0;
    if (!is_transmission) {
        result.color = vec3(0.0);
        result.pdf = 0.0;
        return result; 
    }


    float eta = material.ior;
    if (ray_out.y < 0.0) eta = 1.0 / eta;

    // transmissive half vector
    vec3 h = normalize(ray_in * eta + ray_out);

    float pdf = pdf_ggx(ray_out, h, alpha);

    float g = g_smith(ray_in, h, alpha);
    FresnelTerm f = fresnel(eta, dot(h, ray_out));

    pdf *= (1.0 - f.factor) * det_refraction(ray_in, ray_out, h, eta);

    result.color = material.base_color * g * pdf;
    result.pdf = pdf;

    return result;
}

BSDFSample sample_lobe_transmission(vec3 ray_out, Material material, inout uint seed) {
    BSDFSample result;
    float alpha = max(material.roughness * material.roughness, 0.001);

    float eta = material.ior;
    // if (ray_out.y < 0.0) eta = 1.0 / eta;

    vec3 h = sample_ggx(ray_out, alpha, seed);

    float pdf = pdf_ggx(ray_out, h, alpha);

    FresnelTerm f = fresnel(eta, ray_out.y);

    vec3 ray_in = refract(-ray_out, h, 1.0 / eta);
    if (ray_in.y * ray_out.y > 0) {
        result.weight = vec3(0.0);
        result.pdf = 0.0;
    }

    pdf *= (1.0 - f.factor) * det_refraction(ray_in, ray_out, h, eta);

    float g = g_smith(ray_in, h, alpha);

    result.weight = material.base_color * g;
    result.direction = ray_in;
    result.pdf = pdf;

    return result;
}

#endif