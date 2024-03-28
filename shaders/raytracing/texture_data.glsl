#ifndef TEXTURE_DATA_GLSL
#define TEXTURE_DATA_GLSL

#extension GL_EXT_nonuniform_qualifier : require

#include "interface.glsl"

#ifndef NO_LAYOUT
layout(set = DESCRIPTOR_SET_OBJECTS, binding = 7) uniform sampler2D tex[];
layout(set = DESCRIPTOR_SET_OBJECTS, binding = 8) readonly buffer TextureIndexData {uint data[];} texture_indices;
#endif

#define TEXTURE_OFFSET_DIFFUSE 0
#define TEXTURE_OFFSET_NORMAL 1
#define TEXTURE_OFFSET_ROUGHNESS 2
#define TEXTURE_OFFSET_EMISSIVE 3
#define TEXTURE_OFFSET_TRANSMISSIVE 4
#define TEXTURE_OFFSETS_COUNT 5

bool has_texture(uint instance, uint offset) {
    uint texture_index = texture_indices.data[instance * TEXTURE_OFFSETS_COUNT + offset];
    return texture_index != NULL_TEXTURE_INDEX;
}

vec4 fetch_texture(uint id, ivec2 pixel) {
    return max(texelFetch(tex[nonuniformEXT(id)], pixel, 0), vec4(0.0));
}

vec4 sample_texture(uint id, vec2 uv) {
    return max(texture(tex[nonuniformEXT(id)], uv), vec4(0.0));
}

vec4 sample_texture(uint instance, vec2 uv, uint offset) {
    uint texture_index = texture_indices.data[instance * TEXTURE_OFFSETS_COUNT + offset];
    if (texture_index == NULL_TEXTURE_INDEX) {
        return vec4(0.0);
    }
    return sample_texture(texture_index, uv);
}

#endif