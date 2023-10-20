#ifndef PAYLOAD_GLSL
#define PAYLOAD_GLSL

struct RayPayload
{
    bool hit;
    uint hit_instance;
    uint hit_primitive;
    mat4x3 hit_transform_world;
    mat4x3 hit_transform_object;
    vec2 hit_barycentrics;
};

#endif