//https://github.com/iRath96/raymond/tree/main/raymond/device/bsdf/lobes

#include "../../common.glsl"
#include "../../random.glsl"
#include "../sampling.glsl"
#include "../bsdf.glsl"
#include "../../raytracing/fresnel.glsl"

float d_ggx(float nh, float roughness) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float nh2 = nh * nh;
  float b = (nh2 * (alpha2 - 1.0) + 1.0);
  return alpha2 / (PI * b * b);
}

float g1_ggx_schlick(float nv, float roughness) {
  float r = 0.5 + 0.5 * roughness;
  float k = (r * r) / 2.0;
  float denom = nv * (1.0 - k) + k;
  return nv / denom;
}

float g_smith(float nv, float nl, float roughness) {
  float g1_l = g1_ggx_schlick(nl, roughness);
  float g1_v = g1_ggx_schlick(nv, roughness);
  return g1_l * g1_v;
}

vec3 fresnel_schlick(float cosTheta, vec3 F0) {
  return F0 + (vec3(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
} 

BSDFEvaluation eval_lobe_specular(vec3 ray_out, vec3 ray_in, Material material) {
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

BSDFSample sample_lobe_specular(vec3 ray_out, Material material, inout uint seed) {
    BSDFSample result;

    float a = material.roughness * material.roughness;
    float rnd = random_float(seed);
    float theta = acos(sqrt((1.0 - rnd) / (1.0 + (a * a - 1.0) * rnd)));
    float phi = 2.0 * PI * random_float(seed);

    vec3 h = dir_from_thetaphi(theta, phi);
    vec3 ray_in = reflect(-ray_out, h);

    float nv = clamp(ray_out.y, 0.0, 1.0);
    float nl = clamp(ray_in.y, 0.0, 1.0);
    float nh = clamp(h.y, 0.0, 1.0);
    float vh = clamp(dot(ray_out, h), 0.0, 1.0);

    vec3 f0 = vec3(0.16 * material.fresnel * material.fresnel);
    f0 = mix(f0, material.base_color, material.metallic);
    vec3 f = fresnel_schlick(vh, f0);

    float d = d_ggx(nh, material.roughness);
    float g = g_smith(nv, nl, material.roughness);

    float pdf = d * cos(theta) * sin(theta);
    result.weight = (f * g * vh / max(nh * nv, 0.001));
    result.direction = ray_in;
    result.pdf = pdf;

    return result;
}