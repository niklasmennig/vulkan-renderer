layout(set = 1, binding = 6) uniform sampler2D tex[];
layout(set = 1, binding = 7) readonly buffer TextureIndexData {uint data[];} texture_indices;

#define NULL_TEXTURE_INDEX 10000

#define TEXTURE_OFFSET_DIFFUSE 0
#define TEXTURE_OFFSET_NORMAL 1
#define TEXTURE_OFFSET_ROUGHNESS 2
#define TEXTURE_OFFSET_EMISSIVE 3
#define TEXTURE_OFFSETS_COUNT 4

vec3 sample_texture(uint id, vec2 uv) {
    return texture(tex[nonuniformEXT(id)], uv).rgb;
}

vec3 sample_texture(uint instance, vec2 uv, uint offset) {
    uint texture_index = texture_indices.data[instance * TEXTURE_OFFSETS_COUNT + offset];
    if (texture_index == NULL_TEXTURE_INDEX) return vec3(0);
    return sample_texture(texture_index, uv);
}