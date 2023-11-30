#ifndef PARAMETERS_GLSL
#define PARAMETERS_GLSL

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

layout(std430, set = 1, binding = 9) readonly buffer MaterialParameterData {MaterialParameters[] data;} material_parameters;

MaterialParameters get_material_parameters(uint instance) {
    return material_parameters.data[instance];
}

#endif