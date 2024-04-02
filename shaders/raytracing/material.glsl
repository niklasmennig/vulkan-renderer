#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

#include "../structs.glsl"
#include "texture_data.glsl"

#ifndef NO_LAYOUT
#include "interface.glsl"
layout(std430, set = DESCRIPTOR_SET_OBJECTS, binding = DESCRIPTOR_BINDING_MATERIAL_PARAMETERS) readonly buffer MaterialParameterData {MaterialParameters[] data;} material_parameters;
#endif

MaterialParameters get_material_parameters(uint instance) {
    return material_parameters.data[instance];
}

Material get_material(uint instance, vec2 uv) {
    Material result;

    MaterialParameters material_parameters = get_material_parameters(instance);

    if (has_texture(instance, TEXTURE_OFFSET_DIFFUSE)) {
        vec4 base_color_tex = sample_texture(instance, uv, TEXTURE_OFFSET_DIFFUSE);
        result.base_color = material_parameters.diffuse * base_color_tex.rgb;
        result.opacity = material_parameters.opacity * base_color_tex.a;
    } else {
        result.base_color = material_parameters.diffuse;
        result.opacity = material_parameters.opacity;
    }
    
    if (has_texture(instance, TEXTURE_OFFSET_ROUGHNESS)) {
        vec3 arm = sample_texture(instance, uv, TEXTURE_OFFSET_ROUGHNESS).rgb;
        result.roughness = material_parameters.roughness * arm.y;
        result.metallic = material_parameters.metallic * arm.z;
    } else {
        result.roughness = material_parameters.roughness;
        result.metallic = material_parameters.metallic;
    }

    if (has_texture(instance, TEXTURE_OFFSET_TRANSMISSIVE)) {
        vec4 transmission_tex = sample_texture(instance, uv, TEXTURE_OFFSET_TRANSMISSIVE);
        result.transmission = material_parameters.transmissive * (1.0 - transmission_tex.x);
    } else {
        result.transmission = material_parameters.transmissive;
    }
    
    result.ior = material_parameters.ior;
    result.fresnel = 0.5;
    
    if (has_texture(instance, TEXTURE_OFFSET_EMISSIVE)) {
        vec4 emission_tex = sample_texture(instance, uv, TEXTURE_OFFSET_EMISSIVE);
        result.emission = emission_tex.rgb * material_parameters.emissive * material_parameters.emission_strength;
    } else {
        result.emission = material_parameters.emissive * material_parameters.emission_strength;
    }

    return result;
}

#endif