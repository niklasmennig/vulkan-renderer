#ifndef PARAMETERS_GLSL
#define PARAMETERS_GLSL

struct MaterialParameters {
    // diffuse rgb, opacity a
    vec4 diffuse_opacity;
    // emissive rgb, emission strength a
    vec4 emissive_factor;
    // roughness x, metallic y, transmissive z, ior a
    vec4 roughness_metallic_transmissive_ior;
};

layout(std430, set = 1, binding = 9) readonly buffer MaterialParameterData {MaterialParameters[] data;} material_parameters;

MaterialParameters get_material_parameters(uint instance) {
    return material_parameters.data[instance];
}

#endif