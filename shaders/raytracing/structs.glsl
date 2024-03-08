#ifndef STRUCTS_GLSL
#define STRUCTS_GLSL

struct LightSample {
    vec3 direction;
    float distance;
    vec3 intensity; // already divided by PDF
    float pdf;
};

#endif