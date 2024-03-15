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
    uint sample_count;
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

struct MaterialParameters {
    // diffuse rgb, opacity a
    vec3 diffuse;
    float opacity;
    // emissive rgb, emission strength a
    vec3 emissive;
    float emission_strength;
    // roughness x, metallic y, transmissive z, ior a
    float roughness;
    float metallic;
    float transmissive;
    float ior;
};

struct Material {
    vec3 base_color;
    float opacity;
    float roughness;
    float metallic;
    float transmission;
    float ior;
    float fresnel;
    vec3 emission;
};

struct Reservoir {
    uint num_samples;
    float sum_weights;
    uint sample_seed;
    float weight;
};

#endif