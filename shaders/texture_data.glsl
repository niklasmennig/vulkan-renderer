layout(set = 1, binding = 6) uniform sampler2D tex[16];
layout(set = 1, binding = 7) readonly buffer TextureIndexData {uint data[];} texture_indices;

#define TEXTURE_OFFSET_DIFFUSE 0
#define TEXTURE_OFFSET_NORMAL 1
#define TEXTURE_OFFSET_ROUGHNESS 2

vec3 sample_texture(uint id, vec2 uv) {
    return texture(tex[nonuniformEXT(id)], uv).rgb;
}

vec3 sample_texture(uint instance, vec2 uv, uint offset) {
    return texture(tex[nonuniformEXT(texture_indices.data[instance * 3] + offset)], uv).rgb;
}