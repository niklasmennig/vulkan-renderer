#ifndef BSDF_GLSL
#define BSDF_GLSL

struct BSDFSample {
    vec3 weight; // BSDF value / pdf
    vec3 direction;
    float pdf;
};

struct BSDFEvaluation {
    vec3 color;
    float pdf;
};

#endif