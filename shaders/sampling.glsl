#ifndef SAMPLING_GLSL
#define SAMPLING_GLSL

#include "common.glsl"

struct BSDFSample {
    vec3 contribution;
    vec3 direction;
    float pdf;
    bool specular;
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
    dir_sample.pdf = pdf_cosine_hemisphere(dir_sample.direction.z);

    return dir_sample;
}



// taken from https://www.cim.mcgill.ca/~derek/ecse689_a3.html
vec3 sample_power_hemisphere(float u1, float u2, float n)
{
    float alpha = sqrt(1.0 - pow(u1, 2.0 / (n+1.0)));

    float x = alpha * cos(2.0 * PI * u2);
    float y = alpha * sin(2.0 * PI * u2);
    float z = pow(u1, 1.0 / (n+1.0));

    return vec3(x, y, z);
}

#endif