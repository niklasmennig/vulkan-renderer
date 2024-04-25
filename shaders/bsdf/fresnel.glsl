#ifndef FRESNEL_GLSL
#define FRESNEL_GLSL

float fresnel_factor(float eta, float cos_i, float cos_t) {
    float r_s = (eta * cos_i - cos_t) / (eta * cos_i + cos_t);
    float r_p = (cos_i - eta * cos_t) / (cos_i + eta * cos_t);
    return clamp((r_s * r_s + r_p * r_p) * 0.5, 0, 1);
}

float snell(float eta, float cos_i) {
    return 1.0 - (1.0 - cos_i * cos_i) * eta * eta;
}

struct FresnelTerm {
    float cos_t;
    float factor;
};

FresnelTerm fresnel(float eta, float cos_i) {
    float eta2 = eta;
    if (cos_i < 0) eta2 = 1.0 / eta;
    float cos2_t = snell(eta2, cos_i);
    FresnelTerm result;
    if (cos2_t <= 0.0) {
        result.cos_t = 0.0;
        result.factor = 1.0;
    } else {
        float cos_t = sqrt(cos2_t);
        float factor = fresnel_factor(eta2, abs(cos_i), cos_t);
        if (cos_i < 0) cos_t = -cos_t;
        result.cos_t = cos_t;
        result.factor = factor;
    }
    return result;
}

// https://github.com/iRath96/raymond/blob/main/raymond/device/bsdf/fresnel.hpp
float fresnel_dielectric_cos(float cos_i, float eta) {
    float c = abs(cos_i);
    float g = eta * eta - 1 + c * c;
    if (g > 0) {
        g = sqrt(g);
        float A = (g - c) / (g + c);
        float B = (c * (g + c) - 1) / (c * (g - c) + 1);
        return 0.5f * A * A * (1 + B * B);
    }
    return 1.0f;
}

vec3 interpolate_fresnel(vec3 ray_in, vec3 h, float ior, float f0, vec3 cspec0) {
    float f0_n = 1.0 / (1.0 - f0);
    float fh = (fresnel_dielectric_cos(dot(ray_in, h), ior) - f0) * f0_n;
    return cspec0 * (1.0 - fh) + vec3(1.0) * fh;
}

vec3 fresnel_color(vec3 ray_in, vec3 h, float ior, vec3 cspec0) {
    float f0 = fresnel_dielectric_cos(1.0, ior);
    return interpolate_fresnel(ray_in, h, ior, f0, cspec0);
}

#endif