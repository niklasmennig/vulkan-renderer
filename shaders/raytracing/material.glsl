#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

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


struct Material {
    vec3 base_color;
    float opacity;
    float roughness;
    float metallic;
    float transmission;
    float ior;
    float fresnel;
};

Material get_material(uint instance, vec2 uv) {
    Material result;

    MaterialParameters material_parameters = get_material_parameters(instance);

    vec4 base_color_tex = sample_texture(instance, uv, TEXTURE_OFFSET_DIFFUSE);
    result.base_color = material_parameters.diffuse * base_color_tex.rgb;
    result.opacity = material_parameters.opacity * base_color_tex.a;
    vec3 arm = sample_texture(instance, uv, TEXTURE_OFFSET_ROUGHNESS).rgb;
    result.roughness = material_parameters.roughness * arm.y;
    result.metallic = material_parameters.metallic * arm.z;
    vec4 transmission_tex = sample_texture(instance, uv, TEXTURE_OFFSET_TRANSMISSIVE);
    result.transmission = material_parameters.transmissive * (1.0 - transmission_tex.x);
    result.ior = material_parameters.ior;
    result.fresnel = 0.5;

    return result;
}

#endif