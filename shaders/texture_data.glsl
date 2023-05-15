layout(set = 1, binding = 8) uniform sampler2D tex[16];
layout(set = 1, binding = 9) readonly buffer TextureIndexData {uint data[];} texture_indices;

vec3 sample_texture(uint instance, vec2 uv) {
    return texture(tex[texture_indices.data[instance]], uv).rgb;
}