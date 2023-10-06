#include "lights.glsl"
#include "texture_data.glsl"

LightSample sample_environment_map() {
    vec2 conditional_uv = vec2(0, conditional_position);
    float conditional = sample_texture(1, conditional_uv).r;
    vec2 cdf_uv = vec2();
    float cdf = sample_texture(1, cdf_uv).r;
}