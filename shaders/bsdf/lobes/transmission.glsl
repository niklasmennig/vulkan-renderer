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

    bool is_transmission = ray_out.y * ray_in.y < 0;
    float eta = 1.0 / material.ior;
    if (ray_out.y < 0.0) eta = material.ior;

    vec3 n = vec3(0,1,0);
    vec3 h = is_transmission ? normalize(ray_out + ray_in * eta) : normalize(ray_out + ray_in);

    float nv = clamp(ray_out.y, 0.0, 1.0);
    float nl = clamp(ray_in.y, 0.0, 1.0);
    float nh = clamp(h.y, 0.0, 1.0);
    float vh = clamp(dot(ray_out, h), 0.0, 1.0);

    
    FresnelTerm f = fresnel(eta, abs(ray_out.y));
    float d = d_ggx(nh, material.roughness);
    float g = g_smith(nv, nl, material.roughness);

    if (is_transmission) {
        result.color = material.base_color * (d * g * (1.0 - f.factor)) / max(4.0 * nv * nl, 0.001); // change denominator to transmissive denom
    } else {
        result.color = material.base_color * (d * g * f.factor) / max(4.0 * nv * nl, 0.001);
    }

    result.pdf = d * h.y;

    return result;
}

BSDFSample sample_lobe_transmission(vec3 ray_out, Material material, inout uint seed) {
    BSDFSample result;

    bool inner = ray_out.y < 0.0;

    float eta = 1.0 / material.ior;
    if (inner) eta = material.ior;

    float a = material.roughness * material.roughness;
    float rnd = random_float(seed);
    float theta = acos(sqrt((1.0 - rnd) / (1.0 + (a * a - 1.0) * rnd)));
    float phi = 2.0 * PI * random_float(seed);

    vec3 h = dir_from_thetaphi(theta, phi);
    if (inner) h.y *= -1;

    vec3 ray_in = refract(-ray_out, h, eta);

    float nv = clamp(ray_out.y, 0.0, 1.0);
    float nl = clamp(ray_in.y, 0.0, 1.0);
    float nh = clamp(h.y, 0.0, 1.0);
    float vh = clamp(dot(ray_out, h), 0.0, 1.0);

    FresnelTerm f = fresnel(eta, abs(ray_out.y));
    float d = d_ggx(nh, material.roughness);
    float g = g1_ggx_schlick(-nh, material.roughness);

    float pdf = d * cos(theta) * sin(theta);
    result.weight = material.base_color;
    result.direction = ray_in;
    result.pdf = pdf;

    return result;
}

#endif