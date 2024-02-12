#ifndef PAYLOAD_GLSL
#define PAYLOAD_GLSL

struct RayPayload {
    vec3 color;
    uint depth;

    vec3 contribution;

    vec3 origin;
    vec3 direction;

    uint seed;
    bool end_ray;
};

#endif