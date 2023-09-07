#ifndef INC_STRUCTS
#define INC_STRUCTS

struct Light {
    vec3 position;
    float pad1;
    vec3 intensity;
    float pad2;
};

struct PushConstants {
    uint sbt_stride;
    float time;
    uint clear_accumulated;
    uint light_count;
    uint max_depth;
};

#endif