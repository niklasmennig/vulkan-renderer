#ifndef SAMPLING_GLSL
#define SAMPLING_GLSL

#include "../common.glsl"

struct BSDFSample {
    vec3 weight; // BSDF value / pdf
    vec3 direction;
    float pdf;
};

struct DirectionSample {
    vec3 direction;
    float pdf;
};

DirectionSample sample_uniform_hemisphere(float u, float v) 
{
    float theta = u * PI * 0.5;
    float phi = 2.0 * PI * v;

    DirectionSample dir_sample;
    dir_sample.direction = dir_from_thetaphi(theta, phi);
    dir_sample.pdf = 1.0 / PI;

    return dir_sample;
}

float pdf_cosine_hemisphere(float cos) {
    return cos / PI;
}

DirectionSample sample_cosine_hemisphere(float u, float v)
{
    float theta = acos(sqrt(u));
    float phi = 2.0 * PI * v;

    DirectionSample dir_sample;
    dir_sample.direction = dir_from_thetaphi(theta, phi);
    dir_sample.pdf = pdf_cosine_hemisphere(dir_sample.direction.y);

    return dir_sample;
}

float pdf_power_hemisphere(float cos, float alpha) {
    return ((alpha+1) * pow(cos, alpha)) / (2.0 * PI);
}

// taken from https://ameye.dev/notes/sampling-the-hemisphere/
DirectionSample sample_power_hemisphere(float u, float v, float alpha)
{
    float theta = acos(pow(u, (1.0 / (alpha + 1))));
    float phi = 2.0 * PI * v;

    DirectionSample dir_sample;
    dir_sample.direction = dir_from_thetaphi(theta, phi);
    dir_sample.pdf = pdf_power_hemisphere(dir_sample.direction.y, alpha);

    return dir_sample;
}
#endif