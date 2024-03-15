#ifndef STRUCTS_GLSL
#define STRUCTS_GLSL

struct CameraData
{
    vec4 origin;
    vec4 forward;
    vec4 right;
    vec4 up;
    float fov_x;
};

struct Light {
    uint uint_data[4];
    float float_data[16];
};

struct PushConstants {
    uint sbt_stride;
    float time;
    uint clear_accumulated;
    uint light_count;
    //
    uint max_depth;
    uint flags;
    uint frame_samples;
    float exposure;
    //
    uvec2 environment_cdf_dimensions;
    uvec2 image_extent;
};

struct LightSample {
    vec3 direction;
    float distance;
    vec3 weight; // already divided by PDF
    float pdf;
};

#endif