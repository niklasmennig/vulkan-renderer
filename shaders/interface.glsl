#ifndef INC_INTERFACE
#define INC_INTERFACE

#define DESCRIPTOR_SET_FRAMEWORK 0
#define DESCRIPTOR_SET_OBJECTS 1
#define DESCRIPTOR_SET_CUSTOM 2

#define DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE 0
#define DESCRIPTOR_BINDING_CAMERA_PARAMETERS 1
#define DESCRIPTOR_BINDING_IMAGES 2
#define DESCRIPTOR_BINDING_LIGHTS 3

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
    float padding;
    //
    uvec2 environment_cdf_size;
    vec2 padding2;
};

#endif