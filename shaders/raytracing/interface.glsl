#ifndef INC_INTERFACE_RT
#define INC_INTERFACE_RT

#define DESCRIPTOR_SET_FRAMEWORK 0
#define DESCRIPTOR_SET_OBJECTS 1
#define DESCRIPTOR_SET_CUSTOM 2

// framework data bindings
#define DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE 0
#define DESCRIPTOR_BINDING_CAMERA_PARAMETERS 1
#define DESCRIPTOR_BINDING_OUTPUT_BUFFERS 2
#define DESCRIPTOR_BINDING_LIGHTS 3
#define DESCRIPTOR_BINDING_RESTIR_RESERVOIRS 4

// mesh data bindings
#define DESCRIPTOR_BINDING_MESH_INDICES 0
#define DESCRIPTOR_BINDING_MESH_VERTICES 1
#define DESCRIPTOR_BINDING_MESH_NORMALS 2
#define DESCRIPTOR_BINDING_MESH_TEXCOORDS 3
#define DESCRIPTOR_BINDING_MESH_TANGENTS 4
#define DESCRIPTOR_BINDING_MESH_DATA_OFFSETS 5
#define DESCRIPTOR_BINDING_MESH_OFFSET_INDICES 6

// material data bindings
#define DESCRIPTOR_BINDING_TEXTURES 7
#define DESCRIPTOR_BINDING_TEXTURE_INDICES 8
#define DESCRIPTOR_BINDING_MATERIAL_PARAMETERS 9

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

#endif