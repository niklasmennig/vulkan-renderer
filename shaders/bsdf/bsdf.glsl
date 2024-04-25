#ifndef BSDF_GLSL
#define BSDF_GLSL

//https://github.com/iRath96/raymond/tree/main/raymond/device/bsdf/lobes

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