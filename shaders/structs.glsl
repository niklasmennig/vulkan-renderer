#ifndef STRUCTS_GLSL
#define STRUCTS_GLSL

#define NULL_INSTANCE 999999
#define NULL_TEXTURE_INDEX 10000

struct Light {
    uint uint_data[4];
    float float_data[16];
};

struct PushConstantsPacked {
    uint stride_lcount_fsample_depth;
    uint sample_count;
    uint frame;
    uint flags;
    //
    uint env_dim_xy;
    uint sc_ext_xy;
    uint r_ext_xy;
    float exposure;
    //
    mat4 inv_camera_matrix;
    vec4 camera_position;
};

struct PushConstants {
    uint sbt_stride;
    uint frame;
    uint sample_count;
    uint light_count;
    //
    uint max_depth;
    uint flags;
    uint frame_samples;
    float exposure;
    //
    uvec2 environment_cdf_dimensions;
    uvec2 pad;
    //
    uvec2 swapchain_extent;
    uvec2 render_extent;
    //
    mat4 inv_camera_matrix;
    vec4 camera_position;
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