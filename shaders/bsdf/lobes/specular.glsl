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

    bool is_transmission = ray_out.y * ray_in.y < 0;
    float eta = 1.0 / material.ior;
    if (ray_out.y < 0.0) eta = material.ior;

    vec3 n = vec3(0,1,0);
    vec3 h = is_transmission ? normalize(ray_out + ray_in * eta) : normalize(ray_out + ray_in);

    float nv = clamp(ray_out.y, 0.0, 1.0);
    float nl = clamp(ray_in.y, 0.0, 1.0);
    float nh = abs(h.y);
    float vh = dot(ray_out, h);

    float d = d_ggx(nh, material.roughness);
    float g = g_smith(nv, nl, material.roughness);
    float pdf = d * g * abs(vh) / nv;

    if (pdf <= 0) {
        result.pdf = 0;
        result.color = vec3(0.0);
        return result;
    }
    
    vec3 f = fresnel_color(ray_in, h, material.ior, material.base_color);

    if (is_transmission) {
        result.color = vec3(0.0);
        result.pdf = 0.0;
    } else {
        result.color = f * g * d;
        result.pdf = pdf / abs(4.0 * vh);
    }

    return result;
}

BSDFSample sample_lobe_specular(vec3 ray_out, Material material, inout uint seed) {
    BSDFSample result;

    float eta = 1.0 / material.ior;

    float a = material.roughness * material.roughness;
    float rnd = random_float(seed);
    float cos_theta = sqrt((1.0 - rnd) / (1.0 + (a * a - 1.0) * rnd));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    float phi = 2.0 * PI * random_float(seed);

    vec3 h = dir_from_thetaphi(cos_theta, sin_theta, phi);

    vec3 ray_in = reflect(-ray_out, h);

    // transmission is invalid
    if (ray_out.y * ray_in.y < 0.0) {
        result.weight = vec3(0.0);
        result.pdf = 0.0;
        return result;
    }

    vec3 f = fresnel_color(ray_in, h, material.ior, material.base_color);

    float d = d_ggx(h.y, material.roughness);
    float g = g_smith(ray_in.y, h.y, material.roughness);

    result.weight = material.base_color * f * g;
    result.direction = ray_in;
    result.pdf = d * g * abs(dot(ray_out, h) / ray_out.y) / abs(4.0 * dot(ray_out, h));

    return result;
}

#endif