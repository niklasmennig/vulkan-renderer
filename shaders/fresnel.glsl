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